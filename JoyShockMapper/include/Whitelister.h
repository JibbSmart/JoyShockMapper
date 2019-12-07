#pragma once

#include <string>

// A whitelister object adds its PID to a running HIDCerberus service and removes it on destruciton
// as per the RAII design pattern: https://en.cppreference.com/w/cpp/language/raii
class Whitelister
{
public:
	static bool ShowHIDCerberus();
	static bool IsHIDCerberusRunning();

	Whitelister(bool add = false)
		: _whitelisted(false)
	{
		if (add)
		{
			Add();
		}
	}

	~Whitelister()
	{
		Remove();
	}

	bool Add(std::string *optErrMsg = nullptr);
	bool Remove(std::string *optErrMsg = nullptr);

	// Check whether you're whitelisted or not.
	// Could be replaced with an socket query, if one exists.
	inline operator bool()
	{
		return _whitelisted;
	}

private:
	std::string SendToHIDGuardian(std::string command);
	std::string readUrl2(std::string &szUrl, long &bytesReturnedOut, std::string *headerOut);
	void mParseUrl(std::string mUrl, std::string &serverName, std::string &filepath, std::string &filename);
	int getHeaderLength(char *content);

	bool _whitelisted;
};