#pragma once
#include "TrayIcon.h"
#include <Windows.h>
#include <shellapi.h>

#include <functional>
#include <vector>
#include <map>
#include <string>
#include <atomic>

struct MenuItem;

class WindowsTrayIcon : public TrayIcon
{
	HINSTANCE _hInst;       // current instance
	NOTIFYICONDATA _niData; // notify icon data
	std::vector<MenuItem *> _menuMap;
	std::map<UINT_PTR, std::function<void()>> _clickMap;
	HANDLE _thread;
	std::function<void()> _beforeShow;
	std::atomic_bool _init;

public:
	WindowsTrayIcon(HINSTANCE hInstance, std::function<void()> beforeShow);

	~WindowsTrayIcon();

	operator bool() override
	{
		return _hInst != 0 && _thread != 0;
	}

	// https://stackoverflow.com/questions/18178628/how-do-i-call-setwindowlong-in-the-64-bit-versions-of-windows/18178661#18178661
	inline bool operator==(HWND handle)
	{
		HINSTANCE inst = (HINSTANCE)GetWindowLongPtr(handle, GWLP_HINSTANCE);
		return inst == _hInst;
	}

	bool Show() override
	{
		return Shell_NotifyIcon(NIM_ADD, &_niData) != FALSE;
	}

	bool Hide() override
	{
		return Shell_NotifyIcon(NIM_DELETE, &_niData) != FALSE;
	}

	bool SendNotification(const std::wstring &message) override;

	void AddMenuItem(const std::wstring &label, ClickCallbackType &&onClick) override;

	void AddMenuItem(const std::wstring &label, ClickCallbackTypeChecked &&onClick, StateCallbackType &&getState) override;

	void AddMenuItem(const std::wstring &label, const std::wstring &sublabel, ClickCallbackType &&onClick) override;

	void ClearMenuMap();

private:
	BOOL InitInstance();

	static DWORD WINAPI MessageHandlerLoop(LPVOID param);

	void ShowContextMenu(HWND hWnd);

	ULONGLONG GetDllVersion(LPCTSTR lpszDllName);

	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};
