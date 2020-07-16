#pragma once

#include "JoyShockMapper.h"
#include <functional>
#include <map>
#include <sstream>
#include <algorithm>

using namespace std;

static unsigned int _delegateID = 1;

// JSMVariable is a wrapper class for an underlying variable of type T.
// This class allows other parts of the code be notified of when this changes value.
// It also has a default value defined at construction that can be assigned on Reset.
// Finally it has a customizable filter function, to restrict possible values.
// Setter functions return the object itself so they can be chained.
template<typename T>
class JSMVariable
{
public:
	// A listener function receives the new value as parameter.
	typedef function<void(T newVal)> OnChangeDelegate;

	// A filter function needs to return the value to be assigned.
	// It is given the current value and the next value that wishes to be assigned.
	typedef function<T(T current, T next)> FilterDelegate;

protected:

	// Default value of the variable. Cannot be changed after construction.
	const T _defVal;

	// The variable value itself
	T _value;

	// Parts of the code can be notified of when _value changes. This is
	// really important for a GUI to be possible.
	map<unsigned int, OnChangeDelegate> _onChangeListeners;

	// The filtering function of the variable.
	FilterDelegate _filter;

	// The default filtering function simply accepts the new value.
	static T NoFiltering(T old, T nu)
	{
		return nu;
	}

public:
	JSMVariable(T defaultValue = T(0))
		: _defVal(defaultValue)
		, _value(_defVal)
		, _onChangeListeners()
		, _filter(&NoFiltering) // _filter is always valid
	{	}

	// Make a copy with a different default value
	JSMVariable(const JSMVariable &copy, T defaultValue = T(0))
		: _defVal(defaultValue)
		, _value(_defVal)
		, _onChangeListeners() // Don't copy listeners. This is a different variable!
		, _filter(copy._filter)
	{	}

	virtual ~JSMVariable()
	{
		_onChangeListeners.clear();
	}

	// Sets the filtering function for this variable. Also applies
	// the filter to it's current value.
	virtual JSMVariable *SetFilter(FilterDelegate filterfunction)
	{
		_filter = filterfunction;
		operator =(_value); // Run the new filter on current value.
		return this;
	}

	// Remember to call this listener when the value changes.
	virtual unsigned int AddOnChangeListener(const OnChangeDelegate &listener)
	{
		_onChangeListeners[_delegateID] = listener;
		return _delegateID++;
	}

	// Remove the listener from list
	virtual bool RemoveOnChangeListener(unsigned int id)
	{
		auto found = _onChangeListeners.find(id);
		if (found != _onChangeListeners.end())
		{
			_onChangeListeners.erase(found);
			return true;
		}
		return false;
	}

	// Reset the variable by assigning it its default value.
	virtual JSMVariable *Reset()
	{
		// Use operator to enable notification
		operator =(_defVal);
		return this;
	}

	// Value can be read by implicit or explicit casting
	// This enables easy usage of the variable within an operation.
	virtual operator T() const
	{
		return _value;
	}

	// Value can be written by using operator =.
	// N.B.: It's important to always use this function
	// for changing the member _value
	virtual T operator =(T newValue)
	{
		T oldValue = _value;
		_value = _filter(oldValue, newValue); // Pass new value through filtering
		if (_value != oldValue)
		{
			// Notify listeners of the change
			for (auto listener : _onChangeListeners)
				listener.second(_value);
		}
		return _value; // Return actual value assign. Can be different from newValue because of filtering.
	}
};


template<typename T>
class ChordedVariable : public JSMVariable<T>
{
protected:
	map<ButtonID, JSMVariable<T>> _chorded;

public:
	ChordedVariable(T defval)
		: JSMVariable(defval)
		, _chorded()
	{}
	
	JSMVariable<T> &CreateChord(ButtonID chord)
	{
		auto existingChord = _chorded.find(chord);
		if (existingChord == _chorded.end())
		{
			_chorded.emplace( chord, JSMVariable<T>(*this, _defVal) );
		}
		return _chorded[chord];
	}

	//JSMVariable<T> &operator =(T baseValue)
	//{
	//	base = baseValue;
	//	return base;
	//}


	const JSMVariable<T> *operator [](ButtonID chord) const
	{
		auto existingChord = _chorded.find(chord);
		if (existingChord == _chorded.end())
		{
			return nullptr;
		}
		return &existingChord->second;
	}

	JSMVariable<T> *operator [](ButtonID chord)
	{
		auto existingChord = _chorded.find(chord);
		if (existingChord == _chorded.end())
		{
			return nullptr;
		}
		return &existingChord->second;
	}

	virtual ChordedVariable<T> *Reset() override
	{
		JSMVariable<T>::Reset();
		_chorded.clear();
		return this;
	}
};


template<typename T>
class JSMSetting : public ChordedVariable<T>
{
protected:
	// Identifier of the variable. Cannot be changed after construction.
	const SettingID _id;

	bool isModeshiftWithChord(ButtonID chord, pair<ButtonID, T> &modeshift)
	{
		return modeshift.first == chord;
	}

public:
	JSMSetting(SettingID id, T defaultValue)
		: ChordedVariable(defaultValue)
		, _id(id)
	{}

	Optional<T> get(ButtonID chord = ButtonID::NONE) const
	{
		if (chord == ButtonID::NONE)
		{
			return _value;
		}
		else
		{
			auto existingChord = ChordedVariable<T>::operator [](chord);
			return existingChord ? Optional<T>(T(*existingChord)) : Optional<T>();
		}
	}

	virtual T operator =(T baseValue) override
	{
		return JSMVariable<T>::operator =(baseValue);
	}
};

class JSMMapping : public ChordedVariable<Mapping>
{
protected:
	// Identifier of the variable. Cannot be changed after construction.
	const ButtonID _id;

	vector<ComboMap> _simMappings;

	static bool isSimMapWith(ButtonID simBtn, const ComboMap &simMap)
	{
		return simMap.btn == simBtn;
	}

public:
	JSMMapping(ButtonID id)
		: ChordedVariable(Mapping())
		, _id(id)
	{}

	Optional<Mapping> get(ButtonID chord = ButtonID::NONE) const
	{
		if (chord == ButtonID::NONE)
		{
			return _value;
		}
		else
		{
			auto existingChord = ChordedVariable<Mapping>::operator [](chord);
			return existingChord ? Optional<Mapping>(Mapping(*existingChord)) : Optional<Mapping>();
		}
	}

	const ComboMap *getSimMap(ButtonID simBtn)
	{
		if (simBtn > ButtonID::NONE)
		{
			auto existingSim = find_if(_simMappings.cbegin(), _simMappings.cend(), bind(&JSMMapping::isSimMapWith, simBtn, placeholders::_1));
			return existingSim != _simMappings.cend() ? &*existingSim : nullptr;
		}
		return nullptr;
	}

	void AddSimPress(ButtonID simBtn, WORD press, WORD hold)
	{
		_simMappings.push_back({ (stringstream() << simBtn << '+' << _id).str(), simBtn, press, hold });
	}

	inline bool HasSimMappings() const
	{
		return !_simMappings.empty();
	}

	virtual Mapping operator =(Mapping baseValue) override
	{
		return JSMVariable<Mapping>::operator =(baseValue);
	}

	string getName(ButtonID chord = ButtonID::NONE) const 
	{
		if (chord == ButtonID::NONE)
		{
			return (stringstream() << _id).str();
		}
		else
		{
			return (stringstream() << chord << ',' << _id).str();
		}
	}

	virtual JSMMapping *Reset() override
	{
		ChordedVariable<Mapping>::Reset();
		_simMappings.clear();
		return this;
	}
};