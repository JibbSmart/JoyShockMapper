#pragma once

#include "JoyShockMapper.h"
#include "Mapping.h"
#include <sstream>

// Global ID generator
static unsigned int _delegateID = 1;

class JSMVariableBase
{
	public:
	virtual ~JSMVariableBase() = default;

	string_view label() const
	{
		return _label;
	}

	void updateLabel(in_string label)
	{
		_label = label;
	}

	virtual JSMVariableBase *reset() = 0;

private:
	// a user provided label
	string _label;
};

// JSMVariable is a wrapper class for an underlying variable of type T.
// This class allows other parts of the code be notified of when it changes value.
// It also has a default value defined at construction that can be assigned on Reset.
// Finally it has a customizable filter function, to restrict possible values.
// Setter functions return the object itself so they can be chained.
template<typename T>
class JSMVariable : public JSMVariableBase
{
public:
	// A listener function receives the new value as parameter.
	typedef function<void(const T &newVal)> OnChangeDelegate;

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
	static T noFiltering(T old, T nu)
	{
		return nu;
	}

	// Default value of the variable. Cannot be changed after construction.
	const T _defVal;
public:

	JSMVariable(T defaultValue = T())
	  : _value(defaultValue)
	  , _onChangeListeners()
	  , _filter(&noFiltering) // _filter is always valid
	  , _defVal(defaultValue)
	{
	}

	// Make a copy with a different default value
	JSMVariable(const JSMVariable &copy, T defaultValue)
	  : _value(defaultValue)
	  , _onChangeListeners() // Don't copy listeners. This is a different variable!
	  , _filter(copy._filter)
	  , _defVal(defaultValue)
	{
	}

	virtual ~JSMVariable()
	{
		_onChangeListeners.clear();
	}

	// Sets the filtering function for this variable. Also applies
	// the filter to it's current value.
	virtual JSMVariable *setFilter(FilterDelegate filterfunction)
	{
		_filter = filterfunction;
		set(_value); // Run the new filter on current value.
		return this;
	}

	// Remember to call this listener when the value changes.
	virtual unsigned int addOnChangeListener(OnChangeDelegate listener, bool callListener = false)
	{
		_onChangeListeners[_delegateID] = listener;
		if (callListener)
		{
			_onChangeListeners[_delegateID](_value);
		}
		return _delegateID++;
	}

	// Remove the listener from list
	virtual bool removeOnChangeListener(unsigned int id)
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
	JSMVariable *reset() override
	{
		// Use operator to enable notification
		set(_defVal);
		return this;
	}

	// Value can be read by implicit or explicit casting
	// This enables easy usage of the variable within an operation.
	virtual operator T() const
	{
		return _value;
	}

	virtual const T &value() const
	{
		return _value;
	}

	const T &defaultValue() const
	{
		return _defVal;
	}

	// Value can be written by using set()
	// N.B.: It's important to always use either function
	// for changing the member _value
	virtual T set(T newValue)
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
	{
	}

	// Get the chorded variable, creating one if required.
	JSMVariable<T> *atChord(ButtonID chord)
	{
		auto existingChord = _chordedVariables.find(chord);
		if (existingChord == _chordedVariables.end())
		{
			// Create the chord when requested, using the copy constructor.
			_chordedVariables.emplace(chord, JSMVariable<T>(*this, Base::_defVal));
		}
		return &_chordedVariables[chord];
	}

	const JSMVariable<T> *atChord(ButtonID chord) const
	{
		auto existingChord = _chordedVariables.find(chord);
		return existingChord != _chordedVariables.end() ? &existingChord->second : nullptr;
	}

	// Obtain the value with provided chord if any.
	optional<T> chordedValue(ButtonID chord) const
	{
		if (chord > ButtonID::NONE)
		{
			auto existingChord = _chordedVariables.find(chord);
			return existingChord != _chordedVariables.end() ? make_optional<T>(existingChord->second) : nullopt;
		}
		return chord != ButtonID::INVALID ? make_optional(Base::_value) : nullopt;
	}
	virtual operator T() const
	{
		return Base::value();
	}

	virtual const T &value() const
	{
		return Base::value();
	}

	// Resetting a chorded var always clears all chords.
	virtual ChordedVariable<T> *reset() override
	{
		JSMVariable<T>::reset();
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
	{
	}

	virtual T set(T baseValue) override
	{
		return JSMVariable<T>::set(baseValue);
	}

	inline void markModeshiftForRemoval(ButtonID modeshift)
	{
		_chordToRemove = modeshift;
	}

	void processModeshiftRemoval(ButtonID modeshift)
	{
		if (_chordToRemove == modeshift)
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
typedef pair<const ButtonID, JSMVariable<Mapping>> ComboMap;

// Button mappings includes not only chorded mappings, but also simultaneous press mappings
class JSMButton : public ChordedVariable<Mapping>
{
public:
	// Identifier of the variable. Cannot be changed after construction.
	const ButtonID _id;

protected:
	map<ButtonID, JSMVariable<Mapping>> _simMappings;

	// Store listener IDs for its sim presses. This is required for Cross updates
	map<ButtonID, unsigned int> _simListeners;

public:
	JSMButton(ButtonID id, Mapping def)
	  : ChordedVariable(def)
	  , _id(id)
	  , _simMappings()
	  , _simListeners()
	{
	}

	virtual ~JSMButton()
	{
		for (auto id : _simListeners)
		{
			_simMappings[id.first].removeOnChangeListener(id.second);
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
	inline bool hasSimMappings() const
	{
		return !_simMappings.empty();
	}

	virtual Mapping set(Mapping baseValue) override
	{
		return JSMVariable<Mapping>::set(baseValue);
	}

	// Returns the display name of the chorded press if provided, or itself
	string getName(ButtonID chord = ButtonID::NONE) const
	{
		stringstream ss;
		if (chord > ButtonID::NONE)
		{
			ss << chord << ',' << _id;
			return ss.str();
		}
		else if (chord != ButtonID::INVALID)
		{
			ss << _id;
			return ss.str();
		}
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
	virtual JSMButton *reset() override
	{
		ChordedVariable<Mapping>::reset();
		for (auto id : _simListeners)
		{
			_simMappings[id.first].removeOnChangeListener(id.second);
		}
		_simMappings.clear();
		return this;
	}

	// Get the SimPress variable, creating one if required.
	// An additional listener is required for the complementary sim press
	// to be updated when this value changes.
	JSMVariable<Mapping> *atSimPress(ButtonID chord)
	{
		auto existingSim = getSimMap(chord);
		if (!existingSim)
		{
			JSMVariable<Mapping> var(*this, Mapping());
			_simMappings.emplace(chord, var);
			_simListeners[chord] = _simMappings[chord].addOnChangeListener(
			  bind(&SimPressCrossUpdate, chord, _id, placeholders::_1));
		}
		return &_simMappings[chord];
	}

	const JSMVariable<Mapping> *atSimPress(ButtonID chord) const
	{
		auto existingSim = getSimMap(chord);
		return existingSim ? &existingSim->second : nullptr;
	}

	void processChordRemoval(ButtonID chord, const JSMVariable<Mapping> *value)
	{
		if (value && value->value() == Mapping::NO_MAPPING)
		{
			auto chordVar = _chordedVariables.find(chord);
			if (chordVar != _chordedVariables.end())
			{
				_chordedVariables.erase(chordVar);
			}
		}
	}

	void processSimPressRemoval(ButtonID chord, const JSMVariable<Mapping> *value)
	{
		if (value && value->value() == Mapping::NO_MAPPING)
		{
			auto chordVar = _simMappings.find(chord);
			if (chordVar != _simMappings.end())
			{
				_simMappings.erase(chordVar);
			}
		}
	}
};
