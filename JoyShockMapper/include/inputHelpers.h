#pragma once

#include <Windows.h>
#include <math.h>
#include <functional>
#include <atomic>
#include <vector>

// https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
// Only use undefined keys from the above list for JSM custom commands
#define V_WHEEL_UP 0x03 // Here I intentionally overwride VK_CANCEL because breaking a process with a keybind is not something we want to do
#define V_WHEEL_DOWN 0x07 // I want all mouse bindings to be contiguous IDs for ease to distinguish
#define NO_HOLD_MAPPED 0x0A
#define CALIBRATE 0x0B
#define GYRO_INV_X 0x88
#define GYRO_INV_Y 0x89
#define GYRO_INVERT 0x8A
#define GYRO_OFF_BIND 0x8B // Not to be confused with settings GYRO_ON and GYRO_OFF
#define GYRO_ON_BIND 0x8C  // Those here are bindings

// get the user's mouse sensitivity multiplier from the user. In Windows it's an int, but who cares? it's well within range for float to represent it exactly
// also, if this is ported to other platforms, we might want non-integer sensitivities
float getMouseSpeed();

/// Valid inputs:
/// 0-9, N0-N9, F1-F29, A-Z, (L, R, )CONTROL, (L, R, )ALT, (L, R, )SHIFT, TAB, ENTER
/// (L, M, R)MOUSE, SCROLL(UP, DOWN)
/// NONE
/// And characters: ; ' , . / \ [ ] + - `
/// Yes, this looks slow. But it's only there to help set up faster mappings
WORD nameToKey(std::string& name);

// send mouse button
int pressMouse(WORD vkKey, bool isPressed);

// send key press
int pressKey(WORD vkKey, bool pressed);

void moveMouse(float x, float y);

void setMouseNorm(float x, float y);

// delta time will apply to shaped movement, but the extra (velocity parameters after deltaTime) is applied as given
void shapedSensitivityMoveMouse(float x, float y, std::pair<float, float> lowSensXY, std::pair<float, float> hiSensXY, float minThreshold,
	float maxThreshold, float deltaTime, float extraVelocityX, float extraVelocityY, float calibration);

bool WriteToConsole(const std::string& command);

// Cleanup actions to perform on quit
static std::function<void()> cleanupFunction;

BOOL __stdcall ConsoleCtrlHandler(DWORD dwCtrlType);

// just setting up the console with standard stuff
void initConsole(std::function<void()> todoOnQuit);

std::tuple<std::string, std::string> GetActiveWindowName();

std::vector<std::string> ListDirectory(std::string directory);

std::string GetCWD();

class PollingThread {
private:
	HANDLE _thread; // Not OS agnostic :( pimpl?
	std::function<bool(void*)> _loopContent;
	DWORD _sleepTimeMs;
	DWORD _tid;
	void* _funcParam;
	std::atomic_bool _continue;

public:
	PollingThread(std::function<bool(void*)> loopContent, void* funcParam, DWORD pollPeriodMs, bool startNow);

	~PollingThread();

	inline operator bool()
	{
		return _thread != 0;
	}

	bool Start();

	inline void Stop() {
		_continue = false;
	}

	inline bool isRunning()
	{
		return _thread && _continue;
	}

private:
	static DWORD __stdcall pollFunction(LPVOID param);
};

DWORD ShowOnlineHelp();

void HideConsole();

void ShowConsole();

bool IsVisible();

bool isConsoleMinimized();
