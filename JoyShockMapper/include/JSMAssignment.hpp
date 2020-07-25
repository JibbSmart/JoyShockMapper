#pragma once

#include "JoyShockMapper.h"
#include "CmdRegistry.h" // for JSMCommand
#include "JSMVariable.hpp"
#include <iostream>

// This class handles any kind of assignment command by binding to a JSM variable
// of the parameterized type T. If T is not a base type, implement the following
// functions to reuse the DefaultParser:
// static istream &operator >>(istream & input, T &foobar) { ... }
// static ostream &operator <<(ostream & output, T foobar) { ... }
template<typename T>
class JSMAssignment : public JSMCommand
{
protected:
	// Reference to an existing variable. Make sure the variable's lifetime is longer than
	// the command objects
	JSMVariable<T>& _var;

	// The default parser uses the overloaded >> operator to parse
	// any base type. Custom types can also be extracted if you define
	// a static parse operation for it.
	static bool DefaultParser(JSMCommand* cmd, in_string data)
	{
		auto inst = dynamic_cast<JSMAssignment<T>*>(cmd);
		stringstream ss(data);
		// Ignore whitespaces until the '=' is found
		char c;
		do {
			ss >> c;
		} while (ss.good() && c != '=');
		if (!ss.good())
		{
			//No assignment? Display current assignment
			cout << inst->_name << " = " << (T)inst->_var << endl;
			return true;
		}
		if (c == '=')
		{
			// Read the value
			T value = T();
			ss >> value;
			if (!ss.fail())
			{
				T oldVal = inst->_var;
				inst->_var = value;
				// Command succeeded if the value requested was the current one
				// or if the new value is different from the old.
				return value == oldVal || inst->_var != oldVal; // Command processed successfully
			}
			// Couldn't read the value
		}
		// Not an equal sign? The command is entered wrong!
		return false;
	}

	void DisplayNewValue(T newValue)
	{
		cout << _name << " has been set to " << newValue << endl;
	}

	virtual unique_ptr<JSMCommand> GetModifiedCmd(char op, in_string chord) override
	{
		auto btn = magic_enum::enum_cast<ButtonID>(chord);
		if (btn && op == ',')
		{
			auto chordedVar = dynamic_cast<ChordedVariable<T>*>(&_var);
			if (chordedVar && int(*btn) >= 0)
			{
				string name = chord + op + _name;
				unique_ptr<JSMCommand> chordAssignment(new JSMAssignment<T>(name, chordedVar->AtChord(*btn)));
				chordAssignment->SetHelp(_help)->SetParser(_parse);
				// BE ADVISED! If a custom parser was set using bind(), the very same bound vars will
				// be passed along.
				return chordAssignment;
			}
		}
		else if (btn && op == '+')
		{
			auto mappingVar = dynamic_cast<JSMButton*>(&_var);
			if (mappingVar && int(*btn) >= 0)
			{
				string name = chord + op + _name;
				unique_ptr<JSMCommand> simAssignment(new JSMAssignment<Mapping>(name, mappingVar->AtSimPress(*btn)));
				simAssignment->SetHelp(_help)->SetParser(_parse);
				// BE ADVISED! If a custom parser was set using bind(), the very same bound vars will
				// be passed along.
				return simAssignment;
			}
		}
		return JSMCommand::GetModifiedCmd(op, chord);
	}

	// JSMVariable<T>::OnChangeDelegate _displayNewValue;
	unsigned int _listenerId;

public:
	JSMAssignment(in_string name, JSMVariable<T>& var)
		: JSMCommand(name)
		, _var(var)
		, _listenerId(0)
	{
		// Child Classes assign their own parser. Use bind to convert instance function call
		// into a static function call.
		SetParser(&JSMAssignment::DefaultParser);
		_listenerId = _var.AddOnChangeListener(bind(&JSMAssignment::DisplayNewValue, this, placeholders::_1));
	}

	virtual ~JSMAssignment()
	{
		_var.RemoveOnChangeListener(_listenerId);
	}

	// This setter enables custom parsers to perform assignments
	inline T operator =(T newVal)
	{
		return (_var = newVal);
	}
};

// Specialization for Mapping
template<>
void JSMAssignment<Mapping>::DisplayNewValue(Mapping newValue)
{
	if (newValue.holdBind)
	{
		cout << "Tap " << _name << " mapped to " << newValue.pressBind.name << endl
			<< "Hold " << _name << " mapped to " << newValue.holdBind.name << endl;
	}
	else
	{
		cout << _name << " mapped to " << newValue.pressBind.name << endl;
	}
}