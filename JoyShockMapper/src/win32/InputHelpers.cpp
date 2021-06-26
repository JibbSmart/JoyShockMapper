#include "InputHelpers.h"
#include <thread>

#include <unordered_map>

static float accumulatedX = 0;
static float accumulatedY = 0;

// Windows' mouse speed settings translate non-linearly to speed.
// Thankfully, the mappings are available here: https://liquipedia.net/counterstrike/Mouse_settings#Windows_Sensitivity
static float windowsSensitivityMappings[] = {
	0.0, // mouse sensitivity range is 1-20, so just put nothing in the 0th element
	1.0 / 32.0,
	1.0 / 16.0,
	1.0 / 8.0,
	2.0 / 8.0,
	3.0 / 8.0,
	4.0 / 8.0,
	5.0 / 8.0,
	6.0 / 8.0,
	7.0 / 8.0,
	1.0,
	1.25,
	1.5,
	1.75,
	2.0,
	2.25,
	2.5,
	2.75,
	3.0,
	3.25,
	3.5
};

// get the user's mouse sensitivity multiplier from the user. In Windows it's an int, but who cares? it's well within range for float to represent it exactly
// also, if this is ported to other platforms, we might want non-integer sensitivities
float getMouseSpeed()
{
	int result;
	if (SystemParametersInfo(SPI_GETMOUSESPEED, 0, &result, 0) && result >= 1 && result <= 20)
	{
		return windowsSensitivityMappings[result];
	}
	return 1.0;
}

// Map a VK id to mouse event id press (0) or release (1) and mouseData (2) complementary info
std::unordered_map<WORD, std::tuple<DWORD, DWORD, DWORD>> mouseMaps = {
	{ VK_LBUTTON, { MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP, 0 } },
	{ VK_RBUTTON, { MOUSEEVENTF_RIGHTDOWN, MOUSEEVENTF_RIGHTUP, 0 } },
	{ VK_MBUTTON, { MOUSEEVENTF_MIDDLEDOWN, MOUSEEVENTF_MIDDLEUP, 0 } },
	{ VK_XBUTTON1, { MOUSEEVENTF_XDOWN, MOUSEEVENTF_XUP, XBUTTON1 } },
	{ VK_XBUTTON2, { MOUSEEVENTF_XDOWN, MOUSEEVENTF_XUP, XBUTTON2 } },
	{ V_WHEEL_UP, { MOUSEEVENTF_WHEEL, 0, WHEEL_DELTA } },
	{ V_WHEEL_DOWN, { MOUSEEVENTF_WHEEL, 0, -WHEEL_DELTA } },
};

// send mouse button
int pressMouse(KeyCode vkKey, bool isPressed)
{
	// https://docs.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-mouseinput
	auto val = mouseMaps[vkKey.code];

	INPUT input;
	input.type = INPUT_MOUSE;
	input.mi.time = 0;
	input.mi.dx = 0;
	input.mi.dy = 0;
	input.mi.dwFlags = isPressed ? std::get<0>(val) : std::get<1>(val);
	input.mi.mouseData = std::get<2>(val);
	if (input.mi.dwFlags)
	{ // Ignore if there's no event ID (ex: "wheel release")
		auto result = SendInput(1, &input, sizeof(input));
		//COUT << vkKey.name << endl;
		//COUT << vkKey.code << endl;
		return result;
	}
	return 0;
}

//// send mouse button
//int pressMouse(KeyCode vkKey, bool isPressed)
//{
//	INPUT input;
//	input.type = INPUT_MOUSE;
//	if (vkKey == VK_LBUTTON)
//	{
//		input.mi.dwFlags = isPressed ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
//	}
//	else {
//		input.mi.dwFlags = isPressed ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
//	}
//	input.mi.mouseData = 0;
//	input.mi.time = 0;
//	input.mi.dx = 0;
//	input.mi.dy = 0;
//	return SendInput(1, &input, sizeof(input));
//}

// send key press
int pressKey(KeyCode vkKey, bool pressed)
{
	if (vkKey.code == 0)
		return 0;
	if (vkKey.code <= V_WHEEL_DOWN)
	{ // Highest mouse ID
		return pressMouse(vkKey, pressed);
	}
	INPUT input;
	input.type = INPUT_KEYBOARD;
	input.ki.time = 0;
	input.ki.dwFlags = KEYEVENTF_SCANCODE;
	input.ki.dwFlags |= pressed ? 0 : KEYEVENTF_KEYUP;
	if ((vkKey.code >= VK_LEFT && vkKey.code <= VK_DOWN) ||
	  (vkKey.code >= VK_LWIN && vkKey.code <= VK_NUMPAD9) ||
	  (vkKey.code >= VK_BROWSER_BACK && vkKey.code <= VK_LAUNCH_APP2))
	{
		input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
	}
	//input.ki.wVk = vkKey;
	input.ki.wVk = 0;
	input.ki.wScan = MapVirtualKey(vkKey.code, MAPVK_VK_TO_VSC);
	return SendInput(1, &input, sizeof(input));
}

void moveMouse(float x, float y)
{
	accumulatedX += x;
	accumulatedY += y;

	int applicableX = (int)accumulatedX;
	int applicableY = (int)accumulatedY;

	accumulatedX -= applicableX;
	accumulatedY -= applicableY;
	//COUT << setprecision(4) << accumulatedX << ' ' << accumulatedY << endl;

	INPUT input;
	input.type = INPUT_MOUSE;
	input.mi.mouseData = 0;
	input.mi.time = 0;
	input.mi.dx = applicableX;
	input.mi.dy = applicableY;
	input.mi.dwFlags = MOUSEEVENTF_MOVE;
	SendInput(1, &input, sizeof(input));
}

void setMouseNorm(float x, float y)
{
	INPUT input;
	input.type = INPUT_MOUSE;
	input.mi.mouseData = 0;
	input.mi.time = 0;
	input.mi.dx = roundf(65535.0f * x);
	input.mi.dy = roundf(65535.0f * y);
	input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
	SendInput(1, &input, sizeof(input));
}

BOOL WriteToConsole(in_string command)
{
	static const INPUT_RECORD ESC_DOWN = { KEY_EVENT, { TRUE, 1, VK_ESCAPE, MapVirtualKey(VK_ESCAPE, MAPVK_VK_TO_VSC), VK_ESCAPE, 0 } };
	static const INPUT_RECORD ESC_UP = { KEY_EVENT, { FALSE, 1, VK_ESCAPE, MapVirtualKey(VK_ESCAPE, MAPVK_VK_TO_VSC), VK_ESCAPE, 0 } };
	static const INPUT_RECORD RET_DOWN = { KEY_EVENT, { TRUE, 1, VK_RETURN, MapVirtualKey(VK_RETURN, MAPVK_VK_TO_VSC), VK_RETURN, 0 } };
	static const INPUT_RECORD RET_UP = { KEY_EVENT, { FALSE, 1, VK_RETURN, MapVirtualKey(VK_RETURN, MAPVK_VK_TO_VSC), VK_RETURN, 0 } };

	std::vector<INPUT_RECORD> inputs(0);
	inputs.reserve(command.size() + 4);
	inputs.push_back(ESC_DOWN);
	inputs.push_back(ESC_UP);

	for (auto c : command)
	{
		INPUT_RECORD ir;
		ir.EventType = KEY_EVENT;
		ir.Event.KeyEvent.bKeyDown = TRUE;
		ir.Event.KeyEvent.wRepeatCount = 1;
		string name(1, toupper(c));
		auto vkc = nameToKey(name);
		ir.Event.KeyEvent.wVirtualKeyCode = vkc;
		ir.Event.KeyEvent.wVirtualScanCode = MapVirtualKey(vkc, MAPVK_VK_TO_VSC);
		ir.Event.KeyEvent.uChar.UnicodeChar = c;
		ir.Event.KeyEvent.dwControlKeyState = 0;

		inputs.push_back(ir);
	}
	inputs.push_back(RET_DOWN);
	inputs.push_back(RET_UP);

	DWORD written;
	if (WriteConsoleInput(GetStdHandle(STD_INPUT_HANDLE), inputs.data(), inputs.size(), &written) == FALSE)
	{
		auto err = GetLastError();
		CERR << "Error writing to console input: " << err << endl;
	}
	FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
	return written == inputs.size();
}

BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType)
{
	// https://docs.microsoft.com/en-us/windows/console/handlerroutine
	switch (dwCtrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_CLOSE_EVENT:
		WriteToConsole("QUIT");
		return TRUE;
	}
	return FALSE;
};

// just setting up the console with standard stuff
void initConsole()
{
	AllocConsole();
	SetConsoleTitle(L"JoyShockMapper");
	// https://stackoverflow.com/a/15547699/1130520
	freopen_s((FILE **)stdin, "conin$", "r", stdin);
	freopen_s((FILE **)stdout, "conout$", "w", stdout);
	freopen_s((FILE **)stderr, "conout$", "w", stderr);
	SetConsoleCtrlHandler(&ConsoleCtrlHandler, TRUE);
}

std::tuple<std::string, std::string> GetActiveWindowName()
{
	HWND activeWindow = GetForegroundWindow();
	if (activeWindow)
	{
		std::string module(256, '\0');
		std::string title(256, '\0');
		GetWindowTextA(activeWindow, &title[0], title.size()); //note: C++11 only
		title.resize(strlen(title.c_str()));
		DWORD pid;
		// https://stackoverflow.com/questions/14745320/get-active-processname-in-vc
		if (GetWindowThreadProcessId(activeWindow, &pid))
		{
			HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
			if (hProcess)
			{
				DWORD len = module.size();
				if (QueryFullProcessImageNameA(hProcess, 0, &module[0], &len))
				{
					auto pos = module.find_last_of("\\");
					module = module.substr(pos + 1, module.size() - pos);
					module.resize(strlen(module.c_str()));
					if (title.empty())
					{
						title = module;
					}
				}
				CloseHandle(hProcess);
				return { module, title };
			}
		}
	}
	return { "", "" };
}

std::vector<std::string> ListDirectory(std::string directory)
{
	std::vector<std::string> fileListing;

	// Prepare string for use with FindFile functions.  First, copy the
	// string to a buffer, then append '\*' to the directory name.

	if (directory[directory.size() - 1] != '\\')
		directory.append("\\");
	directory.append("*");

	// Find the first file in the directory.

	WIN32_FIND_DATAA ffd;
	auto hFind = FindFirstFileA(directory.c_str(), &ffd);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		return {};
	}

	// List all the files in the directory with some info about them.

	do
	{
		// Ignore directories
		if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)

		{
			fileListing.push_back(ffd.cFileName);
			//COUT << "File: " << ffd.cFileName << endl;
		}
	} while (FindNextFileA(hFind, &ffd) != 0);

	auto dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES)
	{
		CERR << "Error code " << dwError << " raised with FindNextFile()" << endl;
	}

	FindClose(hFind);
	return fileListing;
}

std::string GetCWD()
{
	std::string cwd(MAX_PATH, '\0');
	GetCurrentDirectoryA(cwd.size(), &cwd[0]);
	cwd.resize(strlen(cwd.c_str()));
	return cwd;
}

bool SetCWD(in_string newCWD)
{
	return SetCurrentDirectoryA(newCWD.c_str()) == TRUE;
}

DWORD ShowOnlineHelp()
{
	COUT << "See the latest user manual at the web page below:" << endl
	     << "https://github.com/JibbSmart/JoyShockMapper/blob/master/README.md" << endl;
	return 0;
	// SECURE CODING! https://www.oreilly.com/library/view/secure-programming-cookbook/0596003943/ch01s08.html
	//STARTUPINFOA startupInfo;
	//PROCESS_INFORMATION procInfo;
	//memset(&startupInfo, 0, sizeof(STARTUPINFOA));
	//memset(&procInfo, 0, sizeof(PROCESS_INFORMATION));
	//auto success = CreateProcessA(NULL, R"(cmd /C "start https://github.com/JibbSmart/JoyShockMapper/blob/master/README.md")", NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &startupInfo, &procInfo);
	//if (success == TRUE)
	//{
	//	CloseHandle(procInfo.hProcess);
	//	CloseHandle(procInfo.hThread);
	//	return 0;
	//}
	//return GetLastError();
}

void HideConsole()
{
	ShowWindow(GetConsoleWindow(), SW_HIDE);
}

void UnhideConsole()
{
	ShowWindow(GetConsoleWindow(), SW_SHOWMINIMIZED);
}

void ShowConsole()
{
	ShowWindow(GetConsoleWindow(), SW_SHOWDEFAULT);
	SetForegroundWindow(GetConsoleWindow());
}

void ReleaseConsole()
{
	::FreeConsole();
}

bool IsVisible()
{
	return IsWindowVisible(GetConsoleWindow());
}

bool isConsoleMinimized()
{
	return IsVisible() != FALSE && IsIconic(GetConsoleWindow()) != FALSE;
}

bool ClearConsole()
{
	HANDLE hStdout;

	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

	// Fetch existing console mode so we correctly add a flag and not turn off others
	DWORD mode = 0;
	if (!GetConsoleMode(hStdout, &mode))
	{
		return false;
	}

	// Hold original mode to restore on exit to be cooperative with other command-line apps.
	const DWORD originalMode = mode;
	mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

	// Try to set the mode.
	if (!SetConsoleMode(hStdout, mode))
	{
		return false;
	}

	// Write the sequence for clearing the display.
	DWORD written = 0;
	constexpr PCWSTR sequence = L"\x1b[2J\x1b[0;0H"; // Clear screen and move cursor to 0,0
	if (!WriteConsoleW(hStdout, sequence, lstrlen(sequence), &written, NULL))
	{
		// If we fail, try to restore the mode on the way out.
		SetConsoleMode(hStdout, originalMode);
		return false;
	}

	// To also clear the scroll back, emit L"\x1b[3J" as well.
	// 2J only clears the visible window and 3J only clears the scroll back.

	// Restore the mode on the way out to be nice to other command-line applications.
	SetConsoleMode(hStdout, originalMode);

	return true;
}
