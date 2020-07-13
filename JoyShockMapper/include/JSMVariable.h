#pragma once

#include "JoyShockMapper.h"
#include <functional>
#include <map>

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

	// The variable value itself
	T _value;

	// Default value of the variable. Cannot be changed after construction.
	const T _defVal;

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
	JSMVariable(T defaultValue = 0)
		: _defVal(defaultValue)
		, _value(_defVal)
		, _onChangeListeners()
		, _filter(&NoFiltering) // _filter is always valid
	{	}

	// Make a copy with a different default value
	JSMVariable(const JSMVariable &copy, T defaultValue = 0)
		: _defVal(defaultValue)
		, _value(_defVal)
		, _onChangeListeners(copy._onChangeListeners)
		, _filter(copy._filter)
	{	}

	virtual ~JSMVariable()
	{
		_onChangeListeners.clear();
	}

	// Sets the filtering function for this variable. Also applies
	// the filter to it's current value.
	JSMVariable *SetFilter(FilterDelegate filterfunction)
	{
		_filter = filterfunction;
		operator =(_value); // Run the new filter on current value.
		return this;
	}

	// Remember to call this listener when the value changes.
	unsigned int AddOnChangeListener(const OnChangeDelegate &listener)
	{
		_onChangeListeners[_delegateID] = listener;
		return _delegateID++;
	}

	// Remove the listener from list
	bool RemoveOnChangeListener(unsigned int id)
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
	JSMVariable *Reset()
	{
		// Use operator to enable notification
		operator =(_defVal);
		return this;
	}

	// Value can be read by implicit or explicit casting
	// This enables easy usage of the variable within an operation.
	operator T() const
	{
		return _value;
	}

	// Value can be written by using operator =.
	// N.B.: It's important to always use this function
	// for changing the member _value
	T operator =(T newValue)
	{
		_value = _filter(_value, newValue); // Pass new value through filtering
		for (auto listener : _onChangeListeners)
			listener.second(_value); // Notify listeners of the change
		return _value; // Return actual value assign. Can be different from newValue because of filtering.
	}
};


class JSMMapping : public JSMVariable<Mapping>
{
public:
};

template<typename T>
class JSMSetting : public JSMVariable<T>
{
protected:
	// Identifier of the variable. Cannot be changed after construction.
	const SettingID _id;

	map<ButtonID, T> modeshifts;

public:
	JSMSetting(SettingID id, T defaultValue)
		:JSMVariable(defaultValue)
		, _id(id)
		, modeshifts()
	{}

	Optional<T> get(ButtonID chord = ButtonID::NONE) const
	{
		if (chord == ButtonID::NONE)
		{
			return _value;
		}
		else
		{
			auto existingChord = modeshifts.find(chord);
			return existingChord != modeshifts.end() ? Optional<T>(existingChord->second) : Optional<T>();
		}
	}

	T &operator [] (int chord)
	{
		return modeshifts[chord];
	}

	T operator =(T newValue)
	{
		return JSMVariable::operator =(newValue);
	}
};
