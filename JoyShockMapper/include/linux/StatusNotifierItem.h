#pragma once

#include "PlatformDefinitions.h"
#include "TrayIcon.h"

#include <algorithm>
#include <functional>
#include <string>
#include <thread>
#include <vector>
#include <memory>
#include <unordered_map>
#include <list>

#include <gtk/gtk.h>
#include <libappindicator/app-indicator.h>

class StatusNotifierItem : public TrayIcon
{
private:

public:
	using StringType = std::string;

public:
	StatusNotifierItem(TrayIconData applicationName, std::function<void()> &&beforeShow);
	~StatusNotifierItem();

public:
	bool Show() override;
	bool Hide() override;

	bool SendNotification(const StringType &message) override;

	void AddMenuItem(const std::string &label, ClickCallbackType &&onClick) override;
	void AddMenuItem(const std::string &label, ClickCallbackTypeChecked &&onClick, StateCallbackType &&getState) override;
	void AddMenuItem(const std::string &label, const std::string &subLabel, ClickCallbackType &&onClick) override;

	void ClearMenuMap() override;

	operator bool() override;

private:
	static void OnActivate(GtkMenuItem *item, void *data) noexcept;

private:
	AppIndicator *indicator_{ nullptr };
	std::unique_ptr<GtkMenu, decltype(&::g_object_unref)> menu_{ nullptr, nullptr };
	std::vector<GtkMenuItem *> menuItems_{};
	std::unordered_map<GtkMenuItem *, std::pair<GtkMenu *, std::vector<GtkMenuItem *>>> subMenus_;

	std::list<ClickCallbackType> callbacks_;

	std::thread thread_;
};
