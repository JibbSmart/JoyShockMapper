#pragma once

#include "JoyShockMapper.h"
#include <functional>
#include <iostream>
#include <memory>
#include <map>
#include <regex>
#include <sstream>


// This is a base class for any Command line operation. It binds a command name to a parser function
// Derivatives from this class have a default parser function and performs specific operations.
class JSMCommand {
public:
	// A Parser function has a pointer to the command being processed and
	// the data string to process. It returns whether the data was recognized or not.
	typedef function<bool(JSMCommand *cmd, in_string &data)> ParseDelegate;

protected:
	// Parse functor to be assigned by derived class or overwritten
	// Use setter to assign
	ParseDelegate _parse;

	// Help string to display about the command. Cannot be changed after construction
	string _help;
public:
	// Name of the command. Cannot be changed after construction.
	// I don't mind leaving this public since it can't be changed.
	const string _name;

	// The constructor should only have mandatory arguments. optional arguments are assigned using setters.
	JSMCommand(in_string name)
		: _name(name)
		, _help("Error in entering the command. See README for help.")
		, _parse()
	{}

	virtual ~JSMCommand() {}

	// A parser needs to be assigned for the command to be run without crashing.
	// It returns a pointer to itself for setter chaining.
	virtual JSMCommand *SetParser(ParseDelegate parserFunction)
	{
		_parse = parserFunction;
		return this;
	}

	// Assign a help description to the command.
	// It returns a pointer to itself for setter chaining.
	virtual JSMCommand *SetHelp(in_string commandDescription)
	{
		_help = commandDescription;
		return this;
	}

	virtual unique_ptr<JSMCommand> GetModifiedCmd(char op, in_string chord)
	{
		return nullptr;
	}

	// Get the help string for the command.
	inline string Help()
	{
		return _help;
	}

	// Request this command to parse the command arguments. Returns true if the command was processed.
	// Derived classes should assign their own parser rather than override this.
	virtual bool ParseData(in_string arguments) final
	{
		_ASSERT_EXPR(_parse, L"There is no function defined to parse this command.");
		if (!_parse(this, arguments))
		{
			// Parsing has failed. Show help.
			cout << _help << endl;
		}
		return true; // Command is completely processed
	}
};

// The command registry holds all JSMCommands object and should not care what the derived type is. 
// It's capable of recognizing a command and requesting it to process arguments. That's it.
// It uses regular expression to breakup a command string in its various components.
// Currently it refuses to accept different commands with the same name but there's an 
// argument to be made to use the return value of JSMCommand::ParseData() to attempt multiple
// commands until one returns true. This can enable multiple parsers for the same command.
class CmdRegistry
{
private:
	typedef multimap<string, unique_ptr<JSMCommand>> CmdMap;

	// multimap allows multiple entries with the same keys
	CmdMap _registry;

public:
	CmdRegistry()
	{
	}

	// Add a command to the registry. The regisrty takes ownership of the memory of this pointer.
	// You can use _ASSERT() on the return value of this function to make sure the commands are
	// accepted.
	bool Add(JSMCommand *newCommand)
	{
		// Check that the pointer is valid, that the name is valid.
		if (newCommand && regex_match(newCommand->_name, regex(R"(^(\+|-|\w+)$)")) )
		{
			// Unique pointers automatically delete the pointer on object destruction
			_registry.emplace(newCommand->_name, unique_ptr<JSMCommand>(newCommand));
			return true;
		}
		delete newCommand;
		return false;
	}

	static bool findCommandWithName(in_string name, CmdMap::value_type &pair)
	{
		return name == pair.first;
	}

	// Process a command entered by the user
	void processLine(in_string line)
	{
		if (!line.empty())
		{
			smatch results;
			string combo, name, arguments, label;
			char op;
			// Break up the line of text in its relevant parts.
			// Pro tip: use regex101.com to develop these beautiful monstrosities. :P
			// Also, use raw strings R"(...)" to avoid the need to escape characters
			if (regex_match(line, results, regex(R"(^ *(\w+) *([,\+]? *\w*)( *=? *[^#\n]*)#? *(.*)$)")))
			{
				if (results[2].length() > 0)
				{
					combo = results[1];
					op = results[2].str()[0];
					name = results[2].str().substr(1);
				}
				else
				{
					name = results[1];
					combo = results[2];
				}
				arguments = results[3];
				label = results[4];
			}

			bool hasProcessed = false;
			CmdMap::iterator cmd = find_if(_registry.begin(), _registry.end(), bind(&CmdRegistry::findCommandWithName, name, placeholders::_1));
			while (cmd != _registry.end())
			{
				if (combo.empty())
				{
					hasProcessed |= cmd->second->ParseData(arguments);
				}
				else
				{
					auto modCommand = cmd->second->GetModifiedCmd(op, combo);
					if (modCommand)
					{
						hasProcessed |= modCommand->ParseData(arguments);
					}
				}
				cmd = find_if(++cmd, _registry.end(), bind(&CmdRegistry::findCommandWithName, name, placeholders::_1));
			}

			if (!hasProcessed)
			{
				cout << "Unrecognized command. Enter HELP to display all commands." << endl;
			}
		}
		// else ignore empty lines
	}

	// Fill vector with registered command names
	void GetCommandList(vector<string> &outList)
	{
		outList.clear();
		for (auto &cmd : _registry)
			outList.push_back(cmd.first);
		return;
	}

	// Return help string for provided command
	string GetHelp(in_string command)
	{
		auto cmd = _registry.find(command);
		if (cmd != _registry.end())
		{
			return cmd->second->Help();
		}
		return "";
	}
};

// Macro commands are simple function calls when recognized. But it could do different things
// by processing arguments given to it.
class JSMMacro : public JSMCommand
{
public:
	// A Macro function has it's command object passed as argument.
	typedef function<void(JSMMacro *macro, in_string arguments)> MacroDelegate;

protected:
	// The macro function to call when the command is recognized.
	MacroDelegate _macro;

	// The default parser for the command processes no arguments.
	static bool DefaultParser(JSMCommand *cmd, in_string arguments)
	{
		// Default macro parser assumes no argument and calls macro when called.
		auto macroCmd = static_cast<JSMMacro *>(cmd);
		// Developper protection to remind you to set a parser.
		_ASSERT_EXPR(macroCmd->_macro, L"No Macro was set for this command.");
		macroCmd->_macro(macroCmd, arguments);
		return true;
	}

public:
	JSMMacro(in_string name)
		: JSMCommand(name)
		, _macro()
	{
		SetParser(&DefaultParser);
	}

	// Assign a Macro function to run. It returns a pointer to itself 
	// to chain setters.
	JSMMacro *SetMacro(MacroDelegate macroFunction)
	{
		_macro = macroFunction;
		return this;
	}
};


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
	JSMVariable<T> &_var;

	// The default parser uses the overloaded >> operator to parse
	// any base type. Custom types can also be extracted if you define
	// a static parse operation for it.
	static bool DefaultParser(JSMCommand *cmd, in_string data)
	{
		auto inst = dynamic_cast<JSMAssignment<T> *>(cmd);
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
		if (op == ',')
		{
			ButtonID btn;
			stringstream(chord) >> btn;
			auto chordedVar = dynamic_cast<ChordedVariable<T>*>(&_var);
			if (chordedVar && int(btn) >= 0)
			{
				string name = chord + "," + _name;
				unique_ptr<JSMCommand> chordAssignment(new JSMAssignment<T>(name, chordedVar->CreateChord(btn)));
				chordAssignment->SetHelp(_help)->SetParser(_parse);
				// BE ADVISED! If a custom parser was set using bind(), the very same bound vars will
				// be passed along.
				return chordAssignment;
			}
		}
		return JSMCommand::GetModifiedCmd(op, chord);
	}

	// JSMVariable<T>::OnChangeDelegate _displayNewValue;
	unsigned int _listenerId;

public:
	JSMAssignment(in_string name, JSMVariable<T> &var)
		: JSMCommand(name)
		, _var(var)
		, _listenerId( 0 )
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

	// This getter enables reading the variable via the Command object.
	// This could not be required since the Variables are usually global anyway
	// And could just be captured in a lambda if required.
	//inline operator T() const
	//{
	//	return _var;
	//}
};

template<>
void JSMAssignment<Mapping>::DisplayNewValue(Mapping newValue)
{
	if (newValue.holdBind)
	{
		cout << "Tap " << _name << " mapped to " << char(newValue.pressBind) << endl
			<< "Hold " << _name << " mapped to " << char(newValue.holdBind) << endl;
	}
	else
	{
		cout << _name << " mapped to " << char(newValue.pressBind) << endl;
	}
}