#pragma once

#include "JoyShockMapper.h"
//#include <functional>
//#include <map>
#include <sstream>
//#include <algorithm>

// Global ID generator
static unsigned int _delegateID = 1;

// JSMVariable is a wrapper class for an underlying variable of type T.
// This class allows other parts of the code be notified of when it changes value.
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

	// Parts of the code can be notified of when _value changes.
	map<unsigned int, OnChangeDelegate> _onChangeListeners;

	// The filtering function of the variable.
	FilterDelegate _filter;

	// The default filtering function simply accepts the new value.
	static T NoFiltering(T old, T nu)
	{
		return nu;
	}

public:
	// Default value of the variable. Cannot be changed after construction.
	const T _defVal;

	JSMVariable(T defaultValue = T(0))
		: _value(defaultValue)
		, _onChangeListeners()
		, _filter(&NoFiltering) // _filter is always valid
		, _defVal(defaultValue)
	{	}

	// Make a copy with a different default value
	JSMVariable(const JSMVariable &copy, T defaultValue)
		: _value(defaultValue)
		, _onChangeListeners() // Don't copy listeners. This is a different variable!
		, _filter(copy._filter)
		, _defVal(defaultValue)
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
	virtual unsigned int AddOnChangeListener(OnChangeDelegate listener)
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

	virtual T get() const
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
			// Notify listeners of the change if there's a change
			for (auto listener : _onChangeListeners)
				listener.second(_value);
		}
		return _value; // Return actual value assign. Can be different from newValue because of filtering.
	}
};

// A chorded variable alternate values depending on buttons enabling the chorded value
template<typename T>
class ChordedVariable : public JSMVariable<T>
{
	using Base = JSMVariable<T>;

protected:
	// Each chord is a separate variable with its own listeners, but will use the same filtering and parsing.
	map<ButtonID, JSMVariable<T>> _chordedVariables;

public:
	ChordedVariable(T defval)
		: Base(defval)
		, _chordedVariables()
	{}

	// Get the chorded variable, creating one if required.
	JSMVariable<T> *AtChord(ButtonID chord)
	{
		auto existingChord = _chordedVariables.find(chord);
		if (existingChord == _chordedVariables.end())
		{
			// Create the chord when requested, using the copy constructor.
			_chordedVariables.emplace( chord, JSMVariable<T>(*this, Base::_defVal) );
		}
		return &_chordedVariables[chord];
	}

	const JSMVariable<T> *AtChord(ButtonID chord) const
	{
		auto existingChord = _chordedVariables.find(chord);
		return existingChord != _chordedVariables.end() ? &existingChord->second : nullptr;
	}

	// Obtain the value with provided chord if any.
	optional<T> get(ButtonID chord = ButtonID::NONE) const
	{
		if (chord > ButtonID::NONE)
		{
			auto existingChord = _chordedVariables.find(chord);
			return existingChord != _chordedVariables.end() ? optional<T>(T(existingChord->second)) : nullopt;
		}
		return chord != ButtonID::INVALID ? optional(Base::_value) : nullopt;
	}

	// Resetting a chorded var always clears all chords.
	virtual ChordedVariable<T> *Reset() override
	{
		JSMVariable<T>::Reset();
		_chordedVariables.clear();
		return this;
	}
};

// A Setting for JSM can be chorded normally. It also has its own setting ID
template<typename T>
class JSMSetting : public ChordedVariable<T>
{
	using Base = ChordedVariable<T>;

protected:
	ButtonID _chordToRemove;
public:
	// Identifier of the variable. Cannot be changed after construction.
	const SettingID _id;

	JSMSetting(SettingID id, T defaultValue)
		: Base(defaultValue)
		, _id(id)
		, _chordToRemove(ButtonID::NONE)
	{}

	virtual T operator =(T baseValue) override
	{
		return JSMVariable<T>::operator =(baseValue);
	}

	inline void MarkModeshiftForRemoval(ButtonID modeshift)
	{
		_chordToRemove = modeshift;
	}

	void ProcessModeshiftRemoval(ButtonID modeshift)
	{
		if(_chordToRemove == modeshift)
		{
			auto modeshiftVar = Base::_chordedVariables.find(modeshift);
			if (modeshiftVar != Base::_chordedVariables.end())
			{
				Base::_chordedVariables.erase(modeshiftVar);
				_chordToRemove = ButtonID::NONE;
			}
		}
	}
};

// A combo map is an item of _simMappings below. It holds an alternative variable when ButtonID is pressed.
typedef pair<const ButtonID, JSMVariable<EventMapping>> ComboMap;

// Button mappings includes not only chorded mappings, but also simultaneous press mappings
class JSMButton : public ChordedVariable<EventMapping>
{
public:
	// Identifier of the variable. Cannot be changed after construction.
	const ButtonID _id;

protected:
	map<ButtonID, JSMVariable<EventMapping>> _simMappings;

	// Store listener IDs for its sim presses. This is required for Cross updates
	map<ButtonID, unsigned int> _simListeners;

public:
	JSMButton(ButtonID id, EventMapping def)
		: ChordedVariable(def)
		, _id(id)
		, _simMappings()
		, _simListeners()
	{}

	virtual ~JSMButton()
	{
		for (auto id : _simListeners)
		{
			_simMappings[id.first].RemoveOnChangeListener(id.second);
		}
	}

	// Obtain the Variable for a sim press if any.
	const ComboMap *getSimMap(ButtonID simBtn) const
	{
		if (simBtn > ButtonID::NONE)
		{
			auto existingSim = _simMappings.find(simBtn);
			return existingSim != _simMappings.cend() ? &*existingSim : nullptr;
		}
		return nullptr;
	}

	// Double Press mappings are stored in the chorded variables
	const ComboMap *getDblPressMap() const
	{
		auto existingChord = _chordedVariables.find(_id);
		return existingChord != _chordedVariables.end() ? &*existingChord : nullptr;
	}

	// Indicate whether any sim press mappings are present
	// This function additionally removes any empty sim mappings.
	inline bool HasSimMappings() const
	{
		return !_simMappings.empty();
	}

	// Operator forwarding
	virtual EventMapping operator =(EventMapping baseValue) override
	{
		return JSMVariable<EventMapping>::operator =(baseValue);
	}

	// Returns the display name of the chorded press if provided, or itself
	string getName(ButtonID chord = ButtonID::NONE) const
	{
		if (chord > ButtonID::NONE)
		{
			stringstream ss;
			ss << chord << ',' << _id;
			return ss.str();
		}
		else if (chord != ButtonID::INVALID)
			return string(magic_enum::enum_name(_id));
		else
			return string();
	}

	// Returns the sim press name of itself with simBtn.
	string getSimPressName(ButtonID simBtn) const
	{
		if (simBtn == _id)
		{
			// It's actually a double press, not a sim press
			return getName(simBtn);
		}
		if (simBtn > ButtonID::NONE)
		{
			stringstream ss;
			ss << simBtn << '+' << _id;
			return ss.str();
		}
		return string();
	}

	// Resetting a button also clears all assigned sim presses
	virtual JSMButton *Reset() override
	{
		ChordedVariable<EventMapping>::Reset();
		for (auto id : _simListeners)
		{
			_simMappings[id.first].RemoveOnChangeListener(id.second);
		}
		_simMappings.clear();
		return this;
	}

	// Get the SimPress variable, creating one if required.
	// An additional listener is required for the complementary sim press
	// to be updated when this value changes.
	JSMVariable<EventMapping> *AtSimPress(ButtonID chord)
	{
		auto existingSim = getSimMap(chord);
		if (!existingSim)
		{
			JSMVariable<EventMapping> var(*this, EventMapping());
			_simMappings.emplace(chord, var);
			_simListeners[chord] = _simMappings[chord].AddOnChangeListener(
				bind(&SimPressCrossUpdate, chord, _id, placeholders::_1));
		}
		return &_simMappings[chord];
	}

	const JSMVariable<EventMapping> *AtSimPress(ButtonID chord) const
	{
		auto existingSim = getSimMap(chord);
		return existingSim ? &existingSim->second : nullptr;
	}

	void ProcessChordRemoval(ButtonID chord, const JSMVariable<EventMapping> *value)
	{
		if (value && value->get().isEmpty())
		{
			auto chordVar = _chordedVariables.find(chord);
			if (chordVar != _chordedVariables.end())
			{
				_chordedVariables.erase(chordVar);
			}
		}
	}

	void ProcessSimPressRemoval(ButtonID chord, const JSMVariable<EventMapping> *value)
	{
		if (value && value->get().isEmpty())
		{
			auto chordVar = _simMappings.find(chord);
			if (chordVar != _simMappings.end())
			{
				_simMappings.erase(chordVar);
			}
		}
	}
};