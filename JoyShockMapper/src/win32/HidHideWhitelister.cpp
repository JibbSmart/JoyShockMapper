#include "Whitelister.h"
#include "HidHideApi.h"
#include <iostream>

class HidHideWhitelister : public Whitelister
{
private:
	std::wstring _fullImageName;
	DWORD _len;
public:
	HidHideWhitelister(bool add = false)
		: Whitelister(add)
		, _fullImageName()
		, _len( 256 )
	{
		std::wstring module(_len, '\0');
		if (!HidHide::Present() || QueryFullProcessImageName(GetCurrentProcess(), 0, &module[0], &_len) == FALSE)
		{
			_len = 0;
		}
		else
		{
			module.resize(_len);
			// Convert the file name into a full image name
			_fullImageName = HidHide::FileNameToFullImageName(module).c_str();
			// Find entries
			auto whitelist{ HidHide::GetWhitelist() };
			for (auto iter = whitelist.begin(); iter != whitelist.end(); ++iter)
			{
				if (_fullImageName == *iter)
				{
					_whitelisted = true;
				}
			} // next entry
		}
	}

	~HidHideWhitelister()
	{
		//Remove();
	}

	virtual bool ShowConsole() override
	{
		std::cout << "Search for \"HidHide Configuration Client\" in your windows search bar." << std::endl;
		return true;
	}

	virtual bool IsAvailable() override
	{
		return HidHide::Present() && _len > 0;
	}

	virtual bool Add(string* optErrMsg = nullptr) override
	{
		if (_len == 0)
		{
			if (optErrMsg) *optErrMsg = "HidHideWhitelister did not initialize properly";
			return false;
		}
		//else if (!HidHide::GetActive())
		//{
		//	if (optErrMsg) *optErrMsg = "HidHide is installed but not active.";
		//	return false;
		//}

		auto whitelist = HidHide::GetWhitelist();

		// Avoid duplicate entries
		for (auto entry : whitelist)
		{
			if (_fullImageName == entry)
			{
				if (optErrMsg)
				{
					*optErrMsg = "JoyShockMapper is already whitelisted";
				}
				return false;
			}
		}

		// No duplicates so add it
		whitelist.push_back(_fullImageName);

		HidHide::SetWhitelist(whitelist);
		_whitelisted = true;
		return true;
	}

	virtual bool Remove(string* optErrMsg = nullptr) override
	{
		if (_len == 0)
		{
			if (optErrMsg) *optErrMsg = "HidHideWhitelister did not initialize properly";
			return false;
		}
		//else if (!HidHide::GetActive())
		//{
		//	if (optErrMsg) *optErrMsg = "HidHide is installed but not active.";
		//	return false;
		//}

		auto whitelist = HidHide::GetWhitelist();

		// Find entries
		for (auto iter = whitelist.begin() ; iter != whitelist.end() ; ++iter)
		{
			if (_fullImageName == *iter)
			{
				// No duplicates so add it
				iter = whitelist.erase(iter);

				HidHide::SetWhitelist(whitelist);
				_whitelisted = false;
				return true;
			}
		}

		if (optErrMsg)
		{
			*optErrMsg = "JoyShockMapper is not whitelisted";
		}
		return false;
	}
};

#include "../src/win32/HidGuardianWhitelister.cpp"

Whitelister* Whitelister::getNew(bool add)
{
	auto hidhide = new HidHideWhitelister(add);
	if (!hidhide->IsAvailable())
	{
		delete hidhide;
		return new HidGuardianWhitelister(add);
	}
	return hidhide;
}