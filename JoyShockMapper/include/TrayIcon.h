#pragma once

#include "PlatformDefinitions.h"

#include <functional>
#include <type_traits>
#include <utility>

template<typename TrayIconImplementation>
class TrayIconInterface
{
public:
	inline TrayIconInterface(TrayIconData platformData, std::function<void()> &&beforeShow)
	  : implementation_{ platformData, std::move(beforeShow) }
	{
	}

	inline ~TrayIconInterface() = default;

public:
	inline bool Show()
	{
		return implementation_.Show();
	}

	inline bool Hide()
	{
		return implementation_.Hide();
	}

	inline bool SendNotification(const UnicodeString &message)
	{
		return implementation_.SendNotification(message);
	}

	template<typename Callback>
	inline void AddMenuItem(const UnicodeString &label, Callback &&onClick)
	{
		// static_assert(std::is_invocable_r_v<void, Callback> || std::is_invocable_r_v<void, Callback, bool>, "");

		implementation_.AddMenuItem(label, std::forward<Callback>(onClick));
	}

	template<typename ClickCallback, typename StateCallback>
	inline void AddMenuItem(const UnicodeString &label, ClickCallback &&onClick, StateCallback &&getState)
	{
		// static_assert(std::is_invocable_r_v<void, ClickCallback, bool>, "");
		// static_assert(std::is_invocable_r_v<bool, ClickCallback>, "");

		implementation_.AddMenuItem(label, std::forward<ClickCallback>(onClick), std::forward<StateCallback>(getState));
	}

	template<typename Callback>
	inline void AddMenuItem(const UnicodeString &label, const UnicodeString &subLabel, Callback &&onClick)
	{
		implementation_.AddMenuItem(label, subLabel, std::forward<Callback>(onClick));
	}

	inline void ClearMenuMap()
	{
		implementation_.ClearMenuMap();
	}

	inline operator bool()
	{
		return implementation_.operator bool();
	}

private:
	TrayIconImplementation implementation_;
};

#ifdef _WIN32
#include "win32/WindowsTrayIcon.h"
using TrayIcon = TrayIconInterface<WindowsTrayIcon>;
#else
#include "linux/StatusNotifierItem.h"
using TrayIcon = TrayIconInterface<StatusNotifierItem>;
#endif
