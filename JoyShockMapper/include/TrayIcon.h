#pragma once
using namespace std;

#include <windows.h>
#include <functional>
#include <vector>
#include <map>

struct MenuItem;

class TrayIcon {
	HINSTANCE		_hInst;	// current instance
	NOTIFYICONDATA	_niData;	// notify icon data
	vector<MenuItem*> _menuMap;
	map<UINT_PTR, function<void()>> _clickMap;
	HANDLE _thread;
	function<void()> _beforeShow;

public:
	TrayIcon(HINSTANCE hInstance,
		HINSTANCE hPrevInstance,
		LPTSTR    lpCmdLine,
		int       nCmdShow,
		function<void()> beforeShow);

	~TrayIcon();

	inline operator bool()
	{
		return _hInst != 0 && _thread != 0;
	}

	inline bool operator ==(HWND handle)
	{
		HINSTANCE inst = (HINSTANCE)GetWindowLong(handle, GWL_HINSTANCE);
		return inst == _hInst;
	}

	inline bool Show()
	{
		return Shell_NotifyIcon(NIM_ADD, &_niData) != FALSE;
	}

	inline bool Hide()
	{
		return Shell_NotifyIcon(NIM_DELETE, &_niData) != FALSE;
	}

	bool SendToast(wstring message);

	void AddMenuItem(const wstring &label, function<void()> onClick);

	void AddMenuItem(const wstring &label, function<void(bool)> onClick, function<bool()> getState);

	void AddMenuItem(const wstring &label, const wstring &sublabel, function<void()> onClick);

	void ClearMenuMap();

private:

	BOOL InitInstance();

	static DWORD WINAPI MessageHandlerLoop(LPVOID param);

	void ShowContextMenu(HWND hWnd);

	ULONGLONG GetDllVersion(LPCTSTR lpszDllName);

	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};