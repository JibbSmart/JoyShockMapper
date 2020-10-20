#pragma once

#include "JoyShockMapper.h"
#include "CmdRegistry.h" // for JSMCommand
#include "JSMVariable.hpp"
#include "PlatformDefinitions.h"

#include <iostream>
#include <regex>

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

	// The display name is usually the same as the name, but in some cases it might be different.
	// For example the two GYRO_SENS assignment commands will display MIN_GYRO_SENS and MAX_GYRO_SENS respectively.
	const string _displayName;

	virtual bool ParseData(in_string arguments) override
	{
		smatch results;
		_ASSERT_EXPR(_parse, L"There is no function defined to parse this command.");
		if (arguments.compare(0, 4, "HELP") == 0)
		{
			// Parsing has failed. Show help.
			cout << _help << endl;
		}
		else if (arguments.empty() || regex_match(arguments, results, regex(R"(\s*=\s*(.*))")))
		{
			string fwd_args(results.empty() ? arguments : results[1].str());
			if (!_parse(this, fwd_args))
			{
				cout << _help << endl; // Parsing has failed. Show help.
			}
		}
		else
		{
			// If no if case processed the command,; it has been entered wrong.
			return false;
		}
		return true; // Command is completely processed
	}

	static bool ModeshiftParser(ButtonID modeshift, JSMSetting<T> *setting, JSMCommand::ParseDelegate parser, JSMCommand* cmd, in_string argument)
	{
		if (setting && argument.compare("NONE") == 0)
		{
			setting->MarkModeshiftForRemoval(modeshift);
			cout << "Modeshift " << modeshift << "," << setting->_id << " has been removed." << endl;
			return true;
		}
		return parser(cmd, argument);
	}

	// The default parser uses the overloaded >> operator to parse
	// any base type. Custom types can also be extracted if you define
	// a static parse operation for it.
	static bool DefaultParser(JSMCommand* cmd, in_string data)
	{
		auto inst = dynamic_cast<JSMAssignment<T>*>(cmd);
		if (data.empty())
		{
			//No assignment? Display current assignment
			cout << inst->_displayName << " = " << inst->_var.get() << endl;
			return true;
		}

		stringstream ss(data);
		// Read the value
		T value = T();
		ss >> value;
		if (!ss.fail())
		{
			T oldVal = inst->_var;
			inst->_var = value;

			// The assignment won't trigger my listener DisplayNewValue if
			// the new value after filtering is the same as the old.
			if (oldVal == inst->_var.get())
			{
				// So I want to do it myself.
				inst->DisplayNewValue(inst->_var);
			}

			// Command succeeded if the value requested was the current one
			// or if the new value is different from the old.
			return value == oldVal || inst->_var.get() != oldVal; // Command processed successfully
		}
		// Couldn't read the value
		return false;
	}

	void DisplayNewValue(T newValue)
	{
		// See Specialization for T=Mapping at the end of this file
		cout << _displayName << " has been set to " << newValue << endl;
	}

	virtual unique_ptr<JSMCommand> GetModifiedCmd(char op, in_string chord) override
	{
		stringstream ss(chord);
		ButtonID btn;
		ss >> btn;
		if (btn > ButtonID::NONE)
		{
			if (op == ',')
			{
				auto settingVar = dynamic_cast<JSMSetting<T>*>(&_var);
				if (settingVar)
				{
					//Create Modeshift
					string name = chord + op + _displayName;
					unique_ptr<JSMCommand> chordAssignment(new JSMAssignment<T>(name, *settingVar->AtChord(btn)));
					chordAssignment->SetHelp(_help)->SetParser(bind(&JSMAssignment<T>::ModeshiftParser, btn, settingVar, _parse, placeholders::_1, placeholders::_2))
						->SetTaskOnDestruction(bind(&JSMSetting<T>::ProcessModeshiftRemoval, settingVar, btn));
					return chordAssignment;
				}
				auto buttonVar = dynamic_cast<JSMButton*>(&_var);
				if (buttonVar && btn > ButtonID::NONE)
				{
					string name = chord + op + _displayName;
					auto chordedVar = buttonVar->AtChord(btn);
					// The reinterpret_cast is required for compilation, but settings will never run this code anyway.
					unique_ptr<JSMCommand> chordAssignment(new JSMAssignment<T>(name, reinterpret_cast<JSMVariable<T>&>(*chordedVar)));
					chordAssignment->SetHelp(_help)->SetParser(_parse)->SetTaskOnDestruction(bind(&JSMButton::ProcessChordRemoval, buttonVar, btn, chordedVar));
					// BE ADVISED! If a custom parser was set using bind(), the very same bound vars will
					// be passed along.
					return chordAssignment;
				}
			}
			else if (op == '+')
			{
				auto buttonVar = dynamic_cast<JSMButton*>(&_var);
				if (buttonVar && btn > ButtonID::NONE)
				{
					string name = chord + op + _displayName;
					auto simPressVar = buttonVar->AtSimPress(btn);
					unique_ptr<JSMCommand> simAssignment(new JSMAssignment<Mapping>(name, *simPressVar));
					simAssignment->SetHelp(_help)->SetParser(_parse)->SetTaskOnDestruction(bind(&JSMButton::ProcessSimPressRemoval, buttonVar, btn, simPressVar));
					// BE ADVISED! If a custom parser was set using bind(), the very same bound vars will
					// be passed along.
					return simAssignment;
				}
			}
		}
		return JSMCommand::GetModifiedCmd(op, chord);
	}

	unsigned int _listenerId;

public:
	JSMAssignment(in_string name, in_string displayName, JSMVariable<T>& var)
		: JSMCommand(name)
		, _var(var)
		, _displayName(displayName)
		, _listenerId(0)
	{
		// Child Classes assign their own parser. Use bind to convert instance function call
		// into a static function call.
		SetParser(&JSMAssignment::DefaultParser);
		_listenerId = _var.AddOnChangeListener(bind(&JSMAssignment::DisplayNewValue, this, placeholders::_1));
	}

	JSMAssignment(in_string name, JSMVariable<T>& var)
		: JSMAssignment(name, name, var)
	{ }

	JSMAssignment(JSMSetting<T>& var)
		: JSMAssignment(magic_enum::enum_name(var._id).data(), var)
	{ }

	JSMAssignment(JSMButton& var)
		: JSMAssignment(magic_enum::enum_name(var._id).data(), var)
	{ }

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
	cout << _name << " mapped to " << newValue.description << endl;
}
