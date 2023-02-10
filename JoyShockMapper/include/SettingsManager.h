#pragma once

#include "JoyShockMapper.h"
#include "JSMVariable.hpp"
#include <unordered_map>

class SettingsManager
{
public:
	SettingsManager() = delete;

	static bool add(SettingID id, JSMVariableBase *setting);

	template <typename T>
	static constexpr bool add(JSMSetting<T> *setting)
	{
		return add(setting->_id, setting);
	}

	template<typename T>
	static JSMSetting<T> *get(SettingID id)
	{
		auto base = _settings.find(id);
		if (base != _settings.end())
		{
			return dynamic_cast<JSMSetting<T> *>(base->second.get());
		}
		return nullptr;
	}

	template<typename T>
	static JSMVariable<T> *getV(SettingID id)
	{
		auto base = _settings.find(id);
		if (base != _settings.end())
		{
			return dynamic_cast<JSMVariable<T> *>(base->second.get());
		}
		return nullptr;
	}

	static void resetAllSettings();

private:
	using SettingsMap = unordered_map<SettingID, shared_ptr<JSMVariableBase>>;
	static SettingsMap _settings;
};
