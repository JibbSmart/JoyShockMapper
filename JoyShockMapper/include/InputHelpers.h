#pragma once

#include "JoyShockMapper.h"
#include "PlatformDefinitions.h"

#include <functional>
#include <string>
#include <cmath>
#include <utility>
#include <vector>
#include <atomic>
#include <thread>

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
	// COUT << "Gyro mag: " << setprecision(4) << magnitude << endl;
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

BOOL WriteToConsole(in_string command);

BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType);

// just setting up the console with standard stuff
void initConsole();

std::tuple<std::string, std::string> GetActiveWindowName();

std::vector<std::string> ListDirectory(std::string directory);

std::string GetCWD();

bool SetCWD(in_string newCWD);

class PollingThread
{
public:
	PollingThread(const char *label, std::function<bool(void *)> loopContent,
	  void *funcParam,
	  DWORD pollPeriodMs,
	  bool startNow)
	  : _label(label)
	  , _thread()
	  , _loopContent(loopContent)
	  , _sleepTimeMs(pollPeriodMs)
	  , _funcParam(funcParam)
	  , _continue(false)
	{
		if (startNow)
			Start();
	}

	~PollingThread()
	{
		if (_thread && _continue)
		{
			Stop();
			_thread->join();
			_thread.reset();
		}
		// Let poll function cleanup
	}

	inline operator bool()
	{
		return _thread != nullptr;
	}

	bool Start()
	{
		if (_thread && !_continue) // thread is running but hasn't stopped yet
		{
			std::this_thread::sleep_for(std::chrono::milliseconds{ _sleepTimeMs });
		}
		if (!_thread) // thread is clear
		{
			_continue = true;
			_thread.reset(new thread(&PollingThread::pollFunction, this));
		}
		return isRunning();
	}

	inline bool Stop()
	{
		_continue = false;
		return true;
	}

	inline bool isRunning()
	{
		return _thread && _continue;
	}

	const char *_label;

private:
	static DWORD WINAPI pollFunction(LPVOID param)
	{
		auto workerThread = static_cast<PollingThread *>(param);
		if (workerThread)
		{
			while (workerThread->_continue && workerThread->_loopContent(workerThread->_funcParam))
			{
				this_thread::sleep_for(
				  std::chrono::milliseconds{ workerThread->_sleepTimeMs });
			}
		}

		return 0;
	}

private:
	unique_ptr<thread> _thread;
	std::function<bool(void *)> _loopContent;
	uint64_t _sleepTimeMs;
	void *_funcParam;
	std::atomic_bool _continue;
};

DWORD ShowOnlineHelp();

void HideConsole();
void UnhideConsole();

void ShowConsole();

void ReleaseConsole();

bool IsVisible();

bool isConsoleMinimized();

bool ClearConsole();
