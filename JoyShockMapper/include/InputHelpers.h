#pragma once

#include "JoyShockMapper.h"

#include <functional>
#include <string>
#include <cmath>
#include <utility>
#include <vector>
#include <atomic>

// get the user's mouse sensitivity multiplier from the user. In Windows it's an int, but who cares? it's well within range for float to represent it exactly
// also, if this is ported to other platforms, we might want non-integer sensitivities
float getMouseSpeed();

// send mouse button
int pressMouse(KeyCode vkKey, bool isPressed);

// send key press
int pressKey(KeyCode vkKey, bool pressed);

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
