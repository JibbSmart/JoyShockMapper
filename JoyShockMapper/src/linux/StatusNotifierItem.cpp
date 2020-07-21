#include "linux/StatusNotifierItem.h"

#include <cstring>

StatusNotifierItem::StatusNotifierItem(TrayIconData, std::function<void()> &&beforeShow)
  : thread_{ [this, &beforeShow] {
	  int argc = 0;
	  gtk_init(&argc, nullptr);

	  std::string iconPath{};
	  const auto APPDIR = ::getenv("APPDIR");
	  if (APPDIR != nullptr)
	  {
		  std::puts("\n\033[1;33mRunning as AppImage, make sure rw permissions are set for the current user for /dev/uinput and /dev/hidraw*\n\033[0m");
		  iconPath = APPDIR;
		  iconPath += "/usr/share/icons/hicolor/24x24/status/jsm-status-dark.svg";
		  gtk_icon_theme_prepend_search_path(gtk_icon_theme_get_default(), iconPath.c_str());
	  }
	  else
	  {
		  iconPath = "jsm-status-dark";
	  }

	  menu_ = std::unique_ptr<GtkMenu, decltype(&::g_object_unref)>{ GTK_MENU(gtk_menu_new()), &g_object_unref };

	  indicator_ = app_indicator_new(APPLICATION_RDN APPLICATION_NAME, iconPath.c_str(), APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
	  app_indicator_set_status(indicator_, APP_INDICATOR_STATUS_ACTIVE);
	  app_indicator_set_menu(indicator_, menu_.get());

	  beforeShow();

	  gtk_main();
  } }
{
}

StatusNotifierItem::~StatusNotifierItem()
{
	g_idle_add([](void *) -> int {
		gtk_main_quit();
		return false;
	},
	  this);

	thread_.join();
}

bool StatusNotifierItem::Show()
{
	g_idle_add([](void *self) -> int {
		gtk_widget_show_all(GTK_WIDGET(static_cast<StatusNotifierItem *>(self)->menu_.get()));
		return false;
	},
	  this);

	return true;
}

bool StatusNotifierItem::Hide()
{
	g_idle_add([](void *self) -> int {
		gtk_widget_hide(GTK_WIDGET(static_cast<StatusNotifierItem *>(self)->menu_.get()));
		return false;
	},
	  this);

	return true;
}

bool StatusNotifierItem::SendNotification(const std::string &)
{
	return true;
}

void StatusNotifierItem::AddMenuItem(const std::string &label, ClickCallbackType &&onClick)
{
	menuItems_.emplace_back(GTK_MENU_ITEM(gtk_menu_item_new_with_label(label.c_str())));

	auto &menuItem = menuItems_.back();

	callbacks_.emplace_back(std::move(onClick));
	auto &cb = callbacks_.back();

	g_signal_connect(menuItem, "activate", G_CALLBACK(&StatusNotifierItem::OnActivate), const_cast<ClickCallbackType *>(&cb));

	gtk_container_add(GTK_CONTAINER(menu_.get()), GTK_WIDGET(menuItem));
	gtk_widget_show_all(GTK_WIDGET(menuItem));
}

void StatusNotifierItem::AddMenuItem(const std::string &label, ClickCallbackTypeChecked &&onClick, StateCallbackType &&getState)
{
	menuItems_.emplace_back(GTK_MENU_ITEM(gtk_check_menu_item_new_with_label(label.c_str())));

	auto &menuItem = menuItems_.back();

	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuItem), getState());

	callbacks_.emplace_back([=] {
		onClick(!getState());
	});
	auto &cb = callbacks_.back();

	g_signal_connect(menuItem, "activate", G_CALLBACK(&StatusNotifierItem::OnActivate), const_cast<ClickCallbackType *>(&cb));

	gtk_container_add(GTK_CONTAINER(menu_.get()), GTK_WIDGET(menuItem));
	gtk_widget_show_all(GTK_WIDGET(menuItem));
}

void StatusNotifierItem::AddMenuItem(const std::string &l, const std::string &sl, ClickCallbackType &&onClick)
{
	const auto *label = l.c_str();
	const auto *subLabel = sl.c_str();

	const auto it = std::find_if(menuItems_.begin(), menuItems_.end(), [&label](GtkMenuItem *item) {
		return strcmp(label, gtk_menu_item_get_label(item)) == 0;
	});

	GtkMenuItem *menuItem = nullptr;

	if (it == menuItems_.end())
	{
		menuItems_.emplace_back(GTK_MENU_ITEM(gtk_check_menu_item_new_with_label(label)));
		menuItem = menuItems_.back();
		gtk_container_add(GTK_CONTAINER(menu_.get()), GTK_WIDGET(menuItem));
		gtk_widget_show_all(GTK_WIDGET(menuItem));
	}
	else
	{
		menuItem = *it;
	}

	auto subMenuIt = subMenus_.find(menuItem);
	if (subMenuIt == subMenus_.end())
	{
		subMenuIt = subMenus_.emplace(menuItem, std::pair<GtkMenu *, std::vector<GtkMenuItem *>>{ GTK_MENU(gtk_menu_new()), std::vector<GtkMenuItem *>{} }).first;
		gtk_menu_item_set_submenu(menuItem, GTK_WIDGET(subMenuIt->second.first));
		gtk_widget_show_all(GTK_WIDGET(menuItem));
	}

	auto &subMenu = subMenuIt->second.first;
	auto &subMenuItems = subMenuIt->second.second;

	subMenuItems.emplace_back(GTK_MENU_ITEM(gtk_menu_item_new_with_label(subLabel)));
	auto &subMenuItem = subMenuItems.back();

	callbacks_.emplace_back(std::move(onClick));
	auto &cb = callbacks_.back();
	g_signal_connect(subMenuItem, "activate", G_CALLBACK(&StatusNotifierItem::OnActivate), const_cast<ClickCallbackType *>(&cb));

	gtk_container_add(GTK_CONTAINER(subMenu), GTK_WIDGET(subMenuItem));
	gtk_widget_show_all(GTK_WIDGET(subMenuItem));
}

void StatusNotifierItem::ClearMenuMap()
{
}

void StatusNotifierItem::OnActivate(GtkMenuItem *, void *data) noexcept
{
	auto *cb = static_cast<ClickCallbackType *>(data);
	(*cb)();
}

StatusNotifierItem::operator bool()
{
	return true;
}
