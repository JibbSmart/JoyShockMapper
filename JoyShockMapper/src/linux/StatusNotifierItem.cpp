#include "linux/StatusNotifierItem.h"

#include <cstring>

StatusNotifierItem::StatusNotifierItem(TrayIconData, std::function<void()> &&beforeShow)
  : thread_{ [this, &beforeShow] {
	  mainLoop_ = Gtk::Application::create(APPLICATION_RDN APPLICATION_NAME);
	  mainLoop_->hold();

	  menu_ = std::make_unique<Gtk::Menu>();

	  indicator_ = app_indicator_new(APPLICATION_RDN APPLICATION_NAME, "image-loading", APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
	  app_indicator_set_status(indicator_, APP_INDICATOR_STATUS_ACTIVE);
	  app_indicator_set_menu(indicator_, menu_->gobj());

	  beforeShow();

	  mainLoop_->run();
  } }
{
}

StatusNotifierItem::~StatusNotifierItem()
{
	g_idle_add([](void *instance) -> int {
		auto *self = static_cast<StatusNotifierItem *>(instance);

		g_object_unref(self->indicator_);
		self->mainLoop_->quit();

		return false;
	},
	  this);

	thread_.join();
}

bool StatusNotifierItem::Show()
{
	g_idle_add([](void *self) -> int {
		static_cast<StatusNotifierItem *>(self)->menu_->show_all();
		return false;
	},
	  this);

	return true;
}

bool StatusNotifierItem::Hide()
{
	g_idle_add([](void *self) -> int {
		static_cast<StatusNotifierItem *>(self)->menu_->hide();
		return false;
	},
	  this);

	return true;
}

bool StatusNotifierItem::SendNotification(const std::string &message)
{
	return true;
}

void StatusNotifierItem::AddMenuItem(const std::string &label, ClickCallbackType &&onClick)
{
	menuItems_.emplace_back(label);

	auto &menuItem = menuItems_.back();
	menuItem.signal_activate().connect(onClick);

	menu_->add(menuItem);
	menuItem.show_all();
}

void StatusNotifierItem::AddMenuItem(const std::string &label, ClickCallbackTypeChecked &&onClick, StateCallbackType &&getState)
{
	menuItems_.push_back(Gtk::CheckMenuItem(label));

	auto &menuItem = menuItems_.back();
	menuItem.signal_activate().connect([=] {
		onClick(!getState());
	});

	menu_->add(menuItem);
	menuItem.show_all();
}

void StatusNotifierItem::AddMenuItem(const std::string &label, const std::string &subLabel, ClickCallbackType &&onClick)
{
	const auto it = std::find_if(menuItems_.begin(), menuItems_.end(), [&label](const Gtk::MenuItem &item){
		return label == item.get_label();
	});

	Gtk::MenuItem *menuItem = nullptr;

	if (it == menuItems_.end())
	{
		menuItems_.emplace_back(label);
		menuItem = &(menuItems_.back());
		menu_->add(*menuItem);
		menuItem->show_all();
	}
	else
	{
		menuItem = &(*it);
	}

	auto subMenuIt = subMenus_.find(menuItem);
	if (subMenuIt == subMenus_.end())
	{
		subMenuIt = subMenus_.emplace(menuItem, std::pair<Gtk::Menu, std::vector<Gtk::MenuItem>>{Gtk::Menu{}, std::vector<Gtk::MenuItem>{}}).first;
		menuItem->set_submenu(subMenuIt->second.first);
		menuItem->show_all();
	}

	auto &subMenu = subMenuIt->second.first;
	auto &subMenuItems = subMenuIt->second.second;

	subMenuItems.emplace_back(subLabel);
	auto &subMenuItem = subMenuItems.back();

	subMenuItem.signal_activate().connect(onClick);

	subMenu.add(subMenuItem);
	subMenuItem.show_all();
}

void StatusNotifierItem::ClearMenuMap()
{
}

StatusNotifierItem::operator bool()
{
	return true;
}
