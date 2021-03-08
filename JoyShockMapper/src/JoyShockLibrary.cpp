#include "JoyShockLibrary.h"
#include "JSMVariable.hpp"
#include "SDL.h"
#include <map>
#include <mutex>
#define INCLUDE_MATH_DEFINES
#include <cmath> // M_PI

struct ControllerDevice;

static std::map<int, ControllerDevice *> _controllerMap;
bool keep_polling = true;
class Joyshock;
void (*g_callback)(int, JOY_SHOCK_STATE, JOY_SHOCK_STATE, IMU_STATE, IMU_STATE, float);

std::mutex controller_lock;

extern JSMVariable<float> tick_time;

static int pollDevices(void *obj)
{
	while (keep_polling)
	{
		SDL_Delay(tick_time.get());

		std::lock_guard guard(controller_lock);
		for (auto iter = _controllerMap.begin(); iter != _controllerMap.end(); ++iter)
		{
			SDL_GameControllerUpdate();
			JOY_SHOCK_STATE dummy1;
			IMU_STATE dummy2;
			memset(&dummy1, 0, sizeof(dummy1));
			memset(&dummy2, 0, sizeof(dummy2));
			g_callback(iter->first, dummy1, dummy1, dummy2, dummy2, tick_time.get());
		}
	}

	return 1;
}

struct ControllerDevice
{
	~ControllerDevice()
	{
		SDL_GameControllerClose(_sdlController);
	}

	inline bool isValid()
	{
		return _sdlController == nullptr;
	}
	bool has_gyro = false;
	bool has_accel = false;
	int split_type = JS_SPLIT_TYPE_FULL;
	SDL_GameController *_sdlController = nullptr;
};

int JslConnectDevices()
{
	return SDL_NumJoysticks();
}

int JslGetConnectedDeviceHandles(int *deviceHandleArray, int size)
{
	std::lock_guard guard(controller_lock);
	auto iter = _controllerMap.begin();
	while (iter != _controllerMap.end())
	{
		delete iter->second;
		iter = _controllerMap.erase(iter);
	}
	for (int i = 0; i < size; i++)
	{
		ControllerDevice *device = new ControllerDevice();
		if (!SDL_IsGameController(i))
		{
			continue;
		}
		device->_sdlController = SDL_GameControllerOpen(i);

		if (SDL_GameControllerHasSensor(device->_sdlController, SDL_SENSOR_GYRO))
		{
			device->has_gyro = true;
			SDL_GameControllerSetSensorEnabled(device->_sdlController, SDL_SENSOR_GYRO, SDL_TRUE);
		}

		if (SDL_GameControllerHasSensor(device->_sdlController, SDL_SENSOR_ACCEL))
		{
			device->has_accel = true;
			SDL_GameControllerSetSensorEnabled(device->_sdlController, SDL_SENSOR_ACCEL, SDL_TRUE);
		}

		int vid = SDL_GameControllerGetVendor(device->_sdlController);
		int pid = SDL_GameControllerGetProduct(device->_sdlController);
		if (vid == 0x057e)
		{
			if (pid == 0x2006)
			{
				device->split_type = JS_SPLIT_TYPE_LEFT;
			}
			else if (pid == 0x2007)
			{
				device->split_type = JS_SPLIT_TYPE_RIGHT;
			}
		}
		int handle = i + 1;
		deviceHandleArray[i] = handle;
		_controllerMap[handle] = device;
	}
	return _controllerMap.size();
}

void JslDisconnectAndDisposeAll()
{
	keep_polling = false;
	controller_lock.lock();
	auto iter = _controllerMap.begin();
	while (iter != _controllerMap.end())
	{
		delete iter->second;
		iter = _controllerMap.erase(iter);
	}
	controller_lock.unlock();
	SDL_Delay(200);

	SDL_Quit();
}

JOY_SHOCK_STATE JslGetSimpleState(int deviceId)
{
	return JOY_SHOCK_STATE();
}

IMU_STATE JslGetIMUState(int deviceId)
{
	IMU_STATE imuState;
	memset(&imuState, 0, sizeof(imuState));
	if (_controllerMap[deviceId]->has_gyro)
	{
		array<float, 3> gyro;
		SDL_GameControllerGetSensorData(_controllerMap[deviceId]->_sdlController, SDL_SENSOR_GYRO, &gyro[0], 3);
		constexpr float toDegPerSec = 180.f / M_PI;
		imuState.gyroX = gyro[0] * toDegPerSec;
		imuState.gyroY = gyro[1] * toDegPerSec;
		imuState.gyroZ = gyro[2] * toDegPerSec;
	}
	if (_controllerMap[deviceId]->has_accel)
	{
		array<float, 3> accel;
		SDL_GameControllerGetSensorData(_controllerMap[deviceId]->_sdlController, SDL_SENSOR_ACCEL, &accel[0], 3);
		constexpr float toGs = 1.f / 9.8f;
		imuState.accelX = accel[0] * toGs;
		imuState.accelY = accel[1] * toGs;
		imuState.accelZ = accel[2] * toGs;
	}
	return imuState;
}

MOTION_STATE JslGetMotionState(int deviceId)
{
	return MOTION_STATE();
}

TOUCH_STATE JslGetTouchState(int deviceId)
{
	return TOUCH_STATE();
}

int JslGetButtons(int deviceId)
{
	static const std::map<int, int> sdl2jsl = {
		{ SDL_CONTROLLER_BUTTON_A, JSOFFSET_S },
		{ SDL_CONTROLLER_BUTTON_B, JSOFFSET_E },
		{ SDL_CONTROLLER_BUTTON_X, JSOFFSET_W },
		{ SDL_CONTROLLER_BUTTON_Y, JSOFFSET_N },
		{ SDL_CONTROLLER_BUTTON_BACK, JSOFFSET_MINUS },
		{ SDL_CONTROLLER_BUTTON_GUIDE, JSOFFSET_HOME },
		{ SDL_CONTROLLER_BUTTON_START, JSOFFSET_PLUS },
		{ SDL_CONTROLLER_BUTTON_LEFTSTICK, JSOFFSET_LCLICK },
		{ SDL_CONTROLLER_BUTTON_RIGHTSTICK, JSOFFSET_RCLICK },
		{ SDL_CONTROLLER_BUTTON_LEFTSHOULDER, JSOFFSET_L },
		{ SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, JSOFFSET_R },
		{ SDL_CONTROLLER_BUTTON_DPAD_UP, JSOFFSET_UP },
		{ SDL_CONTROLLER_BUTTON_DPAD_DOWN, JSOFFSET_DOWN },
		{ SDL_CONTROLLER_BUTTON_DPAD_LEFT, JSOFFSET_LEFT },
		{ SDL_CONTROLLER_BUTTON_DPAD_RIGHT, JSOFFSET_RIGHT },
		{ SDL_CONTROLLER_BUTTON_PADDLE2, JSOFFSET_SL }, // LSL
		{ SDL_CONTROLLER_BUTTON_PADDLE4, JSOFFSET_SR }, // LSR
		{ SDL_CONTROLLER_BUTTON_PADDLE3, JSOFFSET_SL }, // RSL
		{ SDL_CONTROLLER_BUTTON_PADDLE1, JSOFFSET_SR }, // RSR
	};
	int buttons = 0;
	for (auto pair : sdl2jsl)
	{
		buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_GameControllerButton(pair.first)) > 0 ? 1 << pair.second : 0;
	}
	switch (_controllerMap[deviceId]->split_type)
	{
	case SDL_CONTROLLER_TYPE_PS4:
	case SDL_CONTROLLER_TYPE_PS5:
		buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_TOUCHPAD) > 0 ? 1 << JSOFFSET_CAPTURE : 0;
		break;
	default:
		buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_MISC1) > 0 ? 1 << JSOFFSET_CAPTURE : 0;
		break;
	}
	return buttons;
}

float JslGetLeftX(int deviceId)
{
	return SDL_GameControllerGetAxis(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_AXIS_LEFTX) / (float)SDL_JOYSTICK_AXIS_MAX;
}

float JslGetLeftY(int deviceId)
{
	return SDL_GameControllerGetAxis(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_AXIS_LEFTY) / (float)SDL_JOYSTICK_AXIS_MAX;
	;
}

float JslGetRightX(int deviceId)
{
	return SDL_GameControllerGetAxis(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_AXIS_RIGHTX) / (float)SDL_JOYSTICK_AXIS_MAX;
}

float JslGetRightY(int deviceId)
{
	return SDL_GameControllerGetAxis(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_AXIS_RIGHTY) / (float)SDL_JOYSTICK_AXIS_MAX;
}

float JslGetLeftTrigger(int deviceId)
{
	return SDL_GameControllerGetAxis(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_AXIS_TRIGGERLEFT) / (float)SDL_JOYSTICK_AXIS_MAX;
}

float JslGetRightTrigger(int deviceId)
{
	return SDL_GameControllerGetAxis(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) / (float)SDL_JOYSTICK_AXIS_MAX;
}

float JslGetGyroX(int deviceId)
{
	if (_controllerMap[deviceId]->has_gyro)
	{
		float rawGyro[3];
		SDL_GameControllerGetSensorData(_controllerMap[deviceId]->_sdlController, SDL_SENSOR_GYRO, rawGyro, 3);
	}
	return float();
}

float JslGetGyroY(int deviceId)
{
	return float();
}

float JslGetGyroZ(int deviceId)
{
	return float();
}

float JslGetAccelX(int deviceId)
{
	return float();
}

float JslGetAccelY(int deviceId)
{
	return float();
}

float JslGetAccelZ(int deviceId)
{
	return float();
}

int JslGetTouchId(int deviceId, bool secondTouch)
{
	return int();
}

bool JslGetTouchDown(int deviceId, bool secondTouch)
{
	uint8_t touchState = 0;
	if (SDL_GameControllerGetTouchpadFinger(_controllerMap[deviceId]->_sdlController, 0, secondTouch ? 1 : 0, &touchState, nullptr, nullptr, nullptr) == 0)
	{
		return touchState != 0;
	}
	return false;
}

float JslGetTouchX(int deviceId, bool secondTouch)
{
	float x = 0;
	if (SDL_GameControllerGetTouchpadFinger(_controllerMap[deviceId]->_sdlController, 0, secondTouch ? 1 : 0, nullptr, nullptr, &x, nullptr) == 0)
	{
		return x;
	}
	return x;
}

float JslGetTouchY(int deviceId, bool secondTouch)
{
	float y = 0;
	if (SDL_GameControllerGetTouchpadFinger(_controllerMap[deviceId]->_sdlController, 0, secondTouch ? 1 : 0, nullptr, nullptr, &y, nullptr) == 0)
	{
		return y;
	}
	return y;
}

float JslGetStickStep(int deviceId)
{
	return float();
}

float JslGetTriggerStep(int deviceId)
{
	return float();
}

float JslGetPollRate(int deviceId)
{
	return float();
}

void JslResetContinuousCalibration(int deviceId)
{
	return void();
}

void JslStartContinuousCalibration(int deviceId)
{
	return void();
}

void JslPauseContinuousCalibration(int deviceId)
{
	return void();
}

void JslGetCalibrationOffset(int deviceId, float &xOffset, float &yOffset, float &zOffset)
{
	return void();
}

void JslSetCalibrationOffset(int deviceId, float xOffset, float yOffset, float zOffset)
{
	return void();
}

void JslSetCallback(void (*callback)(int, JOY_SHOCK_STATE, JOY_SHOCK_STATE, IMU_STATE, IMU_STATE, float))
{
	SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS, "0");
	SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
	SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_JOY_CONS, "1");
	SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_SWITCH_HOME_LED, "0");
	SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS4_RUMBLE, "1");
	SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5_RUMBLE, "1");
	SDL_SetHint(SDL_HINT_JOYSTICK_THREAD, "1");
	SDL_Init(SDL_INIT_GAMECONTROLLER);
	g_callback = callback;
	SDL_Thread *controllerThread = SDL_CreateThread(pollDevices, "Poll Devices", nullptr);
	SDL_DetachThread(controllerThread);
}

void JslSetTouchCallback(void (*callback)(int, TOUCH_STATE, TOUCH_STATE, float))
{
	return void();
}

int JslGetControllerType(int deviceId)
{
	return SDL_GameControllerGetType(_controllerMap[deviceId]->_sdlController);
}

int JslGetControllerSplitType(int deviceId)
{
	return _controllerMap[deviceId]->split_type;
}

int JslGetControllerColour(int deviceId)
{
	return int();
}

void JslSetLightColour(int deviceId, int colour)
{
	union
	{
		uint32_t raw;
		uint8_t argb[4];
	} uColour;
	uColour.raw = colour;
	SDL_GameControllerSetLED(_controllerMap[deviceId]->_sdlController, uColour.argb[2], uColour.argb[1], uColour.argb[0]);
}

void JslSetRumble(int deviceId, int smallRumble, int bigRumble)
{
	SDL_GameControllerRumble(_controllerMap[deviceId]->_sdlController, smallRumble << 8, bigRumble << 8, tick_time.get() + 1);
}

void JslSetPlayerNumber(int deviceId, int number)
{
	SDL_GameControllerSetPlayerIndex(_controllerMap[deviceId]->_sdlController, number);
}
