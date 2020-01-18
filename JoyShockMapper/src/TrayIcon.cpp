// Win32 Dialog.cpp : Defines the entry point for the application.
//

// https://www.codeproject.com/Articles/4768/Basic-use-of-Shell-NotifyIcon-in-Win32

// Windows Header Files:
#include "TrayIcon.h"

#include <Windowsx.h>
#include <commctrl.h>
#include <Shellapi.h>
#include <Shlwapi.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <algorithm>

#include "resource.h"
//#include "wintoastlib.h"

//using WinToastLib::WinToastTemplate;
//using WinToastLib::WinToast;

struct MenuItem
{
	std::wstring label;
	virtual ~MenuItem() {}
};

struct MenuItemButton : public MenuItem
{
	std::function<void()> onClick;
};

struct MenuItemMenu : public MenuItem
{
	std::vector<MenuItemButton*> list;
	~MenuItemMenu()
	{
		for (auto p : list)
		{
			delete p;
		}
		list.clear();
	}
};

struct MenuItemToggle : public MenuItem
{
	std::function<void(bool)> onClick;
	std::function<bool()> getState;
	void onClickWrapper()
	{
		onClick(!getState());
	}
};

static int TRAYICONID = 1; // ID number for the Notify Icon
constexpr unsigned short SWM_TRAYMSG = WM_APP; // the message ID sent to our window
constexpr UINT_PTR SWM_CUSTOM = WM_APP + 1; //	show the window

std::vector<TrayIcon *> registry;

static bool toastReady = false;

class JSMToasts// : public WinToastLib::IWinToastHandler
{
public:
	JSMToasts() { }
	~JSMToasts() { }
	// Public interfaces
	//void toastActivated() const override { }
	//void toastActivated(int actionIndex) const override { }
	//void toastDismissed(WinToastDismissalReason state) const override { }
	//void toastFailed() const override { }
};

TrayIcon::TrayIcon(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
	: _hInst(0)
	, _niData({0})
	, _menuMap()
	, _thread (0)
{
	registry.push_back(this);

	//auto *winToast = WinToast::instance();
	//if (!toastReady && WinToast::isCompatible() && !winToast->isInitialized())
	//{
	//	winToast->setAppName(L"JoyShockMapper");
	//	// https://docs.microsoft.com/fr-ca/windows/win32/shell/appids
	//	winToast->setAppUserModelId(WinToast::configureAUMI(L"JibbSmart", L"JoyShockMapper"));
	//	toastReady = winToast->initialize();
	//}

	_hInst = hInstance;

	_thread = CreateThread(
		NULL,                   // default security attributes
		0,                      // use default stack size  
		MessageHandlerLoop,       // thread function name
		this,          // argument to thread function 
		0,                      // use default creation flags 
		nullptr);			   // returns the thread identifier 
}

TrayIcon::~TrayIcon()
{
	ClearMenuMap();
	Hide();
	// free icon handle
	if (_niData.hIcon && DestroyIcon(_niData.hIcon))
		_niData.hIcon = NULL;

	CloseHandle(_thread);
	auto entry = std::find(registry.begin(), registry.end(), this);
	if( entry != registry.end() )
		registry.erase(entry);
}


DWORD WINAPI TrayIcon::MessageHandlerLoop(LPVOID param)
{
	MSG msg;
	HACCEL hAccelTable;
	
	TrayIcon *tray = static_cast<TrayIcon*>(param);
	// Perform application initialization:
	if (!tray->InitInstance())
	{
		tray->_hInst = 0;
	}
	else
	{
		hAccelTable = LoadAccelerators(static_cast<TrayIcon*>(param)->_hInst, (LPCTSTR)IDC_STEALTHDIALOG);
		// Main message loop:
		while (GetMessage(&msg, NULL, 0, 0))
		{
			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg) ||
				!IsDialogMessage(msg.hwnd, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		return (int)msg.wParam;
	}
	return 1;
}

/*bool TrayIcon::SendToast(std::wstring message)
{
	WinToastTemplate templ = WinToastTemplate(WinToastTemplate::ImageAndText02);
	templ.setTextField(L"JoyShockMapper", WinToastTemplate::FirstLine);
	templ.setTextField(message.c_str(), WinToastTemplate::SecondLine);
	templ.setImagePath(L"./gyro_icon.png");

	WinToast::WinToastError err = WinToast::NoError;
	if (!toastReady)
	{
		printf("WinToast could not initialize\n");
		return false;
	}
	else if(WinToast::instance()->showToast(templ, std::make_unique<JSMToasts>(), &err) == -1 || err != WinToast::NoError) {
		printf("WinToast Error: %S\n", WinToast::strerror(err).c_str());
		return false;
	}
	return true;
}/**/

void TrayIcon::AddMenuItem(const std::wstring & label, std::function<void()> onClick)
{
	auto btn = new MenuItemButton;
	btn->label = label;
	btn->onClick = onClick;
	_menuMap.push_back( btn );
}

void TrayIcon::AddMenuItem(const std::wstring & label, std::function<void(bool)> onClick, std::function<bool()> getState)
{
	auto tgl = new MenuItemToggle;
	tgl->label = label;
	tgl->onClick = onClick;
	tgl->getState = getState;
	_menuMap.push_back(tgl);
}

void TrayIcon::AddMenuItem(const std::wstring & label, const std::wstring & sublabel, std::function<void()> onClick)
{
	auto menuiter = std::find_if(_menuMap.begin(), _menuMap.end(),
		[label] (auto item) {
			return label == item->label; 
		}
	);
	if (menuiter == _menuMap.end())
	{
		auto menu = new MenuItemMenu;
		menu->label = label;
		_menuMap.push_back(menu);
		menuiter = _menuMap.end().operator--();
	}
	auto btn = new MenuItemButton;
	btn->label = sublabel;
	btn->onClick = onClick;
	static_cast<MenuItemMenu*>(*menuiter)->list.push_back(btn);
}

//	Initialize the window and tray icon
BOOL TrayIcon::InitInstance()
{
	// prepare for XP style controls
	InitCommonControls();

	 // store instance handle and create dialog
	HWND hWnd = CreateDialog( _hInst, MAKEINTRESOURCE(IDD_DLG_DIALOG),
		NULL, (DLGPROC)DlgProc );
	auto err = GetLastError();
	if (!hWnd) return FALSE;

	// Fill the NOTIFYICONDATA structure and call Shell_NotifyIcon

	// zero the structure - note:	Some Windows funtions require this but
	//								I can't be bothered which ones do and
	//								which ones don't.
	ZeroMemory(&_niData,sizeof(NOTIFYICONDATA));

	// get Shell32 version number and set the size of the structure
	//		note:	the MSDN documentation about this is a little
	//				dubious and I'm not at all sure if the method
	//				bellow is correct
	ULONGLONG ullVersion = GetDllVersion(_T("Shell32.dll"));
	if(ullVersion >= MAKEDLLVERULL(5, 0,0,0))
		_niData.cbSize = sizeof(NOTIFYICONDATA);
	else _niData.cbSize = NOTIFYICONDATA_V2_SIZE;

	// the ID number can be anything you choose
	_niData.uID = TRAYICONID++;

	// state which structure members are valid
	_niData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;

	// load the icon
	_niData.hIcon = (HICON)LoadImage(_hInst,MAKEINTRESOURCE(IDI_STEALTHDLG),
		IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),
		LR_DEFAULTCOLOR);

	// the window to send messages to and the message to send
	//		note:	the message value should be in the
	//				range of WM_APP through 0xBFFF
	_niData.hWnd = hWnd;
    _niData.uCallbackMessage = SWM_TRAYMSG;

	// tooltip message
    lstrcpyn(_niData.szTip, _T("JoyShockMapper"), sizeof(_niData.szTip)/sizeof(TCHAR));

	//Shell_NotifyIcon(NIM_ADD, &_niData);
	
	// call ShowWindow here to make the dialog initially visible

	return TRUE;
}

// Name says it all
void TrayIcon::ShowContextMenu(HWND hWnd)
{
	if (!_clickMap.empty()) _clickMap.clear();
	POINT pt;
	GetCursorPos(&pt);
	HMENU hMenu = CreatePopupMenu();
	if(hMenu)
	{
		int id = 0;
		for(auto menuItem : _menuMap)
		{
			auto *btn = dynamic_cast<MenuItemButton *>(menuItem);
			if (btn)
			{
				InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, SWM_CUSTOM + id, btn->label.c_str());
				_clickMap[SWM_CUSTOM + id] = btn->onClick;
			}
			auto *tgl = dynamic_cast<MenuItemToggle*>(menuItem);
			if (tgl)
			{
				bool state = tgl->getState();
				InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING | (state ? MF_CHECKED : MF_UNCHECKED), SWM_CUSTOM + id, tgl->label.c_str());
				_clickMap[SWM_CUSTOM + id] = std::bind(&MenuItemToggle::onClickWrapper, tgl);
			}
			auto *menu = dynamic_cast<MenuItemMenu*>(menuItem);
			if (menu)
			{
				HMENU submenu = CreatePopupMenu();
				for (auto button : menu->list)
				{
					InsertMenu(submenu, -1, MF_BYPOSITION | MF_STRING, SWM_CUSTOM + id, button->label.c_str());
					_clickMap[SWM_CUSTOM + id] = button->onClick;
					++id;
				}
				InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(submenu), menu->label.c_str());
			}
			++id;
		}

		SetMenuDefaultItem(hMenu, 0, TRUE); // 0 is default

		// note:	must set window to the foreground or the
		//			menu won't disappear when it should
		SetForegroundWindow(hWnd);

		TrackPopupMenu(hMenu, TPM_BOTTOMALIGN,
			pt.x, pt.y, 0, hWnd, NULL );
		DestroyMenu(hMenu);
	}
}

// Get dll version number
ULONGLONG TrayIcon::GetDllVersion(LPCTSTR lpszDllName)
{
    ULONGLONG ullVersion = 0;
	HINSTANCE hinstDll;
    hinstDll = LoadLibrary(lpszDllName);
    if(hinstDll)
    {
        DLLGETVERSIONPROC pDllGetVersion;
        pDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinstDll, "DllGetVersion");
        if(pDllGetVersion)
        {
            DLLVERSIONINFO dvi;
            HRESULT hr;
            ZeroMemory(&dvi, sizeof(dvi));
            dvi.cbSize = sizeof(dvi);
            hr = (*pDllGetVersion)(&dvi);
            if(SUCCEEDED(hr))
				ullVersion = MAKEDLLVERULL(dvi.dwMajorVersion, dvi.dwMinorVersion,0,0);
        }
        FreeLibrary(hinstDll);
    }
    return ullVersion;
}

// Message handler for the app
INT_PTR CALLBACK TrayIcon::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	UINT_PTR wmId, wmEvent;

	auto entry = std::find_if(registry.begin(), registry.end(), 
		[hWnd] (auto entry)
		{ 
			return *entry == hWnd; 
		});
	TrayIcon *tray = entry != registry.end() ? *entry : nullptr;

	switch (message) 
	{
	case SWM_TRAYMSG:
		switch(lParam)
		{
		case WM_LBUTTONDOWN:
			if (tray->_clickMap.empty())
			{
				auto *btn = dynamic_cast<MenuItemButton *>(tray->_menuMap[0]);
				if (btn)
				{
					tray->_clickMap[SWM_CUSTOM] = btn->onClick;
				}
				auto *tgl = dynamic_cast<MenuItemToggle*>(tray->_menuMap[0]);
				if (tgl)
				{
					tray->_clickMap[SWM_CUSTOM] = std::bind(&MenuItemToggle::onClickWrapper, tgl);
				}
			}
			tray->_clickMap[SWM_CUSTOM](); // 0 is default
			return FALSE;
		case WM_RBUTTONDOWN:
		case WM_CONTEXTMENU:
			tray->ShowContextMenu(hWnd);
		default:
			return FALSE;
		}
		break;
	case WM_SYSCOMMAND:
		if ((wParam & 0xFFF0) == SC_MINIMIZE)
		{
			ShowWindow(hWnd, SW_HIDE);
			return 1;
		}
		else return FALSE;
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam); 
		if (tray->_clickMap.find(wmId) != tray->_clickMap.end())
		{
			tray->_clickMap[wmId]();
		}
		else return FALSE;
		break;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		tray->_niData.uFlags = 0;
		Shell_NotifyIcon(NIM_DELETE,&tray->_niData);
		PostQuitMessage(0);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

void TrayIcon::ClearMenuMap()
{
	for (auto ptr : _menuMap)
	{
		delete ptr;
	}
	_menuMap.clear();
}
