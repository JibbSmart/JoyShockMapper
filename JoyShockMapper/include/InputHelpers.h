#pragma once

#include "PlatformDefinitions.h"

#include <functional>
#include <string>
#include <cmath>
#include <utility>
#include <vector>
#include <atomic>

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
inline WORD nameToKey(const std::string &name)
{
	// https://msdn.microsoft.com/en-us/library/dd375731%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
	int length = name.length();
	if (length == 1)
	{
		// direct mapping to a number or character key
		char character = name.at(0);
		if (character >= '0' && character <= '9')
		{
			return character - '0' + 0x30;
		}
		if (character >= 'A' && character <= 'Z')
		{
			return character - 'A' + 0x41;
		}
		if (character == '+')
		{
			return VK_OEM_PLUS;
		}
		if (character == '-')
		{
			return VK_OEM_MINUS;
		}
		if (character == ',')
		{
			return VK_OEM_COMMA;
		}
		if (character == '.')
		{
			return VK_OEM_PERIOD;
		}
		if (character == ';')
		{
			return VK_OEM_1;
		}
		if (character == '/')
		{
			return VK_OEM_2;
		}
		if (character == '`')
		{
			return VK_OEM_3;
		}
		if (character == '[')
		{
			return VK_OEM_4;
		}
		if (character == '\\')
		{
			return VK_OEM_5;
		}
		if (character == ']')
		{
			return VK_OEM_6;
		}
		if (character == '\'')
		{
			return VK_OEM_7;
		}
	}
	if (length == 2)
	{
		// function key?
		char character = name.at(0);
		char character2 = name.at(1);
		if (character == 'F')
		{
			if (character2 >= '1' && character2 <= '9')
			{
				return character2 - '1' + VK_F1;
			}
		}
		else if (character == 'N')
		{
			if (character2 >= '0' && character2 <= '9')
			{
				return character2 - '0' + VK_NUMPAD0;
			}
		}
	}
	if (length == 3)
	{
		// could be function keys still
		char character = name.at(0);
		char character2 = name.at(1);
		char character3 = name.at(2);
		if (character == 'F')
		{
			if (character2 == '1' || character2 <= '2')
			{
				if (character3 >= '0' && character3 <= '9')
				{
					return (character2 - '1') * 10 + VK_F10 + (character3 - '0');
				}
			}
		}
	}
	if (name.compare("LEFT") == 0)
	{
		return VK_LEFT;
	}
	if (name.compare("RIGHT") == 0)
	{
		return VK_RIGHT;
	}
	if (name.compare("UP") == 0)
	{
		return VK_UP;
	}
	if (name.compare("DOWN") == 0)
	{
		return VK_DOWN;
	}
	if (name.compare("SPACE") == 0)
	{
		return VK_SPACE;
	}
	if (name.compare("CONTROL") == 0)
	{
		return VK_CONTROL;
	}
	if (name.compare("LCONTROL") == 0)
	{
		return VK_LCONTROL;
	}
	if (name.compare("RCONTROL") == 0)
	{
		return VK_RCONTROL;
	}
	if (name.compare("SHIFT") == 0)
	{
		return VK_SHIFT;
	}
	if (name.compare("LSHIFT") == 0)
	{
		return VK_LSHIFT;
	}
	if (name.compare("RSHIFT") == 0)
	{
		return VK_RSHIFT;
	}
	if (name.compare("ALT") == 0)
	{
		return VK_MENU;
	}
	if (name.compare("LALT") == 0)
	{
		return VK_LMENU;
	}
	if (name.compare("RALT") == 0)
	{
		return VK_RMENU;
	}
	if (name.compare("TAB") == 0)
	{
		return VK_TAB;
	}
	if (name.compare("ENTER") == 0)
	{
		return VK_RETURN;
	}
	if (name.compare("ESC") == 0)
	{
		return VK_ESCAPE;
	}
	if (name.compare("PAGEUP") == 0)
	{
		return VK_PRIOR;
	}
	if (name.compare("PAGEDOWN") == 0)
	{
		return VK_NEXT;
	}
	if (name.compare("HOME") == 0)
	{
		return VK_HOME;
	}
	if (name.compare("END") == 0)
	{
		return VK_END;
	}
	if (name.compare("INSERT") == 0)
	{
		return VK_INSERT;
	}
	if (name.compare("DELETE") == 0)
	{
		return VK_DELETE;
	}
	if (name.compare("LMOUSE") == 0)
	{
		return VK_LBUTTON;
	}
	if (name.compare("RMOUSE") == 0)
	{
		return VK_RBUTTON;
	}
	if (name.compare("MMOUSE") == 0)
	{
		return VK_MBUTTON;
	}
	if (name.compare("BMOUSE") == 0)
	{
		return VK_XBUTTON1;
	}
	if (name.compare("FMOUSE") == 0)
	{
		return VK_XBUTTON2;
	}
	if (name.compare("SCROLLDOWN") == 0)
	{
		return V_WHEEL_DOWN;
	}
	if (name.compare("SCROLLUP") == 0)
	{
		return V_WHEEL_UP;
	}
	if (name.compare("BACKSPACE") == 0)
	{
		return VK_BACK;
	}
	if (name.compare("NONE") == 0)
	{
		return NO_HOLD_MAPPED;
	}
	if (name.compare("CALIBRATE") == 0)
	{
		return CALIBRATE;
	}
	if (name.compare("GYRO_INV_X") == 0)
	{
		return GYRO_INV_X;
	}
	if (name.compare("GYRO_INV_Y") == 0)
	{
		return GYRO_INV_Y;
	}
	if (name.compare("GYRO_INVERT") == 0)
	{
		return GYRO_INVERT;
	}
	if (name.compare("GYRO_ON") == 0)
	{
		return GYRO_ON_BIND;
	}
	if (name.compare("GYRO_OFF") == 0)
	{
		return GYRO_OFF_BIND;
	}
	return 0x00;
}

// send mouse button
int pressMouse(WORD vkKey, bool isPressed);

// send key press
int pressKey(WORD vkKey, bool pressed);

void moveMouse(float x, float y);

void setMouseNorm(float x, float y);

// delta time will apply to shaped movement, but the extra (velocity parameters after deltaTime) is
// applied as given
inline void shapedSensitivityMoveMouse(float x, float y, std::pair<float, float> lowSensXY, std::pair<float, float> hiSensXY,
  float minThreshold, float maxThreshold, float deltaTime, float extraVelocityX, float extraVelocityY, float calibration)
{
	// apply calibration factor
	// get input velocity
	float magnitude = sqrt(x * x + y * y);
	// printf("Gyro mag: %.4f\n", magnitude);
	// calculate position on minThreshold to maxThreshold scale
	magnitude -= minThreshold;
	if (magnitude < 0.0f)
		magnitude = 0.0f;
	float denom = maxThreshold - minThreshold;
	float newSensitivity;
	if (denom <= 0.0f)
	{
		newSensitivity =
		  magnitude > 0.0f ? 1.0f : 0.0f; // if min threshold overlaps max threshold, pop up to
										  // max lowSens as soon as we're above min threshold
	}
	else
	{
		newSensitivity = magnitude / denom;
	}
	if (newSensitivity > 1.0f)
		newSensitivity = 1.0f;

	// interpolate between low sensitivity and high sensitivity
	float newSensitivityX = lowSensXY.first * calibration * (1.0f - newSensitivity) +
	  (hiSensXY.first * calibration) * newSensitivity;
	float newSensitivityY = lowSensXY.second * calibration * (1.0f - newSensitivity) +
	  (hiSensXY.second * calibration) * newSensitivity;

	// apply all values
	moveMouse((x * newSensitivityX) * deltaTime + extraVelocityX,
	  (y * newSensitivityY) * deltaTime + extraVelocityY);
}

BOOL WriteToConsole(const std::string &command);

BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType);

// just setting up the console with standard stuff
void initConsole(std::function<void()> todoOnQuit);

std::tuple<std::string, std::string> GetActiveWindowName();

std::vector<std::string> ListDirectory(std::string directory);

std::string GetCWD();

class PollingThread
{
public:
	PollingThread(std::function<bool(void *)> loopContent,
	  void *funcParam,
	  DWORD pollPeriodMs,
	  bool startNow)
	  : _thread{ 0 }
	  , _loopContent(loopContent)
	  , _sleepTimeMs(pollPeriodMs)
	  , _tid(0)
	  , _funcParam(funcParam)
	  , _continue(false)
	{
		if (startNow)
			Start();
	}

	~PollingThread();

	inline operator bool()
	{
		return _thread != 0;
	}

	bool Start();

	inline bool Stop()
	{
		_continue = false;
		return true;
	}

	inline bool isRunning()
	{
		return _thread && _continue;
	}

private:
	static DWORD WINAPI pollFunction(LPVOID param);

private:
	HANDLE _thread;
	std::function<bool(void *)> _loopContent;
	DWORD _sleepTimeMs;
	DWORD _tid;
	void *_funcParam;
	std::atomic_bool _continue;
};

DWORD ShowOnlineHelp();

void HideConsole();

void ShowConsole();

void ReleaseConsole();

bool IsVisible();

bool isConsoleMinimized();
