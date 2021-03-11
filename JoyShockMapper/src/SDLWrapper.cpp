#include "JoyShockLibrary.h"
#include "JSMVariable.hpp"
#include "SDL.h"
#include <map>
#include <mutex>
#include <atomic>
#define INCLUDE_MATH_DEFINES
#include <cmath> // M_PI

extern JSMVariable<float> tick_time; // defined in main.cc

struct ControllerDevice
{
	ControllerDevice(int id)
	{
		if (SDL_IsGameController(id))
		{
			_sdlController = SDL_GameControllerOpen(id);

			if (SDL_GameControllerHasSensor(_sdlController, SDL_SENSOR_GYRO))
			{
				_has_gyro = true;
				SDL_GameControllerSetSensorEnabled(_sdlController, SDL_SENSOR_GYRO, SDL_TRUE);
			}

			if (SDL_GameControllerHasSensor(_sdlController, SDL_SENSOR_ACCEL))
			{
				_has_accel = true;
				SDL_GameControllerSetSensorEnabled(_sdlController, SDL_SENSOR_ACCEL, SDL_TRUE);
			}

			int vid = SDL_GameControllerGetVendor(_sdlController);
			int pid = SDL_GameControllerGetProduct(_sdlController);
			if (vid == 0x057e)
			{
				if (pid == 0x2006)
				{
					_split_type = JS_SPLIT_TYPE_LEFT;
				}
				else if (pid == 0x2007)
				{
					_split_type = JS_SPLIT_TYPE_RIGHT;
				}
			}
		}
	}

	virtual ~ControllerDevice()
	{
		SDL_GameControllerClose(_sdlController);
	}

	inline bool isValid()
	{
		return _sdlController != nullptr;
	}

	bool _has_gyro = true;
	bool _has_accel = true;
	int _split_type = JS_SPLIT_TYPE_FULL;
	uint16_t _small_rumble = 0;
	uint16_t _big_rumble = 0;
	SDL_GameController *_sdlController = nullptr;
};

struct SdlInstance
{
private:
	SdlInstance()
	{
		SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS, "0");
		SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
		SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_JOY_CONS, "1");
		SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_SWITCH_HOME_LED, "0");
		SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS4_RUMBLE, "1");
		SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5_RUMBLE, "1");
		SDL_SetHint(SDL_HINT_JOYSTICK_THREAD, "1");
		SDL_Init(SDL_INIT_GAMECONTROLLER);
	}

public:
	static const unique_ptr<SdlInstance> _inst;

	~SdlInstance()
	{
		SDL_Quit();
	}

	static int pollDevices(void *obj)
	{
		while (SdlInstance::_inst->keep_polling)
		{
			SDL_Delay(tick_time.get());

			std::lock_guard guard(SdlInstance::_inst->controller_lock);
			for (auto iter = SdlInstance::_inst->_controllerMap.begin(); iter != SdlInstance::_inst->_controllerMap.end(); ++iter)
			{
				SDL_GameControllerUpdate();
				if (SdlInstance::_inst->g_callback)
				{
					JOY_SHOCK_STATE dummy1;
					IMU_STATE dummy2;
					memset(&dummy1, 0, sizeof(dummy1));
					memset(&dummy2, 0, sizeof(dummy2));
					SdlInstance::_inst->g_callback(iter->first, dummy1, dummy1, dummy2, dummy2, tick_time.get());
				}
				if (SdlInstance::_inst->g_touch_callback)
				{
					TOUCH_STATE touch = JslGetTouchState(iter->first), dummy3;
					memset(&dummy3, 0, sizeof(dummy3));
					SdlInstance::_inst->g_touch_callback(iter->first, touch, dummy3, tick_time.get());
				}
				// Perform rumble
				SDL_GameControllerRumble(iter->second->_sdlController, iter->second->_small_rumble, iter->second->_big_rumble, tick_time.get() + 1);
			}
		}

		return 1;
	}

	map<int, ControllerDevice *> _controllerMap;
	void (*g_callback)(int, JOY_SHOCK_STATE, JOY_SHOCK_STATE, IMU_STATE, IMU_STATE, float) = nullptr;
	void (*g_touch_callback)(int, TOUCH_STATE, TOUCH_STATE, float) = nullptr;
	atomic_bool keep_polling = false;
	std::mutex controller_lock;
};

const unique_ptr<SdlInstance> SdlInstance::_inst(new SdlInstance);

int JslConnectDevices()
{
	bool isFalse = false;
	if (SdlInstance::_inst->keep_polling.compare_exchange_strong(isFalse, true))
	{
		// keep polling was false! It is set to true now.
		SDL_Thread *controller_polling_thread = SDL_CreateThread(&SdlInstance::pollDevices, "Poll Devices", nullptr);
		SDL_DetachThread(controller_polling_thread);
	}
	return SDL_NumJoysticks();
}

int JslGetConnectedDeviceHandles(int *deviceHandleArray, int size)
{
	std::lock_guard guard(SdlInstance::_inst->controller_lock);
	auto iter = SdlInstance::_inst->_controllerMap.begin();
	while (iter != SdlInstance::_inst->_controllerMap.end())
	{
		delete iter->second;
		iter = SdlInstance::_inst->_controllerMap.erase(iter);
	}
	for (int i = 0; i < size; i++)
	{
		ControllerDevice *device = new ControllerDevice(i);
		if (device->isValid())
		{
			deviceHandleArray[i] = i + 1;
			SdlInstance::_inst->_controllerMap[deviceHandleArray[i]] = device;
		}
	}
	return SdlInstance::_inst->_controllerMap.size();
}

void JslDisconnectAndDisposeAll()
{
	lock_guard guard(SdlInstance::_inst->controller_lock);
	SdlInstance::_inst->keep_polling = false;
	SdlInstance::_inst->g_callback = nullptr;
	SdlInstance::_inst->g_touch_callback = nullptr;
	auto iter = SdlInstance::_inst->_controllerMap.begin();
	while (iter != SdlInstance::_inst->_controllerMap.end())
	{
		delete iter->second;
		iter = SdlInstance::_inst->_controllerMap.erase(iter);
	}
	SDL_Delay(200);
}

JOY_SHOCK_STATE JslGetSimpleState(int deviceId)
{
	return JOY_SHOCK_STATE();
}

IMU_STATE JslGetIMUState(int deviceId)
{
	IMU_STATE imuState;
	memset(&imuState, 0, sizeof(imuState));
	if (SdlInstance::_inst->_controllerMap[deviceId]->_has_gyro)
	{
		array<float, 3> gyro;
		SDL_GameControllerGetSensorData(SdlInstance::_inst->_controllerMap[deviceId]->_sdlController, SDL_SENSOR_GYRO, &gyro[0], 3);
		constexpr float toDegPerSec = 180.f / M_PI;
		imuState.gyroX = gyro[0] * toDegPerSec;
		imuState.gyroY = gyro[1] * toDegPerSec;
		imuState.gyroZ = gyro[2] * toDegPerSec;
	}
	if (SdlInstance::_inst->_controllerMap[deviceId]->_has_accel)
	{
		array<float, 3> accel;
		SDL_GameControllerGetSensorData(SdlInstance::_inst->_controllerMap[deviceId]->_sdlController, SDL_SENSOR_ACCEL, &accel[0], 3);
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

TOUCH_STATE JslGetTouchState(int deviceId, bool previous)
{
	uint8_t state0 = 0, state1 = 0;
	TOUCH_STATE state;
	if (SDL_GameControllerGetTouchpadFinger(SdlInstance::_inst->_controllerMap[deviceId]->_sdlController, 0, 0, &state0, &state.t0X, &state.t0Y, nullptr) == 0 &&
	  SDL_GameControllerGetTouchpadFinger(SdlInstance::_inst->_controllerMap[deviceId]->_sdlController, 0, 1, &state1, &state.t1X, &state.t1Y, nullptr) == 0)
	{
		state.t0Down = state0 != 0;
		state.t1Down = state1 != 0;
	}
	return state;
}

bool JslGetTouchpadDimension(int deviceId, int &sizeX, int &sizeY)
{
	// I am assuming a single touchpad (or all touchpads are the same dimension)?
	auto *jc = SdlInstance::_inst->_controllerMap[deviceId];
	if (jc != nullptr)
	{
		switch (JslGetControllerType(deviceId))
		{
		case JS_TYPE_DS4:
		case JS_TYPE_DS:
			sizeX = 1920;
			sizeY = 943;
			break;
		default:
			sizeX = 0;
			sizeY = 0;
			break;
		}
		return true;
	}
	return false;
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
		buttons |= SDL_GameControllerGetButton(SdlInstance::_inst->_controllerMap[deviceId]->_sdlController, SDL_GameControllerButton(pair.first)) > 0 ? 1 << pair.second : 0;
	}
	switch (JslGetControllerType(deviceId))
	{
	case SDL_CONTROLLER_TYPE_PS4:
	case SDL_CONTROLLER_TYPE_PS5:
		buttons |= SDL_GameControllerGetButton(SdlInstance::_inst->_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_TOUCHPAD) > 0 ? 1 << JSOFFSET_CAPTURE : 0;
		break;
	default:
		buttons |= SDL_GameControllerGetButton(SdlInstance::_inst->_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_MISC1) > 0 ? 1 << JSOFFSET_CAPTURE : 0;
		break;
	}
	return buttons;
}

float JslGetLeftX(int deviceId)
{
	return SDL_GameControllerGetAxis(SdlInstance::_inst->_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_AXIS_LEFTX) / (float)SDL_JOYSTICK_AXIS_MAX;
}

float JslGetLeftY(int deviceId)
{
	return SDL_GameControllerGetAxis(SdlInstance::_inst->_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_AXIS_LEFTY) / (float)SDL_JOYSTICK_AXIS_MAX;
	;
}

float JslGetRightX(int deviceId)
{
	return SDL_GameControllerGetAxis(SdlInstance::_inst->_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_AXIS_RIGHTX) / (float)SDL_JOYSTICK_AXIS_MAX;
}

float JslGetRightY(int deviceId)
{
	return SDL_GameControllerGetAxis(SdlInstance::_inst->_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_AXIS_RIGHTY) / (float)SDL_JOYSTICK_AXIS_MAX;
}

float JslGetLeftTrigger(int deviceId)
{
	return SDL_GameControllerGetAxis(SdlInstance::_inst->_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_AXIS_TRIGGERLEFT) / (float)SDL_JOYSTICK_AXIS_MAX;
}

float JslGetRightTrigger(int deviceId)
{
	return SDL_GameControllerGetAxis(SdlInstance::_inst->_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) / (float)SDL_JOYSTICK_AXIS_MAX;
}

float JslGetGyroX(int deviceId)
{
	if (SdlInstance::_inst->_controllerMap[deviceId]->_has_gyro)
	{
		float rawGyro[3];
		SDL_GameControllerGetSensorData(SdlInstance::_inst->_controllerMap[deviceId]->_sdlController, SDL_SENSOR_GYRO, rawGyro, 3);
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
	if (SDL_GameControllerGetTouchpadFinger(SdlInstance::_inst->_controllerMap[deviceId]->_sdlController, 0, secondTouch ? 1 : 0, &touchState, nullptr, nullptr, nullptr) == 0)
	{
		return touchState != 0;
	}
	return false;
}

float JslGetTouchX(int deviceId, bool secondTouch)
{
	float x = 0;
	if (SDL_GameControllerGetTouchpadFinger(SdlInstance::_inst->_controllerMap[deviceId]->_sdlController, 0, secondTouch ? 1 : 0, nullptr, nullptr, &x, nullptr) == 0)
	{
		return x;
	}
	return x;
}

float JslGetTouchY(int deviceId, bool secondTouch)
{
	float y = 0;
	if (SDL_GameControllerGetTouchpadFinger(SdlInstance::_inst->_controllerMap[deviceId]->_sdlController, 0, secondTouch ? 1 : 0, nullptr, nullptr, &y, nullptr) == 0)
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
	std::lock_guard guard(SdlInstance::_inst->controller_lock);
	SdlInstance::_inst->g_callback = callback;
}

void JslSetTouchCallback(void (*callback)(int, TOUCH_STATE, TOUCH_STATE, float))
{
	std::lock_guard guard(SdlInstance::_inst->controller_lock);
	SdlInstance::_inst->g_touch_callback = callback;
}

int JslGetControllerType(int deviceId)
{
	int type = SDL_GameControllerGetType(SdlInstance::_inst->_controllerMap[deviceId]->_sdlController);
	if (type == JS_TYPE_PRO_CONTROLLER)
	{
		if (JslGetControllerSplitType(deviceId) == JS_SPLIT_TYPE_LEFT)
			return JS_TYPE_JOYCON_LEFT;
		else if (JslGetControllerSplitType(deviceId) == JS_SPLIT_TYPE_RIGHT)
			return JS_TYPE_JOYCON_RIGHT;
	}
	return type;
}

int JslGetControllerSplitType(int deviceId)
{
	return SdlInstance::_inst->_controllerMap[deviceId]->_split_type;
}

int JslGetControllerColour(int deviceId)
{
	return int();
}

void JslSetLightColour(int deviceId, int colour)
{
	if (SDL_GameControllerHasLED(SdlInstance::_inst->_controllerMap[deviceId]->_sdlController))
	{
		union
		{
			uint32_t raw;
			uint8_t argb[4];
		} uColour;
		uColour.raw = colour;
		SDL_GameControllerSetLED(SdlInstance::_inst->_controllerMap[deviceId]->_sdlController, uColour.argb[2], uColour.argb[1], uColour.argb[0]);
	}
}

void JslSetRumble(int deviceId, int smallRumble, int bigRumble)
{
	// Rumble command needs to be sent at every poll in SDL, so the next value is set here and the actual call
	// is done after the callback return
	SdlInstance::_inst->_controllerMap[deviceId]->_small_rumble = clamp(smallRumble, 0, 255) << 8;
	SdlInstance::_inst->_controllerMap[deviceId]->_big_rumble = clamp(bigRumble, 0, 255) << 8;
}

void JslSetPlayerNumber(int deviceId, int number)
{
	SDL_GameControllerSetPlayerIndex(SdlInstance::_inst->_controllerMap[deviceId]->_sdlController, number);
}
