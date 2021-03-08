#pragma once

#include "JoyShockMapper.h"

#include <functional>
#include <map>
#include <memory>
#include <string_view>

// This is a base class for any Command line operation. It binds a command name to a parser function
// Derivatives from this class have a default parser function and performs specific operations.
class JSMCommand
{
public:
	// A Parser function has a pointer to the command being processed and
	// the data string to process. It returns whether the data was recognized or not.
	typedef function<bool(JSMCommand* cmd, in_string& data)> ParseDelegate;

	// Assignments can be given tasks to perform before destroying themselves.
	// This is used for chorded press, sim presses and modeshifts to remove
	// themselves from their host variable when assigned NONE
	typedef function<void(JSMCommand& me)> TaskOnDestruction;

protected:
	// Parse functor to be assigned by derived class or overwritten
	// Use setter to assign
	ParseDelegate _parse;

	// Help string to display about the command. Cannot be changed after construction
	string _help;

	// Some task to perform when this object is destroyed
	TaskOnDestruction _taskOnDestruction;

public:
	// Name of the command. Cannot be changed after construction.
	// I don't mind leaving this public since it can't be changed.
	const string _name;

	// The constructor should only have mandatory arguments. optional arguments are assigned using setters.
	JSMCommand(in_string name);

	virtual ~JSMCommand();

	// A parser needs to be assigned for the command to be run without crashing.
	// It returns a pointer to itself for setter chaining.
	virtual JSMCommand* SetParser(ParseDelegate parserFunction);

	// Assign a help description to the command.
	// It returns a pointer to itself for setter chaining.
	virtual JSMCommand* SetHelp(in_string commandDescription);

	virtual unique_ptr<JSMCommand> GetModifiedCmd(char op, in_string chord);

	// Get the help string for the command.
	inline string Help() const
	{
		return _help;
	}

	// Set a task to perform when this command is destroyed
	inline JSMCommand* SetTaskOnDestruction(const TaskOnDestruction& task)
	{
		_taskOnDestruction = task;
		return this;
	}

	// Request this command to parse the command arguments. Returns true if the command was processed.
	virtual bool ParseData(in_string arguments);
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

	static string_view strtrim(std::string_view str);

	static bool findCommandWithName(in_string name, CmdMap::value_type& pair);

public:
	CmdRegistry();

	// Not in_string because the string is modified inside
	bool loadConfigFile(string fileName);

	// Add a command to the registry. The regisrty takes ownership of the memory of this pointer.
	// You can use _ASSERT() on the return value of this function to make sure the commands are
	// accepted.
	bool Add(JSMCommand* newCommand);

	bool Remove(in_string name);

	bool hasCommand(in_string name) const;

	bool isCommandValid(in_string line);

	// Process a command entered by the user
	// intentionally dont't use const ref
	void processLine(const string& line);

	// Fill vector with registered command names
	void GetCommandList(vector<string>& outList);

	// Return help string for provided command
	string GetHelp(in_string command);
};

// Macro commands are simple function calls when recognized. But it could do different things
// by processing arguments given to it.
class JSMMacro : public JSMCommand
{
public:
	// A Macro function has it's command object passed as argument.
	typedef function<bool(JSMMacro* macro, in_string arguments)> MacroDelegate;

protected:
	// The macro function to call when the command is recognized.
	MacroDelegate _macro;

	// The default parser for the command processes no arguments.
	static bool DefaultParser(JSMCommand* cmd, in_string arguments);

public:
	JSMMacro(in_string name);

	// Assign a Macro function to run. It returns a pointer to itself
	// to chain setters.
	JSMMacro* SetMacro(MacroDelegate macroFunction);
};
