#pragma once

#include "CmdRegistry.h"
#include <iostream>
#include <memory>
#include <regex>
#include <sstream>
#include <fstream>


JSMCommand::JSMCommand(in_string name)
	: _parse()
	, _help("Error in entering the command. See README for help.")
	, _taskOnDestruction()
	, _name(name)
{}

JSMCommand::~JSMCommand() 
{
	if (_taskOnDestruction)
		_taskOnDestruction(*this);
}

JSMCommand* JSMCommand::SetParser(ParseDelegate parserFunction)
{
	_parse = parserFunction;
	return this;
}

JSMCommand* JSMCommand::SetHelp(in_string commandDescription)
{
	_help = commandDescription;
	return this;
}

unique_ptr<JSMCommand> JSMCommand::GetModifiedCmd(char op, in_string chord)
{
	return nullptr;
}

bool JSMCommand::ParseData(in_string arguments)
{
	_ASSERT_EXPR(_parse, L"There is no function defined to parse this command.");
	if (arguments.compare("HELP") == 0 || !_parse(this, arguments))
	{
		// Parsing has failed. Show help.
		cout << _help << endl;
	}
	return true; // Command is completely processed
}

CmdRegistry::CmdRegistry()
{
}


bool CmdRegistry::loadMappings(in_string fileName) {
	// https://stackoverflow.com/questions/2602013/read-whole-ascii-file-into-c-stdstring
	ifstream file(fileName);
	if (file)
	{
		printf("Loading commands from file %s\n", fileName.c_str());
		// https://stackoverflow.com/questions/6892754/creating-a-simple-configuration-file-and-parser-in-c
		string line;
		while (getline(file, line)) {
			processLine(line);
		}
		file.close();
		return true;
	}
	return false;
}

// https://stackoverflow.com/questions/25345598/c-implementation-to-trim-char-array-of-leading-trailing-white-space-not-workin
void CmdRegistry::strtrim(char* str) {
	int start = 0; // number of leading spaces
	char* buffer = str;
	while (*str && *str++ == ' ') ++start;
	while (*str++); // move to end of string
	auto end = str - buffer - 1;
	while (end > 0 && buffer[end - 1] == ' ') --end; // backup over trailing spaces
	buffer[end] = 0; // remove trailing spaces
	if (end <= start || start == 0) return; // exit if no leading spaces or string is now empty
	str = buffer + start;
	while ((*buffer++ = *str++));  // remove leading spaces: K&R
}

// Add a command to the registry. The regisrty takes ownership of the memory of this pointer.
// You can use _ASSERT() on the return value of this function to make sure the commands are
// accepted.
bool CmdRegistry::Add(JSMCommand* newCommand)
{
	// Check that the pointer is valid, that the name is valid.
	if (newCommand && regex_match(newCommand->_name, regex(R"(^(\+|-|\w+)$)")))
	{
		// Unique pointers automatically delete the pointer on object destruction
		_registry.emplace(newCommand->_name, unique_ptr<JSMCommand>(newCommand));
		return true;
	}
	delete newCommand;
	return false;
}

bool CmdRegistry::findCommandWithName(in_string name, CmdMap::value_type& pair)
{
	return name == pair.first;
}

void CmdRegistry::processLine(string& line)
{
	strtrim(&line[0]);
	if (!line.empty() && line[0] != '#' && !loadMappings(line))
	{
		smatch results;
		string combo, name, arguments, label;
		char op = '\0';
		// Break up the line of text in its relevant parts.
		// Pro tip: use regex101.com to develop these beautiful monstrosities. :P
		// Also, use raw strings R"(...)" to avoid the need to escape characters
		if (regex_match(line, results, regex(R"(^\s*(\w+)\s*([,+]\s*(\w*))?\s*([^#\n]*)(#\s*(.*))?$)")))
		{
			if (results[2].length() > 0)
			{
				combo = results[1];
				op = results[2].str()[0];
				name = results[3];
			}
			else
			{
				name = results[1];
			}

			arguments = results[4];
			label = results[6];
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
				// Any task set to be run on destruction is done here.
			}
			cmd = find_if(++cmd, _registry.end(), bind(&CmdRegistry::findCommandWithName, name, placeholders::_1));
		}

		if (!hasProcessed)
		{
			cout << "Unrecognized command: \"" << line << "\"\nEnter HELP to display all commands." << endl;
		}
	}
	// else ignore empty lines
}

void CmdRegistry::GetCommandList(vector<string>& outList)
{
	outList.clear();
	for (auto& cmd : _registry)
		outList.push_back(cmd.first);
	return;
}

string CmdRegistry::GetHelp(in_string command)
{
	auto cmd = _registry.find(command);
	if (cmd != _registry.end())
	{
		return cmd->second->Help();
	}
	return "";
}

bool JSMMacro::DefaultParser(JSMCommand* cmd, in_string arguments)
{
	// Default macro parser assumes no argument and calls macro when called.
	auto macroCmd = static_cast<JSMMacro*>(cmd);
	// Developper protection to remind you to set a parser.
	_ASSERT_EXPR(macroCmd->_macro, L"No Macro was set for this command.");
	macroCmd->_macro(macroCmd, arguments);
	return true;
}

JSMMacro::JSMMacro(in_string name)
	: JSMCommand(name)
	, _macro()
{
	SetParser(&DefaultParser);
}

JSMMacro* JSMMacro::SetMacro(MacroDelegate macroFunction)
{
	_macro = macroFunction;
	return this;
}