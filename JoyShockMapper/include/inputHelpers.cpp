#pragma once

#include <Windows.h>
#include <math.h>
#include <Winuser.h>
#include <functional>
#include <atomic>

#define NO_HOLD_MAPPED 0x07
#define CALIBRATE 0x0A

static WORD mouseMaps[] = { 0x0002, 0x0008, 0x0000, 0x0020 };
// Windows' mouse speed settings translate non-linearly to speed.
// Thankfully, the mappings are available here: https://liquipedia.net/counterstrike/Mouse_settings#Windows_Sensitivity
static float windowsSensitivityMappings[] =
{
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
float getMouseSpeed() {
	int result;
	if (SystemParametersInfo(SPI_GETMOUSESPEED, 0, &result, 0) && result >= 1 && result <= 20) {
		return windowsSensitivityMappings[result];
	}
	return 1.0;
}

// just setting up the console with standard stuff
void initConsole() {
	AllocConsole();
	// https://stackoverflow.com/a/15547699/1130520
	freopen_s((FILE**)stdin, "conin$", "r", stdin);
	freopen_s((FILE**)stdout, "conout$", "w", stdout);
	freopen_s((FILE**)stderr, "conout$", "w", stderr);
}

/// Valid inputs:
/// 0-9, N0-N9, F1-F29, A-Z, (L, R, )CONTROL, (L, R, )ALT, (L, R, )SHIFT, TAB, ENTER
/// (L, M, R)MOUSE, SCROLL(UP, DOWN)
/// NONE
/// And characters: ; ' , . / \ [ ] + - `
/// Yes, this looks slow. But it's only there to help set up faster mappings
WORD nameToKey(std::string& name) {
	// https://msdn.microsoft.com/en-us/library/dd375731%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
	int length = name.length();
	if (length == 1) {
		// direct mapping to a number or character key
		char character = name.at(0);
		if (character >= '0' && character <= '9') {
			return character - '0' + 0x30;
		}
		if (character >= 'A' && character <= 'Z') {
			return character - 'A' + 0x41;
		}
		if (character == '+') {
			return VK_OEM_PLUS;
		}
		if (character == '-') {
			return VK_OEM_MINUS;
		}
		if (character == ',') {
			return VK_OEM_COMMA;
		}
		if (character == '.') {
			return VK_OEM_PERIOD;
		}
		if (character == ';') {
			return VK_OEM_1;
		}
		if (character == '/') {
			return VK_OEM_2;
		}
		if (character == '`') {
			return VK_OEM_3;
		}
		if (character == '[') {
			return VK_OEM_4;
		}
		if (character == '\\') {
			return VK_OEM_5;
		}
		if (character == ']') {
			return VK_OEM_6;
		}
		if (character == '\'') {
			return VK_OEM_7;
		}
	}
	if (length == 2) {
		// function key?
		char character = name.at(0);
		char character2 = name.at(1);
		if (character == 'F') {
			if (character2 >= '1' && character2 <= '9') {
				return character2 - '1' + VK_F1;
			}
		}
		else if (character == 'N') {
			if (character2 >= '0' && character2 <= '9') {
				return character2 - '0' + VK_NUMPAD0;
			}
		}
	}
	if (length == 3) {
		// could be function keys still
		char character = name.at(0);
		char character2 = name.at(1);
		char character3 = name.at(2);
		if (character == 'F') {
			if (character2 == '1' || character2 <= '2') {
				if (character3 >= '0' && character3 <= '9') {
					return (character2 - '1') * 10 + VK_F10 + (character3 - '0');
				}
			}
		}
	}
	if (name.compare("LEFT") == 0) {
		return VK_LEFT;
	}
	if (name.compare("RIGHT") == 0) {
		return VK_RIGHT;
	}
	if (name.compare("UP") == 0) {
		return VK_UP;
	}
	if (name.compare("DOWN") == 0) {
		return VK_DOWN;
	}
	if (name.compare("SPACE") == 0) {
		return VK_SPACE;
	}
	if (name.compare("CONTROL") == 0) {
		return VK_CONTROL;
	}
	if (name.compare("LCONTROL") == 0) {
		return VK_LCONTROL;
	}
	if (name.compare("RCONTROL") == 0) {
		return VK_RCONTROL;
	}
	if (name.compare("SHIFT") == 0) {
		return VK_SHIFT;
	}
	if (name.compare("LSHIFT") == 0) {
		return VK_LSHIFT;
	}
	if (name.compare("RSHIFT") == 0) {
		return VK_RSHIFT;
	}
	if (name.compare("ALT") == 0) {
		return VK_MENU;
	}
	if (name.compare("LALT") == 0) {
		return VK_LMENU;
	}
	if (name.compare("RALT") == 0) {
		return VK_RMENU;
	}
	if (name.compare("TAB") == 0) {
		return VK_TAB;
	}
	if (name.compare("ENTER") == 0) {
		return VK_RETURN;
	}
	if (name.compare("ESC") == 0) {
		return VK_ESCAPE;
	}
	if (name.compare("PAGEUP") == 0) {
		return VK_PRIOR;
	}
	if (name.compare("PAGEDOWN") == 0) {
		return VK_NEXT;
	}
	if (name.compare("HOME") == 0) {
		return VK_HOME;
	}
	if (name.compare("END") == 0) {
		return VK_END;
	}
	if (name.compare("INSERT") == 0) {
		return VK_INSERT;
	}
	if (name.compare("DELETE") == 0) {
		return VK_DELETE;
	}
	if (name.compare("LMOUSE") == 0) {
		return VK_LBUTTON;
	}
	if (name.compare("RMOUSE") == 0) {
		return VK_RBUTTON;
	}
	if (name.compare("MMOUSE") == 0) {
		return VK_MBUTTON;
	}
	if (name.compare("SCROLLDOWN") == 0) {
		return VK_XBUTTON1;
	}
	if (name.compare("SCROLLUP") == 0) {
		return VK_XBUTTON2;
	}
	if (name.compare("NONE") == 0) {
		return NO_HOLD_MAPPED;
	}
	if (name.compare("CALIBRATE") == 0) {
		return CALIBRATE;
	}
	return 0x00;
}

// send key press
int pressKey(WORD vkKey, bool pressed) {
	if (vkKey == 0) return 0;
	INPUT input;
	if (vkKey <= VK_XBUTTON2) {
		input.type = INPUT_MOUSE;
		input.mi.mouseData = 0;
		input.mi.time = 0;
		input.mi.dx = 0;
		input.mi.dy = 0;
		if (vkKey <= VK_MBUTTON) {
			vkKey = mouseMaps[vkKey - 1];
			if (!pressed)
				vkKey *= 2;
			input.mi.dwFlags = vkKey;
		}
		else {
			input.mi.dwFlags = MOUSEEVENTF_WHEEL;
			input.mi.mouseData = (int16_t)(vkKey - (VK_XBUTTON1 - 1)) * 240 - 360;
			if (!pressed) return 0;
		}
		return SendInput(1, &input, sizeof(input));
	}
	input.type = INPUT_KEYBOARD;
	input.ki.time = 0;
	input.ki.dwFlags = KEYEVENTF_SCANCODE;
	input.ki.dwFlags |= pressed ? 0 : KEYEVENTF_KEYUP;
	if (vkKey >= VK_LEFT && vkKey <= VK_DOWN) {
		input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
	}
	//input.ki.wVk = vkKey;
	input.ki.wVk = 0;
	input.ki.wScan = MapVirtualKey(vkKey, MAPVK_VK_TO_VSC);
	return SendInput(1, &input, sizeof(input));
}

// send mouse button
int pressMouse(bool isLeft, bool isPressed) {
	INPUT input;
	input.type = INPUT_MOUSE;
	if (isLeft) {
		input.mi.dwFlags = isPressed ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
	}
	else {
		input.mi.dwFlags = isPressed ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
	}
	input.mi.mouseData = 0;
	input.mi.time = 0;
	input.mi.dx = 0;
	input.mi.dy = 0;
	return SendInput(1, &input, sizeof(input));
}

float accumulatedX = 0;
float accumulatedY = 0;

void moveMouse(float x, float y) {
	accumulatedX += x;
	accumulatedY += y;

	int applicableX = (int)accumulatedX;
	int applicableY = (int)accumulatedY;

	accumulatedX -= applicableX;
	accumulatedY -= applicableY;
	//printf("%0.4f %0.4f\n", accumulatedX, accumulatedY);

	INPUT input;
	input.type = INPUT_MOUSE;
	input.mi.mouseData = 0;
	input.mi.time = 0;
	input.mi.dx = applicableX;
	input.mi.dy = applicableY;
	input.mi.dwFlags = MOUSEEVENTF_MOVE;
	SendInput(1, &input, sizeof(input));
}

// delta time will apply to shaped movement, but the extra (velocity parameters after deltaTime) is applied as given
void shapedSensitivityMoveMouse(float x, float y, float lowSens, float hiSens, float minThreshold, float maxThreshold, float deltaTime, float extraVelocityX, float extraVelocityY, float calibration) {
	// apply calibration factor
	lowSens *= calibration;
	hiSens *= calibration;
	// get input velocity
	float magnitude = sqrt(x * x + y * y);
	//printf("Gyro mag: %.4f\n", magnitude);
	// calculate position on minThreshold to maxThreshold scale
	magnitude -= minThreshold;
	if (magnitude < 0.0f) magnitude = 0.0f;
	float denom = maxThreshold - minThreshold;
	float newSensitivity;
	if (denom <= 0.0f) {
		newSensitivity = magnitude > 0.0f ? 1.0f : 0.0f; // if min threshold overlaps max threshold, pop up to max lowSens as soon as we're above min threshold
	}
	else {
		newSensitivity = magnitude / denom;
	}
	if (newSensitivity > 1.0f) newSensitivity = 1.0f;
	// interpolate between low sensitivity and high sensitivity
	newSensitivity = lowSens * (1.0f - newSensitivity) + (hiSens)* newSensitivity;

	// apply all values
	moveMouse((x * newSensitivity) * deltaTime + extraVelocityX, (y * newSensitivity) * deltaTime + extraVelocityY);
}

std::tuple<std::string, std::string> GetActiveWindowName() {
	HWND activeWindow = GetForegroundWindow();
	if (activeWindow) {
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
				if (QueryFullProcessImageNameA(hProcess, 0, &module[0], &len)) {
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
			//printf("File: %s\n", ffd.cFileName);
		}
	} while (FindNextFileA(hFind, &ffd) != 0);

	auto dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES)
	{
		printf("Error code %d raised with FindNextFile()\n", dwError);
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

class PollingThread {
private:
	HANDLE _thread;
	std::function<bool(void*)> _loopContent;
	DWORD _sleepTimeMs;
	DWORD _tid;
	void * _funcParam;
	std::atomic_bool _continue;

public:
	PollingThread(std::function<bool(void*)> loopContent, void* funcParam, DWORD pollPeriodMs, bool startNow)
		: _thread(nullptr)
		, _loopContent(loopContent)
		, _sleepTimeMs(pollPeriodMs)
		, _tid(0)
		, _funcParam()
		, _continue(false)
	{
		if (startNow) Start();
	}

	~PollingThread()
	{
		Stop();
		CloseHandle(_thread);
		_thread = nullptr;
	}

	inline operator bool()
	{
		return _thread != 0;
	}

	bool Start() {
		if (!_thread)
		{
			_continue = true;
			_thread = CreateThread(
				NULL,                   // default security attributes
				0,                      // use default stack size  
				&pollFunction,       // thread function name
				this,          // argument to thread function 
				0,                      // use default creation flags 
				&_tid);   // returns the thread identifier 
			return true;
		}
		return false;
	}

	void Stop() {
		_continue = false;
	}

private:
	static DWORD WINAPI pollFunction(LPVOID param)
	{
		auto workerThread = reinterpret_cast<PollingThread*>(param);
		if (workerThread)
		{
			while (workerThread->_loopContent(workerThread->_funcParam) && workerThread->_continue) {
				Sleep(workerThread->_sleepTimeMs);
			}
			CloseHandle(workerThread->_thread);
			workerThread->_thread = nullptr;
			return 0;
		}
		return 1;
	}
};