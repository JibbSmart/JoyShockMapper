#pragma once

#include "PlatformDefinitions.h"

#include <functional>
#include <type_traits>
#include <utility>

class TrayIcon
{
protected:
	TrayIcon()
	{
	}

public:
	using ClickCallbackType = std::function<void()>;
	using ClickCallbackTypeChecked = std::function<void(bool)>;
	using StateCallbackType = std::function<bool()>;

	virtual ~TrayIcon(){};

	static TrayIcon *getNew(TrayIconData applicationName, std::function<void()> &&beforeShow);

	virtual bool Show() = 0;

	virtual bool Hide() = 0;

	virtual bool SendNotification(const UnicodeString &message) = 0;

	virtual void AddMenuItem(const UnicodeString &label, ClickCallbackType &&onClick) = 0;

	virtual void AddMenuItem(const UnicodeString &label, ClickCallbackTypeChecked &&onClick, StateCallbackType &&getState) = 0;

	virtual void AddMenuItem(const UnicodeString &label, const UnicodeString &subLabel, ClickCallbackType &&onClick) = 0;

	virtual void ClearMenuMap() = 0;

	virtual operator bool() = 0;
};
