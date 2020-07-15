#pragma once

#include "PlatformDefinitions.h"

#include <functional>
#include <string>
#include <thread>
#include <vector>
#include <memory>
#include <unordered_map>

#include <gtkmm.h>
#include <libappindicator/app-indicator.h>

class StatusNotifierItem
{
private:
	using ClickCallbackType = std::function<void()>;
	using ClickCallbackTypeChecked = std::function<void(bool)>;
	using StateCallbackType = std::function<bool()>;

public:
	using StringType = std::string;

public:
	StatusNotifierItem(TrayIconData applicationName, std::function<void()> &&beforeShow);
	~StatusNotifierItem();

public:
	bool Show();
	bool Hide();

	bool SendNotification(const StringType &message);

	void AddMenuItem(const std::string &label, ClickCallbackType &&onClick);
	void AddMenuItem(const std::string &label, ClickCallbackTypeChecked &&onClick, StateCallbackType &&getState);
	void AddMenuItem(const std::string &label, const std::string &subLabel, ClickCallbackType &&onClick);

	void ClearMenuMap();

	operator bool();

private:
	Glib::RefPtr<Gtk::Application> mainLoop_{nullptr};

	AppIndicator *indicator_{ nullptr };
	std::unique_ptr<Gtk::Menu> menu_{nullptr};
	std::vector<Gtk::MenuItem> menuItems_{};
	std::unordered_map<Gtk::MenuItem *, std::pair<Gtk::Menu, std::vector<Gtk::MenuItem>>> subMenus_;

	std::thread thread_;
};
