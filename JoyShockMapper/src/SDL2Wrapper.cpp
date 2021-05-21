#include "JslWrapper.h"
#include "JSMVariable.hpp"
#include "SDL.h"
#include <map>
#include <mutex>
#include <atomic>
#define INCLUDE_MATH_DEFINES
#include <cmath> // M_PI
#include <algorithm>

extern JSMVariable<float> tick_time; // defined in main.cc

struct ControllerDevice
{
	ControllerDevice(int id)
	{
		SDL_memset(&_left_trigger_effect, 0, sizeof(SDL_JoystickTriggerEffect));
		SDL_memset(&_right_trigger_effect, 0, sizeof(SDL_JoystickTriggerEffect));
		_left_trigger_effect.mode = SDL_JOYSTICK_TRIGGER_NO_EFFECT;
		_right_trigger_effect.mode = SDL_JOYSTICK_TRIGGER_NO_EFFECT;
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

			auto sdl_ctrlr_type = SDL_GameControllerGetType(_sdlController);
			switch (sdl_ctrlr_type)
			{
			case SDL_GameControllerType::SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO:
				if (_split_type == JS_SPLIT_TYPE_LEFT)
					_ctrlr_type = JS_TYPE_JOYCON_LEFT;
				else if (_split_type == JS_SPLIT_TYPE_RIGHT)
					_ctrlr_type = JS_TYPE_JOYCON_RIGHT;
				else
					_ctrlr_type = JS_TYPE_PRO_CONTROLLER;
				break;
			case SDL_GameControllerType::SDL_CONTROLLER_TYPE_PS4:
				_ctrlr_type = JS_TYPE_DS4;
				break;
			case SDL_GameControllerType::SDL_CONTROLLER_TYPE_PS5:
				_ctrlr_type = JS_TYPE_DS;
				break;
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
	int _ctrlr_type = 0;
	uint16_t _small_rumble = 0;
	uint16_t _big_rumble = 0;
	SDL_JoystickTriggerEffect _left_trigger_effect;
	SDL_JoystickTriggerEffect _right_trigger_effect;
	SDL_GameController *_sdlController = nullptr;
};

struct SdlInstance : public JslWrapper
{
public:
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

	virtual ~SdlInstance()
	{
		SDL_Quit();
	}

	static int pollDevices(void *obj)
	{
		auto inst = static_cast<SdlInstance *>(obj);
		while (inst->keep_polling)
		{
			SDL_Delay(tick_time.get());

			std::lock_guard guard(inst->controller_lock);
			for (auto iter = inst->_controllerMap.begin(); iter != inst->_controllerMap.end(); ++iter)
			{
				SDL_GameControllerUpdate();
				if (inst->g_callback)
				{
					JOY_SHOCK_STATE dummy1;
					IMU_STATE dummy2;
					memset(&dummy1, 0, sizeof(dummy1));
					memset(&dummy2, 0, sizeof(dummy2));
					inst->g_callback(iter->first, dummy1, dummy1, dummy2, dummy2, tick_time.get());
				}
				if (inst->g_touch_callback)
				{
					TOUCH_STATE touch = inst->GetTouchState(iter->first, false), dummy3;
					memset(&dummy3, 0, sizeof(dummy3));
					inst->g_touch_callback(iter->first, touch, dummy3, tick_time.get());
				}
				// Perform rumble
				SDL_GameControllerRumble(iter->second->_sdlController, iter->second->_small_rumble, iter->second->_big_rumble, tick_time.get() + 5);
				SDL_GameControllerSetTriggerEffect(iter->second->_sdlController, &iter->second->_left_trigger_effect, &iter->second->_right_trigger_effect, tick_time.get() + 5);
			}
		}

		return 1;
	}

	map<int, ControllerDevice *> _controllerMap;
	void (*g_callback)(int, JOY_SHOCK_STATE, JOY_SHOCK_STATE, IMU_STATE, IMU_STATE, float) = nullptr;
	void (*g_touch_callback)(int, TOUCH_STATE, TOUCH_STATE, float) = nullptr;
	atomic_bool keep_polling = false;
	std::mutex controller_lock;

	int ConnectDevices() override
	{
		bool isFalse = false;
		if (keep_polling.compare_exchange_strong(isFalse, true))
		{
			// keep polling was false! It is set to true now.
			SDL_Thread *controller_polling_thread = SDL_CreateThread(&SdlInstance::pollDevices, "Poll Devices", this);
			SDL_DetachThread(controller_polling_thread);
		}
		SDL_GameControllerUpdate(); // Refresh driver listing
		return SDL_NumJoysticks();
	}

	int GetConnectedDeviceHandles(int *deviceHandleArray, int size) override
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
			ControllerDevice *device = new ControllerDevice(i);
			if (device->isValid())
			{
				deviceHandleArray[i] = i + 1;
				_controllerMap[deviceHandleArray[i]] = device;
			}
		}
		return _controllerMap.size();
	}

	void DisconnectAndDisposeAll() override
	{
		lock_guard guard(controller_lock);
		keep_polling = false;
		g_callback = nullptr;
		g_touch_callback = nullptr;
		auto iter = _controllerMap.begin();
		while (iter != _controllerMap.end())
		{
			delete iter->second;
			iter = _controllerMap.erase(iter);
		}
		SDL_Delay(200);
	}

	JOY_SHOCK_STATE GetSimpleState(int deviceId) override
	{
		return JOY_SHOCK_STATE();
	}

	IMU_STATE GetIMUState(int deviceId) override
	{
		IMU_STATE imuState;
		memset(&imuState, 0, sizeof(imuState));
		if (_controllerMap[deviceId]->_has_gyro)
		{
			array<float, 3> gyro;
			SDL_GameControllerGetSensorData(_controllerMap[deviceId]->_sdlController, SDL_SENSOR_GYRO, &gyro[0], 3);
			constexpr float toDegPerSec = 180.f / M_PI;
			imuState.gyroX = gyro[0] * toDegPerSec;
			imuState.gyroY = gyro[1] * toDegPerSec;
			imuState.gyroZ = gyro[2] * toDegPerSec;
		}
		if (_controllerMap[deviceId]->_has_accel)
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

	MOTION_STATE GetMotionState(int deviceId) override
	{
		return MOTION_STATE();
	}

	TOUCH_STATE GetTouchState(int deviceId, bool previous) override
	{
		uint8_t state0 = 0, state1 = 0;
		TOUCH_STATE state;
		memset(&state, 0, sizeof(TOUCH_STATE));
		if (SDL_GameControllerGetTouchpadFinger(_controllerMap[deviceId]->_sdlController, 0, 0, &state0, &state.t0X, &state.t0Y, nullptr) == 0 && SDL_GameControllerGetTouchpadFinger(_controllerMap[deviceId]->_sdlController, 0, 1, &state1, &state.t1X, &state.t1Y, nullptr) == 0)
		{
			state.t0Down = state0 == SDL_PRESSED;
			state.t1Down = state1 == SDL_PRESSED;
		}
		return state;
	}

	bool GetTouchpadDimension(int deviceId, int &sizeX, int &sizeY) override
	{
		// I am assuming a single touchpad (or all touchpads are the same dimension)?
		auto *jc = _controllerMap[deviceId];
		if (jc != nullptr)
		{
			switch (_controllerMap[deviceId]->_ctrlr_type)
			{
			case JS_TYPE_DS4:
			case JS_TYPE_DS:
				// Matching SDL2 resolution
				sizeX = 1920;
				sizeY = 920;
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

	int GetButtons(int deviceId) override
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
		switch (_controllerMap[deviceId]->_ctrlr_type)
		{
		case JS_TYPE_DS:
			buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_MISC1) > 0 ? 1 << JSOFFSET_MIC : 0;
			// Intentional fall through to the next case
		case JS_TYPE_DS4:
			buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_TOUCHPAD) > 0 ? 1 << JSOFFSET_CAPTURE : 0;
			break;
		default:
			buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_MISC1) > 0 ? 1 << JSOFFSET_CAPTURE : 0;
			break;
		}
		return buttons;
	}

	float GetLeftX(int deviceId) override
	{
		return SDL_GameControllerGetAxis(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_AXIS_LEFTX) / (float)SDL_JOYSTICK_AXIS_MAX;
	}

	float GetLeftY(int deviceId) override
	{
		return -SDL_GameControllerGetAxis(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_AXIS_LEFTY) / (float)SDL_JOYSTICK_AXIS_MAX;
	}

	float GetRightX(int deviceId) override
	{
		return SDL_GameControllerGetAxis(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_AXIS_RIGHTX) / (float)SDL_JOYSTICK_AXIS_MAX;
	}

	float GetRightY(int deviceId) override
	{
		return -SDL_GameControllerGetAxis(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_AXIS_RIGHTY) / (float)SDL_JOYSTICK_AXIS_MAX;
	}

	float GetLeftTrigger(int deviceId) override
	{
		return SDL_GameControllerGetAxis(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_AXIS_TRIGGERLEFT) / (float)SDL_JOYSTICK_AXIS_MAX;
	}

	float GetRightTrigger(int deviceId) override
	{
		return SDL_GameControllerGetAxis(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) / (float)SDL_JOYSTICK_AXIS_MAX;
	}

	float GetGyroX(int deviceId) override
	{
		if (_controllerMap[deviceId]->_has_gyro)
		{
			float rawGyro[3];
			SDL_GameControllerGetSensorData(_controllerMap[deviceId]->_sdlController, SDL_SENSOR_GYRO, rawGyro, 3);
		}
		return float();
	}

	float GetGyroY(int deviceId) override
	{
		if (_controllerMap[deviceId]->_has_gyro)
		{
			float rawGyro[3];
			SDL_GameControllerGetSensorData(_controllerMap[deviceId]->_sdlController, SDL_SENSOR_GYRO, rawGyro, 3);
		}
		return float();
	}

	float GetGyroZ(int deviceId) override
	{
		if (_controllerMap[deviceId]->_has_gyro)
		{
			float rawGyro[3];
			SDL_GameControllerGetSensorData(_controllerMap[deviceId]->_sdlController, SDL_SENSOR_GYRO, rawGyro, 3);
		}
		return float();
	}

	float GetAccelX(int deviceId) override
	{
		return float();
	}

	float GetAccelY(int deviceId) override
	{
		return float();
	}

	float GetAccelZ(int deviceId) override
	{
		return float();
	}

	int GetTouchId(int deviceId, bool secondTouch = false) override
	{
		return int();
	}

	bool GetTouchDown(int deviceId, bool secondTouch)
	{
		uint8_t touchState = 0;
		if (SDL_GameControllerGetTouchpadFinger(_controllerMap[deviceId]->_sdlController, 0, secondTouch ? 1 : 0, &touchState, nullptr, nullptr, nullptr) == 0)
		{
			return touchState == SDL_PRESSED;
		}
		return false;
	}

	float GetTouchX(int deviceId, bool secondTouch = false) override
	{
		float x = 0;
		if (SDL_GameControllerGetTouchpadFinger(_controllerMap[deviceId]->_sdlController, 0, secondTouch ? 1 : 0, nullptr, nullptr, &x, nullptr) == 0)
		{
			return x;
		}
		return x;
	}

	float GetTouchY(int deviceId, bool secondTouch = false) override
	{
		float y = 0;
		if (SDL_GameControllerGetTouchpadFinger(_controllerMap[deviceId]->_sdlController, 0, secondTouch ? 1 : 0, nullptr, nullptr, &y, nullptr) == 0)
		{
			return y;
		}
		return y;
	}

	float GetStickStep(int deviceId) override
	{
		return float();
	}

	float GetTriggerStep(int deviceId) override
	{
		return float();
	}

	float GetPollRate(int deviceId) override
	{
		return float();
	}

	void ResetContinuousCalibration(int deviceId) override
	{
	}

	void StartContinuousCalibration(int deviceId) override
	{
	}

	void PauseContinuousCalibration(int deviceId) override
	{
	}

	void GetCalibrationOffset(int deviceId, float &xOffset, float &yOffset, float &zOffset) override
	{
	}

	void SetCalibrationOffset(int deviceId, float xOffset, float yOffset, float zOffset) override
	{
	}

	void SetCallback(void (*callback)(int, JOY_SHOCK_STATE, JOY_SHOCK_STATE, IMU_STATE, IMU_STATE, float)) override
	{
		std::lock_guard guard(controller_lock);
		g_callback = callback;
	}

	void SetTouchCallback(void (*callback)(int, TOUCH_STATE, TOUCH_STATE, float)) override
	{
		std::lock_guard guard(controller_lock);
		g_touch_callback = callback;
	}

	int GetControllerType(int deviceId) override
	{
		return _controllerMap[deviceId]->_ctrlr_type;
	}

	int GetControllerSplitType(int deviceId) override
	{
		return _controllerMap[deviceId]->_split_type;
	}

	int GetControllerColour(int deviceId) override
	{
		return int();
	}

	void SetLightColour(int deviceId, int colour) override
	{
		if (SDL_GameControllerHasLED(_controllerMap[deviceId]->_sdlController))
		{
			union
			{
				uint32_t raw;
				uint8_t argb[4];
			} uColour;
			uColour.raw = colour;
			SDL_GameControllerSetLED(_controllerMap[deviceId]->_sdlController, uColour.argb[2], uColour.argb[1], uColour.argb[0]);
		}
	}

	void SetRumble(int deviceId, int smallRumble, int bigRumble) override
	{
		// Rumble command needs to be sent at every poll in SDL, so the next value is set here and the actual call
		// is done after the callback return
		_controllerMap[deviceId]->_small_rumble = clamp(smallRumble, 0, 255) << 8;
		_controllerMap[deviceId]->_big_rumble = clamp(bigRumble, 0, 255) << 8;
	}

	void SetPlayerNumber(int deviceId, int number) override
	{
		SDL_GameControllerSetPlayerIndex(_controllerMap[deviceId]->_sdlController, number);
	}

	void SetLeftTriggerEffect(int deviceId, const JOY_SHOCK_TRIGGER_EFFECT &triggerEffect) override
	{
		SetTriggerEffect(_controllerMap[deviceId]->_left_trigger_effect, triggerEffect);
	}
	void SetRightTriggerEffect(int deviceId, const JOY_SHOCK_TRIGGER_EFFECT &triggerEffect) override
	{
		SetTriggerEffect(_controllerMap[deviceId]->_right_trigger_effect, triggerEffect);
	}

	void SetTriggerEffect(SDL_JoystickTriggerEffect &trigger_effect, const JOY_SHOCK_TRIGGER_EFFECT &triggerEffect)
	{
		trigger_effect = reinterpret_cast<const SDL_JoystickTriggerEffect &>(triggerEffect);
	}

	/*		
		switch (triggerEffect)
		{
		case small_early_rigid:
			trigger_effect.mode = SDL_JOYSTICK_TRIGGER_CONSTANT;
			trigger_effect.start = 0.15 * UINT8_MAX;
			trigger_effect.strength = 0;
			break;
		case small_early_pulse:
			trigger_effect.mode = SDL_JOYSTICK_TRIGGER_SEGMENT;
			trigger_effect.start = 0.2 * UINT8_MAX;
			trigger_effect.end = 0.25 * UINT8_MAX;
			trigger_effect.strength = UINT16_MAX;
			break;
		case large_late_pulse:
			trigger_effect.mode = SDL_JOYSTICK_TRIGGER_SEGMENT;
			trigger_effect.start = 0.56 * UINT8_MAX;
			trigger_effect.end = 0.63 * UINT8_MAX;
			trigger_effect.strength = UINT16_MAX;
			break;
		case large_early_rigid:
			trigger_effect.mode = SDL_JOYSTICK_TRIGGER_CONSTANT;
			trigger_effect.start = 0.15 * UINT8_MAX;
			trigger_effect.strength = UINT16_MAX;
			break;
		default:
			trigger_effect.mode = SDL_JOYSTICK_TRIGGER_NO_EFFECT;
			break;
		}*/
};

JslWrapper *JslWrapper::getNew()
{
	return new SdlInstance();
}