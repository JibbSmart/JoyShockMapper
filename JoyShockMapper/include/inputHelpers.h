#pragma once

#include "JoyShockMapper.h"
#include <atomic>
#include <functional>

float getMouseSpeed();
WORD nameToKey(string& name);
int pressMouse(WORD vkKey, bool isPressed);
int pressKey(WORD vkKey, bool pressed);
void moveMouse(float x, float y);
void setMouseNorm(float x, float y);
void shapedSensitivityMoveMouse(float x, float y, pair<float, float> lowSensXY, pair<float, float> hiSensXY, float minThreshold,
	float maxThreshold, float deltaTime, float extraVelocityX, float extraVelocityY, float calibration);
bool WriteToConsole(const string &command);
void initConsole(function<void()> todoOnQuit);
tuple<string, string> GetActiveWindowName();
vector<string> ListDirectory(string directory);
string GetCWD();

class PollingThread {
private:
	void *_thread; // Windows type HANDLE
	function<bool(void*)> _loopContent;
	DWORD _sleepTimeMs;
	DWORD _tid;
	void * _funcParam;
	atomic_bool _continue;

public:
	PollingThread(function<bool(void*)> loopContent, void* funcParam, DWORD pollPeriodMs, bool startNow);

	~PollingThread();

	bool Start();

	inline operator bool()
	{
		return _thread != 0;
	}
	
	inline void Stop()
	{
		_continue = false;
	}

	inline bool isRunning()
	{
		return _thread && _continue;
	}

private:
	static DWORD __stdcall pollFunction(void *param);
};

DWORD ShowOnlineHelp();

void HideConsole();
void ShowConsole();
bool IsVisible();
bool isConsoleMinimized();
