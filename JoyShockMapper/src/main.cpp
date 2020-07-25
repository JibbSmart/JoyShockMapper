#include "JoyShockMapper.h"
#include "JoyShockLibrary.h"
#include "InputHelpers.h"
#include "Whitelister.h"
#include "TrayIcon.h"
#include "JSMAssignment.hpp"

#include <unordered_map>
#include <mutex>
#include <deque>

#pragma warning(disable:4996) // Disable deprecated API warnings

#define PI 3.14159265359f

class JoyShock;
void joyShockPollCallback(int jcHandle, JOY_SHOCK_STATE state, JOY_SHOCK_STATE lastState, IMU_STATE imuState, IMU_STATE lastImuState, float deltaTime);

// Contains all settings that can be modeshifted. They should be accessed only via Joyshock::getSetting
JSMSetting<StickMode> left_stick_mode = JSMSetting<StickMode>(SettingID::LEFT_STICK_MODE, StickMode::NO_MOUSE);
JSMSetting<StickMode> right_stick_mode = JSMSetting<StickMode>(SettingID::RIGHT_STICK_MODE, StickMode::NO_MOUSE);
JSMSetting<RingMode> left_ring_mode = JSMSetting<RingMode>(SettingID::LEFT_RING_MODE, RingMode::OUTER);
JSMSetting<RingMode> right_ring_mode = JSMSetting<RingMode>(SettingID::LEFT_RING_MODE, RingMode::OUTER);
JSMSetting<GyroAxisMask> mouse_x_from_gyro = JSMSetting<GyroAxisMask>(SettingID::MOUSE_X_FROM_GYRO_AXIS, GyroAxisMask::NONE);
JSMSetting<GyroAxisMask> mouse_y_from_gyro = JSMSetting<GyroAxisMask>(SettingID::MOUSE_Y_FROM_GYRO_AXIS, GyroAxisMask::NONE);
JSMSetting<GyroSettings> gyro_settings = JSMSetting<GyroSettings>(SettingID::GYRO_ON, GyroSettings()); // Ignore mode none means no GYRO_OFF button
JSMSetting<JoyconMask> joycon_gyro_mask = JSMSetting<JoyconMask>(SettingID::JOYCON_GYRO_MASK, JoyconMask::IGNORE_LEFT);
JSMSetting<TriggerMode> zlMode = JSMSetting<TriggerMode>(SettingID::ZL_MODE, TriggerMode::NO_FULL);
JSMSetting<TriggerMode> zrMode = JSMSetting<TriggerMode>(SettingID::ZR_MODE, TriggerMode::NO_FULL);;
JSMSetting<FlickSnapMode> flick_snap_mode = JSMSetting<FlickSnapMode>(SettingID::FLICK_SNAP_MODE, FlickSnapMode::NONE);
JSMSetting<FloatXY> min_gyro_sens = JSMSetting<FloatXY>(SettingID::MIN_GYRO_SENS, { 0.0f, 0.0f });
JSMSetting<FloatXY> max_gyro_sens = JSMSetting<FloatXY>(SettingID::MAX_GYRO_SENS, { 0.0f, 0.0f });
JSMSetting<float> min_gyro_threshold = JSMSetting<float>(SettingID::MIN_GYRO_THRESHOLD, 0.0f);
JSMSetting<float> max_gyro_threshold = JSMSetting<float>(SettingID::MAX_GYRO_THRESHOLD, 0.0f);
JSMSetting<float> stick_power = JSMSetting<float>(SettingID::STICK_POWER, 1.0f);
JSMSetting<float> stick_sens = JSMSetting<float>(SettingID::STICK_SENS, 360.0f);
// There's an argument that RWC has no interest in being modeshifted and thus could be outside this structure.
JSMSetting<float> real_world_calibration = JSMSetting<float>(SettingID::REAL_WORLD_CALIBRATION, 40.0f);
JSMSetting<float> in_game_sens = JSMSetting<float>(SettingID::IN_GAME_SENS, 1.0f);
JSMSetting<float> trigger_threshold = JSMSetting<float>(SettingID::TRIGGER_THRESHOLD, 0.0f);
JSMSetting<AxisMode> aim_x_sign = JSMSetting<AxisMode>(SettingID::STICK_AXIS_X, AxisMode::STANDARD);
JSMSetting<AxisMode> aim_y_sign = JSMSetting<AxisMode>(SettingID::STICK_AXIS_Y, AxisMode::STANDARD);
JSMSetting<AxisMode> gyro_y_sign = JSMSetting<AxisMode>(SettingID::GYRO_AXIS_Y, AxisMode::STANDARD);
JSMSetting<AxisMode> gyro_x_sign = JSMSetting<AxisMode>(SettingID::GYRO_AXIS_X, AxisMode::STANDARD);
JSMSetting<float> flick_time = JSMSetting<float>(SettingID::FLICK_TIME, 0.1f);
JSMSetting<float> gyro_smooth_time = JSMSetting<float>(SettingID::GYRO_SMOOTH_TIME, 0.125f);
JSMSetting<float> gyro_smooth_threshold = JSMSetting<float>(SettingID::GYRO_SMOOTH_THRESHOLD, 0.0f);
JSMSetting<float> gyro_cutoff_speed = JSMSetting<float>(SettingID::GYRO_CUTOFF_SPEED, 0.0f);
JSMSetting<float> gyro_cutoff_recovery = JSMSetting<float>(SettingID::GYRO_CUTOFF_RECOVERY, 0.0f);
JSMSetting<float> stick_acceleration_rate = JSMSetting<float>(SettingID::STICK_ACCELERATION_RATE, 0.0f);
JSMSetting<float> stick_acceleration_cap = JSMSetting<float>(SettingID::STICK_ACCELERATION_CAP, 1000000.0f);
JSMSetting<float> stick_deadzone_inner = JSMSetting<float>(SettingID::STICK_DEADZONE_INNER, 0.15f);
JSMSetting<float> stick_deadzone_outer = JSMSetting<float>(SettingID::STICK_DEADZONE_OUTER, 0.1f);
JSMSetting<float> mouse_ring_radius = JSMSetting<float>(SettingID::MOUSE_RING_RADIUS, 128.0f);
JSMSetting<float> screen_resolution_x = JSMSetting<float>(SettingID::SCREEN_RESOLUTION_X, 1920.0f);
JSMSetting<float> screen_resolution_y = JSMSetting<float>(SettingID::SCREEN_RESOLUTION_Y, 1080.0f);
JSMSetting<float> rotate_smooth_override = JSMSetting<float>(SettingID::ROTATE_SMOOTH_OVERRIDE, -1.0f);
JSMSetting<float> flick_snap_strength = JSMSetting<float>(SettingID::FLICK_SNAP_STRENGTH, 01.0f);
vector<JSMButton> mappings; // array enables use of for each loop and other i/f

mutex loading_lock;


float os_mouse_speed = 1.0;
float last_flick_and_rotation = 0.0;
unique_ptr<PollingThread> autoLoadThread;
unique_ptr<TrayIcon> tray;
bool devicesCalibrating = false;
Whitelister whitelister(false);
unordered_map<int, JoyShock*> handle_to_joyshock;

// This class holds all the logic related to a single digital button. It does not hold the mapping but only a reference
// to it. It also contains it's various states, flags and data.
class DigitalButton
{
public:
	// All digital buttons need a reference to the same instance of a the common structure within the same controller.
	// It enables the buttons to synchroonize and be aware of the state of the whole controller.
	struct Common {
		Common(int handle = 0)
			: intHandle(handle)
		{
			chordStack.push_front(ButtonID::NONE); //Always hold mapping none at the end to handle modeshifts and chords
		}
		bool toggleContinuous = false;
		deque<pair<ButtonID, KeyCode>> gyroActionQueue; // Queue of gyro control actions currently in effect
		deque<ButtonID> chordStack; // Represents the current active buttons in order from most recent to latest
		int intHandle;
	};

	DigitalButton(DigitalButton::Common &btnCommon, ButtonID id)
		: _common(btnCommon)
		, _id(id)
		, _name(magic_enum::enum_name(id))
		, _mapping(mappings[int(_id)])
		, _press_times()
		, _btnState(BtnState::NoPress)
		, _keyToRelease()
		, _nameToRelease()
	{

	}

	Common &_common;
	const ButtonID _id;
	const string _name; // Display name of the mapping
	const JSMButton &_mapping;
	chrono::steady_clock::time_point _press_times;
	BtnState _btnState = BtnState::NoPress;
	KeyCode _keyToRelease; // At key press, remember what to release
	string _nameToRelease;

	float GetTapDuration()
	{
		// for tap duration, we need to know if the key in question is a gyro-related mapping or not
		KeyCode mapping = GetPressMapping(); //Consider chords
		return (mapping.code >= GYRO_INV_X && mapping.code <= GYRO_ON_BIND) ? MAGIC_GYRO_TAP_DURATION : MAGIC_TAP_DURATION;
	}

	KeyCode GetPressMapping()
	{
		// Look at active chord mappings starting with the latest activates chord
		for (auto activeChord = _common.chordStack.begin(); activeChord != _common.chordStack.end(); activeChord++)
		{
			auto binding = _mapping.get(*activeChord);
			if (binding)
			{
				_keyToRelease = Mapping(*binding).pressBind;
				_nameToRelease = _mapping.getName(*activeChord);
				return _keyToRelease;
			}
		}
		// Chord stack should always include NONE which will provide a value in the loop above
		return KeyCode::EMPTY;
	}

	bool HasHoldMapping()
	{
		// Look at active chord mappings starting with the latest activates chord
		for (auto activeChord = _common.chordStack.begin(); activeChord != _common.chordStack.end(); activeChord++)
		{
			auto binding = _mapping.get(*activeChord);
			if(binding) 
			{
				return Mapping(*binding).holdBind;
			}
		}
		// Chord stack should always include NONE which will provide a value in the loop above
		return false;
	}

	KeyCode GetHoldMapping()
	{
		// Look at active chord mappings starting with the latest activates chord
		for (auto activeChord = _common.chordStack.begin(); activeChord != _common.chordStack.end(); activeChord++)
		{
			auto binding = _mapping.get(*activeChord);
			if (binding)
			{
				_keyToRelease = Mapping(*binding).holdBind;
				_nameToRelease = _mapping.getName(*activeChord);
				return _keyToRelease;
			}
		}
		_keyToRelease = Mapping(_mapping).holdBind;
		_nameToRelease = _name;
		return _keyToRelease;
	}

	void ApplyBtnPress(bool tap = false)
	{
		auto key = GetPressMapping();
		if (key.code == CALIBRATE)
		{
			_common.toggleContinuous ^= tap; //Toggle on tap
			if (!tap || _common.toggleContinuous) {
				printf("Starting continuous calibration\n");
				JslResetContinuousCalibration(_common.intHandle);
				JslStartContinuousCalibration(_common.intHandle);
			}
		}
		else if (key.code >= GYRO_INV_X && key.code <= GYRO_ON_BIND)
		{
			_common.gyroActionQueue.push_back({ _id, key });
		}
		else
		{
			printf("%s: %s\n", _nameToRelease.c_str(), tap ? "tapped" : "true");
			pressKey(key, true);
		}
		_keyToRelease = key;
		//if (find(_common.chordStack.begin(), _common.chordStack.end(), _id) == _common.chordStack.end())
		//{
		//	_common.chordStack.push_front(_id); // Always push at the fromt to make it a stack
		//}
	}

	void ApplyBtnHold()
	{
		auto key = GetHoldMapping();
		if (key.code == CALIBRATE)
		{
			printf("Starting continuous calibration\n");
			JslResetContinuousCalibration(_common.intHandle);
			JslStartContinuousCalibration(_common.intHandle);
		}
		else if (key.code >= GYRO_INV_X && key.code <= GYRO_ON_BIND)
		{
			_common.gyroActionQueue.push_back({ _id, key });
		}
		else if (key.code != NO_HOLD_MAPPED)
		{
			printf("%s: held\n", _nameToRelease.c_str());
			pressKey(key, true);
		}
		_keyToRelease = key;
		//if (find(_common.chordStack.begin(), _common.chordStack.end(), _id) == _common.chordStack.end())
		//{
		//	_common.chordStack.push_front(_id); // Always push at the fromt to make it a stack
		//}
	}

	void ApplyBtnRelease(bool tap = false)
	{
		if (_keyToRelease.code == CALIBRATE)
		{
			if (!tap || !_common.toggleContinuous)
			{
				JslPauseContinuousCalibration(_common.intHandle);
				_common.toggleContinuous = false; // if we've held the calibration button, we're disabling continuous calibration
				printf("Gyro calibration set\n");
			}
		}
		else if (_keyToRelease.code >= GYRO_INV_X && _keyToRelease.code <= GYRO_ON_BIND)
		{
			_common.gyroActionQueue.erase(find_if(_common.gyroActionQueue.begin(), _common.gyroActionQueue.end(),
				[this](auto pair)
				{
					return pair.first == _id;
				}));
		}
		else if (_keyToRelease.code != NO_HOLD_MAPPED)
		{
			printf(tap ? "" : "%s: false\n", _nameToRelease.c_str()); // Is this good coding? [Insert meme]
			pressKey(_keyToRelease, false);
		}
		//auto foundChord = find(_common.chordStack.begin(), _common.chordStack.end(), _id);
		//if (foundChord != _common.chordStack.end())
		//{
		//	// The chord is released
		//	_common.chordStack.erase(foundChord);
		//}
	}

	void ApplyBtnPress(const ComboMap &map, bool tap = false)
	{
		_keyToRelease = Mapping(map.second).pressBind;
		_nameToRelease = _mapping.getSimPressName(map.first);
		if (_keyToRelease.code == CALIBRATE)
		{
			_common.toggleContinuous ^= tap; //Toggle on tap
			if (!tap || _common.toggleContinuous) {
				printf("Starting continuous calibration\n");
				JslResetContinuousCalibration(_common.intHandle);
				JslStartContinuousCalibration(_common.intHandle);
			}
		}
		else if (_keyToRelease.code >= GYRO_INV_X && _keyToRelease.code <= GYRO_ON_BIND)
		{
			// I know I don't handle multiple inversion. Otherwise GYRO_INV_X on sim press would do nothing
			_common.gyroActionQueue.push_back({ _id, _keyToRelease });
		}
		else
		{
			printf("%s: %s\n", _nameToRelease.c_str(), tap ? "tapped" : "true");
			pressKey(_keyToRelease, true);
		}
	}

	void ApplyBtnHold(const ComboMap &map)
	{
		_keyToRelease = Mapping(map.second).holdBind;
		_nameToRelease = _mapping.getSimPressName(map.first);
		if (_keyToRelease.code == CALIBRATE)
		{
			printf("Starting continuous calibration\n");
			JslResetContinuousCalibration(_common.intHandle);
			JslStartContinuousCalibration(_common.intHandle);
		}
		else if (_keyToRelease.code >= GYRO_INV_X && _keyToRelease.code <= GYRO_ON_BIND)
		{
			// I know I don't handle multiple inversion. Otherwise GYRO_INV_X on sim press would do nothing
			_common.gyroActionQueue.push_back({ _id, _keyToRelease });
		}
		else if (_keyToRelease.code != NO_HOLD_MAPPED)
		{
			printf("%s: held\n", _nameToRelease.c_str());
			pressKey(_keyToRelease, true);
		}
	}

	void ApplyBtnRelease(const ComboMap &map, bool tap = false)
	{
		if (_keyToRelease.code == CALIBRATE)
		{
			if (!tap || !_common.toggleContinuous)
			{
				JslPauseContinuousCalibration(_common.intHandle);
				_common.toggleContinuous = false; // if we've held the calibration button, we're disabling continuous calibration
				printf("Gyro calibration set\n");
			}
		}
		else if (_keyToRelease.code >= GYRO_INV_X && _keyToRelease.code <= GYRO_ON_BIND)
		{
			_common.gyroActionQueue.erase(find_if(_common.gyroActionQueue.begin(), _common.gyroActionQueue.end(),
				[this](auto pair)
				{
					return pair.first == _id;
				}));
			_common.gyroActionQueue.erase(find_if(_common.gyroActionQueue.begin(), _common.gyroActionQueue.end(),
				[map](auto pair)
				{
					return pair.first == map.first;
				}));
		}
		else if (_keyToRelease.code != NO_HOLD_MAPPED)
		{
			printf(tap ? "" : "%s: false\n", _nameToRelease.c_str());
			pressKey(_keyToRelease, false);
		}
		//auto foundChord = find(_common.chordStack.begin(), _common.chordStack.end(), _id);
		//if (foundChord != _common.chordStack.end())
		//{
		//	// The chord is released
		//	_common.chordStack.erase(foundChord);
		//}
	}

	void SyncSimPress(ButtonID btn, const ComboMap &map)
	{
		_keyToRelease = Mapping(map.second).pressBind;
		_nameToRelease = mappings[int(btn)].getSimPressName(map.first);
		if (_keyToRelease.code >= GYRO_INV_X && _keyToRelease.code <= GYRO_ON_BIND)
		{
			_common.gyroActionQueue.push_back({ _id, _keyToRelease });
		}
	}

	void SyncSimHold(ButtonID btn, const ComboMap &map)
	{
		_keyToRelease = Mapping(map.second).holdBind;
		_nameToRelease = mappings[int(btn)].getSimPressName(map.first);
		if (_keyToRelease.code >= GYRO_INV_X && _keyToRelease.code <= GYRO_ON_BIND)
		{
			// I know I don't handle multiple inversion. Otherwise GYRO_INV_X on sim press would do nothing
			_common.gyroActionQueue.push_back({ _id, _keyToRelease });
		}
	}

	// Pretty wrapper
	inline float GetPressDurationMS(chrono::steady_clock::time_point time_now)
	{
		return static_cast<float>(chrono::duration_cast<chrono::milliseconds>(time_now - _press_times).count());
	}

	// Indicate if the button is currently sending an assigned mapping.
	inline bool IsActive()
	{
		return _btnState == BtnState::BtnPress || _btnState == BtnState::HoldPress; // Add Sim Press State? Only with Setting?
	}
};

// An instance of this class represents a single controller device that JSM is listening to.
class JoyShock {
private:
	float _weightsRemaining[64];
	float _flickSamples[64];
	int _frontSample = 0;

	FloatXY _gyroSamples[64];
	int _frontGyroSample = 0;

	template<typename E1, typename E2>
	static inline optional<E1> GetOptionalSetting(const JSMSetting<E2> &setting, ButtonID chord) 
	{
		return setting.get(chord) ? optional<E1>(static_cast<E1>(*setting.get(chord))) : nullopt;
	}

	int keyToBitOffset(ButtonID index) {
		switch (index) {
		case ButtonID::UP:
			return JSOFFSET_UP;
		case ButtonID::DOWN:
			return JSOFFSET_DOWN;
		case ButtonID::LEFT:
			return JSOFFSET_LEFT;
		case ButtonID::RIGHT:
			return JSOFFSET_RIGHT;
		case ButtonID::L:
			return JSOFFSET_L;
		case ButtonID::ZL:
			return JSOFFSET_ZL;
		case ButtonID::MINUS:
			return JSOFFSET_MINUS;
		case ButtonID::CAPTURE:
			return JSOFFSET_CAPTURE;
		case ButtonID::E:
			return JSOFFSET_E;
		case ButtonID::S:
			return JSOFFSET_S;
		case ButtonID::N:
			return JSOFFSET_N;
		case ButtonID::W:
			return JSOFFSET_W;
		case ButtonID::R:
			return JSOFFSET_R;
		case ButtonID::ZR:
			return JSOFFSET_ZR;
		case ButtonID::PLUS:
			return JSOFFSET_PLUS;
		case ButtonID::HOME:
			return JSOFFSET_HOME;
		case ButtonID::SL:
			return JSOFFSET_SL;
		case ButtonID::SR:
			return JSOFFSET_SR;
		case ButtonID::L3:
			return JSOFFSET_LCLICK;
		case ButtonID::R3:
			return JSOFFSET_RCLICK;
		}
		stringstream ss;
		ss << "Button " << index << " is not valid ";
		throw exception(ss.str().c_str());
	}

public:
	const int MaxGyroSamples = 64;
	const int NumSamples = 64;
	int intHandle;

	vector<DigitalButton> buttons;
	chrono::steady_clock::time_point started_flick;
	chrono::steady_clock::time_point time_now;
	// tap_release_queue has been replaced with button states *TapRelease. The hold time of the tap is effectively quantized to the polling period of the device.
	bool is_flicking_left = false;
	bool is_flicking_right = false;
	float delta_flick = 0.0;
	float flick_percent_done = 0.0;
	float flick_rotation_counter = 0.0;
	FloatXY left_last_cal;
	FloatXY right_last_cal;

	float poll_rate;
	int controller_type = 0;
	float stick_step_size;

	float left_acceleration = 1.0;
	float right_acceleration = 1.0;
	vector<DstState> triggerState; // State of analog triggers when skip mode is active
	DigitalButton::Common btnCommon;

	// Modeshifting the stick mode can create quirky behaviours on transition. These flags 
	// will be set upon returning to standard mode and ignore stick inputs until the stick 
	// returns to neutral
	bool ignore_left_stick_mode = false;
	bool ignore_right_stick_mode = false;

	JoyShock(int uniqueHandle, float pollRate, int controllerSplitType, float stickStepSize)
		: intHandle(uniqueHandle)
		, poll_rate(pollRate)
		, controller_type(controllerSplitType)
		, stick_step_size(stickStepSize)
		, triggerState(NUM_ANALOG_TRIGGERS, DstState::NoPress)
		, btnCommon(intHandle)
		, buttons()
	{
		buttons.reserve(MAPPING_SIZE);
		for (int i = 0; i < MAPPING_SIZE; ++i)
		{
			buttons.push_back( DigitalButton(btnCommon, ButtonID(i)) );
		}
	}


	~JoyShock()
	{}

	template<typename E>
	E getSetting(SettingID index)
	{
		static_assert(is_enum<E>::value, "Parameter of JoyShock::getSetting<E> has to be an enum type");
		// Look at active chord mappings starting with the latest activates chord
		for (auto activeChord = btnCommon.chordStack.begin(); activeChord != btnCommon.chordStack.end(); activeChord++)
		{
			optional<E> opt;
			switch (index) {
			case SettingID::MOUSE_X_FROM_GYRO_AXIS:
				opt = GetOptionalSetting<E>(mouse_x_from_gyro, *activeChord);
				break;
			case SettingID::MOUSE_Y_FROM_GYRO_AXIS:
				opt = GetOptionalSetting<E>(mouse_y_from_gyro, *activeChord);
				break;
			case SettingID::LEFT_STICK_MODE:
				opt = GetOptionalSetting<E>(left_stick_mode, *activeChord);
				if (ignore_left_stick_mode && *activeChord == ButtonID::NONE)
					opt = optional<E>(static_cast<E>(StickMode::INVALID));
				else
					ignore_left_stick_mode |= (opt && *activeChord != ButtonID::NONE);
				break;
			case SettingID::RIGHT_STICK_MODE:
				opt = GetOptionalSetting<E>(right_stick_mode, *activeChord);
				if (ignore_right_stick_mode && *activeChord == ButtonID::NONE)
					opt = optional<E>(static_cast<E>(StickMode::INVALID));
				else
					ignore_right_stick_mode |= (opt && *activeChord != ButtonID::NONE);
				break;
			case SettingID::LEFT_RING_MODE:
				opt = GetOptionalSetting<E>(left_ring_mode, *activeChord);
				break;
			case SettingID::RIGHT_RING_MODE:
				opt = GetOptionalSetting<E>(right_ring_mode, *activeChord);
				break;
			case SettingID::JOYCON_GYRO_MASK:
				opt = GetOptionalSetting<E>(joycon_gyro_mask, *activeChord);
				break;
			case SettingID::ZR_MODE:
				opt = GetOptionalSetting<E>(zrMode, *activeChord);
				break;
			case SettingID::ZL_MODE:
				opt = GetOptionalSetting<E>(zlMode, *activeChord);
				break;
			case SettingID::FLICK_SNAP_MODE:
				opt = GetOptionalSetting<E>(flick_snap_mode, *activeChord);
				break;
			}
			if (opt) return *opt;
		}
		stringstream ss;
		ss << "Index " << index << " is not a valid enum setting";
		throw exception(ss.str().c_str());
	}

	float getSetting(SettingID index)
	{
		// Look at active chord mappings starting with the latest activates chord
		for (auto activeChord = btnCommon.chordStack.begin(); activeChord != btnCommon.chordStack.end(); activeChord++)
		{
			optional<float> opt;
			switch (index)
			{
			case SettingID::MIN_GYRO_THRESHOLD:
				opt = min_gyro_threshold.get(*activeChord);
				break;
			case SettingID::MAX_GYRO_THRESHOLD:
				opt = max_gyro_threshold.get(*activeChord);
				break;
			case SettingID::STICK_POWER:
				opt = stick_power.get(*activeChord);
				break;
			case SettingID::STICK_SENS:
				opt = stick_sens.get(*activeChord);
				break;
			case SettingID::REAL_WORLD_CALIBRATION:
				opt = real_world_calibration.get(*activeChord);
				break;
			case SettingID::IN_GAME_SENS:
				opt = in_game_sens.get(*activeChord);
				break;
			case SettingID::TRIGGER_THRESHOLD:
				opt = trigger_threshold.get(*activeChord);
				break;
			case SettingID::STICK_AXIS_X:
				opt = GetOptionalSetting<float>(aim_x_sign, *activeChord);
				break;
			case SettingID::STICK_AXIS_Y:
				opt = GetOptionalSetting<float>(aim_y_sign, *activeChord);
				break;
			case SettingID::GYRO_AXIS_X:
				opt = GetOptionalSetting<float>(gyro_x_sign, *activeChord);
				break;
			case SettingID::GYRO_AXIS_Y:
				opt = GetOptionalSetting<float>(gyro_y_sign, *activeChord);
				break;
			case SettingID::FLICK_TIME:
				opt = flick_time.get(*activeChord);
				break;
			case SettingID::GYRO_SMOOTH_THRESHOLD:
				opt = gyro_smooth_threshold.get(*activeChord);
				break;
			case SettingID::GYRO_SMOOTH_TIME:
				opt = gyro_smooth_time.get(*activeChord);
				break;
			case SettingID::GYRO_CUTOFF_SPEED:
				opt = gyro_cutoff_speed.get(*activeChord);
				break;
			case SettingID::GYRO_CUTOFF_RECOVERY:
				opt = gyro_cutoff_recovery.get(*activeChord);
				break;
			case SettingID::STICK_ACCELERATION_RATE:
				opt = stick_acceleration_rate.get(*activeChord);
				break;
			case SettingID::STICK_ACCELERATION_CAP:
				opt = stick_acceleration_cap.get(*activeChord);
				break;
			case SettingID::STICK_DEADZONE_INNER:
				opt = stick_deadzone_inner.get(*activeChord);
				break;
			case SettingID::STICK_DEADZONE_OUTER:
				opt = stick_deadzone_outer.get(*activeChord);
				break;
			case SettingID::MOUSE_RING_RADIUS:
				opt = mouse_ring_radius.get(*activeChord);
				break;
			case SettingID::SCREEN_RESOLUTION_X:
				opt = screen_resolution_x.get(*activeChord);
				break;
			case SettingID::SCREEN_RESOLUTION_Y:
				opt = screen_resolution_y.get(*activeChord);
				break;
			case SettingID::ROTATE_SMOOTH_OVERRIDE:
				opt = rotate_smooth_override.get(*activeChord);
				break;
			case SettingID::FLICK_SNAP_STRENGTH:
				opt = flick_snap_strength.get(*activeChord);
				break;
			}
			if (opt) return *opt;
		}

		std::stringstream message;
		message << "Index " << index << " is not a valid float setting";
		throw std::out_of_range(message.str().c_str());
	}

	template<>
	FloatXY getSetting<FloatXY>(SettingID index)
	{
		// Look at active chord mappings starting with the latest activates chord
		for (auto activeChord = btnCommon.chordStack.begin(); activeChord != btnCommon.chordStack.end(); activeChord++)
		{
			optional<FloatXY> opt;
			switch (index)
			{
			case SettingID::MIN_GYRO_SENS:
				opt = min_gyro_sens.get(*activeChord);
				break;
			case SettingID::MAX_GYRO_SENS:
				opt = max_gyro_sens.get(*activeChord);
				break;
			}
			if (opt) return *opt;
		}// Check next Chord

		stringstream ss;
		ss << "Index " << index << " is not a valid FloatXY setting";
		throw exception(ss.str().c_str());
	}

	template<>
	GyroSettings getSetting<GyroSettings>(SettingID index)
	{
		if (index == SettingID::GYRO_ON || index == SettingID::GYRO_OFF)
		{
			// Look at active chord mappings starting with the latest activates chord
			for (auto activeChord = btnCommon.chordStack.begin(); activeChord != btnCommon.chordStack.end(); activeChord++)
			{
				auto opt = gyro_settings.get(*activeChord);
				if (opt) return *opt;
			}
		}
		stringstream ss;
		ss << "Index " << index << " is not a valid GyroSetting";
		throw std::exception( ss.str().c_str() );
	}

public:

	ComboMap *GetMatchingSimMap(ButtonID index)
	{
		// Find the simMapping where the other btn is in the same state as this btn.
		// POTENTIAL FLAW: The mapping you find may not necessarily be the one that got you in a 
		// Simultaneous state in the first place if there is a second SimPress going on where one
		// of the buttons has a third SimMap with this one. I don't know if it's worth solving though...
		for (int id = 0; id < MAPPING_SIZE; ++id)
		{
			auto simMap = mappings[int(index)].getSimMap(ButtonID(id));
			if (simMap && index != simMap->first && buttons[int(simMap->first)]._btnState == buttons[int(index)]._btnState)
			{
				return simMap;
			}
		}
		return nullptr;
	}

	void ResetSmoothSample() {
		_frontSample = 0;
		for (int i = 0; i < NumSamples; i++) {
			_flickSamples[i] = 0.0;
		}
	}

	float GetSmoothedStickRotation(float value, float bottomThreshold, float topThreshold, int maxSamples) {
		// which sample in the circular smoothing buffer do we want to write over?
		_frontSample--;
		if (_frontSample < 0) _frontSample = NumSamples - 1;
		// if this input is bigger than the top threshold, it'll all be consumed immediately; 0 gets put into the smoothing buffer. If it's below the bottomThreshold, it'll all be put in the smoothing buffer
		float length = abs(value);
		float immediateFactor;
		if (topThreshold <= bottomThreshold) {
			immediateFactor = 1.0f;
		}
		else {
			immediateFactor = (length - bottomThreshold) / (topThreshold - bottomThreshold);
		}
		// clamp to [0, 1] range
		if (immediateFactor < 0.0f) {
			immediateFactor = 0.0f;
		}
		else if (immediateFactor > 1.0f) {
			immediateFactor = 1.0f;
		}
		float smoothFactor = 1.0f - immediateFactor;
		// now we can push the smooth sample (or as much of it as we want smoothed)
		float frontSample = _flickSamples[_frontSample] = value * smoothFactor;
		// and now calculate smoothed result
		float result = frontSample / maxSamples;
		for (int i = 1; i < maxSamples; i++) {
			int rotatedIndex = (_frontSample + i) % NumSamples;
			frontSample = _flickSamples[rotatedIndex];
			result += frontSample / maxSamples;
		}
		// finally, add immediate portion
		return result + value * immediateFactor;
	}

	void GetSmoothedGyro(float x, float y, float length, float bottomThreshold, float topThreshold, int maxSamples, float& outX, float& outY) {
		// this is basically the same as we use for smoothing flick-stick rotations, but because this deals in vectors, it's a slightly different function. Not worth abstracting until it'll be used in more ways
		// which item in the circular smoothing buffer will we write over?
		_frontGyroSample--;
		if (_frontGyroSample < 0) _frontGyroSample = MaxGyroSamples - 1;
		float immediateFactor;
		if (topThreshold <= bottomThreshold) {
			immediateFactor = length < bottomThreshold ? 0.0f : 1.0f;
		}
		else {
			immediateFactor = (length - bottomThreshold) / (topThreshold - bottomThreshold);
		}
		// clamp to [0, 1] range
		if (immediateFactor < 0.0f) {
			immediateFactor = 0.0f;
		}
		else if (immediateFactor > 1.0f) {
			immediateFactor = 1.0f;
		}
		float smoothFactor = 1.0f - immediateFactor;
		// now we can push the smooth sample (or as much of it as we want smoothed)
		FloatXY frontSample = _gyroSamples[_frontGyroSample] = { x * smoothFactor, y * smoothFactor };
		// and now calculate smoothed result
		float xResult = frontSample.x() / maxSamples;
		float yResult = frontSample.y() / maxSamples;
		for (int i = 1; i < maxSamples; i++) {
			int rotatedIndex = (_frontGyroSample + i) % MaxGyroSamples;
			frontSample = _gyroSamples[rotatedIndex];
			xResult += frontSample.x() / maxSamples;
			yResult += frontSample.y() / maxSamples;
		}
		// finally, add immediate portion
		outX = xResult + x * immediateFactor;
		outY = yResult + y * immediateFactor;
	}

	void handleButtonChange(ButtonID index, bool pressed)
	{
		auto foundChord = find(btnCommon.chordStack.begin(), btnCommon.chordStack.end(), index);
		if (!pressed)
		{
			if (foundChord != btnCommon.chordStack.end())
			{
				//cout << "Button " << index << " is released!" << endl;
				btnCommon.chordStack.erase(foundChord); // The chord is released
			}
		}
		else if (foundChord == btnCommon.chordStack.end()) {
			//cout << "Button " << index << " is pressed!" << endl;
			btnCommon.chordStack.push_front(index); // Always push at the fromt to make it a stack
		}

		DigitalButton &button = buttons[int(index)];

		switch (button._btnState)
		{
		case BtnState::NoPress:
			if (pressed)
			{
				if (button._mapping.HasSimMappings())
				{
					button._btnState = BtnState::WaitSim;
					button._press_times = time_now;
				}
				else if (button.HasHoldMapping())
				{
					button._btnState = BtnState::WaitHold;
					button._press_times = time_now;
				}
				else if (button._mapping.getDblPressMap())
				{
					// Start counting time between two start presses
					button._btnState = BtnState::DblPressStart;
					button._press_times = time_now;
				}
				else
				{
					button._btnState = BtnState::BtnPress;
					button.ApplyBtnPress();
				}
			}
			break;
		case BtnState::BtnPress:
			if (!pressed)
			{
				button._btnState = BtnState::NoPress;
				button.ApplyBtnRelease();
			}
			break;
		case BtnState::WaitSim:
			if (!pressed)
			{
				button._btnState = BtnState::TapRelease;
				button._press_times = time_now;
				button.ApplyBtnPress(true);
			}
			else
			{
				// Is there a sim mapping on this button where the other button is in WaitSim state too?
				auto simMap = GetMatchingSimMap(index);
				if (simMap)
				{
					// We have a simultaneous press!
					if (Mapping(simMap->second).holdBind)
					{
						button._btnState = BtnState::WaitSimHold;
						buttons[int(simMap->first)]._btnState = BtnState::WaitSimHold;
						button._press_times = time_now; // Reset Timer
						buttons[int(simMap->first)]._press_times = time_now;
					}
					else
					{
						button._btnState = BtnState::SimPress;
						buttons[int(simMap->first)]._btnState = BtnState::SimPress;
						button.ApplyBtnPress(*simMap);
						buttons[int(simMap->first)].SyncSimPress(index, *simMap);
					}
				}
				else if (button.GetPressDurationMS(time_now) > MAGIC_SIM_DELAY)
				{
					// Sim delay expired!
					if (button.HasHoldMapping())
					{
						button._btnState = BtnState::WaitHold;
						// Don't reset time
					}
					else if (button._mapping.getDblPressMap())
					{
						// Start counting time between two start presses
						button._btnState = BtnState::DblPressStart;
					}
					else
					{
						button._btnState = BtnState::BtnPress;
						button.ApplyBtnPress();
					}
				}
				// Else let time flow, stay in this state, no output.
			}
			break;
		case BtnState::WaitHold:
			if (!pressed)
			{
				if (button._mapping.getDblPressMap())
				{
					button._btnState = BtnState::DblPressStart;
				}
				else
				{
					button._btnState = BtnState::TapRelease;
					button._press_times = time_now;
					button.ApplyBtnPress(true);
				}
			}
			else if (button.GetPressDurationMS(time_now) > MAGIC_HOLD_TIME)
			{
				button._btnState = BtnState::HoldPress;
				button.ApplyBtnHold();
			}
			// Else let time flow, stay in this state, no output.
			break;
		case BtnState::SimPress:
		{
			// Which is the sim mapping where the other button is in SimPress state too?
			auto simMap = GetMatchingSimMap(index);
			if (!simMap)
			{
				// Should never happen but added for robustness.
				printf("Error: lost track of matching sim press for %s! Resetting to NoPress.\n", button._name.c_str());
				button._btnState = BtnState::NoPress;
			}
			else if (!pressed)
			{
				button._btnState = BtnState::SimRelease;
				buttons[int(simMap->first)]._btnState = BtnState::SimRelease;
				button.ApplyBtnRelease(*simMap);
			}
			// else sim press is being held, as far as this button is concerned.
			break;
		}
		case BtnState::HoldPress:
			if (!pressed)
			{
				button._btnState = BtnState::NoPress;
				button.ApplyBtnRelease();
			}
			break;
		case BtnState::WaitSimHold:
		{
			// Which is the sim mapping where the other button is in WaitSimHold state too?
			auto simMap = GetMatchingSimMap(index);
			if (!simMap)
			{
				// Should never happen but added for robustness.
				printf("Error: lost track of matching sim press for %s! Resetting to NoPress.\n", button._name.c_str());
				button._btnState = BtnState::NoPress;
			}
			else if (!pressed)
			{
				button._btnState = BtnState::SimTapRelease;
				buttons[int(simMap->first)]._btnState = BtnState::SimRelease;
				button._press_times = time_now;
				buttons[int(simMap->first)]._press_times = time_now;
				button.ApplyBtnPress(*simMap, true);
				buttons[int(simMap->first)].SyncSimPress(index, *simMap);
			}
			else if (button.GetPressDurationMS(time_now) > MAGIC_HOLD_TIME)
			{
				button._btnState = BtnState::SimHold;
				buttons[int(simMap->first)]._btnState = BtnState::SimHold;
				button.ApplyBtnHold(*simMap);
				buttons[int(simMap->first)].SyncSimHold(index, *simMap);
				// Else let time flow, stay in this state, no output.
			}
			break;
		}
		case BtnState::SimHold:
		{
			// Which is the sim mapping where the other button is in SimHold state too?
			auto simMap = GetMatchingSimMap(index);
			if (!simMap)
			{
				// Should never happen but added for robustness.
				printf("Error: lost track of matching sim press for %s! Resetting to NoPress.\n", button._name.c_str());
				button._btnState = BtnState::NoPress;
			}
			else if (!pressed)
			{
				button._btnState = BtnState::SimRelease;
				buttons[int(simMap->first)]._btnState = BtnState::SimRelease;
				button.ApplyBtnRelease(*simMap);
			}
			break;
		}
		case BtnState::SimRelease:
			if (!pressed)
			{
				button._btnState = BtnState::NoPress;
			}
			break;
		case BtnState::SimTapRelease:
		{
			// Which is the sim mapping where the other button is in SimTapRelease state too?
			auto simMap = GetMatchingSimMap(index);
			if (!simMap)
			{
				// Should never happen but added for robustness.
				printf("Error: lost track of matching sim press for %s! Resetting to NoPress.\n", button._name.c_str());
				button._btnState = BtnState::NoPress;
			}
			else if (pressed)
			{
				// The button has been pressed again before tap duration expired
				// Let's move on!!
				button.ApplyBtnRelease(*simMap, true);
				button._btnState = BtnState::NoPress;
				buttons[int(simMap->first)]._btnState = BtnState::SimRelease;
			}
			else if (button.GetPressDurationMS(time_now) > button.GetTapDuration())
			{
				button.ApplyBtnRelease(*simMap, true);
				button._btnState = BtnState::SimRelease;
				buttons[int(simMap->first)]._btnState = BtnState::SimRelease;
			}
			break;
		}
		case BtnState::TapRelease:
			if (pressed || button.GetPressDurationMS(time_now) > button.GetTapDuration())
			{
				button.ApplyBtnRelease(true);
				button._btnState = BtnState::NoPress;
			}
			break;
		case BtnState::DblPressStart:
			if (button.GetPressDurationMS(time_now) > MAGIC_DBL_PRESS_WINDOW)
			{
				button._btnState = BtnState::BtnPress;
				button.ApplyBtnPress();
			}
			else if (!pressed)
			{
				button._btnState = BtnState::DblPressNoPress;
			}
			break;
		case BtnState::DblPressNoPress:
			if (button.GetPressDurationMS(time_now) > MAGIC_DBL_PRESS_WINDOW)
			{
				button._btnState = BtnState::TapRelease;
				button.ApplyBtnPress(true);
			}
			else if (pressed)
			{
				// dblPress will be valid because HasDblPressMapping already returned true.
				if (Mapping(button._mapping.getDblPressMap()->second).holdBind)
				{
					button._btnState = BtnState::DblPressWaitHold;
					button._press_times = time_now;
				}
				else
				{
					button._btnState = BtnState::DblPressPress;
					button.ApplyBtnPress(*button._mapping.getDblPressMap());
				}
			}
			break;
		case BtnState::DblPressPress:
			if (!pressed)
			{
				button._btnState = BtnState::NoPress;
				button.ApplyBtnRelease(*button._mapping.getDblPressMap());
			}
			break;
		case BtnState::DblPressWaitHold:
			if (!pressed)
			{
				button._btnState = BtnState::TapRelease;
				button._press_times = time_now;
				button.ApplyBtnPress(*button._mapping.getDblPressMap(), true);
			}
			else if (button.GetPressDurationMS(time_now) > MAGIC_HOLD_TIME)
			{
				button._btnState = BtnState::DblPressHold;
				button.ApplyBtnHold(*button._mapping.getDblPressMap());
			}
			break;
		case BtnState::DblPressHold:
			if (!pressed)
			{
				button._btnState = BtnState::NoPress;
				button.ApplyBtnRelease(*button._mapping.getDblPressMap());
			}
			break;
		default:
			printf("Invalid button state %d: Resetting to NoPress\n", button._btnState);
			button._btnState = BtnState::NoPress;
			break;

		}
	}

	void handleTriggerChange(ButtonID softIndex, ButtonID fullIndex, TriggerMode mode, float pressed)
	{
		if (JslGetControllerType(intHandle) != JS_TYPE_DS4)
		{
			// Override local variable because the controller has digital triggers. Effectively ignore Full Pull binding.
			mode = TriggerMode::NO_FULL;
		}

		auto idxState = int(fullIndex) - FIRST_ANALOG_TRIGGER; // Get analog trigger index
		if (idxState < 0 || idxState >= (int)triggerState.size())
		{
			printf("Error: Trigger %s does not exist in state map. Dual Stage Trigger not possible.\n", buttons[int(fullIndex)]._name.c_str());
			return;
		}

		// if either trigger is waiting to be tap released, give it a go
		if (buttons[int(softIndex)]._btnState == BtnState::TapRelease || buttons[int(softIndex)]._btnState == BtnState::SimTapRelease)
		{
			// keep triggering until the tap release is complete
			handleButtonChange(softIndex, false);
		}
		if (buttons[int(fullIndex)]._btnState == BtnState::TapRelease || buttons[int(fullIndex)]._btnState == BtnState::SimTapRelease)
		{
			// keep triggering until the tap release is complete
			handleButtonChange(fullIndex, false);
		}

		switch (triggerState[idxState])
		{
		case DstState::NoPress:
			// It actually doesn't matter what the last Press is. Theoretically, we could have missed the edge.
			if (pressed > getSetting(SettingID::TRIGGER_THRESHOLD))
			{
				if (mode == TriggerMode::MAY_SKIP || mode == TriggerMode::MUST_SKIP)
				{
					// Start counting press time to see if soft binding should be skipped
					triggerState[idxState] = DstState::PressStart;
					buttons[int(softIndex)]._press_times = time_now;
				}
				else if (mode == TriggerMode::MAY_SKIP_R || mode == TriggerMode::MUST_SKIP_R)
				{
					triggerState[idxState] = DstState::PressStartResp;
					buttons[int(softIndex)]._press_times = time_now;
					handleButtonChange(softIndex, true);
				}
				else // mode == NO_FULL or NO_SKIP
				{
					triggerState[idxState] = DstState::SoftPress;
					handleButtonChange(softIndex, true);
				}
			}
			else
			{
				handleButtonChange(softIndex, false);
			}
			break;
		case DstState::PressStart:
			if (pressed <= getSetting(SettingID::TRIGGER_THRESHOLD)) {
				// Trigger has been quickly tapped on the soft press
				triggerState[idxState] = DstState::QuickSoftTap;
				handleButtonChange(softIndex, true);
			}
			else if (pressed == 1.0)
			{
				// Trigger has been full pressed quickly
				triggerState[idxState] = DstState::QuickFullPress;
				handleButtonChange(fullIndex, true);
			}
			else if (buttons[int(softIndex)].GetPressDurationMS(time_now) >= MAGIC_DST_DELAY) { // todo: get rid of magic number -- make this a user setting )
				triggerState[idxState] = DstState::SoftPress;
				// Reset the time for hold soft press purposes.
				buttons[int(softIndex)]._press_times = time_now;
				handleButtonChange(softIndex, true);
			}
			// Else, time passes as soft press is being held, waiting to see if the soft binding should be skipped
			break;
		case DstState::PressStartResp:
			if (pressed <= getSetting(SettingID::TRIGGER_THRESHOLD)) {
				// Soft press is being released
				triggerState[idxState] = DstState::NoPress;
				handleButtonChange(softIndex, false);
			}
			else if (pressed == 1.0)
			{
				// Trigger has been full pressed quickly
				triggerState[idxState] = DstState::QuickFullPress;
				handleButtonChange(softIndex, false); // Remove soft press
				handleButtonChange(fullIndex, true);
			}
			else
			{
				if (buttons[int(softIndex)].GetPressDurationMS(time_now) >= MAGIC_DST_DELAY) { // todo: get rid of magic number -- make this a user setting )
					triggerState[idxState] = DstState::SoftPress;
				}
				handleButtonChange(softIndex, true);
			}
			break;
		case DstState::QuickSoftTap:
			// Soft trigger is already released. Send release now!
			triggerState[idxState] = DstState::NoPress;
			handleButtonChange(softIndex, false);
			break;
		case DstState::QuickFullPress:
			if (pressed < 1.0f) {
				// Full press is being release
				triggerState[idxState] = DstState::QuickFullRelease;
				handleButtonChange(fullIndex, false);
			}
			else {
				// Full press is being held
				handleButtonChange(fullIndex, true);
			}
			break;
		case DstState::QuickFullRelease:
			if (pressed <= getSetting(SettingID::TRIGGER_THRESHOLD)) {
				triggerState[idxState] = DstState::NoPress;
			}
			else if (pressed == 1.0f)
			{
				// Trigger is being full pressed again
				triggerState[idxState] = DstState::QuickFullPress;
				handleButtonChange(fullIndex, true);
			}
			// else wait for the the trigger to be fully released
			break;
		case DstState::SoftPress:
			if (pressed <= getSetting(SettingID::TRIGGER_THRESHOLD)) {
				// Soft press is being released
				triggerState[idxState] = DstState::NoPress;
				handleButtonChange(softIndex, false);
			}
			else // Soft Press is being held
			{
				handleButtonChange(softIndex, true);

				if ((mode == TriggerMode::MAY_SKIP || mode == TriggerMode::NO_SKIP || mode == TriggerMode::MAY_SKIP_R)
					&& pressed == 1.0)
				{
					// Full press is allowed in addition to soft press
					triggerState[idxState] = DstState::DelayFullPress;
					handleButtonChange(fullIndex, true);
				}
				// else ignore full press on NO_FULL and MUST_SKIP
			}
			break;
		case DstState::DelayFullPress:
			if (pressed < 1.0)
			{
				// Full Press is being released
				triggerState[idxState] = DstState::SoftPress;
				handleButtonChange(fullIndex, false);
			}
			else // Full press is being held
			{
				handleButtonChange(fullIndex, true);
			}
			// Soft press is always held regardless
			handleButtonChange(softIndex, true);
			break;
		default:
			// TODO: use magic enum to translate enum # to str
			printf("Error: Trigger %s has invalid state #%d. Reset to NoPress.\n", buttons[int(softIndex)]._name.c_str(), triggerState[idxState]);
			triggerState[idxState] = DstState::NoPress;
			break;
		}
	}

	bool IsPressed(const JOY_SHOCK_STATE& state, ButtonID mappingIndex)
	{
		if (mappingIndex > ButtonID::NONE)
		{
			if (mappingIndex == ButtonID::ZLF) return state.lTrigger == 1.0;
			else if (mappingIndex == ButtonID::ZRF) return state.rTrigger == 1.0;
			// Is it better to consider trigger_threshold for GYRO_ON/OFF on ZL/ZR?
			else if (mappingIndex == ButtonID::ZL) return state.lTrigger > getSetting(SettingID::TRIGGER_THRESHOLD);
			else if (mappingIndex == ButtonID::ZR) return state.rTrigger > getSetting(SettingID::TRIGGER_THRESHOLD);
			else return state.buttons & (1 << keyToBitOffset(mappingIndex));
		}
		return false;
	}

	// return true if it hits the outer deadzone
	bool processDeadZones(float& x, float& y) {
		float innerDeadzone = getSetting(SettingID::STICK_DEADZONE_INNER);
		float outerDeadzone = 1.0f - getSetting(SettingID::STICK_DEADZONE_OUTER);
		float length = sqrtf(x*x + y * y);
		if (length <= innerDeadzone) {
			x = 0.0f;
			y = 0.0f;
			return false;
		}
		if (length >= outerDeadzone) {
			// normalize
			x /= length;
			y /= length;
			return true;
		}
		if (length > innerDeadzone) {
			float scaledLength = (length - innerDeadzone) / (outerDeadzone - innerDeadzone);
			float rescale = scaledLength / length;
			x *= rescale;
			y *= rescale;
		}
		return false;
	}
};

static void resetAllMappings() {
	for_each(mappings.begin(), mappings.end(), [] (auto &map) { map.Reset(); });
	// Question: Why is this a default mapping? Shouldn't it be empty? It's always possible to calibrate with RESET_GYRO_CALIBRATION
	min_gyro_sens.Reset();
	max_gyro_sens.Reset();
	min_gyro_threshold.Reset();
	max_gyro_threshold.Reset();
	stick_power.Reset();
	stick_sens.Reset();
	real_world_calibration.Reset();
	in_game_sens.Reset();
	left_stick_mode.Reset();
	right_stick_mode.Reset();
	left_ring_mode.Reset();
	right_ring_mode.Reset();
	mouse_x_from_gyro.Reset();
	mouse_y_from_gyro.Reset();
	joycon_gyro_mask.Reset();
	zlMode.Reset();
	zrMode.Reset();
	trigger_threshold.Reset();
	gyro_settings.Reset();
	aim_y_sign.Reset();
	aim_x_sign.Reset();
	gyro_y_sign.Reset();
	gyro_x_sign.Reset();
	flick_time.Reset();
	gyro_smooth_time.Reset();
	gyro_smooth_threshold.Reset();
	gyro_cutoff_speed.Reset();
	gyro_cutoff_recovery.Reset();
	stick_acceleration_rate.Reset();
	stick_acceleration_cap.Reset();
	stick_deadzone_inner.Reset();
	stick_deadzone_outer.Reset();
	screen_resolution_x.Reset();
	screen_resolution_y.Reset();
	mouse_ring_radius.Reset();
	rotate_smooth_override.Reset();
	flick_snap_strength.Reset();
	flick_snap_mode.Reset();
	
	os_mouse_speed = 1.0f;
	last_flick_and_rotation = 0.0f;
}

void connectDevices() {
	int numConnected = JslConnectDevices();
	int* deviceHandles = new int[numConnected];

	JslGetConnectedDeviceHandles(deviceHandles, numConnected);

	for (int i = 0; i < numConnected; i++) {
		// map handles to extra local data
		int handle = deviceHandles[i];
		JoyShock* js = new JoyShock(handle,
			JslGetPollRate(handle),
			JslGetControllerSplitType(handle),
			JslGetStickStep(handle));
		handle_to_joyshock.emplace(deviceHandles[i], js);

		// calibration?
		//JslStartContinuousCalibration(deviceHandles[i]);
	}

	string msg;
	if (numConnected == 1) {
		msg = "1 device connected\n";
	}
	else {
		stringstream ss;
		ss << numConnected << " devices connected" << endl;
		msg = ss.str();
	}
	printf("%s\n", msg.c_str());
	//if (!IsVisible())
	//{
	//	tray->SendToast(wstring(msg.begin(), msg.end()));
	//}

	//if (numConnected != 0) {
	//	printf("All devices have started continuous gyro calibration\n");
	//}

	delete[] deviceHandles;
}

void SimPressCrossUpdate(ButtonID sim, ButtonID origin, Mapping newVal)
{
	mappings[int(sim)].AtSimPress(origin) = newVal;
}

bool do_NO_GYRO_BUTTON() {
	// TODO: handle chords
	gyro_settings.Reset();
	return true;
}

bool do_RESET_MAPPINGS() {
	printf("Resetting all mappings to defaults\n");
	resetAllMappings();
	return true;
}

bool do_RECONNECT_CONTROLLERS() {
	printf("Reconnecting controllers\n");
	JslDisconnectAndDisposeAll();
	connectDevices();
	JslSetCallback(&joyShockPollCallback);
	return true;
}

bool do_COUNTER_OS_MOUSE_SPEED() {
	printf("Countering OS mouse speed setting\n");
	os_mouse_speed = getMouseSpeed();
	return true;
}

bool do_IGNORE_OS_MOUSE_SPEED() {
	printf("Ignoring OS mouse speed setting\n");
	os_mouse_speed = 1.0;
	return true;
}

bool do_GYRO_SENS(in_string argument)
{
	stringstream ss(argument);
	// Ignore whitespaces until the '=' is found
	char c;
	do {
		ss >> c;
	} while (ss.good() && c != '=');
	if (!ss.good())
	{
		//No assignment? Display current assignment
		cout << "MIN_GYRO_SENS = " << *min_gyro_sens.get() << endl << "MAX_GYRO_SENS = " << *max_gyro_sens.get() << endl;
	}
	if (c == '=')
	{
		// Read the value
		FloatXY newSens;
		ss >> newSens;
		if (!ss.fail())
		{
			min_gyro_sens.operator=(newSens);
			max_gyro_sens.operator=(newSens);
			return true;
		}
		else {
			cout << "Can't convert \"" << argument.substr(argument.find_first_of('=')+1, argument.length()) << "\" to one or two numbers" << endl;
		}
	}
	// Not an equal sign? The command is entered wrong!
	return false;
}

bool do_AUTOLOAD(JSMCommand *cmd, in_string argument)
{
	if (!autoLoadThread)
	{
		cout << "AutoLoad is unavailable" << endl;
		return true; // No need to display help
	}
	stringstream ss(argument);
	// Ignore whitespaces until the '=' is found
	char c;
	for (c = ss.peek(); ss.good() && c == ' '; ss >> c);
	if (!ss.good())
	{
		//No assignment? Display current assignment
		cout << "AUTOLOAD = " << (autoLoadThread->isRunning() ? "ON " : "OFF") << endl;
	}
	if (c == '=')
	{
		string valueName;
		ss >> valueName;
		if (valueName.compare("ON") == 0)
		{
			autoLoadThread->Start();
			cout << "AutoLoad is now running" << endl;
		}
		else if (valueName.compare("OFF") == 0)
		{
			autoLoadThread->Stop();
			cout << "AutoLoad is stopped" << endl;;
		}
		else {
			cout << valueName << " is an invalid option" << endl << "AUTOLOAD can only be assigned ON or OFF" << endl;
		}
	}
	// Not an equal sign? The command is entered wrong!
	return false;
}

bool do_CALCULATE_REAL_WORLD_CALIBRATION(in_string argument) {
	// first, check for a parameter
	float numRotations = 1.0;
	if (argument.length() > 0) {
		try {
			numRotations = stof(argument);
		}
		catch (invalid_argument ia) {
			printf("Can't convert \"%s\" to a number\n", argument.c_str());
			return false;
		}
	}
	if (numRotations == 0) {
		printf("Can't calculate calibration from zero rotations\n");
	}
	else if (last_flick_and_rotation == 0) {
		printf("Need to use the flick stick at least once before calculating an appropriate calibration value\n");
	}
	else {
		printf("Recommendation: REAL_WORLD_CALIBRATION = %.5g\n", *real_world_calibration.get() * last_flick_and_rotation / numRotations);
	}
	return true;
}

bool do_FINISH_GYRO_CALIBRATION() {
	printf("Finishing continuous calibration for all devices\n");
	for (unordered_map<int, JoyShock*>::iterator iter = handle_to_joyshock.begin(); iter != handle_to_joyshock.end(); ++iter) {
		JoyShock* jc = iter->second;
		JslPauseContinuousCalibration(jc->intHandle);
	}
	devicesCalibrating = false;
	return true;
}

bool do_RESTART_GYRO_CALIBRATION() {
	printf("Restarting continuous calibration for all devices\n");
	for (unordered_map<int, JoyShock*>::iterator iter = handle_to_joyshock.begin(); iter != handle_to_joyshock.end(); ++iter) {
		JoyShock* jc = iter->second;
		JslResetContinuousCalibration(jc->intHandle);
		JslStartContinuousCalibration(jc->intHandle);
	}
	devicesCalibrating = true;
	return true;
}

bool do_README() {
	printf("Opening online help in your browser\n");
	if (auto err = ShowOnlineHelp() != 0)
	{
		printf("Could not open online help. Error #%d\n", err);
	}
	return true;
}

bool do_WHITELIST_SHOW() {
	printf("Launching HIDCerberus in your browser. Your PID is %lu\n", GetCurrentProcessId()); // WinAPI call!
	Whitelister::ShowHIDCerberus();
	return true;
}

bool do_WHITELIST_ADD() {
	whitelister.Add();
	if (whitelister)
	{
		printf("JoyShockMapper was successfully whitelisted\n");
	}
	else
	{
		printf("Whitelisting failed!\n");
	}
	return true;
}

bool do_WHITELIST_REMOVE() {
	whitelister.Remove();
	printf("JoyShockMapper removed from whitelist\n");
	return true;
}

static float handleFlickStick(float calX, float calY, float lastCalX, float lastCalY, float stickLength, bool& isFlicking, JoyShock* jc, float mouseCalibrationFactor, bool FLICK_ONLY, bool ROTATE_ONLY) {
	float camSpeedX = 0.0f;
	// let's centre this
	float offsetX = calX;
	float offsetY = calY;
	float lastOffsetX = lastCalX;
	float lastOffsetY = lastCalY;
	float flickStickThreshold = 1.0f - jc->getSetting(SettingID::STICK_DEADZONE_OUTER);
	if (isFlicking)
	{
		flickStickThreshold *= 0.9f;
	}
	if (stickLength >= flickStickThreshold) {
		float stickAngle = atan2(-offsetX, offsetY);
		//printf(", %.4f\n", lastOffsetLength);
		if (!isFlicking) {
			// bam! new flick!
			isFlicking = true;
			if (!ROTATE_ONLY)
			{
				auto flick_snap_mode = jc->getSetting<FlickSnapMode>(SettingID::FLICK_SNAP_MODE);
				if (flick_snap_mode != FlickSnapMode::NONE) {
					// handle snapping
					float snapInterval;
					if (flick_snap_mode == FlickSnapMode::FOUR) {
						snapInterval = PI / 2.0f; // 90 degrees
					}
					else if (flick_snap_mode == FlickSnapMode::EIGHT) {
						snapInterval = PI / 4.0f; // 45 degrees
					}
					float snappedAngle = round(stickAngle / snapInterval) * snapInterval;
					// lerp by snap strength
					auto flick_snap_strength = jc->getSetting(SettingID::FLICK_SNAP_STRENGTH);
					stickAngle = stickAngle * (1.0f - flick_snap_strength) + snappedAngle * flick_snap_strength;
				}

				jc->started_flick = chrono::steady_clock::now();
				jc->delta_flick = stickAngle;
				jc->flick_percent_done = 0.0f;
				jc->ResetSmoothSample();
				jc->flick_rotation_counter = stickAngle; // track all rotation for this flick
				// TODO: All these printfs should be hidden behind a setting. User might not want them.
				printf("Flick: %.3f degrees\n", stickAngle * (180.0f / PI));
			}
		}
		else {
			if (!FLICK_ONLY)
			{
				// not new? turn camera?
				float lastStickAngle = atan2(-lastOffsetX, lastOffsetY);
				float angleChange = stickAngle - lastStickAngle;
				// https://stackoverflow.com/a/11498248/1130520
				angleChange = fmod(angleChange + PI, 2.0f * PI);
				if (angleChange < 0)
					angleChange += 2.0f * PI;
				angleChange -= PI;
				jc->flick_rotation_counter += angleChange; // track all rotation for this flick
				float flickSpeedConstant = jc->getSetting(SettingID::REAL_WORLD_CALIBRATION) * mouseCalibrationFactor / jc->getSetting(SettingID::IN_GAME_SENS);
				float flickSpeed = -(angleChange * flickSpeedConstant);
				int maxSmoothingSamples = min(jc->NumSamples, (int)(64.0f * (jc->poll_rate / 1000.0f))); // target a max smoothing window size of 64ms
				float stepSize = jc->stick_step_size; // and we only want full on smoothing when the stick change each time we poll it is approximately the minimum stick resolution
													  // the fact that we're using radians makes this really easy
				auto rotate_smooth_override = jc->getSetting(SettingID::ROTATE_SMOOTH_OVERRIDE);
				if (rotate_smooth_override < 0.0f)
				{
					camSpeedX = jc->GetSmoothedStickRotation(flickSpeed, flickSpeedConstant * stepSize * 8.0f, flickSpeedConstant * stepSize * 16.0f, maxSmoothingSamples);
				}
				else {
					camSpeedX = jc->GetSmoothedStickRotation(flickSpeed, flickSpeedConstant * rotate_smooth_override, flickSpeedConstant * rotate_smooth_override * 2.0f, maxSmoothingSamples);
				}
			}
		}
	}
	else if (isFlicking) {
		// was a flick! how much was the flick and rotation?
		if (!FLICK_ONLY && !ROTATE_ONLY)
		{
			last_flick_and_rotation = abs(jc->flick_rotation_counter) / (2.0f * PI);
		}
		isFlicking = false;
	}
	// do the flicking
	float secondsSinceFlick = ((float)chrono::duration_cast<chrono::microseconds>(jc->time_now - jc->started_flick).count()) / 1000000.0f;
	float newPercent = secondsSinceFlick / jc->getSetting(SettingID::FLICK_TIME);
	if (newPercent > 1.0f) newPercent = 1.0f;
	// warping towards 1.0
	float oldShapedPercent = 1.0f - jc->flick_percent_done;
	oldShapedPercent *= oldShapedPercent;
	oldShapedPercent = 1.0f - oldShapedPercent;
	//float oldShapedPercent = jc->flick_percent_done;
	jc->flick_percent_done = newPercent;
	newPercent = 1.0f - newPercent;
	newPercent *= newPercent;
	newPercent = 1.0f - newPercent;
	float camSpeedChange = (newPercent - oldShapedPercent) * jc->delta_flick * jc->getSetting(SettingID::REAL_WORLD_CALIBRATION) * -mouseCalibrationFactor / jc->getSetting(SettingID::IN_GAME_SENS);
	camSpeedX += camSpeedChange;

	return camSpeedX;
}

// https://stackoverflow.com/questions/25144887/map-unordered-map-prefer-find-and-then-at-or-try-at-catch-out-of-range
JoyShock* getJoyShockFromHandle(int handle) {
	auto iter = handle_to_joyshock.find(handle);

	if (iter != handle_to_joyshock.end()) {
		return iter->second;
		// iter is item pair in the map. The value will be accessible as `iter->second`.
	}
	return nullptr;
}

void joyShockPollCallback(int jcHandle, JOY_SHOCK_STATE state, JOY_SHOCK_STATE lastState, IMU_STATE imuState, IMU_STATE lastImuState, float deltaTime) {
	bool blockGyro = false;
	bool lockMouse = false;
	bool leftAny = false;
	bool rightAny = false;
	// get jc from handle
	JoyShock* jc = getJoyShockFromHandle(jcHandle);
	//printf("Controller %d\n", jcHandle);
	if (jc == nullptr) return;
	//printf("Found a match for %d\n", jcHandle);
	float gyroX = 0.0;
	float gyroY = 0.0;
	int mouse_x_flag = (int)jc->getSetting<GyroAxisMask>(SettingID::MOUSE_X_FROM_GYRO_AXIS);
	if ((mouse_x_flag & (int)GyroAxisMask::X) > 0) {
		gyroX -= imuState.gyroX; // x axis is negative because that's what worked before, don't want to mess with definitions of "inverted"
	}
	if ((mouse_x_flag & (int)GyroAxisMask::Y) > 0) {
		gyroX -= imuState.gyroY; // x axis is negative because that's what worked before, don't want to mess with definitions of "inverted"
	}
	if ((mouse_x_flag & (int)GyroAxisMask::Z) > 0) {
		gyroX -= imuState.gyroZ; // x axis is negative because that's what worked before, don't want to mess with definitions of "inverted"
	}
	int mouse_y_flag = (int)jc->getSetting<GyroAxisMask>(SettingID::MOUSE_Y_FROM_GYRO_AXIS);
	if ((mouse_y_flag & (int)GyroAxisMask::X) > 0) {
		gyroY += imuState.gyroX;
	}
	if ((mouse_y_flag & (int)GyroAxisMask::Y) > 0) {
		gyroY += imuState.gyroY;
	}
	if ((mouse_y_flag & (int)GyroAxisMask::Z) > 0) {
		gyroY += imuState.gyroZ;
	}
	float gyroLength = sqrt(gyroX * gyroX + gyroY * gyroY);
	// do gyro smoothing
	// convert gyro smooth time to number of samples
	auto numGyroSamples = jc->poll_rate * jc->getSetting(SettingID::GYRO_SMOOTH_TIME); // samples per second * seconds = samples
	if (numGyroSamples < 1) numGyroSamples = 1; // need at least 1 sample
	auto threshold = jc->getSetting(SettingID::GYRO_SMOOTH_THRESHOLD);
	jc->GetSmoothedGyro(gyroX, gyroY, gyroLength, threshold / 2.0f, threshold, int(numGyroSamples), gyroX, gyroY);
	//printf("%d Samples for threshold: %0.4f\n", numGyroSamples, gyro_smooth_threshold * maxSmoothingSamples);

	// now, honour gyro_cutoff_speed
	gyroLength = sqrt(gyroX * gyroX + gyroY * gyroY);
	auto speed = jc->getSetting(SettingID::GYRO_CUTOFF_SPEED);
	auto recovery = jc->getSetting(SettingID::GYRO_CUTOFF_RECOVERY);
	if (recovery > speed) {
		// we can use gyro_cutoff_speed
		float gyroIgnoreFactor = (gyroLength - speed) / (recovery - speed);
		if (gyroIgnoreFactor < 1.0f) {
			if (gyroIgnoreFactor <= 0.0f) {
				gyroX = gyroY = gyroLength = 0.0f;
			}
			else {
				gyroX *= gyroIgnoreFactor;
				gyroY *= gyroIgnoreFactor;
				gyroLength *= gyroIgnoreFactor;
			}
		}
	}
	else if (speed > 0.0f && gyroLength < speed) {
		// gyro_cutoff_recovery is something weird, so we just do a hard threshold
		gyroX = gyroY = gyroLength = 0.0f;
	}

	jc->time_now = std::chrono::steady_clock::now();

	// sticks!
	float camSpeedX = 0.0f;
	float camSpeedY = 0.0f;
	// account for os mouse speed and convert from radians to degrees because gyro reports in degrees per second
	float mouseCalibrationFactor = 180.0f / PI / os_mouse_speed;
	// let's do these sticks... don't want to constantly send input, so we need to compare them to last time
	float lastCalX = lastState.stickLX;
	float lastCalY = lastState.stickLY;
	float calX = state.stickLX;
	float calY = state.stickLY;
	bool leftPegged = jc->processDeadZones(calX, calY);
	float absX = abs(calX);
	float absY = abs(calY);
	bool left = calX < -0.5f * absY;
	bool right = calX > 0.5f * absY;
	bool down = calY < -0.5f * absX;
	bool up = calY > 0.5f * absX;
	float stickLength = sqrt(calX * calX + calY * calY);
	bool ring = jc->getSetting<RingMode>(SettingID::LEFT_RING_MODE) == RingMode::INNER && stickLength > 0.0f && stickLength < 0.7f ||
		jc->getSetting<RingMode>(SettingID::LEFT_RING_MODE) == RingMode::OUTER && stickLength > 0.7f;
	jc->handleButtonChange(ButtonID::LRING, ring);

	bool rotateOnly = jc->getSetting<StickMode>(SettingID::LEFT_STICK_MODE) == StickMode::ROTATE_ONLY;
	bool flickOnly = jc->getSetting<StickMode>(SettingID::LEFT_STICK_MODE) == StickMode::FLICK_ONLY;
	if (jc->ignore_left_stick_mode && jc->getSetting<StickMode>(SettingID::LEFT_STICK_MODE) == StickMode::INVALID && calX == 0 && calY == 0)
	{
		// clear ignore flag when stick is back at neutral
		jc->ignore_left_stick_mode = false;
	}
	else if (jc->getSetting<StickMode>(SettingID::LEFT_STICK_MODE) == StickMode::FLICK || flickOnly || rotateOnly) {
		camSpeedX += handleFlickStick(calX, calY, lastCalX, lastCalY, stickLength, jc->is_flicking_left, jc, mouseCalibrationFactor, flickOnly, rotateOnly);
		leftAny = leftPegged;
	}
	else if (jc->getSetting<StickMode>(SettingID::LEFT_STICK_MODE) == StickMode::AIM) {
		// camera movement
		if (!leftPegged) {
			jc->left_acceleration = 1.0f; // reset
		}
		float stickLength = sqrt(calX * calX + calY * calY);
		if (stickLength != 0.0f) {
			leftAny = true;
			float warpedStickLength = pow(stickLength, jc->getSetting(SettingID::STICK_POWER));
			warpedStickLength *= jc->getSetting(SettingID::STICK_SENS) * jc->getSetting(SettingID::REAL_WORLD_CALIBRATION) / os_mouse_speed / jc->getSetting(SettingID::IN_GAME_SENS);
			camSpeedX += calX / stickLength * warpedStickLength * jc->left_acceleration * deltaTime;
			camSpeedY += calY / stickLength * warpedStickLength * jc->left_acceleration * deltaTime;
			if (leftPegged) {
				jc->left_acceleration += jc->getSetting(SettingID::STICK_ACCELERATION_RATE) * deltaTime;
				auto cap = jc->getSetting(SettingID::STICK_ACCELERATION_CAP);
				if (jc->left_acceleration > cap) {
					jc->left_acceleration = cap;
				}
			}
		}
	}
	else if (jc->getSetting<StickMode>(SettingID::LEFT_STICK_MODE) == StickMode::MOUSE_AREA) {

		auto mouse_ring_radius = jc->getSetting(SettingID::MOUSE_RING_RADIUS);
		if (calX != 0.0f || calY != 0.0f) {
			// use difference with last cal values
			float mouseX = (calX - jc->left_last_cal.x()) * mouse_ring_radius;
			float mouseY = (calY - jc->left_last_cal.y()) * -1 * mouse_ring_radius;
			// do it!
			moveMouse(mouseX, mouseY);
			jc->left_last_cal = { calX, calY };
		}
		else
		{
			// Return to center
			moveMouse(jc->left_last_cal.x() * -1 * mouse_ring_radius, jc->left_last_cal.y() * mouse_ring_radius);
			jc->left_last_cal = { 0, 0 };
		}
	}
	else if (jc->getSetting<StickMode>(SettingID::LEFT_STICK_MODE) == StickMode::MOUSE_RING) {
		if (calX != 0.0f || calY != 0.0f) {
			auto mouse_ring_radius = jc->getSetting(SettingID::MOUSE_RING_RADIUS);
			float stickLength = sqrt(calX * calX + calY * calY);
			float normX = calX / stickLength;
			float normY = calY / stickLength;
			// use screen resolution
			float mouseX = (float)jc->getSetting(SettingID::SCREEN_RESOLUTION_X) * 0.5f + 0.5f + normX * mouse_ring_radius;
			float mouseY = (float)jc->getSetting(SettingID::SCREEN_RESOLUTION_Y) * 0.5f + 0.5f - normY * mouse_ring_radius;
			// normalize
			mouseX = mouseX / jc->getSetting(SettingID::SCREEN_RESOLUTION_X);
			mouseY = mouseY / jc->getSetting(SettingID::SCREEN_RESOLUTION_Y);
			// do it!
			setMouseNorm(mouseX, mouseY);
			lockMouse = true;
		}
	}
	else if (jc->getSetting<StickMode>(SettingID::LEFT_STICK_MODE) == StickMode::NO_MOUSE) { // Do not do if invalid
		// left!
		jc->handleButtonChange(ButtonID::LLEFT, left);
		// right!
		jc->handleButtonChange(ButtonID::LRIGHT, right);
		// up!
		jc->handleButtonChange(ButtonID::LUP, up);
		// down!
		jc->handleButtonChange(ButtonID::LDOWN, down);

		leftAny = left | right | up | down; // ring doesn't count
	}
	lastCalX = lastState.stickRX;
	lastCalY = lastState.stickRY;
	calX = state.stickRX;
	calY = state.stickRY;
	bool rightPegged = jc->processDeadZones(calX, calY);
	absX = abs(calX);
	absY = abs(calY);
	left = calX < -0.5f * absY;
	right = calX > 0.5f * absY;
	down = calY < -0.5f * absX;
	up = calY > 0.5f * absX;
	stickLength = sqrt(calX * calX + calY * calY);
	ring = jc->getSetting<RingMode>(SettingID::RIGHT_RING_MODE) == RingMode::INNER && stickLength > 0.0f && stickLength < 0.7f ||
		jc->getSetting<RingMode>(SettingID::RIGHT_RING_MODE) == RingMode::OUTER && stickLength > 0.7f;
	jc->handleButtonChange(ButtonID::RRING, ring);

	rotateOnly = jc->getSetting<StickMode>(SettingID::RIGHT_STICK_MODE) == StickMode::ROTATE_ONLY;
	flickOnly = jc->getSetting<StickMode>(SettingID::RIGHT_STICK_MODE) == StickMode::FLICK_ONLY;
	if (jc->ignore_right_stick_mode && jc->getSetting<StickMode>(SettingID::RIGHT_STICK_MODE) == StickMode::INVALID && calX == 0 && calY == 0)
	{
		// clear ignore flag when stick is back at neutral
		jc->ignore_right_stick_mode = false;
	}
	else if (jc->getSetting<StickMode>(SettingID::RIGHT_STICK_MODE) == StickMode::FLICK || rotateOnly || flickOnly) {
		camSpeedX += handleFlickStick(calX, calY, lastCalX, lastCalY, stickLength, jc->is_flicking_right, jc, mouseCalibrationFactor, flickOnly, rotateOnly);
		rightAny = rightPegged;
	}
	else if (jc->getSetting<StickMode>(SettingID::RIGHT_STICK_MODE) == StickMode::AIM) {
		// camera movement
		if (!rightPegged) {
			jc->right_acceleration = 1.0f; // reset
		}
		float stickLength = sqrt(calX * calX + calY * calY);
		if (stickLength > 0.0f) {
			rightAny = true;
			float warpedStickLength = pow(stickLength, jc->getSetting(SettingID::STICK_POWER));
			warpedStickLength *= jc->getSetting(SettingID::STICK_SENS) * jc->getSetting(SettingID::REAL_WORLD_CALIBRATION) / os_mouse_speed / jc->getSetting(SettingID::IN_GAME_SENS);
			camSpeedX += calX / stickLength * warpedStickLength * jc->right_acceleration * deltaTime;
			camSpeedY += calY / stickLength * warpedStickLength * jc->right_acceleration * deltaTime;
			if (rightPegged) {
				jc->right_acceleration += jc->getSetting(SettingID::STICK_ACCELERATION_RATE) * deltaTime;
				auto cap = jc->getSetting(SettingID::STICK_ACCELERATION_CAP);
				if (jc->right_acceleration > cap) {
					jc->right_acceleration = cap;
				}
			}
		}
	}
	else if (jc->getSetting<StickMode>(SettingID::RIGHT_STICK_MODE) == StickMode::MOUSE_AREA) {
		auto mouse_ring_radius = jc->getSetting(SettingID::MOUSE_RING_RADIUS);
		if (calX != 0.0f || calY != 0.0f) {
			// use difference with last cal values
			float mouseX = (calX - jc->right_last_cal.x()) * mouse_ring_radius;
			float mouseY = (calY - jc->right_last_cal.y()) * -1 * mouse_ring_radius;
			// do it!
			moveMouse(mouseX, mouseY);
			jc->right_last_cal = { calX, calY };
		}
		else
		{
			// return to center
			moveMouse(jc->right_last_cal.x() * -1 * mouse_ring_radius, jc->right_last_cal.y() * mouse_ring_radius);
			jc->right_last_cal = { 0, 0 };
		}
	}
	else if (jc->getSetting<StickMode>(SettingID::RIGHT_STICK_MODE) == StickMode::MOUSE_RING) {
		if (calX != 0.0f || calY != 0.0f) {
			auto mouse_ring_radius = jc->getSetting(SettingID::MOUSE_RING_RADIUS);
			float stickLength = sqrt(calX * calX + calY * calY);
			float normX = calX / stickLength;
			float normY = calY / stickLength;
			// use screen resolution
			float mouseX = (float)jc->getSetting(SettingID::SCREEN_RESOLUTION_X) * 0.5f + 0.5f + normX * mouse_ring_radius;
			float mouseY = (float)jc->getSetting(SettingID::SCREEN_RESOLUTION_Y) * 0.5f + 0.5f - normY * mouse_ring_radius;
			// normalize
			mouseX = mouseX / jc->getSetting(SettingID::SCREEN_RESOLUTION_X);
			mouseY = mouseY / jc->getSetting(SettingID::SCREEN_RESOLUTION_Y);
			// do it!
			setMouseNorm(mouseX, mouseY);
			lockMouse = true;
		}
	}
	else if (jc->getSetting<StickMode>(SettingID::RIGHT_STICK_MODE) == StickMode::NO_MOUSE) { // Do not do if invalid
		// left!
		jc->handleButtonChange(ButtonID::RLEFT, left);
		// right!
		jc->handleButtonChange(ButtonID::RRIGHT, right);
		// up!
		jc->handleButtonChange(ButtonID::RUP, up);
		// down!
		jc->handleButtonChange(ButtonID::RDOWN, down);

		rightAny = left | right | up | down; // ring doesn't count
	}

	// button mappings
	jc->handleButtonChange(ButtonID::UP, (state.buttons & JSMASK_UP) > 0);
	jc->handleButtonChange(ButtonID::DOWN, (state.buttons & JSMASK_DOWN) > 0);
	jc->handleButtonChange(ButtonID::LEFT, (state.buttons & JSMASK_LEFT) > 0);
	jc->handleButtonChange(ButtonID::RIGHT, (state.buttons & JSMASK_RIGHT) > 0);
	jc->handleButtonChange(ButtonID::L, (state.buttons & JSMASK_L) > 0);
	jc->handleTriggerChange(ButtonID::ZL, ButtonID::ZLF, jc->getSetting<TriggerMode>(SettingID::ZL_MODE), state.lTrigger);
	jc->handleButtonChange(ButtonID::MINUS, (state.buttons & JSMASK_MINUS) > 0);
	jc->handleButtonChange(ButtonID::CAPTURE, (state.buttons & JSMASK_CAPTURE) > 0);
	jc->handleButtonChange(ButtonID::E, (state.buttons & JSMASK_E) > 0);
	jc->handleButtonChange(ButtonID::S, (state.buttons & JSMASK_S) > 0);
	jc->handleButtonChange(ButtonID::N, (state.buttons & JSMASK_N) > 0);
	jc->handleButtonChange(ButtonID::W, (state.buttons & JSMASK_W) > 0);
	jc->handleButtonChange(ButtonID::R, (state.buttons & JSMASK_R) > 0);
	jc->handleTriggerChange(ButtonID::ZR, ButtonID::ZRF, jc->getSetting<TriggerMode>(SettingID::ZR_MODE), state.rTrigger);
	jc->handleButtonChange(ButtonID::PLUS, (state.buttons & JSMASK_PLUS) > 0);
	jc->handleButtonChange(ButtonID::HOME, (state.buttons & JSMASK_HOME) > 0);
	jc->handleButtonChange(ButtonID::SL, (state.buttons & JSMASK_SL) > 0);
	jc->handleButtonChange(ButtonID::SR, (state.buttons & JSMASK_SR) > 0);
	jc->handleButtonChange(ButtonID::L3, (state.buttons & JSMASK_LCLICK) > 0);
	jc->handleButtonChange(ButtonID::R3, (state.buttons & JSMASK_RCLICK) > 0);

	// Handle buttons before GYRO because some of them may affect the value of blockGyro
	auto gyro = jc->getSetting<GyroSettings>(SettingID::GYRO_ON); // same result as getting GYRO_OFF
	switch (gyro.ignore_mode) {
	case GyroIgnoreMode::BUTTON:
		blockGyro = gyro.always_off ^ jc->IsPressed(state, gyro.button); // Use jc->IsActive(gyro_button) to consider button state
		break;
	case GyroIgnoreMode::LEFT_STICK:
		blockGyro = (gyro.always_off ^ leftAny);
		break;
	case GyroIgnoreMode::RIGHT_STICK:
		blockGyro = (gyro.always_off ^ rightAny);
		break;
	}
	float gyro_x_sign_to_use = jc->getSetting(SettingID::GYRO_AXIS_X);
	float gyro_y_sign_to_use = jc->getSetting(SettingID::GYRO_AXIS_Y);

	// Apply gyro modifiers in the queue from oldest to newest (thus giving priority to most recent)
	for (auto pair : jc->btnCommon.gyroActionQueue)
	{
		// TODO: logic optimization
		if (pair.second.code == GYRO_ON_BIND) blockGyro = false;
		else if (pair.second.code == GYRO_OFF_BIND) blockGyro = true;
		else if (pair.second.code == GYRO_INV_X) gyro_x_sign_to_use *= -1; // Intentionally don't support multiple inversions
		else if (pair.second.code == GYRO_INV_Y) gyro_y_sign_to_use *= -1;
		else if (pair.second.code == GYRO_INVERT)
		{
			gyro_x_sign_to_use *= -1;
			gyro_y_sign_to_use *= -1;
		}
	}

	if (blockGyro) {
		gyroX = 0;
		gyroY = 0;
	}
	// optionally ignore the gyro of one of the joycons
	if (!lockMouse &&
		(jc->controller_type == JS_SPLIT_TYPE_FULL ||
		(jc->controller_type & (int)jc->getSetting<JoyconMask>(SettingID::JOYCON_GYRO_MASK)) == 0))
	{
		//printf("GX: %0.4f GY: %0.4f GZ: %0.4f\n", imuState.gyroX, imuState.gyroY, imuState.gyroZ);
		float mouseCalibration = jc->getSetting(SettingID::REAL_WORLD_CALIBRATION) / os_mouse_speed / jc->getSetting(SettingID::IN_GAME_SENS);
		shapedSensitivityMoveMouse(gyroX * gyro_x_sign_to_use, gyroY * gyro_y_sign_to_use, jc->getSetting<FloatXY>(SettingID::MIN_GYRO_SENS), jc->getSetting<FloatXY>(SettingID::MAX_GYRO_SENS),
			jc->getSetting(SettingID::MIN_GYRO_THRESHOLD), jc->getSetting(SettingID::MAX_GYRO_THRESHOLD), deltaTime,
			camSpeedX * jc->getSetting(SettingID::STICK_AXIS_X), -camSpeedY * jc->getSetting(SettingID::STICK_AXIS_Y), mouseCalibration);
	}
}

// https://stackoverflow.com/a/25311622/1130520 says this is why filenames obtained by fgets don't work
static void removeNewLine(char* string) {
	char *p;
	if ((p = strchr(string, '\n')) != NULL) {
		*p = '\0'; /* remove newline */
	}
}


// https://stackoverflow.com/a/4119881/1130520 gives us case insensitive equality
static bool iequals(const string& a, const string& b)
{
	return equal(a.begin(), a.end(),
		b.begin(), b.end(),
		[](char a, char b) {
			return tolower(a) == tolower(b);
		});
}

bool AutoLoadPoll(void *param)
{
	auto registry = reinterpret_cast<CmdRegistry*>(param);
	static string lastModuleName;
	string windowTitle, windowModule;
	tie(windowModule, windowTitle) = GetActiveWindowName();
	if (!windowModule.empty() && windowModule != lastModuleName && windowModule.compare("JoyShockMapper.exe") != 0)
	{
		lastModuleName = windowModule;
		string cwd(GetCWD());
		if (!cwd.empty())
		{
			cwd.append("\\AutoLoad\\");
			auto files = ListDirectory(cwd);
			auto noextmodule = windowModule.substr(0, windowModule.find_first_of('.'));
			printf("[AUTOLOAD] \"%s\" in focus: ", windowTitle.c_str()); // looking for config : " , );
			bool success = false;
			for (auto file : files)
			{
				auto noextconfig = file.substr(0, file.find_first_of('.'));
				if (iequals(noextconfig, noextmodule))
				{
					printf("loading \"AutoLoad\\%s.txt\".\n", noextconfig.c_str());
					loading_lock.lock();
					registry->processLine(cwd + file);
					loading_lock.unlock();
					printf("[AUTOLOAD] Loading completed\n");
					success = true;
					break;
				}
			}
			if (!success)
			{
				printf("create \"AutoLoad\\%s.txt\" to autoload for this application.\n", noextmodule.c_str());
			}
		}
		else
		{
			printf("[AUTOLOAD] ERROR could not load CWD. Disabling AUTOLOAD.");
			return false;
		}
	}
	return true;
}

void beforeShowTrayMenu()
{
	if (!tray || !*tray) printf("ERROR: Cannot create tray item.\n");
	else
	{
		tray->ClearMenuMap();
		tray->AddMenuItem(U("Show Console"), &ShowConsole);
		tray->AddMenuItem(U("Reconnect controllers"), []()
		{
			WriteToConsole("RECONNECT_CONTROLLERS");
		});
		tray->AddMenuItem(U("AutoLoad"), [](bool isChecked)
			{
				isChecked ? 
					autoLoadThread->Start() : 
					autoLoadThread->Stop();
			}, bind(&PollingThread::isRunning, autoLoadThread.get()));

		if (Whitelister::IsHIDCerberusRunning())
		{
			tray->AddMenuItem(U("Whitelist"), [](bool isChecked)
				{
					isChecked ?
						whitelister.Add() :
						whitelister.Remove();
				}, bind(&Whitelister::operator bool, &whitelister));
		}
		tray->AddMenuItem(U("Calibrate all devices"), [](bool isChecked)
		{
			isChecked ?
				WriteToConsole("RESTART_GYRO_CALIBRATION") :
				WriteToConsole("FINISH_GYRO_CALIBRATION");
		}, []()
		{
			return devicesCalibrating;
		});

		//std::string cwd(GetCWD());
		std::string autoloadFolder = "AutoLoad\\";
		for (auto file : ListDirectory(autoloadFolder.c_str()))
		{
			string fullPathName = autoloadFolder + file;
			auto noext = file.substr(0, file.find_last_of('.'));
			tray->AddMenuItem(L"AutoLoad folder", std::wstring(noext.begin(), noext.end()), [fullPathName]
			{
				WriteToConsole(string(fullPathName.begin(), fullPathName.end()));
				autoLoadThread->Stop();
			});
		}
		std::string gyroConfigsFolder = "GyroConfigs\\";
		for (auto file : ListDirectory(gyroConfigsFolder.c_str()))
		{
			string fullPathName = gyroConfigsFolder + file;
			auto noext = file.substr(0, file.find_last_of('.'));
			tray->AddMenuItem(L"GyroConfigs folder", std::wstring(noext.begin(), noext.end()), [fullPathName]
			{
				WriteToConsole(string(fullPathName.begin(), fullPathName.end()));
				autoLoadThread->Stop();
			});
		}
		tray->AddMenuItem(U("Calculate RWC"), []()
		{
			WriteToConsole("CALCULATE_REAL_WORLD_CALIBRATION");
			ShowConsole();
		});
		tray->AddMenuItem(U("Quit"), []()
		{
			WriteToConsole("QUIT");
		});
	}
}

// Perform all cleanup tasks when JSM is exiting
void CleanUp()
{
	tray->Hide();
	JslDisconnectAndDisposeAll();
	ReleaseConsole();
	whitelister.Remove();
}

float filterClamp01(float current, float next)
{
	return max(0.0f, min(1.0f, next));
}

float filterPositive(float current, float next)
{
	return max(0.0f, next);
}

float filterSign(float current, float next)
{
	return next == -1.0f || next == 0.0f || next == 1.0f ?
		next : current;
}

template <typename E, E invalid>
E filterInvalidValue(E current, E next)
{
	return next != invalid ? next : current;
}

float filterFloat(float current, float next)
{
	// Exclude Infinite, NaN and Subnormal
	return fpclassify(next) == FP_NORMAL || fpclassify(next) == FP_ZERO ? next : current;
}

FloatXY filterFloatPair(FloatXY current, FloatXY next)
{
	return (fpclassify(next.x()) == FP_NORMAL || fpclassify(next.x()) == FP_ZERO) &&
		   (fpclassify(next.y()) == FP_NORMAL || fpclassify(next.y()) == FP_ZERO) ?
		next : current;
}

TriggerMode triggerModeNotification(TriggerMode current, TriggerMode next)
{
	for (auto js : handle_to_joyshock)
	{
		if (JslGetControllerType(js.first) != JS_TYPE_DS4)
		{
			printf("WARNING: Dual Stage Triggers are only valid on analog triggers. Full pull bindings will be ignored on non DS4 controllers.\n");
			break;
		}
	}
	return next;
}

void UpdateRingModeFromStickMode(JSMVariable<RingMode> *stickRingMode, StickMode newValue)
{
	if (newValue == StickMode::INNER_RING)
	{
		*stickRingMode = RingMode::INNER;
	}
	else if (newValue == StickMode::OUTER_RING)
	{
		*stickRingMode = RingMode::OUTER;
	}
}

class GyroAssignment : public JSMAssignment<GyroSettings>
{
private:
	const bool _always_off;

	bool GyroParser(in_string data)
	{
		stringstream ss(data);
		// Ignore whitespaces until the '=' is found
		char c;
		do {
			ss >> c;
		} while (ss.good() && c != '=');
		if (!ss.good())
		{
			GyroSettings value(_var);
			//No assignment? Display current assignment
			cout << (value.always_off ? string("GYRO_ON") : string("GYRO_OFF")) << " = " << value << endl;;
		}
		if (c == '=')
		{
			// Read the value
			GyroSettings value;
			value.always_off = _always_off; // Added line from DefaultParser
			ss >> value;
			if (!ss.fail())
			{
				GyroSettings oldVal = _var;
				_var = value;
				// Command succeeded if the value requested was the current one
				// or if the new value is different from the old.
				return value == oldVal || _var != oldVal; // Command processed successfully
			}
			// Couldn't read the value
		}
		// Not an equal sign? The command is entered wrong!
		return false;
	}

	void DisplayGyroSettingValue(GyroSettings value)
	{
		cout << (value.always_off ? string("GYRO_ON") : string("GYRO_OFF")) << " is set to " << value << endl;;
	}
public:
	GyroAssignment(in_string name, bool always_off)
		: JSMAssignment(name, gyro_settings)
		, _always_off(always_off)
	{
		SetParser(bind(&GyroAssignment::GyroParser, this, placeholders::_2));
		_var.RemoveOnChangeListener(_listenerId);
	}

	GyroAssignment *SetListener()
	{
		_listenerId = _var.AddOnChangeListener(bind(&GyroAssignment::DisplayGyroSettingValue, this, placeholders::_1));
		return this;
	}

	virtual ~GyroAssignment() = default;
};

//int main(int argc, char *argv[]) {
#ifdef _WIN32
int __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPWSTR cmdLine, int cmdShow) {
	auto trayIconData = hInstance;
#else
int main(int argc, char *argv[]) {
	static_cast<void>(argc);
	static_cast<void>(argv);
	void *trayIconData = nullptr;
#endif // _WIN32
	mappings.reserve(MAPPING_SIZE);
	static const KeyCode CALIBRATE_KEY = KeyCode("CALIBRATE");

	for (int id = 0; id < MAPPING_SIZE; ++id)
	{
		Mapping def = (id == int(ButtonID::HOME) || id == int(ButtonID::CAPTURE)) ? Mapping(CALIBRATE_KEY, CALIBRATE_KEY) : Mapping();
		mappings.push_back(JSMButton(ButtonID(id), def));
	}
	tray.reset(new TrayIcon(trayIconData, &beforeShowTrayMenu ));
	// console
	initConsole(&CleanUp);
	printf("Welcome to JoyShockMapper version %s!\n", version);
	//if (whitelister) printf("JoyShockMapper was successfully whitelisted!\n");
	// prepare for input
	connectDevices();
	JslSetCallback(&joyShockPollCallback);
	tray->Show();

	left_stick_mode.SetFilter(&filterInvalidValue<StickMode, StickMode::INVALID>)->
		AddOnChangeListener(bind(&UpdateRingModeFromStickMode, &left_ring_mode, ::placeholders::_1));
	right_stick_mode.SetFilter(&filterInvalidValue<StickMode, StickMode::INVALID>)->
		AddOnChangeListener(bind(&UpdateRingModeFromStickMode, &right_ring_mode, ::placeholders::_1));
	left_ring_mode.SetFilter(&filterInvalidValue<RingMode, RingMode::INVALID>);
	right_ring_mode.SetFilter(&filterInvalidValue<RingMode, RingMode::INVALID>);
	mouse_x_from_gyro.SetFilter(&filterInvalidValue<GyroAxisMask, GyroAxisMask::INVALID>);
	mouse_y_from_gyro.SetFilter(&filterInvalidValue<GyroAxisMask, GyroAxisMask::INVALID>);
	gyro_settings.SetFilter( [] (GyroSettings current, GyroSettings next)
		{
			return next.ignore_mode != GyroIgnoreMode::INVALID ? next : current;
		});
	joycon_gyro_mask.SetFilter(&filterInvalidValue<JoyconMask, JoyconMask::INVALID>);
	zlMode.SetFilter(&filterInvalidValue<TriggerMode, TriggerMode::INVALID>);
	zrMode.SetFilter(&filterInvalidValue<TriggerMode, TriggerMode::INVALID>);
	zlMode.SetFilter(&triggerModeNotification);
	zrMode.SetFilter(&triggerModeNotification);
	flick_snap_mode.SetFilter(&filterInvalidValue<FlickSnapMode, FlickSnapMode::INVALID>);
	min_gyro_sens.SetFilter(&filterFloatPair);
	max_gyro_sens.SetFilter(&filterFloatPair);
	min_gyro_threshold.SetFilter(&filterFloat);
	max_gyro_threshold.SetFilter(&filterFloat);
	stick_power.SetFilter(&filterFloat);
	real_world_calibration.SetFilter(&filterFloat);
	in_game_sens.SetFilter(bind(&fmaxf, 0.0001f, ::placeholders::_2));
	trigger_threshold.SetFilter(&filterClamp01);
	aim_x_sign.SetFilter(&filterInvalidValue<AxisMode, AxisMode::INVALID>);
	aim_y_sign.SetFilter(&filterInvalidValue<AxisMode, AxisMode::INVALID>);
	gyro_x_sign.SetFilter(&filterInvalidValue<AxisMode, AxisMode::INVALID>);
	gyro_y_sign.SetFilter(&filterInvalidValue<AxisMode, AxisMode::INVALID>);
	flick_time.SetFilter(bind(&fmaxf, 0.0001f, ::placeholders::_2));
	gyro_smooth_time.SetFilter(bind(&fmaxf, 0.0001f, ::placeholders::_2));
	gyro_smooth_threshold.SetFilter(&filterPositive);
	gyro_cutoff_speed.SetFilter(&filterPositive);
	gyro_cutoff_recovery.SetFilter(&filterPositive);
	stick_acceleration_rate.SetFilter(&filterPositive);
	stick_acceleration_cap.SetFilter(bind(&fmaxf, 1.0f, ::placeholders::_2));
	stick_deadzone_inner.SetFilter(&filterClamp01);
	stick_deadzone_outer.SetFilter(&filterClamp01);
	mouse_ring_radius.SetFilter([](float c, float n) { return n <= screen_resolution_y ? floorf(n) : c; });
	screen_resolution_x.SetFilter(&filterPositive);
	screen_resolution_y.SetFilter(&filterPositive);
	rotate_smooth_override.SetFilter(&filterClamp01);
	flick_snap_strength.SetFilter(&filterClamp01);
	
	resetAllMappings();
	
	CmdRegistry commandRegistry;
	for (auto &mapping : mappings) // Add all button mappings as commands
	{
		commandRegistry.Add(new JSMAssignment<Mapping>(mapping.getName(), mapping));
	}
	commandRegistry.Add((new JSMAssignment<FloatXY>("MIN_GYRO_SENS", min_gyro_sens))
		->SetHelp("Minimal gyro sensitivity factor before ramping linearily to maximum value.\nYou can assign a second value as a different vertical sensitivity."));
	commandRegistry.Add((new JSMAssignment<FloatXY>("MAX_GYRO_SENS", max_gyro_sens))
		->SetHelp("Maximal gyro sensitivity factor after ramping linearily from minimal value.\nYou can assign a second value as a different vertical sensitivity."));
	commandRegistry.Add((new JSMAssignment<float>("MIN_GYRO_THRESHOLD", min_gyro_threshold))
		->SetHelp("Number of degrees per second at which to apply minimal gyro sensitivity."));
	commandRegistry.Add((new JSMAssignment<float>("MAX_GYRO_THRESHOLD", max_gyro_threshold))
		->SetHelp("Number of degrees per second at which to apply maximal gyro sensitivity."));
	commandRegistry.Add((new JSMAssignment<float>("STICK_POWER", stick_power))
	    ->SetHelp(""));
	commandRegistry.Add((new JSMAssignment<float>("STICK_SENS", stick_sens))
		->SetHelp("Stick sensitivity when using classic AIM mode."));
	commandRegistry.Add((new JSMAssignment<float>("REAL_WORLD_CALIBRATION", real_world_calibration))
		->SetHelp("Calibration value mapping mouse values to in game degrees. This value is used for FLICK mode.")); // And other things?
	commandRegistry.Add((new JSMAssignment<float>("IN_GAME_SENS", in_game_sens))
		->SetHelp("Set this value to the sensitivity you use in game. It is used by stick FLICK and AIM modes."));
	commandRegistry.Add((new JSMAssignment<float>("TRIGGER_THRESHOLD", trigger_threshold))
		->SetHelp("Set this to a value between 0 and 1, representing the analog threshold point at which to apply soft press binding."));
	commandRegistry.Add((new JSMMacro("RESET_MAPPINGS"))->SetMacro(bind(&do_RESET_MAPPINGS))
		->SetHelp("Delete all cusstom bindings and reset to default.\nHOME and CAPTURE are set to CALIBRATE on both tap and hold by default."));
	commandRegistry.Add((new JSMMacro("NO_GYRO_BUTTON"))->SetMacro(bind(&do_NO_GYRO_BUTTON))
		->SetHelp("Enable gyro at all times, withhout any GYRO_OFF binding."));
	commandRegistry.Add((new JSMAssignment<StickMode>("LEFT_STICK_MODE", left_stick_mode))
		->SetHelp("Set a mouse mode for the left stick. Valid values are the following:\nNO_MOUSE, AIM, FLICK, FLICK_ONLY, ROTATE_ONLY, MOUSE_RING, MOUSE_AREA, OUTER_RING, INNER_RING"));
	commandRegistry.Add((new JSMAssignment<StickMode>("RIGHT_STICK_MODE", right_stick_mode))
		->SetHelp("Set a mouse mode for the right stick. Valid values are the following:\nNO_MOUSE, AIM, FLICK, FLICK_ONLY, ROTATE_ONLY, MOUSE_RING, MOUSE_AREA, OUTER_RING, INNER_RING"));
	commandRegistry.Add((new GyroAssignment("GYRO_OFF", false))
		->SetHelp("Assign a controller button to disable the gyro when pressed."));
	commandRegistry.Add((new GyroAssignment("GYRO_ON", true))->SetListener() // Set only one listener
		->SetHelp("Assign a controller button to enable the gyro when pressed."));
	commandRegistry.Add((new JSMAssignment<AxisMode>("STICK_AXIS_X", aim_x_sign))
		->SetHelp("Specify the stick X axis inversion value. Valid values are the following:\nSTANDARD or 1, and INVERTED or -1"));
	commandRegistry.Add((new JSMAssignment<AxisMode>("STICK_AXIS_Y", aim_y_sign))
		->SetHelp("Specify the stick Y axis inversion value. Valid values are the following:\nSTANDARD or 1, and INVERTED or -1"));
	commandRegistry.Add((new JSMAssignment<AxisMode>("GYRO_AXIS_X", gyro_x_sign))
		->SetHelp("Specify the gyro X axis inversion value. Valid values are the following:\nSTANDARD or 1, and INVERTED or -1"));
	commandRegistry.Add((new JSMAssignment<AxisMode>("GYRO_AXIS_Y", gyro_y_sign))
		->SetHelp("Specify the gyro Y axis inversion value. Valid values are the following:\nSTANDARD or 1, and INVERTED or -1"));
	commandRegistry.Add((new JSMMacro("RECONNECT_CONTROLLERS"))->SetMacro(bind(&do_RECONNECT_CONTROLLERS))
		->SetHelp("Reload the controller listing."));
	commandRegistry.Add((new JSMMacro("COUNTER_OS_MOUSE_SPEED"))->SetMacro(bind(do_COUNTER_OS_MOUSE_SPEED))
		->SetHelp("JoyShockMapper will load the user's OS mouse sensitivity value to consider it in it's calculations."));
	commandRegistry.Add((new JSMMacro("IGNORE_OS_MOUSE_SPEED"))->SetMacro(bind(do_IGNORE_OS_MOUSE_SPEED))
		->SetHelp("Disable JoyShockMapper's consideration of the the user's OS mouse sensitivity value."));
	commandRegistry.Add((new JSMAssignment<JoyconMask>("JOYCON_GYRO_MASK", joycon_gyro_mask))
		->SetHelp("When using two Joycons, select which one will be used for gyro. Valid values are the following:\nUSE_BOTH, IGNORE_LEFT, IGNORE_RIGHT, IGNORE_BOTH"));
	commandRegistry.Add((new JSMMacro("GYRO_SENS"))->SetMacro(bind(&do_GYRO_SENS, placeholders::_2))
		->SetHelp("Sets a gyro sensitivity to use. This sets both MIN_GYRO_SENS and MAX_GYRO_SENS to the same values. You can assign a second value as a different vertical sensitivity."));
	commandRegistry.Add((new JSMAssignment<float>("FLICK_TIME", flick_time))
		->SetHelp("Sets the duration for which a flick will last in seconds. This vlaue is used by stick FLICK mode."));
	commandRegistry.Add((new JSMAssignment<float>("GYRO_SMOOTH_THRESHOLD", gyro_smooth_threshold))
		->SetHelp(""));
	commandRegistry.Add((new JSMAssignment<float>("GYRO_SMOOTH_TIME", gyro_smooth_time))
		->SetHelp(""));
	commandRegistry.Add((new JSMAssignment<float>("GYRO_CUTOFF_SPEED", gyro_cutoff_speed))
		->SetHelp(""));
	commandRegistry.Add((new JSMAssignment<float>("GYRO_CUTOFF_RECOVERY", gyro_cutoff_recovery))
		->SetHelp(""));
	commandRegistry.Add((new JSMAssignment<float>("STICK_ACCELERATION_RATE", stick_acceleration_rate))
		->SetHelp(""));
	commandRegistry.Add((new JSMAssignment<float>("STICK_ACCELERATION_CAP", stick_acceleration_cap))
		->SetHelp(""));
	commandRegistry.Add((new JSMAssignment<float>("STICK_DEADZONE_INNER", stick_deadzone_inner))
		->SetHelp("Defines a radius of the stick for which all values will be null. This value can only be between 0 and 1 but it should be small."));
	commandRegistry.Add((new JSMAssignment<float>("STICK_DEADZONE_OUTER", stick_deadzone_outer))
		->SetHelp("Defines a distance from the stick's outer edge for which all values will be maximal. This value can only be between 0 and 1 but it should be small."));
	commandRegistry.Add((new JSMMacro("CALCULATE_REAL_WORLD_CALIBRATION"))->SetMacro(bind(&do_CALCULATE_REAL_WORLD_CALIBRATION, placeholders::_2))
		->SetHelp("Get JoyShockMapper to recommend you a REAL_WORLD_CALIBRATION value after performing the calibration sequence. Visit GyroWiki for details:\nhttp://gyrowiki.jibbsmart.com/blog:joyshockmapper-guide#calibrating"));
	commandRegistry.Add((new JSMMacro("FINISH_GYRO_CALIBRATION"))->SetMacro(bind(&do_FINISH_GYRO_CALIBRATION))
		->SetHelp("Stop the calibration of the gyro of all controllers."));
	commandRegistry.Add((new JSMMacro("RESTART_GYRO_CALIBRATION"))->SetMacro(bind(&do_RESTART_GYRO_CALIBRATION))
		->SetHelp("Start the calibration of the gyro of all controllers."));
	commandRegistry.Add((new JSMAssignment<GyroAxisMask>("MOUSE_X_FROM_GYRO_AXIS", mouse_x_from_gyro))
		->SetHelp("Pick a gyro axis to operate on the mouse's X axis. Valid values are the following: X, Y and Z."));
	commandRegistry.Add((new JSMAssignment<GyroAxisMask>("MOUSE_Y_FROM_GYRO_AXIS", mouse_y_from_gyro))
		->SetHelp("Pick a gyro axis to operate on the mouse's Y axis. Valid values are the following: X, Y and Z."));
	commandRegistry.Add((new JSMAssignment<TriggerMode>("ZR_MODE", zlMode))
		->SetHelp("Controllers with a right analog trigger can use one of the following dual stage trigger modes:\nNO_FULL, NO_SKIP, MAY_SKIP, MUST_SKIP, MAY_SKIP_R, MUST_SKIP_R"));
	commandRegistry.Add((new JSMAssignment<TriggerMode>("ZL_MODE", zrMode))
		->SetHelp("Controllers with a left analog trigger can use one of the following dual stage trigger modes:\nNO_FULL, NO_SKIP, MAY_SKIP, MUST_SKIP, MAY_SKIP_R, MUST_SKIP_R"));
	stringstream ss("AUTOLOAD will automatically load file from the following folder when a window with a matching executable name enters focus:\n");
	ss << GetCWD() << "\\AutoLoad\\";
	commandRegistry.Add((new JSMMacro("AUTOLOAD"))->SetParser(&do_AUTOLOAD)
		->SetHelp(ss.str()));
	commandRegistry.Add((new JSMMacro("README"))->SetMacro(bind(&do_README))
		->SetHelp("Open the latest JoyShockMapper README in your browser."));
	commandRegistry.Add((new JSMMacro("WHITELIST_SHOW"))->SetMacro(bind(&do_WHITELIST_SHOW))
		->SetHelp("Open HIDCerberus configuration page in your browser."));
	commandRegistry.Add((new JSMMacro("WHITELIST_ADD"))->SetMacro(bind(&do_WHITELIST_ADD))
		->SetHelp("Add JoyShockMapper to HIDGuardian whitelisted applications."));
	commandRegistry.Add((new JSMMacro("WHITELIST_REMOVE"))->SetMacro(bind(&do_WHITELIST_REMOVE))
		->SetHelp("Remove JoyShockMapper from HIDGuardian whitelisted applications."));
	commandRegistry.Add((new JSMAssignment<RingMode>("LEFT_RING_MODE", left_ring_mode))
		->SetHelp("Pick a ring where to apply the LEFT_RING binding. Valid values are the following: INNER and OUTER."));
	commandRegistry.Add((new JSMAssignment<RingMode>("RIGHT_RING_MODE", right_ring_mode))
		->SetHelp("Pick a ring where to apply the RIGHT_RING binding. Valid values are the following: INNER and OUTER."));
	commandRegistry.Add((new JSMAssignment<float>("MOUSE_RING_RADIUS", mouse_ring_radius))
		->SetHelp("Pick a radius on which the cursor will be allowed to move. This value is used for stick mode MOUSE_RING and MOUSE_AREA."));
	commandRegistry.Add((new JSMAssignment<float>("SCREEN_RESOLUTION_X", screen_resolution_x))
		->SetHelp("Indicate your monitor's horizontal resolution when using the stick mode MOUSE_RING."));
	commandRegistry.Add((new JSMAssignment<float>("SCREEN_RESOLUTION_Y", screen_resolution_y))
		->SetHelp("Indicate your monitor's vertical resolution when using the stick mode MOUSE_RING."));
	commandRegistry.Add((new JSMAssignment<float>("ROTATE_SMOOTH_OVERRIDE", rotate_smooth_override))
		->SetHelp(""));
	commandRegistry.Add((new JSMAssignment<FlickSnapMode>("FLICK_SNAP_MODE", flick_snap_mode))
		->SetHelp("Constrain flicks within cardinal directions. Valid values are the following: NONE or 0, FOUR or 4 and EIGHT or 8."));
	commandRegistry.Add((new JSMAssignment<float>("FLICK_SNAP_STRENGTH", flick_snap_strength))
		->SetHelp(""));
	// TODO Add HELP command

	bool quit = false;
	commandRegistry.Add((new JSMMacro("QUIT"))
		->SetMacro( [&quit] (JSMMacro *, in_string)
			{
				quit = true;
			})
		->SetHelp("Close the application.")
	);

	autoLoadThread.reset(new PollingThread(&AutoLoadPoll, &commandRegistry, 1000, true)); // Start by default
	if (autoLoadThread && *autoLoadThread)
	{
		printf("AutoLoad is enabled. Configurations in \"%s\" folder will get loaded when matching application is in focus.\n", AUTOLOAD_FOLDER);
	}
	else printf("[AUTOLOAD] AutoLoad is unavailable\n");


	// The main loop is simple and reads like pseudocode
	string enteredCommand;
	while (!quit)
	{
		getline(cin, enteredCommand);
		loading_lock.lock();
		commandRegistry.processLine(enteredCommand);
		loading_lock.unlock();
	}
	CleanUp();
	return 0;
}
