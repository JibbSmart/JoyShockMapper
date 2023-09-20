#include "JslWrapper.h"
#include "JSMVariable.hpp"
 #include "TriggerEffectGenerator.h"
#include "SettingsManager.h"
#include "SDL.h"
#include <map>
#include <mutex>
#include <atomic>
#define INCLUDE_MATH_DEFINES
#include <cmath> // M_PI
#include <algorithm>
#include <memory>
#include <iostream>
#include <cstring>

typedef struct
{
	Uint8 ucEnableBits1;              /* 0 */
	Uint8 ucEnableBits2;              /* 1 */
	Uint8 ucRumbleRight;              /* 2 */
	Uint8 ucRumbleLeft;               /* 3 */
	Uint8 ucHeadphoneVolume;          /* 4 */
	Uint8 ucSpeakerVolume;            /* 5 */
	Uint8 ucMicrophoneVolume;         /* 6 */
	Uint8 ucAudioEnableBits;          /* 7 */
	Uint8 ucMicLightMode;             /* 8 */
	Uint8 ucAudioMuteBits;            /* 9 */
	Uint8 rgucRightTriggerEffect[11]; /* 10 */
	Uint8 rgucLeftTriggerEffect[11];  /* 21 */
	Uint8 rgucUnknown1[6];            /* 32 */
	Uint8 ucLedFlags;                 /* 38 */
	Uint8 rgucUnknown2[2];            /* 39 */
	Uint8 ucLedAnim;                  /* 41 */
	Uint8 ucLedBrightness;            /* 42 */
	Uint8 ucPadLights;                /* 43 */
	Uint8 ucLedRed;                   /* 44 */
	Uint8 ucLedGreen;                 /* 45 */
	Uint8 ucLedBlue;                  /* 46 */
} DS5EffectsState_t;

struct ControllerDevice
{
	ControllerDevice(int id)
	  : _has_accel(false)
	  , _has_gyro(false)
	{
		_prevTouchState.t0Down = false;
		_prevTouchState.t1Down = false;
		if (SDL_IsGameController(id))
		{
			_sdlController = SDL_GameControllerOpen(id);

			if (_sdlController)
			{
				_has_gyro = SDL_GameControllerHasSensor(_sdlController, SDL_SENSOR_GYRO);
				_has_accel = SDL_GameControllerHasSensor(_sdlController, SDL_SENSOR_ACCEL);

				if (_has_gyro)
				{
					SDL_GameControllerSetSensorEnabled(_sdlController, SDL_SENSOR_GYRO, SDL_TRUE);
				}
				if (_has_accel)
				{
					SDL_GameControllerSetSensorEnabled(_sdlController, SDL_SENSOR_ACCEL, SDL_TRUE);
				}

				int vid = SDL_GameControllerGetVendor(_sdlController);
				int pid = SDL_GameControllerGetProduct(_sdlController);

				auto sdl_ctrlr_type = SDL_GameControllerGetType(_sdlController);
				switch (sdl_ctrlr_type)
				{
				case SDL_GameControllerType::SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
					_ctrlr_type = JS_TYPE_JOYCON_LEFT;
					_split_type = JS_SPLIT_TYPE_LEFT;
					break;
				case SDL_GameControllerType::SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
					_ctrlr_type = JS_TYPE_JOYCON_RIGHT;
					_split_type = JS_SPLIT_TYPE_RIGHT;
					break;
				case SDL_GameControllerType::SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
				case SDL_GameControllerType::SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO:
						_ctrlr_type = JS_TYPE_PRO_CONTROLLER;
					break;
				case SDL_GameControllerType::SDL_CONTROLLER_TYPE_PS4:
					_ctrlr_type = JS_TYPE_DS4;
					break;
				case SDL_GameControllerType::SDL_CONTROLLER_TYPE_PS5:
					_ctrlr_type = JS_TYPE_DS;
					break;
				case SDL_GameControllerType::SDL_CONTROLLER_TYPE_XBOXONE:
                    _ctrlr_type = JS_TYPE_XBOXONE;
                    if (vid == 0x0e6f) // PDP Vendor ID
                    {    
						_ctrlr_type = JS_TYPE_XBOX_SERIES;
                    }
                    if (vid == 0x24c6) // PowerA
                    {
						_ctrlr_type = JS_TYPE_XBOX_SERIES;
                    }
                    if (vid == 0x045e) // Microsoft Vendor ID
                    {
                        switch (pid)
                        {
                            case(0x02e3): // Xbox Elite Series 1
                            // Intentional fall through to the next case
                            case(0x0b05): //Xbox Elite Series 2 - Bluetooth
                            // Intentional fall through to the next case
                            case(0x0b00): //Xbox Elite Series 2
                            case (0x02ff): //XBOXGIP driver software PID - not sure what this is, might be from Valve's driver for using Elite paddles
                                           // in any case, this is what my ELite Series 2 is showing as currently, so adding it here for now    
                                _ctrlr_type = JS_TYPE_XBOXONE_ELITE;
                                break;
                            case(0x0b12): //Xbox Series controller
                                // Intentional fall through to the next case
                            case(0x0b13): // Xbox Series controller - bluetooth
                                _ctrlr_type = JS_TYPE_XBOX_SERIES;
                                break;
                        }
                    }
                    break;   
				}
			}
		}
	}

	virtual ~ControllerDevice()
	{
		_micLight = 0;
		memset(&_leftTriggerEffect, 0, sizeof(_leftTriggerEffect));
		memset(&_rightTriggerEffect, 0, sizeof(_rightTriggerEffect));
		_big_rumble = 0;
		_small_rumble = 0;
		SendEffect();
		SDL_GameControllerClose(_sdlController);
	}

	inline bool isValid()
	{
		return _sdlController != nullptr;
	}

private:
	void LoadTriggerEffect(Uint8 *rgucTriggerEffect, const AdaptiveTriggerSetting *trigger_effect)
	{
		using namespace ExtendInput::DataTools::DualSense;
		//ExtendInput::DataTools::DualSense::TriggerEffectGenerator::Bow(rgucTriggerEffect, 0, 0, 5, 3, 8);
		//ExtendInput::DataTools::DualSense::TriggerEffectGenerator::Galloping(rgucTriggerEffect, 0, 0, 9, 3, 5, 3);
		rgucTriggerEffect[0] = (uint8_t)trigger_effect->mode;
		switch (trigger_effect->mode)
		{
        case AdaptiveTriggerMode::RESISTANCE_RAW:
			TriggerEffectGenerator::SimpleResistance(rgucTriggerEffect, 0, trigger_effect->start, trigger_effect->force);
			break;
		case AdaptiveTriggerMode::SEGMENT:
			rgucTriggerEffect[1] = trigger_effect->start;
			rgucTriggerEffect[2] = trigger_effect->end;
			rgucTriggerEffect[3] = trigger_effect->force >> 8;
			break;
		case AdaptiveTriggerMode::RESISTANCE:
			TriggerEffectGenerator::Resistance(rgucTriggerEffect, 0, trigger_effect->start, trigger_effect->force);
			break;
		case AdaptiveTriggerMode::BOW:
			TriggerEffectGenerator::Bow(rgucTriggerEffect, 0, trigger_effect->start, trigger_effect->end, trigger_effect->force, trigger_effect->forceExtra);
			break;
		case AdaptiveTriggerMode::GALLOPING:
			TriggerEffectGenerator::Galloping(rgucTriggerEffect, 0, trigger_effect->start, trigger_effect->end, trigger_effect->force, trigger_effect->forceExtra, trigger_effect->frequency);
			break;
	    case AdaptiveTriggerMode::SEMI_AUTOMATIC:
			TriggerEffectGenerator::SemiAutomaticGun(rgucTriggerEffect, 0, trigger_effect->start, trigger_effect->end, trigger_effect->force);
			break;
		case AdaptiveTriggerMode::AUTOMATIC:
			TriggerEffectGenerator::AutomaticGun(rgucTriggerEffect, 0, trigger_effect->start, trigger_effect->force, trigger_effect->frequency);
			break;
		case AdaptiveTriggerMode::MACHINE:
			TriggerEffectGenerator::Machine(rgucTriggerEffect, 0, trigger_effect->start, trigger_effect->end, trigger_effect->force, trigger_effect->forceExtra, trigger_effect->frequency, trigger_effect->frequencyExtra);
			break;
		default:
			rgucTriggerEffect[0] = 0x05; // no effect
		}
	}

public:
	void SendEffect()
	{
		if (_ctrlr_type == JS_TYPE_DS)
		{
			DS5EffectsState_t effectPacket;
			memset(&effectPacket, 0, sizeof(effectPacket));

			// Add adaptive trigger data
			effectPacket.ucEnableBits1 |= 0x08 | 0x04; // Enable left and right trigger effect respectively
			LoadTriggerEffect(effectPacket.rgucLeftTriggerEffect, &_leftTriggerEffect);
			LoadTriggerEffect(effectPacket.rgucRightTriggerEffect, &_rightTriggerEffect);

			// Add current rumbling data
			effectPacket.ucEnableBits1 |= 0x01 | 0x02;
			effectPacket.ucRumbleLeft = _big_rumble >> 8;
			effectPacket.ucRumbleRight = _small_rumble >> 8;

			// Add current mic light
			effectPacket.ucEnableBits2 |= 0x01;      /* Enable microphone light */
			effectPacket.ucMicLightMode = _micLight; /* Bitmask, 0x00 = off, 0x01 = solid, 0x02 = pulse */

			// Send to controller
			SDL_GameControllerSendEffect(_sdlController, &effectPacket, sizeof(effectPacket));
		}
	}

	bool _has_gyro;
	bool _has_accel;
	int _split_type = JS_SPLIT_TYPE_FULL;
	int _ctrlr_type = 0;
	uint16_t _small_rumble = 0;
	uint16_t _big_rumble = 0;
	AdaptiveTriggerSetting _leftTriggerEffect;
	AdaptiveTriggerSetting _rightTriggerEffect;
	uint8_t _micLight = 0;
	SDL_GameController *_sdlController = nullptr;
	TOUCH_STATE _prevTouchState;
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
		SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_XBOX, "1");
		SDL_SetHint(SDL_HINT_JOYSTICK_THREAD, "1");
		SDL_Init(SDL_INIT_GAMECONTROLLER);
	}

	virtual ~SdlInstance()
	{
		SDL_Quit();
	}

	int pollDevices()
	{
		while (keep_polling)
		{
			auto tick_time = SettingsManager::get<float>(SettingID::TICK_TIME)->value();
			SDL_Delay(Uint32(tick_time));

			std::lock_guard guard(controller_lock);
			SDL_GameControllerUpdate();
			for (auto iter = _controllerMap.begin(); iter != _controllerMap.end(); ++iter)
			{
				if (g_callback)
				{
					JOY_SHOCK_STATE dummy1;
					IMU_STATE dummy2;
					memset(&dummy1, 0, sizeof(dummy1));
					memset(&dummy2, 0, sizeof(dummy2));
					g_callback(iter->first, dummy1, dummy1, dummy2, dummy2, tick_time);
				}
				if (g_touch_callback)
				{
					TOUCH_STATE touch = GetTouchState(iter->first, false);
					g_touch_callback(iter->first, touch, iter->second->_prevTouchState, tick_time);
					iter->second->_prevTouchState = touch;
				}
				// Perform rumble
				SDL_GameControllerRumble(iter->second->_sdlController, iter->second->_big_rumble, iter->second->_small_rumble, Uint32(tick_time + 5));
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
			SDL_Thread* controller_polling_thread = SDL_CreateThread([] (void *obj)
			{
				  auto this_ = static_cast<SdlInstance *>(obj);
				  return this_->pollDevices();
			  },
			  "Poll Devices", this);
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
			else
			{
                deviceHandleArray[i] = -1;
				delete device;
			}
		}
		return int(_controllerMap.size());
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
			static constexpr float toDegPerSec = float(180. / M_PI);
			imuState.gyroX = gyro[0] * toDegPerSec;
			imuState.gyroY = gyro[1] * toDegPerSec;
			imuState.gyroZ = gyro[2] * toDegPerSec;
		}
		if (_controllerMap[deviceId]->_has_accel)
		{
			array<float, 3> accel;
			SDL_GameControllerGetSensorData(_controllerMap[deviceId]->_sdlController, SDL_SENSOR_ACCEL, &accel[0], 3);
			static constexpr float toGs = 1.f / 9.8f;
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
		if( SDL_GameControllerGetTouchpadFinger(_controllerMap[deviceId]->_sdlController, 0, 0, &state0, &state.t0X, &state.t0Y, nullptr) == 0 && 
			SDL_GameControllerGetTouchpadFinger(_controllerMap[deviceId]->_sdlController, 0, 1, &state1, &state.t1X, &state.t1Y, nullptr) == 0 )
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
			{ SDL_CONTROLLER_BUTTON_DPAD_RIGHT, JSOFFSET_RIGHT }
		};

		int buttons = 0;
		if (_controllerMap[deviceId]->_ctrlr_type == JS_TYPE_JOYCON_LEFT)
		{
			buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_A) > 0 ? 1 << JSOFFSET_LEFT : 0;
			buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_B) > 0 ? 1 << JSOFFSET_DOWN : 0;
			buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_X) > 0 ? 1 << JSOFFSET_UP : 0;
			buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_Y) > 0 ? 1 << JSOFFSET_RIGHT : 0;
			buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_GUIDE) > 0 ? 1 << JSOFFSET_CAPTURE : 0;
			buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_START) > 0 ? 1 << JSOFFSET_MINUS : 0;
			buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_LEFTSHOULDER) > 0 ? 1 << JSOFFSET_SL : 0;
			buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) > 0 ? 1 << JSOFFSET_SR : 0;
			buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_PADDLE2) > 0 ? 1 << JSOFFSET_L : 0;
			buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_PADDLE4) > 0 ? 1 << JSOFFSET_ZL : 0;
			buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_LEFTSTICK) > 0 ? 1 << JSOFFSET_LCLICK : 0;
		}
		else if (_controllerMap[deviceId]->_ctrlr_type == JS_TYPE_JOYCON_RIGHT)
		{
			buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_A) > 0 ? 1 << JSOFFSET_E : 0;
			buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_B) > 0 ? 1 << JSOFFSET_N : 0;
			buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_X) > 0 ? 1 << JSOFFSET_S : 0;
			buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_Y) > 0 ? 1 << JSOFFSET_W : 0;
			buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_GUIDE) > 0 ? 1 << JSOFFSET_HOME : 0;
			buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_START) > 0 ? 1 << JSOFFSET_PLUS : 0;
			buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_LEFTSHOULDER) > 0 ? 1 << JSOFFSET_SL : 0;
			buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) > 0 ? 1 << JSOFFSET_SR : 0;
			buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_PADDLE1) > 0 ? 1 << JSOFFSET_R : 0;
			buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_PADDLE3) > 0 ? 1 << JSOFFSET_ZR : 0;
			buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_LEFTSTICK) > 0 ? 1 << JSOFFSET_RCLICK : 0;
		}
		else
		{
			for (auto pair : sdl2jsl)
			{
				buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_GameControllerButton(pair.first)) > 0 ? 1 << pair.second : 0;
			}
			switch (_controllerMap[deviceId]->_ctrlr_type)
			{
			case JS_TYPE_DS:
				buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_MISC1) > 0 ? 1 << JSOFFSET_MIC : 0;
				buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_PADDLE1) > 0 ? 1 << JSOFFSET_SR : 0;
				buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_PADDLE2) > 0 ? 1 << JSOFFSET_SL : 0;
				buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_PADDLE3) > 0 ? 1 << JSOFFSET_FNR : 0;
				buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_PADDLE4) > 0 ? 1 << JSOFFSET_FNL : 0;
				// Intentional fall through to the next case
			case JS_TYPE_DS4:
				buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_TOUCHPAD) > 0 ? 1 << JSOFFSET_CAPTURE : 0;
				break;
			case JS_TYPE_PRO_CONTROLLER:
				buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_MISC1) > 0 ? 1 << JSOFFSET_CAPTURE : 0;
				buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_PADDLE1) > 0 ? 1 << JSOFFSET_SR : 0;
				buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_PADDLE2) > 0 ? 1 << JSOFFSET_SL : 0;
				buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_PADDLE3) > 0 ? 1 << JSOFFSET_FNR : 0;
				buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_PADDLE4) > 0 ? 1 << JSOFFSET_FNL : 0;
				break;
			default:
				buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_MISC1) > 0 ? 1 << JSOFFSET_CAPTURE : 0;
				buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_PADDLE3) > 0 ? 1 << JSOFFSET_FNL : 0;
				buttons |= SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_PADDLE1) > 0 ? 1 << JSOFFSET_FNR : 0;
				break;
			}
		}
		return buttons;
	}

	float GetLeftX(int deviceId) override
	{
		if (_controllerMap[deviceId]->_ctrlr_type == JS_TYPE_JOYCON_LEFT)
			return -SDL_GameControllerGetAxis(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_AXIS_LEFTY) / (float)SDL_JOYSTICK_AXIS_MAX;
		return SDL_GameControllerGetAxis(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_AXIS_LEFTX) / (float)SDL_JOYSTICK_AXIS_MAX;
	}

	float GetLeftY(int deviceId) override
	{
		if (_controllerMap[deviceId]->_ctrlr_type == JS_TYPE_JOYCON_LEFT)
			return -SDL_GameControllerGetAxis(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_AXIS_LEFTX) / (float)SDL_JOYSTICK_AXIS_MAX;
		return -SDL_GameControllerGetAxis(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_AXIS_LEFTY) / (float)SDL_JOYSTICK_AXIS_MAX;
	}

	float GetRightX(int deviceId) override
	{
		if (_controllerMap[deviceId]->_ctrlr_type == JS_TYPE_JOYCON_RIGHT)
			return -GetLeftY(deviceId);
		return SDL_GameControllerGetAxis(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_AXIS_RIGHTX) / (float)SDL_JOYSTICK_AXIS_MAX;
	}

	float GetRightY(int deviceId) override
	{
		if (_controllerMap[deviceId]->_ctrlr_type == JS_TYPE_JOYCON_RIGHT)
			return GetLeftX(deviceId);
		return -SDL_GameControllerGetAxis(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_AXIS_RIGHTY) / (float)SDL_JOYSTICK_AXIS_MAX;
	}

	float GetLeftTrigger(int deviceId) override
	{
		if (_controllerMap[deviceId]->_ctrlr_type == JS_TYPE_JOYCON_LEFT)
			return SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_PADDLE4) > 0 ? 1.f : 0.f;
		return SDL_GameControllerGetAxis(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_AXIS_TRIGGERLEFT) / (float)SDL_JOYSTICK_AXIS_MAX;
	}

	float GetRightTrigger(int deviceId) override
	{
		if (_controllerMap[deviceId]->_ctrlr_type == JS_TYPE_JOYCON_RIGHT)
			return SDL_GameControllerGetButton(_controllerMap[deviceId]->_sdlController, SDL_CONTROLLER_BUTTON_PADDLE3) > 0 ? 1.f : 0.f;
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
		_controllerMap[deviceId]->_small_rumble = clamp(smallRumble, 0, int(UINT16_MAX));
		_controllerMap[deviceId]->_big_rumble = clamp(bigRumble, 0, int(UINT16_MAX));
	}

	void SetPlayerNumber(int deviceId, int number) override
	{
		SDL_GameControllerSetPlayerIndex(_controllerMap[deviceId]->_sdlController, number);
	}

	void SetTriggerEffect(int deviceId, const AdaptiveTriggerSetting &_leftTriggerEffect, const AdaptiveTriggerSetting &_rightTriggerEffect) override
	{
		if (_leftTriggerEffect != _controllerMap[deviceId]->_leftTriggerEffect || _rightTriggerEffect != _controllerMap[deviceId]->_rightTriggerEffect)
		{
			// Update active trigger effect
			_controllerMap[deviceId]->_leftTriggerEffect = _leftTriggerEffect;
			_controllerMap[deviceId]->_rightTriggerEffect = _rightTriggerEffect;

			_controllerMap[deviceId]->SendEffect();
		}
	}

	virtual void SetMicLight(int deviceId, uint8_t mode) override
	{
		if (mode != _controllerMap[deviceId]->_micLight)
		{
			_controllerMap[deviceId]->_micLight = mode;

			_controllerMap[deviceId]->SendEffect();
		}
	}
};

JslWrapper *JslWrapper::getNew()
{
	return new SdlInstance();
}
