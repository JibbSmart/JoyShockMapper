#include "CmdRegistry.h"
#include "PlatformDefinitions.h"

#include <cctype>
#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <fstream>

JSMCommand::JSMCommand(in_string name)
  : _parse()
  , _help("Enter README to bring up the user manual.")
  , _taskOnDestruction()
  , _name(name)
{
}

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

bool JSMCommand::ParseData(in_string arguments, in_string label)
{
	_ASSERT_EXPR(_parse, L"There is no function defined to parse this command.");
	if (arguments.compare("HELP") == 0)
	{
		// Parsing has failed. Show help.
		COUT << _help << endl;
	}
	else if (!_parse(this, arguments, label))
	{
		CERR << _help << endl;
	}
	return true; // Command is completely processed
}

CmdRegistry::CmdRegistry()
{
	NONAME = { 0b01001011, 0b01001111 };
}

bool CmdRegistry::loadConfigFile(string fileName)
{
	// https://stackoverflow.com/questions/2602013/read-whole-ascii-file-into-c-stdstring
	auto comment = fileName.find_first_of('#');
	if (comment != string::npos)
	{
		fileName = fileName.substr(0, comment - 1);
	}
	// Trim away quotation marks from drag and drop
	if (*fileName.begin() == '\"' && *(fileName.end() - 1) == '\"')
		fileName = fileName.substr(1, fileName.size() - 2);

	ifstream file(fileName);
	if (!file.is_open())
	{
		file.open(std::string{ BASE_JSM_CONFIG_FOLDER() } + fileName);
	}
	if (file)
	{
		COUT << "Loading commands from file ";
		COUT_INFO << fileName << endl;
		// https://stackoverflow.com/questions/6892754/creating-a-simple-configuration-file-and-parser-in-c
		string line;
		while (getline(file, line))
		{
			processLine(line);
		}
		file.close();
		return true;
	}
	return false;
}

string_view CmdRegistry::strtrim(std::string_view str)
{
	if (str.empty())
		return {};

	while (isspace(str[0]))
	{
		str.remove_prefix(1);
		if (str.empty())
			return {};
	}

	while (isspace(str.back()))
	{
		str.remove_suffix(1);
		if (str.empty())
			return {};
	}

	return str;
}

// Add a command to the registry. The regisrty takes ownership of the memory of this pointer.
// You can use _ASSERT() on the return value of this function to make sure the commands are
// accepted.
bool CmdRegistry::add(JSMCommand* newCommand)
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

bool CmdRegistry::Remove(in_string name)
{
	// If I allow multiple commands with the same name, I should have a way to specify which one I want to remove.
	CmdMap::iterator cmd = find_if(_registry.begin(), _registry.end(), bind(&CmdRegistry::findCommandWithName, name, placeholders::_1));
	if (cmd != _registry.end())
	{
		_registry.erase(cmd);
		return true;
	}
	return false;
}

bool CmdRegistry::findCommandWithName(in_string name, const CmdMap::value_type& pair)
{
	return name == pair.first;
}

bool CmdRegistry::isCommandValid(in_string line) const
{
	ifstream file(line.data());
	if (file.is_open())
	{
		file.close();
		return true;
	}
	smatch results;
	string combo, name, arguments, label;
	char op = '\0';
	const string lineStr(line);
	if (regex_match(lineStr, results, regex(R"(^\s*([+-]?\w*)\s*([,+]\s*([+-]?\w*))?\s*([^#\n]*)(#\s*(.*))?$)")))
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
	CmdMap::const_iterator cmd = find_if(_registry.cbegin(), _registry.cend(), bind(&CmdRegistry::findCommandWithName, name, placeholders::_1));
	return cmd != _registry.end();
}

void CmdRegistry::processLine(const string& line)
{
	auto trimmedLine = std::string{ strtrim(line) };

	if (!trimmedLine.empty() && trimmedLine.front() != '#' && !loadConfigFile(trimmedLine))
	{
		smatch results;
		string combo, name, arguments, label;
		char op = '\0';
		// Break up the line of text in its relevant parts.
		// Pro tip: use regex101.com to develop these beautiful monstrosities. :P
		// Also, use raw strings R"(...)" to avoid the need to escape characters
		// I dislike having to code in exception for + and - buttons not being \w characters
		if (regex_match(trimmedLine, results, regex(R"(^\s*([+-]?\w*)\s*([,+]\s*([+-]?\w*))?\s*([^#\n]*)(#\s*(.*))?$)")))
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
				hasProcessed |= cmd->second->ParseData(arguments, label);
			}
			else
			{
				auto modCommand = cmd->second->GetModifiedCmd(op, combo);
				if (modCommand)
				{
					hasProcessed |= modCommand->ParseData(arguments, label);
				}
				// Any task set to be run on destruction is done here.
			}
			cmd = find_if(++cmd, _registry.end(), bind(&CmdRegistry::findCommandWithName, name, placeholders::_1));
		}

		if (!hasProcessed)
		{
			CERR << "Unrecognized command: \"" << trimmedLine << "\"\nEnter ";
			COUT_INFO << "HELP";
			CERR << " to display all commands." << endl;
		}
	}
	// else ignore empty lines
}

void CmdRegistry::GetCommandList(vector<string_view>& outList) const
{
	outList.clear();
	for (auto& cmd : _registry)
		outList.push_back(cmd.first);
	return;
}

bool CmdRegistry::hasCommand(in_string name) const
{
	return _registry.find(name) != _registry.end();
}

string_view CmdRegistry::GetHelp(in_string command) const
{
	auto cmd = _registry.find(command);
	if (cmd != _registry.end())
	{
		return cmd->second->Help();
	}
	return "";
}

bool JSMMacro::DefaultParser(JSMCommand* cmd, in_string arguments, in_string label)
{
	// Default macro parser assumes no argument and calls macro when called.
	auto macroCmd = static_cast<JSMMacro*>(cmd);
	// Developper protection to remind you to set a parser.
	_ASSERT_EXPR(macroCmd->_macro, L"No Macro was set for this command.");
	if (!macroCmd->_macro(macroCmd, arguments) && !macroCmd->_help.empty())
	{
		COUT << macroCmd->_help << endl;
		COUT << "The "; // Parsing has failed. Show help.
		COUT_INFO << "README";
		COUT << " command can lead you to further details on this command." << endl;
	}
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
