#pragma once

#include <string>

using namespace std;

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

	bool Add(string *optErrMsg = nullptr);
	bool Remove(string *optErrMsg = nullptr);

	// Check whether you're whitelisted or not.
	// Could be replaced with an socket query, if one exists.
	inline operator bool()
	{
		return _whitelisted;
	}

private:
	string SendToHIDGuardian(string command);
	string readUrl2(string &szUrl, long &bytesReturnedOut, string *headerOut);
	void mParseUrl(string mUrl, string &serverName, string &filepath, string &filename);
	int getHeaderLength(char *content);

	bool _whitelisted;
};