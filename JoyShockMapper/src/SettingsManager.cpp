#include "SettingsManager.h"
#include <algorithm>
#include <ranges>
#include <set>

SettingsManager::SettingsMap SettingsManager::_settings;

bool SettingsManager::add(SettingID id, JSMVariableBase *setting)
{
	return _settings.emplace(id, setting).second;
}


void SettingsManager::resetAllSettings()
{
	static constexpr auto callReset = [](SettingsMap::value_type &kvPair)
	{
		kvPair.second->reset();
	};
	static constexpr auto exceptions = [](SettingsMap::value_type &kvPair)
	{
		static std::set<SettingID> exceptions = {
			SettingID::AUTOLOAD,
			SettingID::JSM_DIRECTORY,
			SettingID::HIDE_MINIMIZED,
			SettingID::VIRTUAL_CONTROLLER,
			SettingID::ADAPTIVE_TRIGGER,
			SettingID::RUMBLE,
		};
		return exceptions.find(kvPair.first) == exceptions.end();
	};
	ranges::for_each(_settings | views::filter(exceptions), callReset);
}
