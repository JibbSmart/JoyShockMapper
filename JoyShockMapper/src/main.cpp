#include "JoyShockMapper.h"
#include "JSMVersion.h"
#include "JslWrapper.h"
#include "DigitalButton.h"
#include "InputHelpers.h"
#include "Whitelister.h"
#include "TrayIcon.h"
#include "JSMAssignment.hpp"
#include "quatMaths.cpp"
#include "Gamepad.h"

#include <mutex>
#include <deque>
#include <iomanip>
#include <filesystem>
#include <memory>
#include <cfloat>
#include <cuchar>
#include <cstring>

#ifdef _WIN32
#include <shellapi.h>
#else
#define UCHAR unsigned char
#endif

#pragma warning(disable : 4996) // Disable deprecated API warnings

#define PI 3.14159265359f

const KeyCode KeyCode::EMPTY = KeyCode();
const Mapping Mapping::NO_MAPPING = Mapping("NONE");
std::string NONAME;
function<bool(in_string)> Mapping::_isCommandValid = function<bool(in_string)>();
unique_ptr<JslWrapper> jsl;
unique_ptr<TrayIcon> tray;
unique_ptr<Whitelister> whitelister;

class JoyShock;
void joyShockPollCallback(int jcHandle, JOY_SHOCK_STATE state, JOY_SHOCK_STATE lastState, IMU_STATE imuState, IMU_STATE lastImuState, float deltaTime);
void TouchCallback(int jcHandle, TOUCH_STATE newState, TOUCH_STATE prevState, float delta_time);

// Contains all settings that can be modeshifted. They should be accessed only via Joyshock::getSetting
JSMSetting<StickMode> left_stick_mode = JSMSetting<StickMode>(SettingID::LEFT_STICK_MODE, StickMode::NO_MOUSE);
JSMSetting<StickMode> right_stick_mode = JSMSetting<StickMode>(SettingID::RIGHT_STICK_MODE, StickMode::NO_MOUSE);
JSMSetting<StickMode> motion_stick_mode = JSMSetting<StickMode>(SettingID::MOTION_STICK_MODE, StickMode::NO_MOUSE);
JSMSetting<RingMode> left_ring_mode = JSMSetting<RingMode>(SettingID::LEFT_RING_MODE, RingMode::OUTER);
JSMSetting<RingMode> right_ring_mode = JSMSetting<RingMode>(SettingID::RIGHT_RING_MODE, RingMode::OUTER);
JSMSetting<RingMode> motion_ring_mode = JSMSetting<RingMode>(SettingID::MOTION_RING_MODE, RingMode::OUTER);
JSMSetting<GyroAxisMask> mouse_x_from_gyro = JSMSetting<GyroAxisMask>(SettingID::MOUSE_X_FROM_GYRO_AXIS, GyroAxisMask::Y);
JSMSetting<GyroAxisMask> mouse_y_from_gyro = JSMSetting<GyroAxisMask>(SettingID::MOUSE_Y_FROM_GYRO_AXIS, GyroAxisMask::X);
JSMSetting<GyroSettings> gyro_settings = JSMSetting<GyroSettings>(SettingID::GYRO_ON, GyroSettings()); // Ignore mode none means no GYRO_OFF button
JSMSetting<JoyconMask> joycon_gyro_mask = JSMSetting<JoyconMask>(SettingID::JOYCON_GYRO_MASK, JoyconMask::IGNORE_LEFT);
JSMSetting<JoyconMask> joycon_motion_mask = JSMSetting<JoyconMask>(SettingID::JOYCON_MOTION_MASK, JoyconMask::IGNORE_RIGHT);
JSMSetting<TriggerMode> zlMode = JSMSetting<TriggerMode>(SettingID::ZL_MODE, TriggerMode::NO_FULL);
JSMSetting<TriggerMode> zrMode = JSMSetting<TriggerMode>(SettingID::ZR_MODE, TriggerMode::NO_FULL);
JSMSetting<FlickSnapMode> flick_snap_mode = JSMSetting<FlickSnapMode>(SettingID::FLICK_SNAP_MODE, FlickSnapMode::NONE);
JSMSetting<FloatXY> min_gyro_sens = JSMSetting<FloatXY>(SettingID::MIN_GYRO_SENS, { 0.0f, 0.0f });
JSMSetting<FloatXY> max_gyro_sens = JSMSetting<FloatXY>(SettingID::MAX_GYRO_SENS, { 0.0f, 0.0f });
JSMSetting<float> min_gyro_threshold = JSMSetting<float>(SettingID::MIN_GYRO_THRESHOLD, 0.0f);
JSMSetting<float> max_gyro_threshold = JSMSetting<float>(SettingID::MAX_GYRO_THRESHOLD, 0.0f);
JSMSetting<float> stick_power = JSMSetting<float>(SettingID::STICK_POWER, 1.0f);
JSMSetting<FloatXY> stick_sens = JSMSetting<FloatXY>(SettingID::STICK_SENS, { 360.0f, 360.0f });
// There's an argument that RWC has no interest in being modeshifted and thus could be outside this structure.
JSMSetting<float> real_world_calibration = JSMSetting<float>(SettingID::REAL_WORLD_CALIBRATION, 40.0f);
JSMSetting<float> virtual_stick_calibration = JSMSetting<float>(SettingID::VIRTUAL_STICK_CALIBRATION, 360.0f);
JSMSetting<float> in_game_sens = JSMSetting<float>(SettingID::IN_GAME_SENS, 1.0f);
JSMSetting<float> trigger_threshold = JSMSetting<float>(SettingID::TRIGGER_THRESHOLD, 0.0f);
JSMSetting<AxisSignPair> left_stick_axis = JSMSetting<AxisSignPair>(SettingID::LEFT_STICK_AXIS, { AxisMode::STANDARD, AxisMode::STANDARD });
JSMSetting<AxisSignPair> right_stick_axis = JSMSetting<AxisSignPair>(SettingID::RIGHT_STICK_AXIS, { AxisMode::STANDARD, AxisMode::STANDARD });
JSMSetting<AxisSignPair> motion_stick_axis = JSMSetting<AxisSignPair>(SettingID::MOTION_STICK_AXIS, { AxisMode::STANDARD, AxisMode::STANDARD });
JSMSetting<AxisSignPair> touch_stick_axis = JSMSetting<AxisSignPair>(SettingID::TOUCH_STICK_AXIS, { AxisMode::STANDARD, AxisMode::STANDARD });
JSMSetting<AxisMode> aim_x_sign = JSMSetting<AxisMode>(SettingID::STICK_AXIS_X, AxisMode::STANDARD); // Legacy command
JSMSetting<AxisMode> aim_y_sign = JSMSetting<AxisMode>(SettingID::STICK_AXIS_Y, AxisMode::STANDARD); // Legacy command
JSMSetting<AxisMode> gyro_y_sign = JSMSetting<AxisMode>(SettingID::GYRO_AXIS_Y, AxisMode::STANDARD);
JSMSetting<AxisMode> gyro_x_sign = JSMSetting<AxisMode>(SettingID::GYRO_AXIS_X, AxisMode::STANDARD);
JSMSetting<float> flick_time = JSMSetting<float>(SettingID::FLICK_TIME, 0.1f);
JSMSetting<float> flick_time_exponent = JSMSetting<float>(SettingID::FLICK_TIME_EXPONENT, 0.0f);
JSMSetting<float> gyro_smooth_time = JSMSetting<float>(SettingID::GYRO_SMOOTH_TIME, 0.125f);
JSMSetting<float> gyro_smooth_threshold = JSMSetting<float>(SettingID::GYRO_SMOOTH_THRESHOLD, 0.0f);
JSMSetting<float> gyro_cutoff_speed = JSMSetting<float>(SettingID::GYRO_CUTOFF_SPEED, 0.0f);
JSMSetting<float> gyro_cutoff_recovery = JSMSetting<float>(SettingID::GYRO_CUTOFF_RECOVERY, 0.0f);
JSMSetting<float> stick_acceleration_rate = JSMSetting<float>(SettingID::STICK_ACCELERATION_RATE, 0.0f);
JSMSetting<float> stick_acceleration_cap = JSMSetting<float>(SettingID::STICK_ACCELERATION_CAP, 1000000.0f);
JSMSetting<float> left_stick_deadzone_inner = JSMSetting<float>(SettingID::LEFT_STICK_DEADZONE_INNER, 0.15f);
JSMSetting<float> left_stick_deadzone_outer = JSMSetting<float>(SettingID::LEFT_STICK_DEADZONE_OUTER, 0.1f);
JSMSetting<float> flick_deadzone_angle = JSMSetting<float>(SettingID::FLICK_DEADZONE_ANGLE, 0.0f);
JSMSetting<float> right_stick_deadzone_inner = JSMSetting<float>(SettingID::RIGHT_STICK_DEADZONE_INNER, 0.15f);
JSMSetting<float> right_stick_deadzone_outer = JSMSetting<float>(SettingID::RIGHT_STICK_DEADZONE_OUTER, 0.1f);
JSMSetting<float> motion_deadzone_inner = JSMSetting<float>(SettingID::MOTION_DEADZONE_INNER, 15.f);
JSMSetting<float> motion_deadzone_outer = JSMSetting<float>(SettingID::MOTION_DEADZONE_OUTER, 135.f);
JSMSetting<float> angle_to_axis_deadzone_inner = JSMSetting<float>(SettingID::ANGLE_TO_AXIS_DEADZONE_INNER, 0.f);
JSMSetting<float> angle_to_axis_deadzone_outer = JSMSetting<float>(SettingID::ANGLE_TO_AXIS_DEADZONE_OUTER, 10.f);
JSMSetting<float> lean_threshold = JSMSetting<float>(SettingID::LEAN_THRESHOLD, 15.f);
JSMSetting<ControllerOrientation> controller_orientation = JSMSetting<ControllerOrientation>(SettingID::CONTROLLER_ORIENTATION, ControllerOrientation::FORWARD);
JSMSetting<GyroSpace> gyro_space = JSMSetting<GyroSpace>(SettingID::GYRO_SPACE, GyroSpace::LOCAL);
JSMSetting<float> trackball_decay = JSMSetting<float>(SettingID::TRACKBALL_DECAY, 1.0f);
JSMSetting<float> mouse_ring_radius = JSMSetting<float>(SettingID::MOUSE_RING_RADIUS, 128.0f);
JSMSetting<float> screen_resolution_x = JSMSetting<float>(SettingID::SCREEN_RESOLUTION_X, 1920.0f);
JSMSetting<float> screen_resolution_y = JSMSetting<float>(SettingID::SCREEN_RESOLUTION_Y, 1080.0f);
JSMSetting<float> rotate_smooth_override = JSMSetting<float>(SettingID::ROTATE_SMOOTH_OVERRIDE, -1.0f);
JSMSetting<float> flick_snap_strength = JSMSetting<float>(SettingID::FLICK_SNAP_STRENGTH, 01.0f);
JSMSetting<float> trigger_skip_delay = JSMSetting<float>(SettingID::TRIGGER_SKIP_DELAY, 150.0f);
JSMSetting<float> turbo_period = JSMSetting<float>(SettingID::TURBO_PERIOD, 80.0f);
JSMSetting<float> hold_press_time = JSMSetting<float>(SettingID::HOLD_PRESS_TIME, 150.0f);
JSMVariable<float> sim_press_window = JSMVariable<float>(50.0f);
JSMSetting<float> dbl_press_window = JSMSetting<float>(SettingID::DBL_PRESS_WINDOW, 150.0f);
JSMVariable<float> tick_time = JSMSetting<float>(SettingID::TICK_TIME, 3);
JSMSetting<Color> light_bar = JSMSetting<Color>(SettingID::LIGHT_BAR, 0xFFFFFF);
JSMSetting<FloatXY> scroll_sens = JSMSetting<FloatXY>(SettingID::SCROLL_SENS, { 30.f, 30.f });
JSMVariable<Switch> autoloadSwitch = JSMVariable<Switch>(Switch::ON);
JSMVariable<FloatXY> grid_size = JSMVariable(FloatXY{ 2.f, 1.f }); // Default left side and right side button
JSMSetting<TouchpadMode> touchpad_mode = JSMSetting<TouchpadMode>(SettingID::TOUCHPAD_MODE, TouchpadMode::GRID_AND_STICK);
JSMSetting<StickMode> touch_stick_mode = JSMSetting<StickMode>(SettingID::TOUCH_STICK_MODE, StickMode::NO_MOUSE);
JSMSetting<float> touch_stick_radius = JSMSetting<float>(SettingID::TOUCH_STICK_RADIUS, 300.f);
JSMSetting<float> touch_deadzone_inner = JSMSetting<float>(SettingID::TOUCH_DEADZONE_INNER, 0.3f);
JSMSetting<RingMode> touch_ring_mode = JSMSetting<RingMode>(SettingID::TOUCH_RING_MODE, RingMode::OUTER);
JSMSetting<FloatXY> touchpad_sens = JSMSetting<FloatXY>(SettingID::TOUCHPAD_SENS, { 1.f, 1.f });
JSMVariable<Switch> hide_minimized = JSMVariable<Switch>(Switch::OFF);
JSMVariable<ControllerScheme> virtual_controller = JSMVariable<ControllerScheme>(ControllerScheme::NONE);
JSMSetting<TriggerMode> touch_ds_mode = JSMSetting<TriggerMode>(SettingID::TOUCHPAD_DUAL_STAGE_MODE, TriggerMode::NO_SKIP);
JSMSetting<Switch> rumble_enable = JSMSetting<Switch>(SettingID::RUMBLE, Switch::ON);
JSMSetting<Switch> adaptive_trigger = JSMSetting<Switch>(SettingID::ADAPTIVE_TRIGGER, Switch::ON );
JSMSetting<AdaptiveTriggerSetting> left_trigger_effect = JSMSetting<AdaptiveTriggerSetting>(SettingID::LEFT_TRIGGER_EFFECT, AdaptiveTriggerSetting{});
JSMSetting<AdaptiveTriggerSetting> right_trigger_effect = JSMSetting<AdaptiveTriggerSetting>(SettingID::RIGHT_TRIGGER_EFFECT, AdaptiveTriggerSetting{});
JSMVariable<int> left_trigger_offset = JSMVariable<int>(25);
JSMVariable<int> left_trigger_range = JSMVariable<int>(150);
JSMVariable<int> right_trigger_offset = JSMVariable<int>(25);
JSMVariable<int> right_trigger_range = JSMVariable<int>(150);
JSMVariable<Switch> auto_calibrate_gyro = JSMVariable<Switch>(Switch::OFF);
JSMSetting<float> left_stick_undeadzone_inner = JSMSetting<float>(SettingID::LEFT_STICK_UNDEADZONE_INNER, 0.f);
JSMSetting<float> left_stick_undeadzone_outer = JSMSetting<float>(SettingID::LEFT_STICK_UNDEADZONE_OUTER, 0.f);
JSMSetting<float> left_stick_unpower = JSMSetting<float>(SettingID::LEFT_STICK_UNPOWER, 0.f);
JSMSetting<float> right_stick_undeadzone_inner = JSMSetting<float>(SettingID::RIGHT_STICK_UNDEADZONE_INNER, 0.f);
JSMSetting<float> right_stick_undeadzone_outer = JSMSetting<float>(SettingID::RIGHT_STICK_UNDEADZONE_OUTER, 0.f);
JSMSetting<float> right_stick_unpower = JSMSetting<float>(SettingID::RIGHT_STICK_UNPOWER, 0.f);
JSMSetting<float> left_stick_virtual_scale = JSMSetting<float>(SettingID::LEFT_STICK_VIRTUAL_SCALE, 1.f);
JSMSetting<float> right_stick_virtual_scale = JSMSetting<float>(SettingID::RIGHT_STICK_VIRTUAL_SCALE, 1.f);
JSMSetting<float> wind_stick_range = JSMSetting<float>(SettingID::WIND_STICK_RANGE, 900.f);
JSMSetting<float> wind_stick_power = JSMSetting<float>(SettingID::WIND_STICK_POWER, 1.f);
JSMSetting<float> unwind_rate = JSMSetting<float>(SettingID::UNWIND_RATE, 1800.f);
JSMSetting<GyroOutput> gyro_output = JSMSetting<GyroOutput>(SettingID::GYRO_OUTPUT, GyroOutput::MOUSE);
JSMSetting<GyroOutput> flick_stick_output = JSMSetting<GyroOutput>(SettingID::FLICK_STICK_OUTPUT, GyroOutput::MOUSE);

JSMVariable<PathString> currentWorkingDir = JSMVariable<PathString>(PathString());
vector<JSMButton> grid_mappings; // array of virtual buttons on the touchpad grid
vector<JSMButton> mappings;      // array enables use of for each loop and other i/f
mutex loading_lock;

float os_mouse_speed = 1.0;
float last_flick_and_rotation = 0.0;
unique_ptr<PollingThread> autoLoadThread;
unique_ptr<PollingThread> minimizeThread;
bool devicesCalibrating = false;
unordered_map<int, shared_ptr<JoyShock>> handle_to_joyshock;
int triggerCalibrationStep = 0;

class TouchStick
{
	int _index = -1;
	FloatXY _currentLocation = { 0.f, 0.f };
	bool _prevDown = false;

	bool is_flicking_touch = false;
	FloatXY touch_last_cal;
	float touch_stick_acceleration = 1.0;
	bool ignore_motion_stick = false;
	// Handle a single touch related action. On per touch point
public:
	map<ButtonID, DigitalButton> buttons; // Each touchstick gets it's own digital buttons. Is that smart?

	TouchStick(int index, shared_ptr<DigitalButton::Context> common, int handle)
	  : _index(index)
	{
		buttons.emplace(ButtonID::TUP, DigitalButton(common, mappings[int(ButtonID::TUP)]));
		buttons.emplace(ButtonID::TDOWN, DigitalButton(common, mappings[int(ButtonID::TDOWN)]));
		buttons.emplace(ButtonID::TLEFT, DigitalButton(common, mappings[int(ButtonID::TLEFT)]));
		buttons.emplace(ButtonID::TRIGHT, DigitalButton(common, mappings[int(ButtonID::TRIGHT)]));
		buttons.emplace(ButtonID::TRING, DigitalButton(common, mappings[int(ButtonID::TRING)]));
	}

	void handleTouchStickChange(shared_ptr<JoyShock> js, bool down, short movX, short movY, float delta_time);

	inline bool wasDown()
	{
		return _prevDown;
	}
};

KeyCode::KeyCode()
  : code()
  , name()
{
}

KeyCode::KeyCode(in_string keyName)
  : code(nameToKey(keyName))
  , name()
{
	if (code == COMMAND_ACTION)
		name = keyName.substr(1, keyName.size() - 2); // Remove opening and closing quotation marks
	else if (keyName.compare("SMALL_RUMBLE") == 0)
	{
		name = SMALL_RUMBLE;
		code = RUMBLE;
	}
	else if (keyName.compare("BIG_RUMBLE") == 0)
	{
		name = BIG_RUMBLE;
		code = RUMBLE;
	}
	else if (code != 0)
		name = keyName;
}

class ScrollAxis
{
protected:
	float _leftovers;
	DigitalButton *_negativeButton;
	DigitalButton *_positiveButton;
	int _touchpadId;
	ButtonID _pressedBtn;

public:
	static function<void(JoyShock *, ButtonID, int, bool)> _handleButtonChange;

	ScrollAxis()
	  : _leftovers(0.f)
	  , _negativeButton(nullptr)
	  , _positiveButton(nullptr)
	  , _touchpadId(-1)
	  , _pressedBtn(ButtonID::NONE)
	{
	}

	void init(DigitalButton &negativeBtn, DigitalButton &positiveBtn, int touchpadId = -1)
	{
		_negativeButton = &negativeBtn;
		_positiveButton = &positiveBtn;
		_touchpadId = touchpadId;
	}

	void ProcessScroll(float distance, float sens, chrono::steady_clock::time_point now)
	{
		if (!_negativeButton || !_positiveButton)
			return; // not initalized!

		_leftovers += distance;
		if (distance != 0)
			DEBUG_LOG << " leftover is now " << _leftovers << endl;
		//"[" << _negativeId << "," << _positiveId << "] moved " << distance << " so that

		Pressed isPressed;
		isPressed.time_now = now;
		isPressed.turboTime = 50;
		isPressed.holdTime = 150;
		Released isReleased;
		isReleased.time_now = now;
		isReleased.turboTime = 50;
		isReleased.holdTime = 150;
		if (_pressedBtn != ButtonID::NONE)
		{
			float pressedTime = 0;
			if (_pressedBtn == _negativeButton->_id)
			{
				GetDuration dur{ now };
				pressedTime = _negativeButton->sendEvent(dur).out_duration;
				if (pressedTime < MAGIC_TAP_DURATION)
				{
					_negativeButton->sendEvent(isPressed);
					_positiveButton->sendEvent(isReleased);
					return;
				}
			}
			else // _pressedBtn == _positiveButton->_id
			{
				GetDuration dur{ now };
				pressedTime = _positiveButton->sendEvent(dur).out_duration;
				if (pressedTime < MAGIC_TAP_DURATION)
				{
					_negativeButton->sendEvent(isReleased);
					_positiveButton->sendEvent(isPressed);
					return;
				}
			}
			// pressed time > TAP_DURATION meaning release the tap
			_negativeButton->sendEvent(isReleased);
			_positiveButton->sendEvent(isReleased);
			_pressedBtn = ButtonID::NONE;
		}
		else if (fabsf(_leftovers) > sens)
		{
			if (_leftovers > 0)
			{
				_negativeButton->sendEvent(isPressed);
				_positiveButton->sendEvent(isReleased);
				_pressedBtn = _negativeButton->_id;
			}
			else
			{
				_negativeButton->sendEvent(isReleased);
				_positiveButton->sendEvent(isPressed);
				_pressedBtn = _positiveButton->_id;
			}
			_leftovers = _leftovers > 0 ? _leftovers - sens : _leftovers + sens;
		}
		// else do nothing and accumulate leftovers
	}

	void Reset(chrono::steady_clock::time_point now)
	{
		_leftovers = 0;
		Released isReleased;
		isReleased.time_now = now;
		isReleased.turboTime = 50;
		isReleased.holdTime = 150;
		_negativeButton->sendEvent(isReleased);
		_positiveButton->sendEvent(isReleased);
		_pressedBtn = ButtonID::NONE;
	}
};

struct TOUCH_POINT
{
	float posX = -1.f;
	float posY = -1.f;
	short movX = 0;
	short movY = 0;
	inline bool isDown()
	{
		return posX >= 0.f && posX <= 1.f && posY >= 0.f && posY <= 1.f;
	}
};

// An instance of this class represents a single controller device that JSM is listening to.
class JoyShock
{
private:
	float _flickSamples[256];
	int _frontSample = 0;

	FloatXY _gyroSamples[256];
	int _frontGyroSample = 0;

	template<typename E1, typename E2>
	static inline optional<E1> GetOptionalSetting(const JSMSetting<E2> &setting, ButtonID chord)
	{
		return setting.get(chord) ? optional<E1>(static_cast<E1>(*setting.get(chord))) : nullopt;
	}

public:
	const int MaxGyroSamples = 256;
	const int NumSamples = 256;
	int handle;
	shared_ptr<MotionIf> motion;
	int platform_controller_type;

	float gyroXVelocity = 0.f;
	float gyroYVelocity = 0.f;

	Vec lastGrav = Vec(0.f, -1.f, 0.f);

	float windingAngleLeft = 0.f;
	float windingAngleRight = 0.f;

	vector<DigitalButton> buttons;
	vector<DigitalButton> gridButtons;
	vector<TouchStick> touchpads;
	chrono::steady_clock::time_point started_flick;
	chrono::steady_clock::time_point time_now;
	bool is_flicking_left = false;
	bool is_flicking_right = false;
	bool is_flicking_motion = false;
	float delta_flick = 0.0;
	float flick_percent_done = 0.0;
	float flick_rotation_counter = 0.0;
	FloatXY left_last_cal;
	FloatXY right_last_cal;
	FloatXY motion_last_cal;
	ScrollAxis left_scroll;
	ScrollAxis right_scroll;
	//ScrollAxis motion_scroll_x;
	//ScrollAxis motion_scroll_y;
	ScrollAxis touch_scroll_x;
	ScrollAxis touch_scroll_y;
	TOUCH_STATE prevTouchState;

	int controller_split_type = 0;

	float left_acceleration = 1.0;
	float right_acceleration = 1.0;
	float motion_stick_acceleration = 1.0;
	vector<DstState> triggerState; // State of analog triggers when skip mode is active
	vector<deque<float>> prevTriggerPosition;
	shared_ptr<DigitalButton::Context> _context;

	// Modeshifting the stick mode can create quirky behaviours on transition. These flags
	// will be set upon returning to standard mode and ignore stick inputs until the stick
	// returns to neutral
	bool ignore_left_stick_mode = false;
	bool ignore_right_stick_mode = false;
	bool ignore_motion_stick_mode = false;

	bool processed_gyro_stick = false;

	float lastMotionStickX = 0.0f;
	float lastMotionStickY = 0.0f;

	float lastLX = 0.f;
	float lastLY = 0.f;
	float lastRX = 0.f;
	float lastRY = 0.f;

	float neutralQuatW = 1.0f;
	float neutralQuatX = 0.0f;
	float neutralQuatY = 0.0f;
	float neutralQuatZ = 0.0f;

	bool set_neutral_quat = false;

	int numLastGyroSamples = 100;
	float lastGyroX[100] = { 0.f };
	float lastGyroY[100] = { 0.f };
	float lastGyroAbsX = 0.f;
	float lastGyroAbsY = 0.f;
	int lastGyroIndexX = 0;
	int lastGyroIndexY = 0;

	Color _light_bar;
	AdaptiveTriggerSetting left_effect;
	AdaptiveTriggerSetting right_effect;
	AdaptiveTriggerSetting unused_effect;

	JoyShock(int uniqueHandle, int controllerSplitType, shared_ptr<DigitalButton::Context> sharedButtonCommon = nullptr)
	  : handle(uniqueHandle)
	  , controller_split_type(controllerSplitType)
	  , platform_controller_type(jsl->GetControllerType(uniqueHandle))
	  , triggerState(NUM_ANALOG_TRIGGERS, DstState::NoPress)
	  , prevTriggerPosition(NUM_ANALOG_TRIGGERS, deque<float>(MAGIC_TRIGGER_SMOOTHING, 0.f))
	  , _light_bar(*light_bar.get())
	  , _context(sharedButtonCommon)
	  , motion(MotionIf::getNew())
	{
		if (!sharedButtonCommon)
		{
			_context = shared_ptr<DigitalButton::Context>(new DigitalButton::Context(
			  bind(&JoyShock::handleViGEmNotification, this, placeholders::_1, placeholders::_2, placeholders::_3), motion));
		}
		_light_bar = getSetting<Color>(SettingID::LIGHT_BAR);

		_context->_getMatchingSimBtn = bind(&JoyShock::GetMatchingSimBtn, this, placeholders::_1);
		_context->_rumble = bind(&JoyShock::Rumble, this, placeholders::_1, placeholders::_2);

		buttons.reserve(LAST_ANALOG_TRIGGER); // Don't include touch stick buttons
		for (int i = 0; i <= LAST_ANALOG_TRIGGER; ++i)
		{
			buttons.push_back(DigitalButton(_context, mappings[i]));
		}
		right_scroll.init(buttons[int(ButtonID::RLEFT)], buttons[int(ButtonID::RRIGHT)]);
		left_scroll.init(buttons[int(ButtonID::LLEFT)], buttons[int(ButtonID::LRIGHT)]);
		ResetSmoothSample();
		if (!CheckVigemState())
		{
			virtual_controller = ControllerScheme::NONE;
		}
		jsl->SetLightColour(handle, getSetting<Color>(SettingID::LIGHT_BAR).raw);
		for (int i = 0; i < MAX_NO_OF_TOUCH; ++i)
		{
			touchpads.push_back(TouchStick(i, _context, handle));
		}
		touch_scroll_x.init(touchpads[0].buttons.find(ButtonID::TLEFT)->second, touchpads[0].buttons.find(ButtonID::TRIGHT)->second);
		touch_scroll_y.init(touchpads[0].buttons.find(ButtonID::TUP)->second, touchpads[0].buttons.find(ButtonID::TDOWN)->second);
		updateGridSize();
		prevTouchState.t0Down = false;
		prevTouchState.t1Down = false;
	}

	~JoyShock()
	{
		if (controller_split_type == JS_SPLIT_TYPE_LEFT)
		{
			_context->leftMotion = nullptr;
		}
		else
		{
			_context->rightMainMotion = nullptr;
		}
	}

	void Rumble(int smallRumble, int bigRumble)
	{
		if (getSetting<Switch>(SettingID::RUMBLE) == Switch::ON)
		{
			// DEBUG_LOG << "Rumbling at " << smallRumble << " and " << bigRumble << endl;
			jsl->SetRumble(handle, smallRumble, bigRumble);
		}
	}

	bool CheckVigemState()
	{
		if (virtual_controller.get() != ControllerScheme::NONE)
		{
			string error = "There is no controller object";
			if (!_context->_vigemController || _context->_vigemController->isInitialized(&error) == false)
			{
				CERR << "[ViGEm Client] " << error << endl;
				return false;
			}
			else if (_context->_vigemController->getType() != virtual_controller.get())
			{
				CERR << "[ViGEm Client] The controller is of the wrong type!" << endl;
				return false;
			}
		}
		return true;
	}

	void handleViGEmNotification(UCHAR largeMotor, UCHAR smallMotor, Indicator indicator)
	{
		//static chrono::steady_clock::time_point last_call;
		//auto now = chrono::steady_clock::now();
		//auto diff = ((float)chrono::duration_cast<chrono::microseconds>(now - last_call).count()) / 1000000.0f;
		//last_call = now;
		//COUT_INFO << "Time since last vigem rumble is " << diff << " us" << endl;
		lock_guard guard(this->_context->callback_lock);
		switch (platform_controller_type)
		{
		case JS_TYPE_DS4:
		case JS_TYPE_DS:
			jsl->SetLightColour(handle, _light_bar.raw);
			break;
		default:
			jsl->SetPlayerNumber(handle, indicator.led);
			break;
		}
		Rumble(smallMotor << 8, largeMotor << 8);
	}

	template<typename E>
	E getSetting(SettingID index)
	{
		static_assert(is_enum<E>::value, "Parameter of JoyShock::getSetting<E> has to be an enum type");
		// Look at active chord mappings starting with the latest activates chord
		for (auto activeChord = _context->chordStack.begin(); activeChord != _context->chordStack.end(); activeChord++)
		{
			optional<E> opt;
			switch (index)
			{
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
			case SettingID::MOTION_STICK_MODE:
				opt = GetOptionalSetting<E>(motion_stick_mode, *activeChord);
				if (ignore_motion_stick_mode && *activeChord == ButtonID::NONE)
					opt = optional<E>(static_cast<E>(StickMode::INVALID));
				else
					ignore_motion_stick_mode |= (opt && *activeChord != ButtonID::NONE);
				break;
			case SettingID::LEFT_RING_MODE:
				opt = GetOptionalSetting<E>(left_ring_mode, *activeChord);
				break;
			case SettingID::RIGHT_RING_MODE:
				opt = GetOptionalSetting<E>(right_ring_mode, *activeChord);
				break;
			case SettingID::MOTION_RING_MODE:
				opt = GetOptionalSetting<E>(motion_ring_mode, *activeChord);
				break;
			case SettingID::JOYCON_GYRO_MASK:
				opt = GetOptionalSetting<E>(joycon_gyro_mask, *activeChord);
				break;
			case SettingID::JOYCON_MOTION_MASK:
				opt = GetOptionalSetting<E>(joycon_motion_mask, *activeChord);
				break;
			case SettingID::CONTROLLER_ORIENTATION:
				opt = GetOptionalSetting<E>(controller_orientation, *activeChord);
				if (opt && int(*opt) == int(ControllerOrientation::JOYCON_SIDEWAYS))
				{
					if (controller_split_type == JS_SPLIT_TYPE_LEFT)
					{
						opt = optional<E>(static_cast<E>(ControllerOrientation::LEFT));
					}
					else if (controller_split_type == JS_SPLIT_TYPE_RIGHT)
					{
						opt = optional<E>(static_cast<E>(ControllerOrientation::RIGHT));
					}
					else
					{
						opt = optional<E>(static_cast<E>(ControllerOrientation::FORWARD));
					}
				}
				break;
			case SettingID::GYRO_SPACE:
				opt = GetOptionalSetting<E>(gyro_space, *activeChord);
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
			case SettingID::TOUCHPAD_MODE:
				opt = GetOptionalSetting<E>(touchpad_mode, *activeChord);
				break;
			case SettingID::TOUCH_STICK_MODE:
				opt = GetOptionalSetting<E>(touch_stick_mode, *activeChord);
				break;
			case SettingID::TOUCH_RING_MODE:
				opt = GetOptionalSetting<E>(touch_stick_mode, *activeChord);
				break;
			case SettingID::TOUCHPAD_DUAL_STAGE_MODE:
				opt = GetOptionalSetting<E>(touch_ds_mode, *activeChord);
				break;
			case SettingID::RUMBLE:
				opt = GetOptionalSetting<E>(rumble_enable, *activeChord);
				break;
			case SettingID::ADAPTIVE_TRIGGER:
				opt = GetOptionalSetting<E>(adaptive_trigger, *activeChord);
				break;
			case SettingID::GYRO_OUTPUT:
				opt = GetOptionalSetting<E>(gyro_output, *activeChord);
				break;
			case SettingID::FLICK_STICK_OUTPUT:
				opt = GetOptionalSetting<E>(flick_stick_output, *activeChord);
				break;
			}
			if (opt)
				return *opt;
		}
		stringstream ss;
		ss << "Index " << index << " is not a valid enum setting";
		throw invalid_argument(ss.str().c_str());
	}

	float getSetting(SettingID index)
	{
		// Look at active chord mappings starting with the latest activates chord
		for (auto activeChord = _context->chordStack.begin(); activeChord != _context->chordStack.end(); activeChord++)
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
			case SettingID::REAL_WORLD_CALIBRATION:
				opt = real_world_calibration.get(*activeChord);
				break;
			case SettingID::VIRTUAL_STICK_CALIBRATION:
				opt = virtual_stick_calibration.get(*activeChord);
				break;
			case SettingID::IN_GAME_SENS:
				opt = in_game_sens.get(*activeChord);
				break;
			case SettingID::TRIGGER_THRESHOLD:
				opt = trigger_threshold.get(*activeChord);
				if (opt && platform_controller_type == JS_TYPE_DS && getSetting<Switch>(SettingID::ADAPTIVE_TRIGGER) == Switch::ON)
					opt = optional(max(0.f, *opt)); // hair trigger disabled on dual sense when adaptive triggers are active
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
			case SettingID::FLICK_TIME_EXPONENT:
				opt = flick_time_exponent.get(*activeChord);
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
			case SettingID::LEFT_STICK_DEADZONE_INNER:
				opt = left_stick_deadzone_inner.get(*activeChord);
				break;
			case SettingID::LEFT_STICK_DEADZONE_OUTER:
				opt = left_stick_deadzone_outer.get(*activeChord);
				break;
			case SettingID::RIGHT_STICK_DEADZONE_INNER:
				opt = right_stick_deadzone_inner.get(*activeChord);
				break;
			case SettingID::RIGHT_STICK_DEADZONE_OUTER:
				opt = right_stick_deadzone_outer.get(*activeChord);
				break;
			case SettingID::MOTION_DEADZONE_INNER:
				opt = motion_deadzone_inner.get(*activeChord);
				break;
			case SettingID::MOTION_DEADZONE_OUTER:
				opt = motion_deadzone_outer.get(*activeChord);
				break;
			case SettingID::ANGLE_TO_AXIS_DEADZONE_INNER:
				opt = angle_to_axis_deadzone_inner.get(*activeChord);
				break;
			case SettingID::ANGLE_TO_AXIS_DEADZONE_OUTER:
				opt = angle_to_axis_deadzone_outer.get(*activeChord);
				break;
			case SettingID::LEAN_THRESHOLD:
				opt = lean_threshold.get(*activeChord);
				break;
			case SettingID::FLICK_DEADZONE_ANGLE:
				opt = flick_deadzone_angle.get(*activeChord);
				break;
			case SettingID::TRACKBALL_DECAY:
				opt = trackball_decay.get(*activeChord);
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
			case SettingID::TRIGGER_SKIP_DELAY:
				opt = trigger_skip_delay.get(*activeChord);
				break;
			case SettingID::TURBO_PERIOD:
				opt = turbo_period.get(*activeChord);
				break;
			case SettingID::HOLD_PRESS_TIME:
				opt = hold_press_time.get(*activeChord);
				break;
			case SettingID::TOUCH_STICK_RADIUS:
				opt = touch_stick_radius.get(*activeChord);
				break;
			case SettingID::TOUCH_DEADZONE_INNER:
				opt = touch_deadzone_inner.get(*activeChord);
				break;
			case SettingID::DBL_PRESS_WINDOW:
				opt = dbl_press_window.get(*activeChord);
				break;
				// SIM_PRESS_WINDOW are not chorded, they can be accessed as is.
			case SettingID::LEFT_STICK_UNDEADZONE_INNER:
				opt = left_stick_undeadzone_inner.get(*activeChord);
				break;
			case SettingID::LEFT_STICK_UNDEADZONE_OUTER:
				opt = left_stick_undeadzone_outer.get(*activeChord);
				break;
			case SettingID::LEFT_STICK_UNPOWER:
				opt = left_stick_unpower.get(*activeChord);
				break;
			case SettingID::RIGHT_STICK_UNDEADZONE_INNER:
				opt = right_stick_undeadzone_inner.get(*activeChord);
				break;
			case SettingID::RIGHT_STICK_UNDEADZONE_OUTER:
				opt = right_stick_undeadzone_outer.get(*activeChord);
				break;
			case SettingID::RIGHT_STICK_UNPOWER:
				opt = right_stick_unpower.get(*activeChord);
				break;
			case SettingID::LEFT_STICK_VIRTUAL_SCALE:
				opt = left_stick_virtual_scale.get(*activeChord);
				break;
			case SettingID::RIGHT_STICK_VIRTUAL_SCALE:
				opt = right_stick_virtual_scale.get(*activeChord);
				break;
			case SettingID::WIND_STICK_RANGE:
				opt = wind_stick_range.get(*activeChord);
				break;
			case SettingID::WIND_STICK_POWER:
				opt = wind_stick_power.get(*activeChord);
				break;
			case SettingID::UNWIND_RATE:
				opt = unwind_rate.get(*activeChord);
				break;
			}
			if (opt)
				return *opt;
		}

		std::stringstream message;
		message << "Index " << index << " is not a valid float setting";
		throw std::out_of_range(message.str().c_str());
	}

	template<>
	FloatXY getSetting<FloatXY>(SettingID index)
	{
		// Look at active chord mappings starting with the latest activates chord
		for (auto activeChord = _context->chordStack.begin(); activeChord != _context->chordStack.end(); activeChord++)
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
			case SettingID::STICK_SENS:
				opt = stick_sens.get(*activeChord);
				break;
			case SettingID::TOUCHPAD_SENS:
				opt = touchpad_sens.get(*activeChord);
				break;
			case SettingID::SCROLL_SENS:
				opt = scroll_sens.get(*activeChord);
				break;
			}
			if (opt)
				return *opt;
		} // Check next Chord

		stringstream ss;
		ss << "Index " << index << " is not a valid FloatXY setting";
		throw invalid_argument(ss.str().c_str());
	}

	template<>
	GyroSettings getSetting<GyroSettings>(SettingID index)
	{
		if (index == SettingID::GYRO_ON || index == SettingID::GYRO_OFF)
		{
			// Look at active chord mappings starting with the latest activates chord
			for (auto activeChord = _context->chordStack.begin(); activeChord != _context->chordStack.end(); activeChord++)
			{
				auto opt = gyro_settings.get(*activeChord);
				if (opt)
					return *opt;
			}
		}
		stringstream ss;
		ss << "Index " << index << " is not a valid GyroSetting";
		throw invalid_argument(ss.str().c_str());
	}

	template<>
	Color getSetting<Color>(SettingID index)
	{
		if (index == SettingID::LIGHT_BAR)
		{
			// Look at active chord mappings starting with the latest activates chord
			for (auto activeChord = _context->chordStack.begin(); activeChord != _context->chordStack.end(); activeChord++)
			{
				auto opt = light_bar.get(*activeChord);
				if (opt)
					return *opt;
			}
		}
		stringstream ss;
		ss << "Index " << index << " is not a valid Color";
		throw invalid_argument(ss.str().c_str());
	}

	template<>
	AdaptiveTriggerSetting getSetting<AdaptiveTriggerSetting>(SettingID index)
	{
		// Look at active chord mappings starting with the latest activates chord
		for (auto activeChord = _context->chordStack.begin(); activeChord != _context->chordStack.end(); activeChord++)
		{
			optional<AdaptiveTriggerSetting> opt;
			switch (index)
			{
			case SettingID::LEFT_TRIGGER_EFFECT:
				opt = left_trigger_effect.get(*activeChord);
				if (opt)
					return *opt;
			case SettingID::RIGHT_TRIGGER_EFFECT:
				opt = right_trigger_effect.get(*activeChord);
				if (opt)
					return *opt;
			}
		}
		stringstream ss;
		ss << "Index " << index << " is not a valid AdaptiveTriggerSetting";
		throw invalid_argument(ss.str().c_str());
	}

	template<>
	AxisSignPair getSetting<AxisSignPair>(SettingID index)
	{
		// Look at active chord mappings starting with the latest activates chord
		for (auto activeChord = _context->chordStack.begin(); activeChord != _context->chordStack.end(); activeChord++)
		{
			optional<AxisSignPair> opt;
			switch (index)
			{
			case SettingID::LEFT_STICK_AXIS:
				opt = left_stick_axis.get(*activeChord);
				break;
			case SettingID::RIGHT_STICK_AXIS:
				opt = right_stick_axis.get(*activeChord);
				break;
			case SettingID::MOTION_STICK_AXIS:
				opt = motion_stick_axis.get(*activeChord);
				break;
			case SettingID::TOUCH_STICK_AXIS:
				opt = touch_stick_axis.get(*activeChord);
				break;
			}
			if (opt)
				return *opt;
		} // Check next Chord

		stringstream ss;
		ss << "Index " << index << " is not a valid AxisSignPair setting";
		throw invalid_argument(ss.str().c_str());
	}

public:
	DigitalButton *GetMatchingSimBtn(ButtonID index)
	{
		// Find the simMapping where the other btn is in the same state as this btn.
		// POTENTIAL FLAW: The mapping you find may not necessarily be the one that got you in a
		// Simultaneous state in the first place if there is a second SimPress going on where one
		// of the buttons has a third SimMap with this one. I don't know if it's worth solving though...
		for (int id = 0; id < mappings.size(); ++id)
		{
			auto simMap = mappings[int(index)].getSimMap(ButtonID(id));
			if (simMap && index != simMap->first && buttons[int(simMap->first)].getState() == buttons[int(index)].getState())
			{
				return &buttons[int(simMap->first)];
			}
		}
		return nullptr;
	}

	void ResetSmoothSample()
	{
		_frontSample = 0;
		for (int i = 0; i < NumSamples; i++)
		{
			_flickSamples[i] = 0.0;
		}
	}

	float GetSmoothedStickRotation(float value, float bottomThreshold, float topThreshold, int maxSamples)
	{
		// which sample in the circular smoothing buffer do we want to write over?
		_frontSample--;
		if (_frontSample < 0)
			_frontSample = NumSamples - 1;
		// if this input is bigger than the top threshold, it'll all be consumed immediately; 0 gets put into the smoothing buffer. If it's below the bottomThreshold, it'll all be put in the smoothing buffer
		float length = abs(value);
		float immediateFactor;
		if (topThreshold <= bottomThreshold)
		{
			immediateFactor = 1.0f;
		}
		else
		{
			immediateFactor = (length - bottomThreshold) / (topThreshold - bottomThreshold);
		}
		// clamp to [0, 1] range
		if (immediateFactor < 0.0f)
		{
			immediateFactor = 0.0f;
		}
		else if (immediateFactor > 1.0f)
		{
			immediateFactor = 1.0f;
		}
		float smoothFactor = 1.0f - immediateFactor;
		// now we can push the smooth sample (or as much of it as we want smoothed)
		float frontSample = _flickSamples[_frontSample] = value * smoothFactor;
		// and now calculate smoothed result
		float result = frontSample / maxSamples;
		for (int i = 1; i < maxSamples; i++)
		{
			int rotatedIndex = (_frontSample + i) % NumSamples;
			frontSample = _flickSamples[rotatedIndex];
			result += frontSample / maxSamples;
		}
		// finally, add immediate portion
		return result + value * immediateFactor;
	}

	void GetSmoothedGyro(float x, float y, float length, float bottomThreshold, float topThreshold, int maxSamples, float &outX, float &outY)
	{
		// this is basically the same as we use for smoothing flick-stick rotations, but because this deals in vectors, it's a slightly different function. Not worth abstracting until it'll be used in more ways
		// which item in the circular smoothing buffer will we write over?
		_frontGyroSample--;
		if (_frontGyroSample < 0)
			_frontGyroSample = MaxGyroSamples - 1;
		float immediateFactor;
		if (topThreshold <= bottomThreshold)
		{
			immediateFactor = length < bottomThreshold ? 0.0f : 1.0f;
		}
		else
		{
			immediateFactor = (length - bottomThreshold) / (topThreshold - bottomThreshold);
		}
		// clamp to [0, 1] range
		if (immediateFactor < 0.0f)
		{
			immediateFactor = 0.0f;
		}
		else if (immediateFactor > 1.0f)
		{
			immediateFactor = 1.0f;
		}
		float smoothFactor = 1.0f - immediateFactor;
		// now we can push the smooth sample (or as much of it as we want smoothed)
		FloatXY frontSample = _gyroSamples[_frontGyroSample] = { x * smoothFactor, y * smoothFactor };
		// and now calculate smoothed result
		float xResult = frontSample.x() / maxSamples;
		float yResult = frontSample.y() / maxSamples;
		for (int i = 1; i < maxSamples; i++)
		{
			int rotatedIndex = (_frontGyroSample + i) % MaxGyroSamples;
			frontSample = _gyroSamples[rotatedIndex];
			xResult += frontSample.x() / maxSamples;
			yResult += frontSample.y() / maxSamples;
		}
		// finally, add immediate portion
		outX = xResult + x * immediateFactor;
		outY = yResult + y * immediateFactor;
	}

private:
	bool isSoftPullPressed(int triggerIndex, float triggerPosition)
	{
		float threshold = getSetting(SettingID::TRIGGER_THRESHOLD);
		if (platform_controller_type == JS_TYPE_DS && getSetting<Switch>(SettingID::ADAPTIVE_TRIGGER) != Switch::OFF)
			threshold = max(0.f, threshold); // hair trigger disabled on dual sense when adaptive triggers are active
		if (threshold >= 0)
		{
			return triggerPosition > threshold;
		}
		// else HAIR TRIGGER

		// Calculate 3 sample averages with the last MAGIC_TRIGGER_SMOOTHING samples + new sample
		float sum = 0.f;
		for_each(prevTriggerPosition[triggerIndex].begin(), prevTriggerPosition[triggerIndex].begin() + 3, [&sum](auto data) { sum += data; });
		float avg_tm3 = sum / 3.0f;
		sum = sum - *(prevTriggerPosition[triggerIndex].begin()) + *(prevTriggerPosition[triggerIndex].end() - 2);
		float avg_tm2 = sum / 3.0f;
		sum = sum - *(prevTriggerPosition[triggerIndex].begin() + 1) + *(prevTriggerPosition[triggerIndex].end() - 1);
		float avg_tm1 = sum / 3.0f;
		sum = sum - *(prevTriggerPosition[triggerIndex].begin() + 2) + triggerPosition;
		float avg_t0 = sum / 3.0f;
		//if (avg_t0 > 0) COUT << "Trigger: " << avg_t0 << endl;

		// Soft press is pressed if we got three averaged samples in a row that are pressed
		bool isPressed;
		if (avg_t0 > avg_tm1 && avg_tm1 > avg_tm2 && avg_tm2 > avg_tm3)
		{
			//DEBUG_LOG << "Hair Trigger pressed: " << avg_t0 << " > " << avg_tm1 << " > " << avg_tm2 << " > " << avg_tm3 << endl;
			isPressed = true;
		}
		else if (avg_t0 < avg_tm1 && avg_tm1 < avg_tm2 && avg_tm2 < avg_tm3)
		{
			//DEBUG_LOG << "Hair Trigger released: " << avg_t0 << " < " << avg_tm1 << " < " << avg_tm2 << " < " << avg_tm3 << endl;
			isPressed = false;
		}
		else
		{
			isPressed = triggerState[triggerIndex] != DstState::NoPress && triggerState[triggerIndex] != DstState::QuickSoftTap;
		}
		prevTriggerPosition[triggerIndex].pop_front();
		prevTriggerPosition[triggerIndex].push_back(triggerPosition);
		return isPressed;
	}

public:
	void handleButtonChange(ButtonID id, bool pressed, int touchpadID = -1)
	{
		DigitalButton *button = int(id) <= LAST_ANALOG_TRIGGER ? &buttons[int(id)] :
		  touchpadID >= 0 && touchpadID < touchpads.size()     ? &touchpads[touchpadID].buttons.find(id)->second :
		  id >= ButtonID::T1                                   ? &gridButtons[int(id) - int(ButtonID::T1)] :
                                                                 nullptr;

		if (!button)
		{
			CERR << "Button " << id << " with tocuchpadId " << touchpadID << " could not be found" << endl;
			return;
		}
		else if ((!_context->nn && pressed) || (_context->nn > 0 && (id >= ButtonID::UP || id <= ButtonID::DOWN || id == ButtonID::S || id == ButtonID::E) && nnm.find(_context->nn) != nnm.end() && nnm.find(_context->nn)->second == id))
		{
			Pressed evt;
			evt.time_now = time_now;
			evt.turboTime = getSetting(SettingID::TURBO_PERIOD);
			evt.holdTime = getSetting(SettingID::HOLD_PRESS_TIME);
			evt.dblPressWindow = getSetting(SettingID::DBL_PRESS_WINDOW);
			button->sendEvent(evt);
		}
		else
		{
			Released evt;
			evt.time_now = time_now;
			evt.turboTime = getSetting(SettingID::TURBO_PERIOD);
			evt.holdTime = getSetting(SettingID::HOLD_PRESS_TIME);
			evt.dblPressWindow = getSetting(SettingID::DBL_PRESS_WINDOW);
			button->sendEvent(evt);
		}
	}

	float getTriggerEffectStartPos()
	{
		float threshold = getSetting(SettingID::TRIGGER_THRESHOLD);
		if (platform_controller_type == JS_TYPE_DS && getSetting<Switch>(SettingID::ADAPTIVE_TRIGGER) != Switch::OFF)
			threshold = max(0.f, threshold); // hair trigger disabled on dual sense when adaptive triggers are active
		return clamp(threshold + 0.05f, 0.0f, 1.0f);
	}

	void handleTriggerChange(ButtonID softIndex, ButtonID fullIndex, TriggerMode mode, float position, AdaptiveTriggerSetting &trigger_rumble)
	{
		uint8_t offset = softIndex == ButtonID::ZL ? left_trigger_offset : right_trigger_offset;
		uint8_t range = softIndex == ButtonID::ZL ? left_trigger_range : right_trigger_range;
		auto idxState = int(fullIndex) - FIRST_ANALOG_TRIGGER; // Get analog trigger index
		if (idxState < 0 || idxState >= (int)triggerState.size())
		{
			COUT << "Error: Trigger " << fullIndex << " does not exist in state map. Dual Stage Trigger not possible." << endl;
			return;
		}

		if (mode != TriggerMode::X_LT && mode != TriggerMode::X_RT && (platform_controller_type == JS_TYPE_PRO_CONTROLLER || platform_controller_type == JS_TYPE_JOYCON_LEFT || platform_controller_type == JS_TYPE_JOYCON_RIGHT))
		{
			// Override local variable because the controller has digital triggers. Effectively ignore Full Pull binding.
			mode = TriggerMode::NO_FULL;
		}

		if (mode == TriggerMode::X_LT)
		{
			if (_context->_vigemController)
				_context->_vigemController->setLeftTrigger(position);
			trigger_rumble.mode = AdaptiveTriggerMode::RESISTANCE_RAW;
			trigger_rumble.force = 0;
			trigger_rumble.start = offset + 0.05 * range;
			_context->updateChordStack(position > 0, softIndex);
			_context->updateChordStack(position >= 1.0, fullIndex);
			return;
		}
		else if (mode == TriggerMode::X_RT)
		{
			if (_context->_vigemController)
				_context->_vigemController->setRightTrigger(position);
			trigger_rumble.mode = AdaptiveTriggerMode::RESISTANCE_RAW;
			trigger_rumble.force = 0;
			trigger_rumble.start = offset + 0.05 * range;
			_context->updateChordStack(position > 0, softIndex);
			_context->updateChordStack(position >= 1.0, fullIndex);
			return;
		}

		// if either trigger is waiting to be tap released, give it a go
		if (buttons[int(softIndex)].getState() == BtnState::TapPress)
		{
			// keep triggering until the tap release is complete
			handleButtonChange(softIndex, false);
		}
		if (buttons[int(fullIndex)].getState() == BtnState::TapPress)
		{
			// keep triggering until the tap release is complete
			handleButtonChange(fullIndex, false);
		}

		switch (triggerState[idxState])
		{
		case DstState::NoPress:
			// It actually doesn't matter what the last Press is. Theoretically, we could have missed the edge.
			if (mode == TriggerMode::NO_FULL)
			{
				trigger_rumble.mode = AdaptiveTriggerMode::RESISTANCE_RAW;
				trigger_rumble.force = UINT16_MAX;
				trigger_rumble.start = offset + getTriggerEffectStartPos() * range;
			}
			else
			{
				trigger_rumble.mode = AdaptiveTriggerMode::SEGMENT;
				trigger_rumble.force = 0.1 * UINT16_MAX;
				trigger_rumble.start = offset + getTriggerEffectStartPos() * range;
				trigger_rumble.end = offset + min(1.f, getTriggerEffectStartPos() + 0.1f) * range;
			}
			if (isSoftPullPressed(idxState, position))
			{
				if (mode == TriggerMode::MAY_SKIP || mode == TriggerMode::MUST_SKIP)
				{
					// Start counting press time to see if soft binding should be skipped
					triggerState[idxState] = DstState::PressStart;
					buttons[int(softIndex)].sendEvent(time_now);
				}
				else if (mode == TriggerMode::MAY_SKIP_R || mode == TriggerMode::MUST_SKIP_R)
				{
					triggerState[idxState] = DstState::PressStartResp;
					buttons[int(softIndex)].sendEvent(time_now);
					handleButtonChange(softIndex, true);
				}
				else // mode == NO_FULL or NO_SKIP, NO_SKIP_EXCLUSIVE
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
			// don't change trigger rumble : keep whatever was set at no press
			if (!isSoftPullPressed(idxState, position))
			{
				// Trigger has been quickly tapped on the soft press
				triggerState[idxState] = DstState::QuickSoftTap;
				handleButtonChange(softIndex, true);
			}
			else if (position == 1.0)
			{
				// Trigger has been full pressed quickly
				triggerState[idxState] = DstState::QuickFullPress;
				handleButtonChange(fullIndex, true);
			}
			else
			{
				GetDuration dur{ time_now };
				if (buttons[int(softIndex)].sendEvent(dur).out_duration >= getSetting(SettingID::TRIGGER_SKIP_DELAY))
				{
					if (mode == TriggerMode::MUST_SKIP)
					{
						trigger_rumble.start = offset + (position + 0.05) * range;
					}
					triggerState[idxState] = DstState::SoftPress;
					// Reset the time for hold soft press purposes.
					buttons[int(softIndex)].sendEvent(time_now);
					handleButtonChange(softIndex, true);
				}
			}
			// Else, time passes as soft press is being held, waiting to see if the soft binding should be skipped
			break;
		case DstState::PressStartResp:
			// don't change trigger rumble : keep whatever was set at no press
			if (!isSoftPullPressed(idxState, position))
			{
				// Soft press is being released
				triggerState[idxState] = DstState::NoPress;
				handleButtonChange(softIndex, false);
			}
			else if (position == 1.0)
			{
				// Trigger has been full pressed quickly
				triggerState[idxState] = DstState::QuickFullPress;
				handleButtonChange(softIndex, false); // Remove soft press
				handleButtonChange(fullIndex, true);
			}
			else
			{
				GetDuration dur{ time_now };
				if (buttons[int(softIndex)].sendEvent(dur).out_duration >= getSetting(SettingID::TRIGGER_SKIP_DELAY))
				{
					if (mode == TriggerMode::MUST_SKIP_R)
					{
						trigger_rumble.start = offset + (position + 0.05) * range;
					}
					triggerState[idxState] = DstState::SoftPress;
				}
				handleButtonChange(softIndex, true);
			}
			break;
		case DstState::QuickSoftTap:
			// Soft trigger is already released. Send release now!
			// don't change trigger rumble : keep whatever was set at no press
			triggerState[idxState] = DstState::NoPress;
			handleButtonChange(softIndex, false);
			break;
		case DstState::QuickFullPress:
			trigger_rumble.mode = AdaptiveTriggerMode::SEGMENT;
			trigger_rumble.force = UINT16_MAX;
			trigger_rumble.start = offset + 0.89 * range;
			trigger_rumble.end = offset + 0.99 * range;
			if (position < 1.0f)
			{
				// Full press is being release
				triggerState[idxState] = DstState::QuickFullRelease;
				handleButtonChange(fullIndex, false);
			}
			else
			{
				// Full press is being held
				handleButtonChange(fullIndex, true);
			}
			break;
		case DstState::QuickFullRelease:
			trigger_rumble.mode = AdaptiveTriggerMode::SEGMENT;
			trigger_rumble.force = UINT16_MAX;
			trigger_rumble.start = offset + 0.89 * range;
			trigger_rumble.end = offset + 0.99 * range;
			if (!isSoftPullPressed(idxState, position))
			{
				triggerState[idxState] = DstState::NoPress;
			}
			else if (position == 1.0f)
			{
				// Trigger is being full pressed again
				triggerState[idxState] = DstState::QuickFullPress;
				handleButtonChange(fullIndex, true);
			}
			// else wait for the the trigger to be fully released
			break;
		case DstState::SoftPress:
			if (!isSoftPullPressed(idxState, position))
			{
				// Soft press is being released
				handleButtonChange(softIndex, false);
				triggerState[idxState] = DstState::NoPress;
			}
			else // Soft Press is being held
			{
				if (mode == TriggerMode::NO_SKIP || mode == TriggerMode::MAY_SKIP || mode == TriggerMode::MAY_SKIP_R)
				{
					trigger_rumble.force = min(int(UINT16_MAX), trigger_rumble.force + int(1 / 30.f * tick_time * UINT16_MAX));
					trigger_rumble.start = min(offset + 0.89 * range, trigger_rumble.start + 1 / 150. * tick_time * range);
					trigger_rumble.end = trigger_rumble.start + 0.1 * range;
					handleButtonChange(softIndex, true);
					if (position == 1.0)
					{
						// Full press is allowed in addition to soft press
						triggerState[idxState] = DstState::DelayFullPress;
						handleButtonChange(fullIndex, true);
					}
				}
				else if (mode == TriggerMode::NO_SKIP_EXCLUSIVE)
				{
					trigger_rumble.force = min(int(UINT16_MAX), trigger_rumble.force + int(1 / 30.f * tick_time * UINT16_MAX));
					trigger_rumble.start = min(offset + 0.89 * range, trigger_rumble.start + 1 / 150. * tick_time * range);
					trigger_rumble.end = trigger_rumble.start + 0.1 * range;
					handleButtonChange(softIndex, false);
					if (position == 1.0)
					{
						triggerState[idxState] = DstState::ExclFullPress;
						handleButtonChange(fullIndex, true);
					}
				}
				else // NO_FULL, MUST_SKIP and MUST_SKIP_R
				{
					trigger_rumble.mode = AdaptiveTriggerMode::RESISTANCE_RAW;
					trigger_rumble.force = min(int(UINT16_MAX), trigger_rumble.force + int(1 / 30.f * tick_time * UINT16_MAX));
					// keep old trigger_rumble.start
					handleButtonChange(softIndex, true);
				}
			}
			break;
		case DstState::DelayFullPress:
			trigger_rumble.mode = AdaptiveTriggerMode::SEGMENT;
			trigger_rumble.force = UINT16_MAX;
			trigger_rumble.start = offset + 0.8 * range;
			trigger_rumble.end = offset + 0.99 * range;
			if (position < 1.0)
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
		case DstState::ExclFullPress:
			trigger_rumble.mode = AdaptiveTriggerMode::SEGMENT;
			trigger_rumble.force = UINT16_MAX;
			trigger_rumble.start = offset + 0.89 * range;
			trigger_rumble.end = offset + 0.99 * range;
			if (position < 1.0f)
			{
				// Full press is being release
				triggerState[idxState] = DstState::SoftPress;
				handleButtonChange(fullIndex, false);
				handleButtonChange(softIndex, true);
			}
			else
			{
				// Full press is being held
				handleButtonChange(fullIndex, true);
			}
			break;
		default:
			CERR << "Trigger " << softIndex << " has invalid state " << triggerState[idxState] << ". Reset to NoPress." << endl;
			triggerState[idxState] = DstState::NoPress;
			break;
		}

		return;
	}

	bool IsPressed(ButtonID btn)
	{
		// Use chord stack to know if a button is pressed, because the state from the callback
		// only holds half the information when it comes to a joycon pair.
		// Also, NONE is always part of the stack (for chord handling) but NONE is never pressed.
		return btn != ButtonID::NONE && find(_context->chordStack.begin(), _context->chordStack.end(), btn) != _context->chordStack.end();
	}

	// return true if it hits the outer deadzone
	bool processDeadZones(float &x, float &y, float innerDeadzone, float outerDeadzone)
	{
		float length = sqrtf(x * x + y * y);
		if (length <= innerDeadzone)
		{
			x = 0.0f;
			y = 0.0f;
			return false;
		}
		if (length >= outerDeadzone)
		{
			// normalize
			x /= length;
			y /= length;
			return true;
		}
		if (length > innerDeadzone)
		{
			float scaledLength = (length - innerDeadzone) / (outerDeadzone - innerDeadzone);
			float rescale = scaledLength / length;
			x *= rescale;
			y *= rescale;
		}
		return false;
	}

	void updateGridSize()
	{
		while (gridButtons.size() > grid_mappings.size())
			gridButtons.pop_back();

		for (size_t i = gridButtons.size(); i < grid_mappings.size(); ++i)
		{
			JSMButton &map(grid_mappings[i]);
			gridButtons.push_back(DigitalButton(_context, map));
		}
	}
};

function<void(JoyShock *, ButtonID, int, bool)> ScrollAxis::_handleButtonChange = [](JoyShock *js, ButtonID id, int tid, bool pressed) {
	js->handleButtonChange(id, pressed, tid);
};

static void resetAllMappings()
{
	for_each(mappings.begin(), mappings.end(), [](auto &map) { map.Reset(); });
	// Question: Why is this a default mapping? Shouldn't it be empty? It's always possible to calibrate with RESET_GYRO_CALIBRATION
	min_gyro_sens.Reset();
	max_gyro_sens.Reset();
	min_gyro_threshold.Reset();
	max_gyro_threshold.Reset();
	stick_power.Reset();
	stick_sens.Reset();
	real_world_calibration.Reset();
	virtual_stick_calibration.Reset();
	in_game_sens.Reset();
	left_stick_mode.Reset();
	right_stick_mode.Reset();
	motion_stick_mode.Reset();
	left_ring_mode.Reset();
	right_ring_mode.Reset();
	motion_ring_mode.Reset();
	mouse_x_from_gyro.Reset();
	mouse_y_from_gyro.Reset();
	joycon_gyro_mask.Reset();
	joycon_motion_mask.Reset();
	zlMode.Reset();
	zrMode.Reset();
	trigger_threshold.Reset();
	gyro_settings.Reset();
	left_stick_axis.Reset();
	right_stick_axis.Reset();
	motion_stick_axis.Reset();
	touch_stick_axis.Reset();
    aim_y_sign.Reset();
	aim_x_sign.Reset();
	gyro_y_sign.Reset();
	gyro_x_sign.Reset();
	gyro_space.Reset();
	flick_time.Reset();
	flick_time_exponent.Reset();
	gyro_smooth_time.Reset();
	gyro_smooth_threshold.Reset();
	gyro_cutoff_speed.Reset();
	gyro_cutoff_recovery.Reset();
	stick_acceleration_rate.Reset();
	stick_acceleration_cap.Reset();
	left_stick_deadzone_inner.Reset();
	left_stick_deadzone_outer.Reset();
	flick_deadzone_angle.Reset();
	right_stick_deadzone_inner.Reset();
	right_stick_deadzone_outer.Reset();
	motion_deadzone_inner.Reset();
	motion_deadzone_outer.Reset();
	angle_to_axis_deadzone_inner.Reset();
	angle_to_axis_deadzone_outer.Reset();
	lean_threshold.Reset();
	controller_orientation.Reset();
	screen_resolution_x.Reset();
	screen_resolution_y.Reset();
	mouse_ring_radius.Reset();
	trackball_decay.Reset();
	rotate_smooth_override.Reset();
	flick_snap_strength.Reset();
	flick_snap_mode.Reset();
	trigger_skip_delay.Reset();
	turbo_period.Reset();
	hold_press_time.Reset();
	sim_press_window.Reset();
	dbl_press_window.Reset();
	grid_size.Reset();
	touchpad_mode.Reset();
	touch_stick_mode.Reset();
	touch_stick_radius.Reset();
	touch_deadzone_inner.Reset();
	touch_ring_mode.Reset();
	touchpad_sens.Reset();
	tick_time.Reset();
	light_bar.Reset();
	scroll_sens.Reset();
	rumble_enable.Reset();
	adaptive_trigger.Reset();
    left_trigger_effect.Reset();
    right_trigger_effect.Reset();
	left_trigger_offset.Reset();
	left_trigger_range.Reset();
	right_trigger_offset.Reset();
	right_trigger_range.Reset();
	touch_ds_mode.Reset();
	left_stick_undeadzone_inner.Reset();
	left_stick_undeadzone_outer.Reset();
	left_stick_unpower.Reset();
	right_stick_undeadzone_inner.Reset();
	right_stick_undeadzone_outer.Reset();
	right_stick_unpower.Reset();
	left_stick_virtual_scale.Reset();
	right_stick_virtual_scale.Reset();
	wind_stick_range.Reset();
	wind_stick_power.Reset();
	unwind_rate.Reset();
	gyro_output.Reset();
	flick_stick_output.Reset();
	for_each(grid_mappings.begin(), grid_mappings.end(), [](auto &map) { map.Reset(); });

	os_mouse_speed = 1.0f;
	last_flick_and_rotation = 0.0f;
}

void connectDevices(bool mergeJoycons = true)
{
	handle_to_joyshock.clear();
	int numConnected = jsl->ConnectDevices();
	vector<int> deviceHandles(numConnected, 0);
	if (numConnected > 0)
	{
		numConnected = jsl->GetConnectedDeviceHandles(&deviceHandles[0], numConnected);

		if (numConnected < deviceHandles.size())
		{
            deviceHandles.erase(std::remove(deviceHandles.begin(), deviceHandles.end(), -1), deviceHandles.end());
			//deviceHandles.resize(numConnected);
		}

		for (auto handle : deviceHandles) // Don't use foreach!
		{
			auto type = jsl->GetControllerSplitType(handle);
			auto otherJoyCon = find_if(handle_to_joyshock.begin(), handle_to_joyshock.end(),
			  [type](auto &pair) {
				  return type == JS_SPLIT_TYPE_LEFT && pair.second->controller_split_type == JS_SPLIT_TYPE_RIGHT ||
				    type == JS_SPLIT_TYPE_RIGHT && pair.second->controller_split_type == JS_SPLIT_TYPE_LEFT;
			  });
			shared_ptr<JoyShock> js = nullptr;
			if (mergeJoycons && otherJoyCon != handle_to_joyshock.end())
			{
				// The second JC points to the same common buttons as the other one.
				js.reset(new JoyShock(handle, type, otherJoyCon->second->_context));
			}
			else
			{
				js.reset(new JoyShock(handle, type));
			}
			handle_to_joyshock[handle] = js;
		}
	}

	if (numConnected == 1)
	{
		COUT << "1 device connected" << endl;
	}
	else if (numConnected == 0)
	{
		CERR << numConnected << " devices connected" << endl;
	}
	else
	{
		COUT << numConnected << " devices connected" << endl;
	}
	//if (!IsVisible())
	//{
	//	tray->SendNotification(wstring(msg.begin(), msg.end()));
	//}

	//if (numConnected != 0) {
	//	COUT << "All devices have started continuous gyro calibration" << endl;
	//}
}

void SimPressCrossUpdate(ButtonID sim, ButtonID origin, const Mapping &newVal)
{
	mappings[int(sim)].AtSimPress(origin)->operator=(newVal);
}

bool do_NO_GYRO_BUTTON()
{
	// TODO: handle chords
	gyro_settings.Reset();
	return true;
}

bool do_RESET_MAPPINGS(CmdRegistry *registry)
{
	COUT << "Resetting all mappings to defaults" << endl;
	resetAllMappings();
	if (registry)
	{
		if (!registry->loadConfigFile("OnReset.txt"))
		{
			COUT << "There is no ";
			COUT_INFO << "OnReset.txt";
			COUT << " file to load." << endl;
		}
	}
	return true;
}

bool do_RECONNECT_CONTROLLERS(in_string arguments)
{
	bool mergeJoycons = arguments.empty() || (arguments.compare("MERGE") == 0);
	if (mergeJoycons || arguments.rfind("SPLIT", 0) == 0)
	{
		COUT << "Reconnecting controllers: " << arguments << endl;
		jsl->DisconnectAndDisposeAll();
		connectDevices(mergeJoycons);
		jsl->SetCallback(&joyShockPollCallback);
		jsl->SetTouchCallback(&TouchCallback);
		return true;
	}
	return false;
}

bool do_COUNTER_OS_MOUSE_SPEED()
{
	COUT << "Countering OS mouse speed setting" << endl;
	os_mouse_speed = getMouseSpeed();
	return true;
}

bool do_IGNORE_OS_MOUSE_SPEED()
{
	COUT << "Ignoring OS mouse speed setting" << endl;
	os_mouse_speed = 1.0;
	return true;
}

void UpdateThread(PollingThread *thread, const Switch &newValue)
{
	if (thread)
	{
		if (newValue == Switch::ON)
		{
			thread->Start();
		}
		else if (newValue == Switch::OFF)
		{
			thread->Stop();
		}
	}
	else
	{
		COUT << "The thread does not exist" << endl;
	}
}

bool do_CALCULATE_REAL_WORLD_CALIBRATION(in_string argument)
{
	// first, check for a parameter
	float numRotations = 1.0;
	if (argument.length() > 0)
	{
		try
		{
			numRotations = stof(argument);
		}
		catch (invalid_argument ia)
		{
			COUT << "Can't convert \"" << argument << "\" to a number" << endl;
			return false;
		}
	}
	if (numRotations == 0)
	{
		COUT << "Can't calculate calibration from zero rotations" << endl;
	}
	else if (last_flick_and_rotation == 0)
	{
		COUT << "Need to use the flick stick at least once before calculating an appropriate calibration value" << endl;
	}
	else
	{
		COUT << "Recommendation: REAL_WORLD_CALIBRATION = " << setprecision(5) << (*real_world_calibration.get() * last_flick_and_rotation / numRotations) << endl;
	}
	return true;
}

bool do_FINISH_GYRO_CALIBRATION()
{
	COUT << "Finishing continuous calibration for all devices" << endl;
	for (auto iter = handle_to_joyshock.begin(); iter != handle_to_joyshock.end(); ++iter)
	{
		iter->second->motion->PauseContinuousCalibration();
	}
	devicesCalibrating = false;
	return true;
}

bool do_RESTART_GYRO_CALIBRATION()
{
	COUT << "Restarting continuous calibration for all devices" << endl;
	for (auto iter = handle_to_joyshock.begin(); iter != handle_to_joyshock.end(); ++iter)
	{
		iter->second->motion->ResetContinuousCalibration();
		iter->second->motion->StartContinuousCalibration();
	}
	devicesCalibrating = true;
	return true;
}

bool do_SET_MOTION_STICK_NEUTRAL()
{
	COUT << "Setting neutral motion stick orientation..." << endl;
	for (auto iter = handle_to_joyshock.begin(); iter != handle_to_joyshock.end(); ++iter)
	{
		iter->second->set_neutral_quat = true;
	}
	return true;
}

bool do_SLEEP(in_string argument)
{
	// first, check for a parameter
	float sleepTime = 1.0;
	if (argument.length() > 0)
	{
		try
		{
			sleepTime = stof(argument);
		}
		catch (invalid_argument ia)
		{
			COUT << "Can't convert \"" << argument << "\" to a number" << endl;
			return false;
		}
	}

	if (sleepTime <= 0)
	{
		COUT << "Sleep time must be greater than 0 and less than or equal to 10" << endl;
		return false;
	}

	if (sleepTime > 10)
	{
		COUT << "Sleep is capped at 10s per command" << endl;
		sleepTime = 10.f;
	}
	COUT << "Sleeping for " << setprecision(3) << sleepTime << " second(s)..." << endl;
	std::this_thread::sleep_for(std::chrono::milliseconds((int)(sleepTime * 1000)));
	COUT << "Finished sleeping." << endl;

	return true;
}

bool do_README()
{
	auto err = ShowOnlineHelp();
	if (err != 0)
	{
		COUT << "Could not open online help. Error #" << err << endl;
	}
	return true;
}

bool do_WHITELIST_SHOW()
{
	if (whitelister)
	{
		COUT << "Your PID is " << GetCurrentProcessId() << endl;
		whitelister->ShowConsole();
	}
	return true;
}

bool do_WHITELIST_ADD()
{
	string errMsg = "Whitelister is not implemented";
	if (whitelister && whitelister->Add(&errMsg))
	{
		COUT << "JoyShockMapper was successfully whitelisted" << endl;
	}
	else
	{
		CERR << "Whitelist operation failed: " << errMsg << endl;
	}
	return true;
}

bool do_WHITELIST_REMOVE()
{
	string errMsg = "Whitelister is not implemented";
	if (whitelister && whitelister->Remove(&errMsg))
	{
		COUT << "JoyShockMapper removed from whitelist" << endl;
	}
	else
	{
		CERR << "Whitelist operation failed: " << errMsg << endl;
	}
	return true;
}

bool processGyroStick(shared_ptr<JoyShock> jc, float stickX, float stickY, float stickLength, StickMode stickMode, bool forceOutput)
{
	GyroOutput gyroOutput = jc->getSetting<GyroOutput>(SettingID::GYRO_OUTPUT);
	bool isLeft = stickMode == StickMode::LEFT_STICK;
	bool gyroMatchesStickMode = (gyroOutput == GyroOutput::LEFT_STICK && stickMode == StickMode::LEFT_STICK) || (gyroOutput == GyroOutput::RIGHT_STICK && stickMode == StickMode::RIGHT_STICK) || stickMode == StickMode::INVALID;

	float undeadzoneInner, undeadzoneOuter, unpower, virtualScale;
	if (isLeft)
	{
		undeadzoneInner = jc->getSetting(SettingID::LEFT_STICK_UNDEADZONE_INNER);
		undeadzoneOuter = jc->getSetting(SettingID::LEFT_STICK_UNDEADZONE_OUTER);
		unpower = jc->getSetting(SettingID::LEFT_STICK_UNPOWER);
		virtualScale = jc->getSetting(SettingID::LEFT_STICK_VIRTUAL_SCALE);
	}
	else
	{
		undeadzoneInner = jc->getSetting(SettingID::RIGHT_STICK_UNDEADZONE_INNER);
		undeadzoneOuter = jc->getSetting(SettingID::RIGHT_STICK_UNDEADZONE_OUTER);
		unpower = jc->getSetting(SettingID::RIGHT_STICK_UNPOWER);
		virtualScale = jc->getSetting(SettingID::RIGHT_STICK_VIRTUAL_SCALE);
	}

	// in order to correctly combine gyro and stick, we need to calculate what the stick aiming is supposed to be doing, add gyro result to it, and convert back to stick
	float maxStickGameSpeed = jc->getSetting(SettingID::VIRTUAL_STICK_CALIBRATION);
	float livezoneSize = 1.f - undeadzoneOuter - undeadzoneInner;
	if (livezoneSize <= 0.f || maxStickGameSpeed <= 0.f)
	{
		// can't do anything with that
		jc->processed_gyro_stick |= gyroMatchesStickMode;
		return false;
	}
	if (unpower == 0.f)
		unpower = 1.f;
	float stickVelocity = pow(clamp<float>((stickLength - undeadzoneInner) / livezoneSize, 0.f, 1.f), unpower) * maxStickGameSpeed * virtualScale;
	float expectedX = 0.f;
	float expectedY = 0.f;
	if (stickVelocity > 0.f)
	{
		expectedX = stickX / stickLength * stickVelocity;
		expectedY = -stickY / stickLength * stickVelocity;
	}

	if (gyroMatchesStickMode || forceOutput)
	{
		expectedX += jc->gyroXVelocity;
		expectedY += jc->gyroYVelocity;
	}

	float targetGyroVelocity = sqrtf(expectedX * expectedX + expectedY * expectedY);
	// map gyro velocity to achievable range in 0-1
	float gyroInStickStrength = targetGyroVelocity >= maxStickGameSpeed ? 1.f : targetGyroVelocity / maxStickGameSpeed;
	// unpower curve
	if (unpower != 0.f)
	{
		gyroInStickStrength = pow(gyroInStickStrength, 1.f / unpower);
	}
	// remap to between inner and outer deadzones
	float gyroStickX = 0.f;
	float gyroStickY = 0.f;
	if (gyroInStickStrength > 0.01f)
	{
		gyroInStickStrength = undeadzoneInner + gyroInStickStrength * livezoneSize;
		gyroStickX = expectedX / targetGyroVelocity * gyroInStickStrength;
		gyroStickY = expectedY / targetGyroVelocity * gyroInStickStrength;
	}
	if (jc->_context->_vigemController)
	{
		if (stickLength <= undeadzoneInner)
		{
			if (gyroInStickStrength == 0.f)
			{
				// hack to help with finding deadzones more quickly
				jc->_context->_vigemController->setStick(undeadzoneInner, 0.f, isLeft);
			}
			else
			{
				jc->_context->_vigemController->setStick(gyroStickX, -gyroStickY, isLeft);
			}
		}
		else
		{
			jc->_context->_vigemController->setStick(gyroStickX, -gyroStickY, isLeft);
		}
	}

	jc->processed_gyro_stick |= gyroMatchesStickMode;

	return stickLength > undeadzoneInner;
}

static float handleFlickStick(float calX, float calY, float lastCalX, float lastCalY, float stickLength, bool &isFlicking, shared_ptr<JoyShock> jc, float mouseCalibrationFactor, bool FLICK_ONLY, bool ROTATE_ONLY)
{
	GyroOutput flickStickOutput = jc->getSetting<GyroOutput>(SettingID::FLICK_STICK_OUTPUT);
	bool isMouse = flickStickOutput == GyroOutput::MOUSE;

	float camSpeedX = 0.0f;
	// let's centre this
	float offsetX = calX;
	float offsetY = calY;
	float lastOffsetX = lastCalX;
	float lastOffsetY = lastCalY;
	float flickStickThreshold = 0.995f;
	if (isFlicking)
	{
		flickStickThreshold *= 0.9f;
	}
	if (stickLength >= flickStickThreshold)
	{
		float stickAngle = atan2f(-offsetX, offsetY);
		//COUT << ", %.4f\n", lastOffsetLength);
		if (!isFlicking)
		{
			// bam! new flick!
			isFlicking = true;
			if (!ROTATE_ONLY)
			{
				auto flick_snap_mode = jc->getSetting<FlickSnapMode>(SettingID::FLICK_SNAP_MODE);
				if (flick_snap_mode != FlickSnapMode::NONE)
				{
					// handle snapping
					float snapInterval = PI;
					if (flick_snap_mode == FlickSnapMode::FOUR)
					{
						snapInterval = PI / 2.0f; // 90 degrees
					}
					else if (flick_snap_mode == FlickSnapMode::EIGHT)
					{
						snapInterval = PI / 4.0f; // 45 degrees
					}
					float snappedAngle = round(stickAngle / snapInterval) * snapInterval;
					// lerp by snap strength
					auto flick_snap_strength = jc->getSetting(SettingID::FLICK_SNAP_STRENGTH);
					stickAngle = stickAngle * (1.0f - flick_snap_strength) + snappedAngle * flick_snap_strength;
				}
				if (abs(stickAngle) * (180.0f / PI) < jc->getSetting(SettingID::FLICK_DEADZONE_ANGLE))
				{
					stickAngle = 0.0f;
				}

				jc->started_flick = chrono::steady_clock::now();
				jc->delta_flick = stickAngle;
				jc->flick_percent_done = 0.0f;
				jc->ResetSmoothSample();
				jc->flick_rotation_counter = stickAngle; // track all rotation for this flick
				COUT << "Flick: " << setprecision(3) << stickAngle * (180.0f / (float)PI) << " degrees" << endl;
			}
		}
		else
		{
			if (!FLICK_ONLY)
			{
				// not new? turn camera?
				float lastStickAngle = atan2f(-lastOffsetX, lastOffsetY);
				float angleChange = stickAngle - lastStickAngle;
				// https://stackoverflow.com/a/11498248/1130520
				angleChange = fmod(angleChange + PI, 2.0f * PI);
				if (angleChange < 0)
					angleChange += 2.0f * PI;
				angleChange -= PI;
				jc->flick_rotation_counter += angleChange; // track all rotation for this flick
				float flickSpeedConstant = isMouse ? jc->getSetting(SettingID::REAL_WORLD_CALIBRATION) * mouseCalibrationFactor / jc->getSetting(SettingID::IN_GAME_SENS) : 1.f;
				float flickSpeed = -(angleChange * flickSpeedConstant);
				int maxSmoothingSamples = min(jc->NumSamples, (int)ceil(64.0f / tick_time.get())); // target a max smoothing window size of 64ms
				float stepSize = 0.01f;                                                            // and we only want full on smoothing when the stick change each time we poll it is approximately the minimum stick resolution
				                                                                                   // the fact that we're using radians makes this really easy
				auto rotate_smooth_override = jc->getSetting(SettingID::ROTATE_SMOOTH_OVERRIDE);
				if (rotate_smooth_override < 0.0f)
				{
					camSpeedX = jc->GetSmoothedStickRotation(flickSpeed, flickSpeedConstant * stepSize * 2.0f, flickSpeedConstant * stepSize * 4.0f, maxSmoothingSamples);
				}
				else
				{
					camSpeedX = jc->GetSmoothedStickRotation(flickSpeed, flickSpeedConstant * rotate_smooth_override, flickSpeedConstant * rotate_smooth_override * 2.0f, maxSmoothingSamples);
				}

				if (!isMouse)
				{
					// convert to a velocity
					camSpeedX *= 180.0f / (PI * 0.001f * tick_time.get());
				}
			}
		}
	}
	else if (isFlicking)
	{
		// was a flick! how much was the flick and rotation?
		if (!FLICK_ONLY && !ROTATE_ONLY)
		{
			last_flick_and_rotation = abs(jc->flick_rotation_counter) / (2.0f * PI);
		}
		isFlicking = false;
	}
	// do the flicking. this works very differently if it's mouse vs stick
	if (isMouse)
	{
		float secondsSinceFlick = ((float)chrono::duration_cast<chrono::microseconds>(jc->time_now - jc->started_flick).count()) / 1000000.0f;
		float newPercent = secondsSinceFlick / jc->getSetting(SettingID::FLICK_TIME);

		// don't divide by zero
		if (abs(jc->delta_flick) > 0.0f)
		{
			newPercent = newPercent / pow(abs(jc->delta_flick) / PI, jc->getSetting(SettingID::FLICK_TIME_EXPONENT));
		}

		if (newPercent > 1.0f)
			newPercent = 1.0f;
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
	else
	{
		float secondsSinceFlick = ((float)chrono::duration_cast<chrono::microseconds>(jc->time_now - jc->started_flick).count()) / 1000000.0f;
		float maxStickGameSpeed = jc->getSetting(SettingID::VIRTUAL_STICK_CALIBRATION);
		float flickTime = abs(jc->delta_flick) / (maxStickGameSpeed * PI / 180.f);

		if (secondsSinceFlick <= flickTime)
		{
			camSpeedX -= jc->delta_flick >= 0 ? maxStickGameSpeed : -maxStickGameSpeed;
		}

		// alright, but what happens if we've set gyro to one stick and flick stick to another?
		// Nic: FS is mouse output and gyrostick is stick output. The game handles the merging (or not)
		GyroOutput gyroOutput = jc->getSetting<GyroOutput>(SettingID::GYRO_OUTPUT);
		if (gyroOutput == flickStickOutput)
		{
			jc->gyroXVelocity += camSpeedX;
			processGyroStick(jc, 0.f, 0.f, 0.f, flickStickOutput == GyroOutput::LEFT_STICK ? StickMode::LEFT_STICK : StickMode::RIGHT_STICK, false);
		}
		else
		{
			float tempGyroXVelocity = jc->gyroXVelocity;
			float tempGyroYVelocity = jc->gyroYVelocity;
			jc->gyroXVelocity = camSpeedX;
			jc->gyroYVelocity = 0.f;
			processGyroStick(jc, 0.f, 0.f, 0.f, flickStickOutput == GyroOutput::LEFT_STICK ? StickMode::LEFT_STICK : StickMode::RIGHT_STICK, true);
			jc->gyroXVelocity = tempGyroXVelocity;
			jc->gyroYVelocity = tempGyroYVelocity;
		}

		return 0.f;
	}
}

void processStick(shared_ptr<JoyShock> jc, float stickX, float stickY, float lastX, float lastY, float innerDeadzone, float outerDeadzone,
  RingMode ringMode, StickMode stickMode, ButtonID ringId, ButtonID leftId, ButtonID rightId, ButtonID upId, ButtonID downId,
  ControllerOrientation controllerOrientation, float mouseCalibrationFactor, float deltaTime, float &acceleration, FloatXY &lastAreaCal,
  bool &isFlicking, bool &ignoreStickMode, bool &anyStickInput, bool &lockMouse, float &camSpeedX, float &camSpeedY, ScrollAxis *scroll, int touchpadIndex = -1)
{
	float temp;
	switch (controllerOrientation)
	{
	case ControllerOrientation::LEFT:
		temp = stickX;
		stickX = -stickY;
		stickY = temp;
		temp = lastX;
		lastX = -lastY;
		lastY = temp;
		break;
	case ControllerOrientation::RIGHT:
		temp = stickX;
		stickX = stickY;
		stickY = -temp;
		temp = lastX;
		lastX = lastY;
		lastY = -temp;
		break;
	case ControllerOrientation::BACKWARD:
		stickX = -stickX;
		stickY = -stickY;
		lastX = -lastX;
		lastY = -lastY;
		break;
	}

	outerDeadzone = 1.0f - outerDeadzone;
	float rawX = stickX;
	float rawY = stickY;
	float rawLength = sqrtf(rawX * rawX + rawY * rawY);
	jc->processDeadZones(lastX, lastY, innerDeadzone, outerDeadzone);
	bool pegged = jc->processDeadZones(stickX, stickY, innerDeadzone, outerDeadzone);
	float absX = abs(stickX);
	float absY = abs(stickY);
	bool left = stickX < -0.5f * absY;
	bool right = stickX > 0.5f * absY;
	bool down = stickY < -0.5f * absX;
	bool up = stickY > 0.5f * absX;
	float stickLength = sqrtf(stickX * stickX + stickY * stickY);
	bool ring = ringMode == RingMode::INNER && stickLength > 0.0f && stickLength < 0.7f ||
	  ringMode == RingMode::OUTER && stickLength > 0.7f;
	jc->handleButtonChange(ringId, ring, touchpadIndex);

	bool rotateOnly = stickMode == StickMode::ROTATE_ONLY;
	bool flickOnly = stickMode == StickMode::FLICK_ONLY;
	if (ignoreStickMode && stickMode == StickMode::INVALID && stickX == 0 && stickY == 0)
	{
		// clear ignore flag when stick is back at neutral
		ignoreStickMode = false;
	}
	else if (stickMode == StickMode::FLICK || flickOnly || rotateOnly)
	{
		camSpeedX += handleFlickStick(stickX, stickY, lastX, lastY, stickLength, isFlicking, jc, mouseCalibrationFactor, flickOnly, rotateOnly);
		anyStickInput = pegged;
	}
	else if (stickMode == StickMode::AIM)
	{
		// camera movement
		if (!pegged)
		{
			acceleration = 1.0f; // reset
		}
		float stickLength = sqrt(stickX * stickX + stickY * stickY);
		if (stickLength != 0.0f)
		{
			anyStickInput = true;
			float warpedStickLengthX = pow(stickLength, jc->getSetting(SettingID::STICK_POWER));
			float warpedStickLengthY = warpedStickLengthX;
			warpedStickLengthX *= jc->getSetting<FloatXY>(SettingID::STICK_SENS).first * jc->getSetting(SettingID::REAL_WORLD_CALIBRATION) / os_mouse_speed / jc->getSetting(SettingID::IN_GAME_SENS);
			warpedStickLengthY *= jc->getSetting<FloatXY>(SettingID::STICK_SENS).second * jc->getSetting(SettingID::REAL_WORLD_CALIBRATION) / os_mouse_speed / jc->getSetting(SettingID::IN_GAME_SENS);
			camSpeedX += stickX / stickLength * warpedStickLengthX * acceleration * deltaTime;
			camSpeedY += stickY / stickLength * warpedStickLengthY * acceleration * deltaTime;
			if (pegged)
			{
				acceleration += jc->getSetting(SettingID::STICK_ACCELERATION_RATE) * deltaTime;
				auto cap = jc->getSetting(SettingID::STICK_ACCELERATION_CAP);
				if (acceleration > cap)
				{
					acceleration = cap;
				}
			}
		}
	}
	else if (stickMode == StickMode::MOUSE_AREA)
	{
		auto mouse_ring_radius = jc->getSetting(SettingID::MOUSE_RING_RADIUS);
		if (stickX != 0.0f || stickY != 0.0f)
		{
			// use difference with last cal values
			float mouseX = (stickX - lastAreaCal.x()) * mouse_ring_radius;
			float mouseY = (stickY - lastAreaCal.y()) * -1 * mouse_ring_radius;
			// do it!
			moveMouse(mouseX, mouseY);
			lastAreaCal = { stickX, stickY };
		}
		else
		{
			// Return to center
			moveMouse(lastAreaCal.x() * -1 * mouse_ring_radius, lastAreaCal.y() * mouse_ring_radius);
			lastAreaCal = { 0, 0 };
		}
	}
	else if (stickMode == StickMode::MOUSE_RING)
	{
		if (stickX != 0.0f || stickY != 0.0f)
		{
			auto mouse_ring_radius = jc->getSetting(SettingID::MOUSE_RING_RADIUS);
			float stickLength = sqrt(stickX * stickX + stickY * stickY);
			float normX = stickX / stickLength;
			float normY = stickY / stickLength;
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
	else if (stickMode == StickMode::SCROLL_WHEEL)
	{
		if (scroll)
		{
			if (stickX == 0 && stickY == 0)
			{
				scroll->Reset(jc->time_now);
			}
			else if (lastX != 0 && lastY != 0)
			{
				float lastAngle = atan2f(lastY, lastX) / PI * 180.f;
				float angle = atan2f(stickY, stickX) / PI * 180.f;
				if (((lastAngle > 0) ^ (angle > 0)) && fabsf(angle - lastAngle) > 270.f) // Handle loop the loop
				{
					lastAngle = lastAngle > 0 ? lastAngle - 360.f : lastAngle + 360.f;
				}
				//COUT << "Stick moved from " << lastAngle << " to " << angle; // << endl;
				scroll->ProcessScroll(angle - lastAngle, jc->getSetting<FloatXY>(SettingID::SCROLL_SENS).x(), jc->time_now);
			}
		}
	}
	else if (stickMode == StickMode::NO_MOUSE || stickMode == StickMode::INNER_RING || stickMode == StickMode::OUTER_RING)
	{ // Do not do if invalid
		// left!
		jc->handleButtonChange(leftId, left, touchpadIndex);
		// right!
		jc->handleButtonChange(rightId, right, touchpadIndex);
		// up!
		jc->handleButtonChange(upId, up, touchpadIndex);
		// down!
		jc->handleButtonChange(downId, down, touchpadIndex);

		anyStickInput = left || right || up || down; // ring doesn't count
	}
	else if (stickMode == StickMode::LEFT_STICK || stickMode == StickMode::RIGHT_STICK)
	{
		if (jc->_context->_vigemController)
		{
			anyStickInput = processGyroStick(jc, rawX, rawY, rawLength, stickMode, false);
		}
	}
	else if (stickMode >= StickMode::LEFT_ANGLE_TO_X && stickMode <= StickMode::RIGHT_ANGLE_TO_Y)
	{
		if (jc->_context->_vigemController && rawLength > innerDeadzone)
		{
			bool isX = stickMode == StickMode::LEFT_ANGLE_TO_X || stickMode == StickMode::RIGHT_ANGLE_TO_X;
			bool isLeft = stickMode == StickMode::LEFT_ANGLE_TO_X || stickMode == StickMode::LEFT_ANGLE_TO_Y;

			float stickAngle = isX ? atan2f(stickX, absY) : atan2f(stickY, absX);
			float absAngle = abs(stickAngle * 180.0f / PI);
			float signAngle = stickAngle < 0.f ? -1.f : 1.f;
			float angleDeadzoneInner = jc->getSetting(SettingID::ANGLE_TO_AXIS_DEADZONE_INNER);
			float angleDeadzoneOuter = jc->getSetting(SettingID::ANGLE_TO_AXIS_DEADZONE_OUTER);
			float absStickValue = clamp((absAngle - angleDeadzoneInner) / (90.f - angleDeadzoneOuter - angleDeadzoneInner), 0.f, 1.f);

			absStickValue *= pow(stickLength, jc->getSetting(SettingID::STICK_POWER));

			// now actually convert to output stick value, taking deadzones and power curve into account
			float undeadzoneInner, undeadzoneOuter, unpower;
			if (isLeft)
			{
				undeadzoneInner = jc->getSetting(SettingID::LEFT_STICK_UNDEADZONE_INNER);
				undeadzoneOuter = jc->getSetting(SettingID::LEFT_STICK_UNDEADZONE_OUTER);
				unpower = jc->getSetting(SettingID::LEFT_STICK_UNPOWER);
			}
			else
			{
				undeadzoneInner = jc->getSetting(SettingID::RIGHT_STICK_UNDEADZONE_INNER);
				undeadzoneOuter = jc->getSetting(SettingID::RIGHT_STICK_UNDEADZONE_OUTER);
				unpower = jc->getSetting(SettingID::RIGHT_STICK_UNPOWER);
			}

			float livezoneSize = 1.f - undeadzoneOuter - undeadzoneInner;
			if (livezoneSize > 0.f)
			{
				anyStickInput = true;

				// unpower curve
				if (unpower != 0.f)
				{
					absStickValue = pow(absStickValue, 1.f / unpower);
				}

				if (absStickValue < 1.f)
				{
					absStickValue = undeadzoneInner + absStickValue * livezoneSize;
				}

				float signedStickValue = signAngle * absStickValue;
				if (isX)
				{
					jc->_context->_vigemController->setStick(signedStickValue, 0.f, isLeft);
				}
				else
				{
					jc->_context->_vigemController->setStick(0.f, signedStickValue, isLeft);
				}
			}
		}
	}
	else if (stickMode == StickMode::LEFT_WIND_X || stickMode == StickMode::RIGHT_WIND_X)
	{
		if (jc->_context->_vigemController)
		{
			bool isLeft = stickMode == StickMode::LEFT_WIND_X;

			float &currentWindingAngle = isLeft ? jc->windingAngleLeft : jc->windingAngleRight;

			// currently, just use the same hard-coded thresholds we use for flick stick. These are affected by deadzones
			if (stickLength > 0.f && lastX != 0.f && lastY != 0.f)
			{
				// use difference between last stick angle and current
				float stickAngle = atan2f(-stickX, stickY);
				float lastStickAngle = atan2f(-lastX, lastY);
				float angleChange = fmod((stickAngle - lastStickAngle) + PI, 2.0f * PI);
				if (angleChange < 0)
					angleChange += 2.0f * PI;
				angleChange -= PI;

				currentWindingAngle -= angleChange * stickLength * 180.f / PI;

				anyStickInput = true;
			}

			if (stickLength < 1.f)
			{
				float absWindingAngle = abs(currentWindingAngle);
				float unwindAmount = jc->getSetting(SettingID::UNWIND_RATE) * (1.f - stickLength) * deltaTime;
				float windingSign = currentWindingAngle < 0.f ? -1.f : 1.f;
				if (absWindingAngle <= unwindAmount)
				{
					currentWindingAngle = 0.f;
				}
				else
				{
					currentWindingAngle -= unwindAmount * windingSign;
				}
			}

			float newWindingSign = currentWindingAngle < 0.f ? -1.f : 1.f;
			float newAbsWindingAngle = abs(currentWindingAngle);

			float windingRange = jc->getSetting(SettingID::WIND_STICK_RANGE);
			float windingPower = jc->getSetting(SettingID::WIND_STICK_POWER);
			if (windingPower == 0.f)
			{
				windingPower = 1.f;
			}

			float windingRemapped = std::min(pow(newAbsWindingAngle / windingRange * 2.f, windingPower), 1.f);

			// let's account for deadzone!
			float undeadzoneInner, undeadzoneOuter, unpower;
			if (isLeft)
			{
				undeadzoneInner = jc->getSetting(SettingID::LEFT_STICK_UNDEADZONE_INNER);
				undeadzoneOuter = jc->getSetting(SettingID::LEFT_STICK_UNDEADZONE_OUTER);
				unpower = jc->getSetting(SettingID::LEFT_STICK_UNPOWER);
			}
			else
			{
				undeadzoneInner = jc->getSetting(SettingID::RIGHT_STICK_UNDEADZONE_INNER);
				undeadzoneOuter = jc->getSetting(SettingID::RIGHT_STICK_UNDEADZONE_OUTER);
				unpower = jc->getSetting(SettingID::RIGHT_STICK_UNPOWER);
			}

			float livezoneSize = 1.f - undeadzoneOuter - undeadzoneInner;
			if (livezoneSize > 0.f)
			{
				anyStickInput = true;

				// unpower curve
				if (unpower != 0.f)
				{
					windingRemapped = pow(windingRemapped, 1.f / unpower);
				}

				if (windingRemapped < 1.f)
				{
					windingRemapped = undeadzoneInner + windingRemapped * livezoneSize;
				}

				float signedStickValue = newWindingSign * windingRemapped;
				jc->_context->_vigemController->setStick(signedStickValue, 0.f, isLeft);
			}
		}
	}
}

void TouchStick::handleTouchStickChange(shared_ptr<JoyShock> js, bool down, short movX, short movY, float delta_time)
{
	float stickX = down ? clamp<float>((_currentLocation.x() + movX) / js->getSetting(SettingID::TOUCH_STICK_RADIUS), -1.f, 1.f) : 0.f;
	float stickY = down ? clamp<float>((_currentLocation.y() - movY) / js->getSetting(SettingID::TOUCH_STICK_RADIUS), -1.f, 1.f) : 0.f;
	float innerDeadzone = js->getSetting(SettingID::TOUCH_DEADZONE_INNER);
	RingMode ringMode = js->getSetting<RingMode>(SettingID::TOUCH_RING_MODE);
	StickMode stickMode = js->getSetting<StickMode>(SettingID::TOUCH_STICK_MODE);
	ControllerOrientation controllerOrientation = js->getSetting<ControllerOrientation>(SettingID::CONTROLLER_ORIENTATION);
	float mouseCalibrationFactor = 180.0f / PI / os_mouse_speed;

	bool anyStickInput = false;
	bool lockMouse = false;
	float camSpeedX = 0.f;
	float camSpeedY = 0.f;
	auto axisSign = js->getSetting<AxisSignPair>(SettingID::TOUCH_STICK_AXIS);

	processStick(js, stickX * float(axisSign.first), stickY *float(axisSign.second), _currentLocation.x() * float(axisSign.first), _currentLocation.y() * float(axisSign.second), innerDeadzone, 0.f,
	  ringMode, stickMode, ButtonID::TRING, ButtonID::TLEFT, ButtonID::TRIGHT, ButtonID::TUP, ButtonID::TDOWN,
	  controllerOrientation, mouseCalibrationFactor, delta_time, touch_stick_acceleration, touch_last_cal,
	  is_flicking_touch, ignore_motion_stick, anyStickInput, lockMouse, camSpeedX, camSpeedY, &js->touch_scroll_x, _index);

	moveMouse(camSpeedX * float(js->getSetting<AxisSignPair>(SettingID::TOUCH_STICK_AXIS).first), -camSpeedY *float(js->getSetting<AxisSignPair>(SettingID::TOUCH_STICK_AXIS).second));

	if (!down && _prevDown)
	{
		_currentLocation = { 0.f, 0.f };

		is_flicking_touch = false;
		touch_last_cal;
		touch_stick_acceleration = 1.0;
		ignore_motion_stick = false;
	}
	else
	{
		_currentLocation = { _currentLocation.x() + movX, _currentLocation.y() - movY };
	}

	_prevDown = down;
}

void DisplayTouchInfo(int id, optional<FloatXY> xy, optional<FloatXY> prevXY = nullopt)
{
	if (xy)
	{
		if (!prevXY)
		{
			cout << "New touch " << id << " at " << *xy << endl;
		}
		else if (fabsf(xy->x() - prevXY->x()) > FLT_EPSILON || fabsf(xy->y() - prevXY->y()) > FLT_EPSILON)
		{
			cout << "Touch " << id << " moved to " << *xy << endl;
		}
	}
	else if (prevXY)
	{
		cout << "Touch " << id << " has been released" << endl;
	}
}

void TouchCallback(int jcHandle, TOUCH_STATE newState, TOUCH_STATE prevState, float delta_time)
{

	//if (current.t0Down || previous.t0Down)
	//{
	//	DisplayTouchInfo(current.t0Down ? current.t0Id : previous.t0Id,
	//		current.t0Down ? optional<FloatXY>({ current.t0X, current.t0Y }) : nullopt,
	//		previous.t0Down ? optional<FloatXY>({ previous.t0X, previous.t0Y }) : nullopt);
	//}

	//if (current.t1Down || previous.t1Down)
	//{
	//	DisplayTouchInfo(current.t1Down ? current.t1Id : previous.t1Id,
	//		current.t1Down ? optional<FloatXY>({ current.t1X, current.t1Y }) : nullopt,
	//		previous.t1Down ? optional<FloatXY>({ previous.t1X, previous.t1Y }) : nullopt);
	//}

	shared_ptr<JoyShock> js = handle_to_joyshock[jcHandle];
	int tpSizeX, tpSizeY;
	if (!js || jsl->GetTouchpadDimension(jcHandle, tpSizeX, tpSizeY) == false)
		return;

	lock_guard guard(js->_context->callback_lock);

	TOUCH_POINT point0, point1;

	point0.posX = newState.t0Down ? newState.t0X : -1.f; // Absolute position in percentage
	point0.posY = newState.t0Down ? newState.t0Y : -1.f;
	point0.movX = js->prevTouchState.t0Down ? (newState.t0X - js->prevTouchState.t0X) * tpSizeX : 0.f; // Relative movement in unit
	point0.movY = js->prevTouchState.t0Down ? (newState.t0Y - js->prevTouchState.t0Y) * tpSizeY : 0.f;
	point1.posX = newState.t1Down ? newState.t1X : -1.f;
	point1.posY = newState.t1Down ? newState.t1Y : -1.f;
	point1.movX = js->prevTouchState.t1Down ? (newState.t1X - js->prevTouchState.t1X) * tpSizeX : 0.f;
	point1.movY = js->prevTouchState.t1Down ? (newState.t1Y - js->prevTouchState.t1Y) * tpSizeY : 0.f;

	auto mode = js->getSetting<TouchpadMode>(SettingID::TOUCHPAD_MODE);
	// js->handleButtonChange(ButtonID::TOUCH, point0.isDown() || point1.isDown()); // This is handled by dual stage "trigger" step
	if (!point0.isDown() && !point1.isDown())
	{

		static const std::function<bool(ButtonID)> IS_TOUCH_BUTTON = [](ButtonID id) {
			return id >= ButtonID::T1;
		};

		for (auto currentlyActive = find_if(js->_context->chordStack.begin(), js->_context->chordStack.end(), IS_TOUCH_BUTTON);
		     currentlyActive != js->_context->chordStack.end();
		     currentlyActive = find_if(js->_context->chordStack.begin(), js->_context->chordStack.end(), IS_TOUCH_BUTTON))
		{
			js->_context->chordStack.erase(currentlyActive);
		}
	}
	if (mode == TouchpadMode::GRID_AND_STICK)
	{
		// Handle grid
		int index0 = -1, index1 = -1;
		if (point0.isDown())
		{
			float row = floorf(point0.posY * grid_size.get().y());
			float col = floorf(point0.posX * grid_size.get().x());
			//cout << "I should be in button " << row << " " << col << endl;
			index0 = int(row * grid_size.get().x() + col);
		}

		if (point1.isDown())
		{
			float row = floorf(point1.posY * grid_size.get().y());
			float col = floorf(point1.posX * grid_size.get().x());
			//cout << "I should be in button " << row << " " << col << endl;
			index1 = int(row * grid_size.get().x() + col);
		}

		for (size_t i = 0; i < grid_mappings.size(); ++i)
		{
			auto optId = magic_enum::enum_cast<ButtonID>(FIRST_TOUCH_BUTTON + i);

			// JSM can get touch button callbacks before the grid buttons are setup at startup. Just skip then.
			if (optId && js->gridButtons.size() == grid_mappings.size())
				js->handleButtonChange(*optId, i == index0 || i == index1);
		}

		// Handle stick
		js->touchpads[0].handleTouchStickChange(js, point0.isDown(), point0.movX, point0.movY, delta_time);
		js->touchpads[1].handleTouchStickChange(js, point1.isDown(), point1.movX, point1.movY, delta_time);
	}
	else if (mode == TouchpadMode::MOUSE)
	{
		// Disable gestures
		//if (point0.isDown() && point1.isDown())
		//{
		//if (js->prevTouchState.t0Down && js->prevTouchState.t1Down)
		//{
		//	float x = fabsf(newState.t0X - newState.t1X);
		//	float y = fabsf(newState.t0Y - newState.t1Y);
		//	float angle = atan2f(y, x) / PI * 360;
		//	float dist = sqrt(x * x + y * y);
		//	x = fabsf(js->prevTouchState.t0X - js->prevTouchState.t1X);
		//	y = fabsf(js->prevTouchState.t0Y - js->prevTouchState.t1Y);
		//	float oldAngle = atan2f(y, x) / PI * 360;
		//	float oldDist = sqrt(x * x + y * y);
		//	if (angle != oldAngle)
		//		DEBUG_LOG << "Angle went from " << oldAngle << " degrees to " << angle << " degress. Diff is " << angle - oldAngle << " degrees. ";
		//	js->touch_scroll_x.ProcessScroll(angle - oldAngle, js->getSetting<FloatXY>(SettingID::SCROLL_SENS).x(), js->time_now);
		//	if (dist != oldDist)
		//		DEBUG_LOG << "Dist went from " << oldDist << " points to " << dist << " points. Diff is " << dist - oldDist << " points. ";
		//	js->touch_scroll_y.ProcessScroll(dist - oldDist, js->getSetting<FloatXY>(SettingID::SCROLL_SENS).y(), js->time_now);
		//}
		//else
		//{
		//	js->touch_scroll_x.Reset(js->time_now);
		//	js->touch_scroll_y.Reset(js->time_now);
		//}
		//}
		//else
		//{
		//	js->touch_scroll_x.Reset(js->time_now);
		//	js->touch_scroll_y.Reset(js->time_now);
		//  if (point0.isDown() ^ point1.isDown()) // XOR
		if (point0.isDown() || point1.isDown())
		{
			TOUCH_POINT *downPoint = point0.isDown() ? &point0 : &point1;
			FloatXY sens = js->getSetting<FloatXY>(SettingID::TOUCHPAD_SENS);
			// if(downPoint->movX || downPoint->movY) cout << "Moving the cursor by " << std::dec << int(downPoint->movX) << " h and " << int(downPoint->movY) << " v" << endl;
			moveMouse(downPoint->movX * sens.x(), downPoint->movY * sens.y());
			// Ignore second touch point in this mode for now until gestures gets handled here
		}
		//}
	}
	js->prevTouchState = newState;
}

void CalibrateTriggers(shared_ptr<JoyShock> jc)
{
	if (jsl->GetButtons(jc->handle) & (1 << JSOFFSET_HOME))
	{
		COUT << "Abandonning calibration" << endl;
		triggerCalibrationStep = 0;
		return;
	}

	auto rpos = jsl->GetRightTrigger(jc->handle);
	auto lpos = jsl->GetLeftTrigger(jc->handle);
	switch (triggerCalibrationStep)
	{
	case 1:
		COUT << "Softly press on the right trigger until you just feel the resistance." << endl;
		COUT << "Then press the dpad down button to proceed, or press HOME to abandon." << endl;
		tick_time = 100;
		jc->right_effect.mode = AdaptiveTriggerMode::SEGMENT;
		jc->right_effect.start = 0;
		jc->right_effect.end = 255;
		jc->right_effect.force = 255;
		triggerCalibrationStep++;
		break;
	case 2:
		if (jsl->GetButtons(jc->handle) & (1 << JSOFFSET_DOWN))
		{
			triggerCalibrationStep++;
		}
		break;
	case 3:
		DEBUG_LOG << "trigger pos is at " << int(rpos * 255.f) << " (" << int(rpos * 100.f) << "%) and effect pos is at " << int(jc->right_effect.start) << endl;
		if (int(rpos * 255.f) > 0)
		{
			right_trigger_offset = jc->right_effect.start;
			tick_time = 40;
			triggerCalibrationStep++;
		}
		++jc->right_effect.start;
		break;
	case 4:
		DEBUG_LOG << "trigger pos is at " << int(rpos * 255.f) << " (" << int(rpos * 100.f) << "%) and effect pos is at " << int(jc->right_effect.start) << endl;
		if (int(rpos * 255.f) > 240)
		{
			tick_time = 100;
			triggerCalibrationStep++;
		}
		++jc->right_effect.start;
		break;
	case 5:
		DEBUG_LOG << "trigger pos is at " << int(rpos * 255.f) << " (" << int(rpos * 100.f) << "%) and effect pos is at " << int(jc->right_effect.start) << endl;
		if (int(rpos * 255.f) == 255)
		{
			triggerCalibrationStep++;
			right_trigger_range = int(jc->right_effect.start - right_trigger_offset);
		}
		++jc->right_effect.start;
		break;
	case 6:
		COUT << "Softly press on the left trigger until you just feel the resistance." << endl;
		COUT << "Then press the cross button to proceed, or press HOME to abandon." << endl;
		tick_time = 100;
		jc->left_effect.mode = AdaptiveTriggerMode::SEGMENT;
		jc->left_effect.start = 0;
		jc->left_effect.end = 255;
		jc->left_effect.force = 255;
		triggerCalibrationStep++;
		break;
	case 7:
		if (jsl->GetButtons(jc->handle) & (1 << JSOFFSET_S))
		{
			triggerCalibrationStep++;
		}
		break;
	case 8:
		DEBUG_LOG << "trigger pos is at " << int(lpos * 255.f) << " (" << int(lpos * 100.f) << "%) and effect pos is at " << int(jc->left_effect.start) << endl;
		if (int(lpos * 255.f) > 0)
		{
			left_trigger_offset = jc->left_effect.start;
			tick_time = 40;
			triggerCalibrationStep++;
		}
		++jc->left_effect.start;
		break;
	case 9:
		DEBUG_LOG << "trigger pos is at " << int(lpos * 255.f) << " (" << int(lpos * 100.f) << "%) and effect pos is at " << int(jc->left_effect.start) << endl;
		if (int(lpos * 255.f) > 240)
		{
			tick_time = 100;
			triggerCalibrationStep++;
		}
		++jc->left_effect.start;
		break;
	case 10:
		DEBUG_LOG << "trigger pos is at " << int(lpos * 255.f) << " (" << int(lpos * 100.f) << "%) and effect pos is at " << int(jc->left_effect.start) << endl;
		if (int(lpos * 255.f) == 255)
		{
			triggerCalibrationStep++;
			left_trigger_range = int(jc->left_effect.start - left_trigger_offset);
		}
		++jc->left_effect.start;
		break;
	case 11:
		COUT << "Your triggers have been successfully calibrated. Add the trigger offset and range values in your OnReset.txt file to have those values set by default." << endl;
		COUT_INFO << SettingID::RIGHT_TRIGGER_OFFSET << " = " << right_trigger_offset << endl;
		COUT_INFO << SettingID::RIGHT_TRIGGER_RANGE << " = " << right_trigger_range << endl;
		COUT_INFO << SettingID::LEFT_TRIGGER_OFFSET << " = " << left_trigger_offset << endl;
		COUT_INFO << SettingID::LEFT_TRIGGER_RANGE << " = " << left_trigger_range << endl;
		triggerCalibrationStep = 0;
		tick_time.Reset();
		break;
	}
	jsl->SetTriggerEffect(jc->handle, jc->left_effect, jc->right_effect);
}

void joyShockPollCallback(int jcHandle, JOY_SHOCK_STATE state, JOY_SHOCK_STATE lastState, IMU_STATE imuState, IMU_STATE lastImuState, float deltaTime)
{

	shared_ptr<JoyShock> jc = handle_to_joyshock[jcHandle];
	if (jc == nullptr)
		return;
	jc->_context->callback_lock.lock();

	auto timeNow = chrono::steady_clock::now();
	deltaTime = ((float)chrono::duration_cast<chrono::microseconds>(timeNow - jc->time_now).count()) / 1000000.0f;
	jc->time_now = timeNow;

	if (triggerCalibrationStep)
	{
		CalibrateTriggers(jc);
		jc->_context->callback_lock.unlock();
		return;
	}

	MotionIf &motion = *jc->motion;

	IMU_STATE imu = jsl->GetIMUState(jc->handle);

	if (auto_calibrate_gyro.get() == Switch::ON)
	{
		motion.SetAutoCalibration(true, 1.2f, 0.015f);
	}
	else
	{
		motion.SetAutoCalibration(false, 0.f, 0.f);
	}
	motion.ProcessMotion(imu.gyroX, imu.gyroY, imu.gyroZ, imu.accelX, imu.accelY, imu.accelZ, deltaTime);

	float inGyroX, inGyroY, inGyroZ;
	motion.GetCalibratedGyro(inGyroX, inGyroY, inGyroZ);

	float inGravX, inGravY, inGravZ;
	motion.GetGravity(inGravX, inGravY, inGravZ);

	float inQuatW, inQuatX, inQuatY, inQuatZ;
	motion.GetOrientation(inQuatW, inQuatX, inQuatY, inQuatZ);

	//// These are for sanity checking sensor fusion against a simple complementary filter:
	//float angle = sqrtf(inGyroX * inGyroX + inGyroY * inGyroY + inGyroZ * inGyroZ) * PI / 180.f * deltaTime;
	//Vec normAxis = Vec(-inGyroX, -inGyroY, -inGyroZ).Normalized();
	//Quat reverseRotation = Quat(cosf(angle * 0.5f), normAxis.x, normAxis.y, normAxis.z);
	//reverseRotation.Normalize();
	//jc->lastGrav *= reverseRotation;
	//Vec newGrav = Vec(-imu.accelX, -imu.accelY, -imu.accelZ);
	//jc->lastGrav += (newGrav - jc->lastGrav) * 0.01f;
	//
	//Vec normFancyGrav = Vec(inGravX, inGravY, inGravZ).Normalized();
	//Vec normSimpleGrav = jc->lastGrav.Normalized();
	//
	//float gravAngleDiff = acosf(normFancyGrav.Dot(normSimpleGrav)) * 180.f / PI;

	//COUT << "Angle diff: " << gravAngleDiff << "\n\tFancy gravity: " << normFancyGrav.x << ", " << normFancyGrav.y << ", " << normFancyGrav.z << "\n\tSimple gravity: " << normSimpleGrav.x << ", " << normSimpleGrav.y << ", " << normSimpleGrav.z << "\n";
	//COUT << "Quat: " << inQuatW << ", " << inQuatX << ", " << inQuatY << ", " << inQuatZ << "\n";

	//inGravX = normSimpleGrav.x;
	//inGravY = normSimpleGrav.y;
	//inGravZ = normSimpleGrav.z;

	//COUT << "DS4 accel: %.4f, %.4f, %.4f\n", imuState.accelX, imuState.accelY, imuState.accelZ);
	//COUT << "\tDS4 gyro: %.4f, %.4f, %.4f\n", imuState.gyroX, imuState.gyroY, imuState.gyroZ);
	//COUT << "\tDS4 quat: %.4f, %.4f, %.4f, %.4f | accel: %.4f, %.4f, %.4f | grav: %.4f, %.4f, %.4f\n",
	//	inQuatW, inQuatX, inQuatY, inQuatZ,
	//	motion.accelX, motion.accelY, motion.accelZ,
	//	inGravvX, inGravY, inGravZ);

	bool blockGyro = false;
	bool lockMouse = false;
	bool leftAny = false;
	bool rightAny = false;
	bool motionAny = false;

	if (jc->set_neutral_quat)
	{
		// motion stick neutral should be calculated from the gravity vector
		Vec gravDirection = Vec(inGravX, inGravY, inGravZ);
		Vec normalizedGravDirection = gravDirection.Normalized();
		float diffAngle = acosf(std::clamp(-gravDirection.y, -1.f, 1.f));
		Vec neutralGravAxis = Vec(0.0f, -1.0f, 0.0f).Cross(normalizedGravDirection);
		Quat neutralQuat = Quat(cosf(diffAngle * 0.5f), neutralGravAxis.x, neutralGravAxis.y, neutralGravAxis.z);
		neutralQuat.Normalize();

		jc->neutralQuatW = neutralQuat.w;
		jc->neutralQuatX = neutralQuat.x;
		jc->neutralQuatY = neutralQuat.y;
		jc->neutralQuatZ = neutralQuat.z;
		jc->set_neutral_quat = false;
		COUT << "Neutral orientation for device " << jc->handle << " set..." << endl;
	}

	float gyroX = 0.0;
	float gyroY = 0.0;
	GyroSpace gyroSpace = jc->getSetting<GyroSpace>(SettingID::GYRO_SPACE);
	if (gyroSpace == GyroSpace::LOCAL)
	{
		int mouse_x_flag = (int)jc->getSetting<GyroAxisMask>(SettingID::MOUSE_X_FROM_GYRO_AXIS);
		if ((mouse_x_flag & (int)GyroAxisMask::X) > 0)
		{
			gyroX += inGyroX;
		}
		if ((mouse_x_flag & (int)GyroAxisMask::Y) > 0)
		{
			gyroX -= inGyroY;
		}
		if ((mouse_x_flag & (int)GyroAxisMask::Z) > 0)
		{
			gyroX -= inGyroZ;
		}
		int mouse_y_flag = (int)jc->getSetting<GyroAxisMask>(SettingID::MOUSE_Y_FROM_GYRO_AXIS);
		if ((mouse_y_flag & (int)GyroAxisMask::X) > 0)
		{
			gyroY -= inGyroX;
		}
		if ((mouse_y_flag & (int)GyroAxisMask::Y) > 0)
		{
			gyroY += inGyroY;
		}
		if ((mouse_y_flag & (int)GyroAxisMask::Z) > 0)
		{
			gyroY += inGyroZ;
		}
	}
	else
	{
		float gravLength = sqrtf(inGravX * inGravX + inGravY * inGravY + inGravZ * inGravZ);
		float normGravX = 0.f;
		float normGravY = 0.f;
		float normGravZ = 0.f;
		if (gravLength > 0.f)
		{
			float gravNormalizer = 1.f / gravLength;
			normGravX = inGravX * gravNormalizer;
			normGravY = inGravY * gravNormalizer;
			normGravZ = inGravZ * gravNormalizer;
		}

		float flatness = std::abs(normGravY);
		float upness = std::abs(normGravZ);
		float sideReduction = std::clamp((std::max(flatness, upness) - 0.125f) / 0.125f, 0.f, 1.f);

		if (gyroSpace == GyroSpace::PLAYER_TURN || gyroSpace == GyroSpace::PLAYER_LEAN)
		{
			if (gyroSpace == GyroSpace::PLAYER_TURN)
			{
				// grav dot gyro axis (but only Y (yaw) and Z (roll))
				float worldYaw = normGravY * inGyroY + normGravZ * inGyroZ;
				float worldYawSign = worldYaw < 0.f ? -1.f : 1.f;
				const float yawRelaxFactor = 2.f; // 60 degree buffer
				//const float yawRelaxFactor = 1.41f; // 45 degree buffer
				//const float yawRelaxFactor = 1.15f; // 30 degree buffer
				gyroX += worldYawSign * std::min(std::abs(worldYaw) * yawRelaxFactor, sqrtf(inGyroY * inGyroY + inGyroZ * inGyroZ));
			}
			else // PLAYER_LEAN
			{
				// project local pitch axis (X) onto gravity plane
				// super simple since our point is only non-zero in one axis
				float gravDotPitchAxis = normGravX;
				float pitchAxisX = 1.f - normGravX * gravDotPitchAxis;
				float pitchAxisY = -normGravY * gravDotPitchAxis;
				float pitchAxisZ = -normGravZ * gravDotPitchAxis;
				// normalize
				float pitchAxisLengthSquared = pitchAxisX * pitchAxisX + pitchAxisY * pitchAxisY + pitchAxisZ * pitchAxisZ;
				if (pitchAxisLengthSquared > 0.f)
				{
					// world roll axis is cross (yaw, pitch)
					float rollAxisX = pitchAxisY * normGravZ - pitchAxisZ * normGravY;
					float rollAxisY = pitchAxisZ * normGravX - pitchAxisX * normGravZ;
					float rollAxisZ = pitchAxisX * normGravY - pitchAxisY * normGravX;

					// normalize
					float rollAxisLengthSquared = rollAxisX * rollAxisX + rollAxisY * rollAxisY + rollAxisZ * rollAxisZ;
					if (rollAxisLengthSquared > 0.f)
					{
						float rollAxisLength = sqrtf(rollAxisLengthSquared);
						float lengthReciprocal = 1.f / rollAxisLength;
						rollAxisX *= lengthReciprocal;
						rollAxisY *= lengthReciprocal;
						rollAxisZ *= lengthReciprocal;

						float worldRoll = rollAxisY * inGyroY + rollAxisZ * inGyroZ;
						float worldRollSign = worldRoll < 0.f ? -1.f : 1.f;
						//const float rollRelaxFactor = 2.f; // 60 degree buffer
						const float rollRelaxFactor = 1.41f; // 45 degree buffer
						//const float rollRelaxFactor = 1.15f; // 30 degree buffer
						gyroX += worldRollSign * std::min(std::abs(worldRoll) * rollRelaxFactor, sqrtf(inGyroY * inGyroY + inGyroZ * inGyroZ));
						gyroX *= sideReduction;
					}
				}
			}

			gyroY -= inGyroX;
		}
		else // WORLD_TURN or WORLD_LEAN
		{
			// grav dot gyro axis
			float worldYaw = normGravX * inGyroX + normGravY * inGyroY + normGravZ * inGyroZ;
			// project local pitch axis (X) onto gravity plane
			// super simple since our point is only non-zero in one axis
			float gravDotPitchAxis = normGravX;
			float pitchAxisX = 1.f - normGravX * gravDotPitchAxis;
			float pitchAxisY = -normGravY * gravDotPitchAxis;
			float pitchAxisZ = -normGravZ * gravDotPitchAxis;
			// normalize
			float pitchAxisLengthSquared = pitchAxisX * pitchAxisX + pitchAxisY * pitchAxisY + pitchAxisZ * pitchAxisZ;
			if (pitchAxisLengthSquared > 0.f)
			{
				float pitchAxisLength = sqrtf(pitchAxisLengthSquared);
				float lengthReciprocal = 1.f / pitchAxisLength;
				pitchAxisX *= lengthReciprocal;
				pitchAxisY *= lengthReciprocal;
				pitchAxisZ *= lengthReciprocal;

				// get global pitch factor (dot)
				gyroY = -(pitchAxisX * inGyroX + pitchAxisY * inGyroY + pitchAxisZ * inGyroZ);
				// by the way, pinch it towards the nonsense limit
				gyroY *= sideReduction;

				if (gyroSpace == GyroSpace::WORLD_LEAN)
				{
					// world roll axis is cross (yaw, pitch)
					float rollAxisX = pitchAxisY * normGravZ - pitchAxisZ * normGravY;
					float rollAxisY = pitchAxisZ * normGravX - pitchAxisX * normGravZ;
					float rollAxisZ = pitchAxisX * normGravY - pitchAxisY * normGravX;

					// normalize
					float rollAxisLengthSquared = rollAxisX * rollAxisX + rollAxisY * rollAxisY + rollAxisZ * rollAxisZ;
					if (rollAxisLengthSquared > 0.f)
					{
						float rollAxisLength = sqrtf(rollAxisLengthSquared);
						lengthReciprocal = 1.f / rollAxisLength;
						rollAxisX *= lengthReciprocal;
						rollAxisY *= lengthReciprocal;
						rollAxisZ *= lengthReciprocal;

						// get global roll factor (dot)
						gyroX = rollAxisX * inGyroX + rollAxisY * inGyroY + rollAxisZ * inGyroZ;
						// by the way, pinch because we rely on a good pitch vector here
						gyroX *= sideReduction;
					}
				}
			}

			if (gyroSpace == GyroSpace::WORLD_TURN)
			{
				gyroX += worldYaw;
			}
		}
	}
	float gyroLength = sqrt(gyroX * gyroX + gyroY * gyroY);
	// do gyro smoothing
	// convert gyro smooth time to number of samples
	auto numGyroSamples = jc->getSetting(SettingID::GYRO_SMOOTH_TIME) * 1000.f / tick_time.get();
	if (numGyroSamples < 1)
		numGyroSamples = 1; // need at least 1 sample
	auto threshold = jc->getSetting(SettingID::GYRO_SMOOTH_THRESHOLD);
	jc->GetSmoothedGyro(gyroX, gyroY, gyroLength, threshold / 2.0f, threshold, int(numGyroSamples), gyroX, gyroY);
	//COUT << "%d Samples for threshold: %0.4f\n", numGyroSamples, gyro_smooth_threshold * maxSmoothingSamples);

	// now, honour gyro_cutoff_speed
	gyroLength = sqrt(gyroX * gyroX + gyroY * gyroY);
	auto speed = jc->getSetting(SettingID::GYRO_CUTOFF_SPEED);
	auto recovery = jc->getSetting(SettingID::GYRO_CUTOFF_RECOVERY);
	if (recovery > speed)
	{
		// we can use gyro_cutoff_speed
		float gyroIgnoreFactor = (gyroLength - speed) / (recovery - speed);
		if (gyroIgnoreFactor < 1.0f)
		{
			if (gyroIgnoreFactor <= 0.0f)
			{
				gyroX = gyroY = gyroLength = 0.0f;
			}
			else
			{
				gyroX *= gyroIgnoreFactor;
				gyroY *= gyroIgnoreFactor;
				gyroLength *= gyroIgnoreFactor;
			}
		}
	}
	else if (speed > 0.0f && gyroLength < speed)
	{
		// gyro_cutoff_recovery is something weird, so we just do a hard threshold
		gyroX = gyroY = gyroLength = 0.0f;
	}
	
	// Handle buttons before GYRO because some of them may affect the value of blockGyro
	auto gyro = jc->getSetting<GyroSettings>(SettingID::GYRO_ON); // same result as getting GYRO_OFF
	switch (gyro.ignore_mode)
	{
	case GyroIgnoreMode::BUTTON:
		blockGyro = gyro.always_off ^ jc->IsPressed(gyro.button);
		break;
	case GyroIgnoreMode::LEFT_STICK:
		{
			float leftX = jsl->GetLeftX(jc->handle);
			float leftY = jsl->GetLeftY(jc->handle);
			float leftLength = sqrtf(leftX * leftX + leftY * leftY);
		    float deadzoneInner = jc->getSetting(SettingID::LEFT_STICK_DEADZONE_INNER);
		    float deadzoneOuter = jc->getSetting(SettingID::LEFT_STICK_DEADZONE_OUTER);
			leftAny = false;
			switch (jc->getSetting<StickMode>(SettingID::LEFT_STICK_MODE))
			{
			case StickMode::AIM:
				leftAny = leftLength > deadzoneInner;
				break;
			case StickMode::FLICK:
				leftAny = leftLength > (1.f - deadzoneOuter);
				break;
			case StickMode::LEFT_STICK:
			case StickMode::RIGHT_STICK:
				leftAny = leftLength > jc->getSetting(SettingID::LEFT_STICK_UNDEADZONE_INNER);
				break;
		    case StickMode::NO_MOUSE:
		    case StickMode::INNER_RING:
		    case StickMode::OUTER_RING:
				{
					jc->processDeadZones(leftX, leftY, deadzoneInner, deadzoneOuter);
				    float absX = abs(leftX);
				    float absY = abs(leftY);
				    bool left = leftX < -0.5f * absY;
				    bool right = leftX > 0.5f * absY;
				    bool down = leftY < -0.5f * absX;
				    bool up = leftY > 0.5f * absX;
				    leftAny = left || right || up || down;
				}
			    break;
			default:
				break;
			}
			blockGyro = (gyro.always_off ^ leftAny);
		}
		break;
	case GyroIgnoreMode::RIGHT_STICK:
	    {
		    float rightX = jsl->GetRightX(jc->handle);
		    float rightY = jsl->GetRightY(jc->handle);
		    float rightLength = sqrtf(rightX * rightX + rightY * rightY);
		    float deadzoneInner = jc->getSetting(SettingID::RIGHT_STICK_DEADZONE_INNER);
		    float deadzoneOuter = jc->getSetting(SettingID::RIGHT_STICK_DEADZONE_OUTER);
		    rightAny = false;
		    switch (jc->getSetting<StickMode>(SettingID::RIGHT_STICK_MODE))
		    {
		    case StickMode::AIM:
			    rightAny = rightLength > deadzoneInner;
			    break;
		    case StickMode::FLICK:
			    rightAny = rightLength > (1.f - deadzoneOuter);
			    break;
		    case StickMode::LEFT_STICK:
		    case StickMode::RIGHT_STICK:
			    rightAny = rightLength > jc->getSetting(SettingID::RIGHT_STICK_UNDEADZONE_INNER);
			    break;
		    case StickMode::NO_MOUSE:
		    case StickMode::INNER_RING:
		    case StickMode::OUTER_RING:
		    {
			    jc->processDeadZones(rightX, rightY, deadzoneInner, deadzoneOuter);
			    float absX = abs(rightX);
			    float absY = abs(rightY);
			    bool left = rightX < -0.5f * absY;
			    bool right = rightX > 0.5f * absY;
			    bool down = rightY < -0.5f * absX;
			    bool up = rightY > 0.5f * absX;
			    rightAny = left || right || up || down;
		    }
		    break;
		    default:
			    break;
		    }
		    blockGyro = (gyro.always_off ^ rightAny);
	    }
		break;
	}
	float gyro_x_sign_to_use = jc->getSetting(SettingID::GYRO_AXIS_X);
	float gyro_y_sign_to_use = jc->getSetting(SettingID::GYRO_AXIS_Y);

	bool trackball_x_pressed = false;
	bool trackball_y_pressed = false;

	// Apply gyro modifiers in the queue from oldest to newest (thus giving priority to most recent)
	for (auto pair : jc->_context->gyroActionQueue)
	{
		if (pair.second.code == GYRO_ON_BIND)
			blockGyro = false;
		else if (pair.second.code == GYRO_OFF_BIND)
			blockGyro = true;
		else if (pair.second.code == GYRO_INV_X)
			gyro_x_sign_to_use = jc->getSetting(SettingID::GYRO_AXIS_X) * -1; // Intentionally don't support multiple inversions
		else if (pair.second.code == GYRO_INV_Y)
			gyro_y_sign_to_use = jc->getSetting(SettingID::GYRO_AXIS_Y) * -1; // Intentionally don't support multiple inversions
		else if (pair.second.code == GYRO_INVERT)
		{
			// Intentionally don't support multiple inversions
			gyro_x_sign_to_use = jc->getSetting(SettingID::GYRO_AXIS_X) * -1;
			gyro_y_sign_to_use = jc->getSetting(SettingID::GYRO_AXIS_Y) * -1;
		}
		else if (pair.second.code == GYRO_TRACK_X)
			trackball_x_pressed = true;
		else if (pair.second.code == GYRO_TRACK_Y)
			trackball_y_pressed = true;
		else if (pair.second.code == GYRO_TRACKBALL)
		{
			trackball_x_pressed = true;
			trackball_y_pressed = true;
		}
	}

	float decay = exp2f(-deltaTime * jc->getSetting(SettingID::TRACKBALL_DECAY));
	int maxTrackballSamples = max(1, min(jc->numLastGyroSamples, (int)(1.f / deltaTime * 0.125f)));

	if (!trackball_x_pressed && !trackball_y_pressed)
	{
		jc->lastGyroAbsX = abs(gyroX);
		jc->lastGyroAbsY = abs(gyroY);
	}

	if (!trackball_x_pressed)
	{
		int gyroSampleIndex = jc->lastGyroIndexX = (jc->lastGyroIndexX + 1) % maxTrackballSamples;
		jc->lastGyroX[gyroSampleIndex] = gyroX;
	}
	else
	{
		float lastGyroX = 0.f;
		for (int gyroAverageIdx = 0; gyroAverageIdx < maxTrackballSamples; gyroAverageIdx++)
		{
			lastGyroX += jc->lastGyroX[gyroAverageIdx];
			jc->lastGyroX[gyroAverageIdx] *= decay;
		}
		lastGyroX /= maxTrackballSamples;
		float lastGyroAbsX = abs(lastGyroX);
		if (lastGyroAbsX > jc->lastGyroAbsX)
		{
			lastGyroX *= jc->lastGyroAbsX / lastGyroAbsX;
		}
		gyroX = lastGyroX;
	}
	if (!trackball_y_pressed)
	{
		int gyroSampleIndex = jc->lastGyroIndexY = (jc->lastGyroIndexY + 1) % maxTrackballSamples;
		jc->lastGyroY[gyroSampleIndex] = gyroY;
	}
	else
	{
		float lastGyroY = 0.f;
		for (int gyroAverageIdx = 0; gyroAverageIdx < maxTrackballSamples; gyroAverageIdx++)
		{
			lastGyroY += jc->lastGyroY[gyroAverageIdx];
			jc->lastGyroY[gyroAverageIdx] *= decay;
		}
		lastGyroY /= maxTrackballSamples;
		float lastGyroAbsY = abs(lastGyroY);
		if (lastGyroAbsY > jc->lastGyroAbsY)
		{
			lastGyroY *= jc->lastGyroAbsY / lastGyroAbsY;
		}
		gyroY = lastGyroY;
	}

	if (blockGyro)
	{
		gyroX = 0;
		gyroY = 0;
	}

	float camSpeedX = 0.0f;
	float camSpeedY = 0.0f;

	float gyroXVelocity = gyroX * gyro_x_sign_to_use;
	float gyroYVelocity = gyroY * gyro_y_sign_to_use;

	std::pair<float, float> lowSensXY = jc->getSetting<FloatXY>(SettingID::MIN_GYRO_SENS);
	std::pair<float, float> hiSensXY = jc->getSetting<FloatXY>(SettingID::MAX_GYRO_SENS);

	// apply calibration factor
	// get input velocity
	float magnitude = sqrt(gyroX * gyroX + gyroY * gyroY);
	// COUT << "Gyro mag: " << setprecision(4) << magnitude << endl;
	// calculate position on minThreshold to maxThreshold scale
	float minThreshold = jc->getSetting(SettingID::MIN_GYRO_THRESHOLD);
	float maxThreshold = jc->getSetting(SettingID::MAX_GYRO_THRESHOLD);
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
	gyroXVelocity *= lowSensXY.first * (1.0f - newSensitivity) + hiSensXY.first * newSensitivity;
	gyroYVelocity *= lowSensXY.second * (1.0f - newSensitivity) + hiSensXY.second * newSensitivity;

	jc->gyroXVelocity = gyroXVelocity;
	jc->gyroYVelocity = gyroYVelocity;

	jc->time_now = std::chrono::steady_clock::now();

	// sticks!
	jc->processed_gyro_stick = false;
	ControllerOrientation controllerOrientation = jc->getSetting<ControllerOrientation>(SettingID::CONTROLLER_ORIENTATION);
	// account for os mouse speed and convert from radians to degrees because gyro reports in degrees per second
	float mouseCalibrationFactor = 180.0f / PI / os_mouse_speed;
	if (jc->controller_split_type != JS_SPLIT_TYPE_RIGHT)
	{
		// let's do these sticks... don't want to constantly send input, so we need to compare them to last time
		auto axisSign = jc->getSetting<AxisSignPair>(SettingID::LEFT_STICK_AXIS);
		float calX = jsl->GetLeftX(jc->handle) * float(axisSign.first);
		float calY = jsl->GetLeftY(jc->handle) * float(axisSign.second);

		processStick(jc, calX, calY, jc->lastLX, jc->lastLY, jc->getSetting(SettingID::LEFT_STICK_DEADZONE_INNER), jc->getSetting(SettingID::LEFT_STICK_DEADZONE_OUTER),
		  jc->getSetting<RingMode>(SettingID::LEFT_RING_MODE), jc->getSetting<StickMode>(SettingID::LEFT_STICK_MODE),
		  ButtonID::LRING, ButtonID::LLEFT, ButtonID::LRIGHT, ButtonID::LUP, ButtonID::LDOWN, controllerOrientation,
		  mouseCalibrationFactor, deltaTime, jc->left_acceleration, jc->left_last_cal, jc->is_flicking_left, jc->ignore_left_stick_mode, leftAny, lockMouse, camSpeedX, camSpeedY, &jc->left_scroll);

		jc->lastLX = calX;
		jc->lastLY = calY;
	}

	if (jc->controller_split_type != JS_SPLIT_TYPE_LEFT)
	{
		auto axisSign = jc->getSetting<AxisSignPair>(SettingID::RIGHT_STICK_AXIS);
		float calX = jsl->GetRightX(jc->handle) * float(axisSign.first);
		float calY = jsl->GetRightY(jc->handle) * float(axisSign.second);

		processStick(jc, calX, calY, jc->lastRX, jc->lastRY, jc->getSetting(SettingID::RIGHT_STICK_DEADZONE_INNER), jc->getSetting(SettingID::RIGHT_STICK_DEADZONE_OUTER),
		  jc->getSetting<RingMode>(SettingID::RIGHT_RING_MODE), jc->getSetting<StickMode>(SettingID::RIGHT_STICK_MODE),
		  ButtonID::RRING, ButtonID::RLEFT, ButtonID::RRIGHT, ButtonID::RUP, ButtonID::RDOWN, controllerOrientation,
		  mouseCalibrationFactor, deltaTime, jc->right_acceleration, jc->right_last_cal, jc->is_flicking_right, jc->ignore_right_stick_mode, rightAny, lockMouse, camSpeedX, camSpeedY, &jc->right_scroll);

		jc->lastRX = calX;
		jc->lastRY = calY;
	}

	if (jc->controller_split_type == JS_SPLIT_TYPE_FULL ||
	  (jc->controller_split_type & (int)jc->getSetting<JoyconMask>(SettingID::JOYCON_MOTION_MASK)) == 0)
	{
		Quat neutralQuat = Quat(jc->neutralQuatW, jc->neutralQuatX, jc->neutralQuatY, jc->neutralQuatZ);
		Vec grav = Vec(inGravX, inGravY, inGravZ) * neutralQuat.Inverse();

		float lastCalX = jc->lastMotionStickX;
		float lastCalY = jc->lastMotionStickY;
		// use gravity vector deflection
		auto axisSign = jc->getSetting<AxisSignPair>(SettingID::MOTION_STICK_AXIS);
		float calX = grav.x * float(axisSign.first);
		float calY = -grav.z * float(axisSign.second);
		float gravLength2D = sqrtf(grav.x * grav.x + grav.z * grav.z);
		float gravStickDeflection = atan2f(gravLength2D, -grav.y) / PI;
		if (gravLength2D > 0)
		{
			calX *= gravStickDeflection / gravLength2D;
			calY *= gravStickDeflection / gravLength2D;
		}

		processStick(jc, calX, calY, jc->lastMotionStickX, jc->lastMotionStickY, jc->getSetting(SettingID::MOTION_DEADZONE_INNER) / 180.f, jc->getSetting(SettingID::MOTION_DEADZONE_OUTER) / 180.f,
		  jc->getSetting<RingMode>(SettingID::MOTION_RING_MODE), jc->getSetting<StickMode>(SettingID::MOTION_STICK_MODE),
		  ButtonID::MRING, ButtonID::MLEFT, ButtonID::MRIGHT, ButtonID::MUP, ButtonID::MDOWN, controllerOrientation,
		  mouseCalibrationFactor, deltaTime, jc->motion_stick_acceleration, jc->motion_last_cal, jc->is_flicking_motion, jc->ignore_motion_stick_mode, motionAny, lockMouse, camSpeedX, camSpeedY, nullptr);

		jc->lastMotionStickX = calX;
		jc->lastMotionStickY = calY;

		float gravLength3D = grav.Length();
		if (gravLength3D > 0)
		{
			float gravSideDir;
			switch (controllerOrientation)
			{
			case ControllerOrientation::FORWARD:
				gravSideDir = grav.x;
				break;
			case ControllerOrientation::LEFT:
				gravSideDir = grav.z;
				break;
			case ControllerOrientation::RIGHT:
				gravSideDir = -grav.z;
				break;
			case ControllerOrientation::BACKWARD:
				gravSideDir = -grav.x;
				break;
			}
			float gravDirX = gravSideDir / gravLength3D;
			float sinLeanThreshold = sin(jc->getSetting(SettingID::LEAN_THRESHOLD) * PI / 180.f);
			jc->handleButtonChange(ButtonID::LEAN_LEFT, gravDirX < -sinLeanThreshold);
			jc->handleButtonChange(ButtonID::LEAN_RIGHT, gravDirX > sinLeanThreshold);

			// motion stick can be set to control steering by leaning
			StickMode motionStickMode = jc->getSetting<StickMode>(SettingID::MOTION_STICK_MODE);
			if (jc->_context->_vigemController && (motionStickMode == StickMode::LEFT_STEER_X || motionStickMode == StickMode::RIGHT_STEER_X))
			{
				bool isLeft = motionStickMode == StickMode::LEFT_STEER_X;
				float leanAngle = asinf(clamp(gravDirX, -1.f, 1.f)) * 180.f / PI;
				float leanSign = leanAngle < 0.f ? -1.f : 1.f;
				float absLeanAngle = abs(leanAngle);
				if (grav.y > 0.f)
				{
					absLeanAngle = 180.f - absLeanAngle;
				}
				float motionDZInner = jc->getSetting(SettingID::MOTION_DEADZONE_INNER);
				float motionDZOuter = jc->getSetting(SettingID::MOTION_DEADZONE_OUTER);
				float remappedLeanAngle = pow(clamp((absLeanAngle - motionDZInner) / (180.f - motionDZOuter - motionDZInner), 0.f, 1.f), jc->getSetting(SettingID::STICK_POWER));

				// now actually convert to output stick value, taking deadzones and power curve into account
				float undeadzoneInner, undeadzoneOuter, unpower;
				if (isLeft)
				{
					undeadzoneInner = jc->getSetting(SettingID::LEFT_STICK_UNDEADZONE_INNER);
					undeadzoneOuter = jc->getSetting(SettingID::LEFT_STICK_UNDEADZONE_OUTER);
					unpower = jc->getSetting(SettingID::LEFT_STICK_UNPOWER);
				}
				else
				{
					undeadzoneInner = jc->getSetting(SettingID::RIGHT_STICK_UNDEADZONE_INNER);
					undeadzoneOuter = jc->getSetting(SettingID::RIGHT_STICK_UNDEADZONE_OUTER);
					unpower = jc->getSetting(SettingID::RIGHT_STICK_UNPOWER);
				}

				float livezoneSize = 1.f - undeadzoneOuter - undeadzoneInner;
				if (livezoneSize > 0.f)
				{
					// unpower curve
					if (unpower != 0.f)
					{
						remappedLeanAngle = pow(remappedLeanAngle, 1.f / unpower);
					}

					if (remappedLeanAngle < 1.f)
					{
						remappedLeanAngle = undeadzoneInner + remappedLeanAngle * livezoneSize;
					}

					float signedStickValue = leanSign * remappedLeanAngle;
					//COUT << "LEAN ANGLE: " << (leanSign * absLeanAngle) << "    REMAPPED: " << (leanSign * remappedLeanAngle) << "     STICK OUT: " << signedStickValue << endl;
					jc->_context->_vigemController->setStick(signedStickValue, 0.f, isLeft);
				}
			}
		}
	}

	int buttons = jsl->GetButtons(jc->handle);
	// button mappings
	if (jc->controller_split_type != JS_SPLIT_TYPE_RIGHT)
	{
		jc->handleButtonChange(ButtonID::UP, buttons & (1 << JSOFFSET_UP));
		jc->handleButtonChange(ButtonID::DOWN, buttons & (1 << JSOFFSET_DOWN));
		jc->handleButtonChange(ButtonID::LEFT, buttons & (1 << JSOFFSET_LEFT));
		jc->handleButtonChange(ButtonID::RIGHT, buttons & (1 << JSOFFSET_RIGHT));
		jc->handleButtonChange(ButtonID::L, buttons & (1 << JSOFFSET_L));
		jc->handleButtonChange(ButtonID::MINUS, buttons & (1 << JSOFFSET_MINUS));
		// for backwards compatibility, we need need to account for the fact that SDL2 maps the touchpad button differently to SDL
		jc->handleButtonChange(ButtonID::L3, buttons & (1 << JSOFFSET_LCLICK));

		float lTrigger = jsl->GetLeftTrigger(jc->handle);
		jc->handleTriggerChange(ButtonID::ZL, ButtonID::ZLF, jc->getSetting<TriggerMode>(SettingID::ZL_MODE), lTrigger, jc->left_effect);

		bool touch = jsl->GetTouchDown(jc->handle, false) || jsl->GetTouchDown(jc->handle, true);
		switch (jc->platform_controller_type)
		{
		case JS_TYPE_DS:
			// JSL mapps mic button on the SL index
			jc->handleButtonChange(ButtonID::MIC, buttons & (1 << JSOFFSET_MIC));
			// Don't break but continue onto DS4 stuff too
		case JS_TYPE_DS4:
		{
			float triggerpos = buttons & (1 << JSOFFSET_CAPTURE) ? 1.f :
			  touch                                              ? 0.99f :
                                                                   0.f;
			jc->handleTriggerChange(ButtonID::TOUCH, ButtonID::CAPTURE, jc->getSetting<TriggerMode>(SettingID::TOUCHPAD_DUAL_STAGE_MODE), triggerpos, jc->unused_effect);
		}
		break;
		case JS_TYPE_XBOXONE_ELITE:
			jc->handleButtonChange(ButtonID::RSL, buttons & (1 << JSOFFSET_SL2)); //Xbox Elite back paddles
			jc->handleButtonChange(ButtonID::RSR, buttons & (1 << JSOFFSET_SR2));
			jc->handleButtonChange(ButtonID::LSL, buttons & (1 << JSOFFSET_SL));
			jc->handleButtonChange(ButtonID::LSR, buttons & (1 << JSOFFSET_SR));
			break;
		case JS_TYPE_XBOX_SERIES:
			jc->handleButtonChange(ButtonID::CAPTURE, buttons & (1 << JSOFFSET_CAPTURE));
			break;
		default: // Switch Pro controllers and left joycon
		{
			jc->handleButtonChange(ButtonID::CAPTURE, buttons & (1 << JSOFFSET_CAPTURE));
			jc->handleButtonChange(ButtonID::LSL, buttons & (1 << JSOFFSET_SL));
			jc->handleButtonChange(ButtonID::LSR, buttons & (1 << JSOFFSET_SR));
		}
		break;
		}
	}
	else // split type IS right
	{
		// Right joycon bumpers
		jc->handleButtonChange(ButtonID::RSL, buttons & (1 << JSOFFSET_SL));
		jc->handleButtonChange(ButtonID::RSR, buttons & (1 << JSOFFSET_SR));
	}

	if (jc->controller_split_type != JS_SPLIT_TYPE_LEFT)
	{
		jc->handleButtonChange(ButtonID::E, buttons & (1 << JSOFFSET_E));
		jc->handleButtonChange(ButtonID::S, buttons & (1 << JSOFFSET_S));
		jc->handleButtonChange(ButtonID::N, buttons & (1 << JSOFFSET_N));
		jc->handleButtonChange(ButtonID::W, buttons & (1 << JSOFFSET_W));
		jc->handleButtonChange(ButtonID::R, buttons & (1 << JSOFFSET_R));
		jc->handleButtonChange(ButtonID::PLUS, buttons & (1 << JSOFFSET_PLUS));
		jc->handleButtonChange(ButtonID::HOME, buttons & (1 << JSOFFSET_HOME));
		jc->handleButtonChange(ButtonID::R3, buttons & (1 << JSOFFSET_RCLICK));

		float rTrigger = jsl->GetRightTrigger(jc->handle);
		jc->handleTriggerChange(ButtonID::ZR, ButtonID::ZRF, jc->getSetting<TriggerMode>(SettingID::ZR_MODE), rTrigger, jc->right_effect);
	}

	auto at = jc->getSetting<Switch>(SettingID::ADAPTIVE_TRIGGER);
	if (at == Switch::OFF)
	{
		AdaptiveTriggerSetting none;
		jsl->SetTriggerEffect(jc->handle, none, none);
	}
	else
	{
		auto leftEffect = jc->getSetting<AdaptiveTriggerSetting>(SettingID::LEFT_TRIGGER_EFFECT);
		auto rightEffect = jc->getSetting<AdaptiveTriggerSetting>(SettingID::RIGHT_TRIGGER_EFFECT);
		jsl->SetTriggerEffect(jc->handle, leftEffect.mode == AdaptiveTriggerMode::ON ? jc->left_effect : leftEffect,
										  rightEffect.mode == AdaptiveTriggerMode::ON ? jc->right_effect : rightEffect);
	}

	bool currentMicToggleState = find_if(jc->_context->activeTogglesQueue.cbegin(), jc->_context->activeTogglesQueue.cend(),
	                               [](const auto &pair) {
		                               return pair.first == ButtonID::MIC;
	                               }) != jc->_context->activeTogglesQueue.cend();
	for (auto controller : handle_to_joyshock)
	{
		jsl->SetMicLight(controller.first, currentMicToggleState ? 1 : 0);
	}

	GyroOutput gyroOutput = jc->getSetting<GyroOutput>(SettingID::GYRO_OUTPUT);
	if (!jc->processed_gyro_stick)
	{
		if (gyroOutput == GyroOutput::LEFT_STICK)
		{
			processGyroStick(jc, 0.f, 0.f, 0.f, StickMode::LEFT_STICK, false);
		}
		else if (gyroOutput == GyroOutput::RIGHT_STICK)
		{
			processGyroStick(jc, 0.f, 0.f, 0.f, StickMode::RIGHT_STICK, false);
		}
	}

	// optionally ignore the gyro of one of the joycons
	if (!lockMouse && gyroOutput == GyroOutput::MOUSE &&
	  (jc->controller_split_type == JS_SPLIT_TYPE_FULL ||
	    (jc->controller_split_type & (int)jc->getSetting<JoyconMask>(SettingID::JOYCON_GYRO_MASK)) == 0))
	{
		//COUT << "GX: %0.4f GY: %0.4f GZ: %0.4f\n", imuState.gyroX, imuState.gyroY, imuState.gyroZ);
		float mouseCalibration = jc->getSetting(SettingID::REAL_WORLD_CALIBRATION) / os_mouse_speed / jc->getSetting(SettingID::IN_GAME_SENS);
		shapedSensitivityMoveMouse(gyroXVelocity * mouseCalibration, gyroYVelocity * mouseCalibration, deltaTime, camSpeedX, -camSpeedY);
	}

	if (jc->_context->_vigemController)
	{
		jc->_context->_vigemController->update(); // Check for initialized built-in
	}
	auto newColor = jc->getSetting<Color>(SettingID::LIGHT_BAR);
	if (jc->_light_bar != newColor)
	{
		jsl->SetLightColour(jc->handle, newColor.raw);
		jc->_light_bar = newColor;
	}
	if (jc->_context->nn)
	{
		jc->_context->nn = (jc->_context->nn + 1) % 22;
	}
	jc->_context->callback_lock.unlock();
}

// https://stackoverflow.com/a/25311622/1130520 says this is why filenames obtained by fgets don't work
static void removeNewLine(char *string)
{
	char *p;
	if ((p = strchr(string, '\n')) != NULL)
	{
		*p = '\0'; /* remove newline */
	}
}

// https://stackoverflow.com/a/4119881/1130520 gives us case insensitive equality
static bool iequals(const string &a, const string &b)
{
	return equal(a.begin(), a.end(),
	  b.begin(), b.end(),
	  [](char a, char b) {
		  return tolower(a) == tolower(b);
	  });
}

bool AutoLoadPoll(void *param)
{
	auto registry = reinterpret_cast<CmdRegistry *>(param);
	static string lastModuleName;
	string windowTitle, windowModule;
	tie(windowModule, windowTitle) = GetActiveWindowName();
	if (!windowModule.empty() && windowModule != lastModuleName && windowModule.compare("JoyShockMapper.exe") != 0)
	{
		lastModuleName = windowModule;
		string path(AUTOLOAD_FOLDER());
		auto files = ListDirectory(path);
		auto noextmodule = windowModule.substr(0, windowModule.find_first_of('.'));
		COUT_INFO << "[AUTOLOAD] \"" << windowTitle << "\" in focus: "; // looking for config : " , );
		bool success = false;
		for (auto file : files)
		{
			auto noextconfig = file.substr(0, file.find_first_of('.'));
			if (iequals(noextconfig, noextmodule))
			{
				COUT_INFO << "loading \"AutoLoad\\" << noextconfig << ".txt\"." << endl;
				loading_lock.lock();
				registry->processLine(path + file);
				loading_lock.unlock();
				COUT_INFO << "[AUTOLOAD] Loading completed" << endl;
				success = true;
				break;
			}
		}
		if (!success)
		{
			COUT_INFO << "create ";
			COUT << "AutoLoad\\" << noextmodule << ".txt";
			COUT_INFO << " to autoload for this application." << endl;
		}
	}
	return true;
}

bool MinimizePoll(void *param)
{
	if (isConsoleMinimized())
	{
		HideConsole();
	}
	return true;
}

void beforeShowTrayMenu()
{
	if (!tray || !*tray)
		CERR << "ERROR: Cannot create tray item." << endl;
	else
	{
		tray->ClearMenuMap();
		tray->AddMenuItem(U("Show Console"), &ShowConsole);
		tray->AddMenuItem(U("Reconnect controllers"), []() {
			WriteToConsole("RECONNECT_CONTROLLERS");
		});
		tray->AddMenuItem(
		  U("AutoLoad"), [](bool isChecked) {
			  autoloadSwitch = isChecked ? Switch::ON : Switch::OFF;
		  },
		  bind(&PollingThread::isRunning, autoLoadThread.get()));

		if (whitelister && whitelister->IsAvailable())
		{
			tray->AddMenuItem(
			  U("Whitelist"), [](bool isChecked) {
				  isChecked ?
                    do_WHITELIST_ADD() :
                    do_WHITELIST_REMOVE();
			  },
			  bind(&Whitelister::operator bool, whitelister.get()));
		}
		tray->AddMenuItem(
		  U("Calibrate all devices"), [](bool isChecked) { isChecked ?
                                                             WriteToConsole("RESTART_GYRO_CALIBRATION") :
                                                             WriteToConsole("FINISH_GYRO_CALIBRATION"); }, []() { return devicesCalibrating; });

		string autoloadFolder{ AUTOLOAD_FOLDER() };
		for (auto file : ListDirectory(autoloadFolder.c_str()))
		{
			string fullPathName = ".\\AutoLoad\\" + file;
			auto noext = file.substr(0, file.find_last_of('.'));
			tray->AddMenuItem(U("AutoLoad folder"), UnicodeString(noext.begin(), noext.end()), [fullPathName] {
				WriteToConsole(string(fullPathName.begin(), fullPathName.end()));
				autoLoadThread->Stop();
			});
		}
		std::string gyroConfigsFolder{ GYRO_CONFIGS_FOLDER() };
		for (auto file : ListDirectory(gyroConfigsFolder.c_str()))
		{
			string fullPathName = ".\\GyroConfigs\\" + file;
			auto noext = file.substr(0, file.find_last_of('.'));
			tray->AddMenuItem(U("GyroConfigs folder"), UnicodeString(noext.begin(), noext.end()), [fullPathName] {
				WriteToConsole(string(fullPathName.begin(), fullPathName.end()));
				autoLoadThread->Stop();
			});
		}
		tray->AddMenuItem(U("Calculate RWC"), []() {
			WriteToConsole("CALCULATE_REAL_WORLD_CALIBRATION");
			ShowConsole();
		});
		tray->AddMenuItem(
		  U("Hide when minimized"), [](bool isChecked) {
			  hide_minimized = isChecked ? Switch::ON : Switch::OFF;
			  if (!isChecked)
				  UnhideConsole();
		  },
		  bind(&PollingThread::isRunning, minimizeThread.get()));
		tray->AddMenuItem(U("Quit"), []() {
			WriteToConsole("QUIT");
		});
	}
}

// Perform all cleanup tasks when JSM is exiting
void CleanUp()
{
	if (tray)
	{
		tray->Hide();
	}
	HideConsole();
	jsl->DisconnectAndDisposeAll();
	handle_to_joyshock.clear(); // Destroy Vigem Gamepads
	ReleaseConsole();
}

int filterClampByte(int current, int next)
{
	return max(0, min(0xff, next));
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
      next :
      current;
}

template<typename E, E invalid>
E filterInvalidValue(E current, E next)
{
	return next != invalid ? next : current;
}

AdaptiveTriggerSetting filterInvalidValue(AdaptiveTriggerSetting current, AdaptiveTriggerSetting next)
{
	return next.mode != AdaptiveTriggerMode::INVALID ? next : current;
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
      next :
      current;
}

AxisSignPair filterSignPair(AxisSignPair current, AxisSignPair next)
{
	return next.first != AxisMode::INVALID && next.second != AxisMode::INVALID ?
      next :
      current;
}

float filterHoldPressDelay(float c, float next)
{
	if (next <= sim_press_window)
	{
		CERR << SettingID::HOLD_PRESS_TIME << " can only be set to a value higher than " << SettingID::SIM_PRESS_WINDOW << " which is " << sim_press_window << "ms." << endl;
		return c;
	}
	return next;
}

float filterTickTime(float c, float next)
{
	return max(1.f, min(100.f, round(next)));
}

Mapping filterMapping(Mapping current, Mapping next)
{
	if (next.hasViGEmBtn())
	{
		if (virtual_controller.get() == ControllerScheme::NONE)
		{
			COUT_WARN << "Before using this mapping, you need to set VIRTUAL_CONTROLLER." << endl;
			return current;
		}
		for (auto &js : handle_to_joyshock)
		{
			if (js.second->CheckVigemState() == false)
				return current;
		}
	}
	return next.isValid() ? next : current;
}

TriggerMode filterTriggerMode(TriggerMode current, TriggerMode next)
{
	// With SDL, I'm not sure if we have a reliable way to check if the device has analog or digital triggers. There's a function to query them, but I don't know if it works with the devices with custom readers (Switch, PS)
	/*	for (auto &js : handle_to_joyshock)
	{
		if (jsl->GetControllerType(js.first) != JS_TYPE_DS4 && next != TriggerMode::NO_FULL)
		{
			COUT_WARN << "WARNING: Dual Stage Triggers are only valid on analog triggers. Full pull bindings will be ignored on non DS4 controllers." << endl;
			break;
		}
	}
*/
	if (next == TriggerMode::X_LT || next == TriggerMode::X_RT)
	{
		if (virtual_controller.get() == ControllerScheme::NONE)
		{
			COUT_WARN << "Before using this trigger mode, you need to set VIRTUAL_CONTROLLER." << endl;
			return current;
		}
		for (auto &js : handle_to_joyshock)
		{
			if (js.second->CheckVigemState() == false)
				return current;
		}
	}
	return filterInvalidValue<TriggerMode, TriggerMode::INVALID>(current, next);
}

TriggerMode filterTouchpadDualStageMode(TriggerMode current, TriggerMode next)
{
	if (next == TriggerMode::X_LT || next == TriggerMode::X_RT || next == TriggerMode::INVALID)
	{
		COUT_WARN << SettingID::TOUCHPAD_DUAL_STAGE_MODE << " doesn't support vigem analog modes." << endl;
		return current;
	}
	return next;
}

StickMode filterMotionStickMode(StickMode current, StickMode next)
{
	if (next >= StickMode::LEFT_STICK && next <= StickMode::RIGHT_WIND_X)
	{
		if (virtual_controller.get() == ControllerScheme::NONE)
		{
			COUT_WARN << "Before using this stick mode, you need to set VIRTUAL_CONTROLLER." << endl;
			return current;
		}
		for (auto &js : handle_to_joyshock)
		{
			if (js.second->CheckVigemState() == false)
				return current;
		}
	}
	return filterInvalidValue<StickMode, StickMode::INVALID>(current, next);
}

StickMode filterStickMode(StickMode current, StickMode next)
{
	// these modes are only available to motion stick
	if (next == StickMode::LEFT_STEER_X || next == StickMode::RIGHT_STEER_X)
	{
		COUT_WARN << "This mode is only available for MOTION_STICK_MODE." << endl;
		return current;
	}
	return filterMotionStickMode(current, next);
}

GyroOutput filterGyroOutput(GyroOutput current, GyroOutput next)
{
	if (next == GyroOutput::LEFT_STICK || next == GyroOutput::RIGHT_STICK)
	{
		if (virtual_controller.get() == ControllerScheme::NONE)
		{
			COUT_WARN << "Before using this gyro mode, you need to set VIRTUAL_CONTROLLER." << endl;
			return current;
		}
		for (auto& js : handle_to_joyshock)
		{
			if (js.second->CheckVigemState() == false)
				return current;
		}
	}
	return filterInvalidValue<GyroOutput, GyroOutput::INVALID>(current, next);
}

void UpdateRingModeFromStickMode(JSMVariable<RingMode> *stickRingMode, const StickMode &newValue)
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

ControllerScheme UpdateVirtualController(ControllerScheme prevScheme, ControllerScheme nextScheme)
{
	bool success = true;
	for (auto &js : handle_to_joyshock)
	{
		if (!js.second->_context->_vigemController ||
		  js.second->_context->_vigemController->getType() != nextScheme)
		{
			if (nextScheme == ControllerScheme::NONE)
			{
				js.second->_context->_vigemController.reset(nullptr);
			}
			else
			{
				js.second->_context->_vigemController.reset(Gamepad::getNew(nextScheme, bind(&JoyShock::handleViGEmNotification, js.second.get(), placeholders::_1, placeholders::_2, placeholders::_3)));
				success &= js.second->_context->_vigemController && js.second->_context->_vigemController->isInitialized();
			}
		}
	}
	return success ? nextScheme : prevScheme;
}

void OnVirtualControllerChange(const ControllerScheme &newScheme)
{
	for (auto &js : handle_to_joyshock)
	{
		// Display an error message if any vigem is no good.
		if (!js.second->CheckVigemState())
		{
			break;
		}
	}
}

void RefreshAutoLoadHelp(JSMAssignment<Switch> *autoloadCmd)
{
	stringstream ss;
	ss << "AUTOLOAD will attempt load a file from the following folder when a window with a matching executable name enters focus:" << endl
	   << AUTOLOAD_FOLDER();
	autoloadCmd->SetHelp(ss.str());
}

void OnNewGridDimensions(CmdRegistry *registry, const FloatXY &newGridDims)
{
	_ASSERT_EXPR(registry, U("You forgot to bind the command registry properly!"));
	auto numberOfButtons = size_t(newGridDims.first * newGridDims.second);

	if (numberOfButtons < grid_mappings.size())
	{
		// Remove all extra touch button commands
		bool successfulRemove = true;
		for (int id = FIRST_TOUCH_BUTTON + numberOfButtons; successfulRemove; ++id)
		{
			string name(magic_enum::enum_name(*magic_enum::enum_cast<ButtonID>(id)));
			successfulRemove = registry->Remove(name);
		}

		// For all joyshocks, remove extra touch DigitalButtons
		for (auto &js : handle_to_joyshock)
		{
			lock_guard guard(js.second->_context->callback_lock);
			js.second->updateGridSize();
		}

		// Remove extra touch button variables
		while (grid_mappings.size() > numberOfButtons)
			grid_mappings.pop_back();
	}
	else if (numberOfButtons > grid_mappings.size())
	{
		// Add new touch button variables and commands
		for (int id = FIRST_TOUCH_BUTTON + int(grid_mappings.size()); grid_mappings.size() < numberOfButtons; ++id)
		{
			JSMButton touchButton(*magic_enum::enum_cast<ButtonID>(id), Mapping::NO_MAPPING);
			touchButton.SetFilter(&filterMapping);
			grid_mappings.push_back(touchButton);
			registry->Add(new JSMAssignment<Mapping>(grid_mappings.back()));
		}

		// For all joyshocks, remove extra touch DigitalButtons
		for (auto &js : handle_to_joyshock)
		{
			lock_guard guard(js.second->_context->callback_lock);
			js.second->updateGridSize();
		}
	}
	// Else numbers are the same, possibly just reconfigured
}

void OnNewStickAxis(AxisMode newAxisMode, bool isVertical)
{
	if (isVertical)
	{
		left_stick_axis = { left_stick_axis.get()->first, newAxisMode };
		right_stick_axis = { right_stick_axis.get()->first, newAxisMode };
		motion_stick_axis = { motion_stick_axis.get()->first, newAxisMode };
		touch_stick_axis = { touch_stick_axis.get()->first, newAxisMode };
	}
	else // is horizontal
	{
		left_stick_axis = { newAxisMode, left_stick_axis.get()->second };
		right_stick_axis = { newAxisMode, right_stick_axis.get()->second };
		motion_stick_axis = { newAxisMode, motion_stick_axis.get()->second };
		touch_stick_axis = { newAxisMode, touch_stick_axis.get()->second };
	}
}

class GyroSensAssignment : public JSMAssignment<FloatXY>
{
public:
	GyroSensAssignment(SettingID id, JSMSetting<FloatXY> &gyroSens)
	  : JSMAssignment(magic_enum::enum_name(id).data(), string(magic_enum::enum_name(gyroSens._id)), gyroSens)
	{
		// min and max gyro sens already have a listener
		gyroSens.RemoveOnChangeListener(_listenerId);
	}
};

class StickDeadzoneAssignment : public JSMAssignment<float>
{
public:
	StickDeadzoneAssignment(SettingID id, JSMSetting<float> &stickDeadzone)
	  : JSMAssignment(magic_enum::enum_name(id).data(), string(magic_enum::enum_name(stickDeadzone._id)), stickDeadzone)
	{
		// min and max gyro sens already have a listener
		stickDeadzone.RemoveOnChangeListener(_listenerId);
	}
};

class GyroButtonAssignment : public JSMAssignment<GyroSettings>
{
protected:
	const bool _always_off;
	const ButtonID _chordButton;

	virtual void DisplayCurrentValue() override
	{
		GyroSettings value(_var);
		if (_chordButton > ButtonID::NONE)
		{
			COUT << _chordButton << ',';
		}
		COUT << (value.always_off ? string("GYRO_ON") : string("GYRO_OFF")) << " = " << value << endl;
	}

	virtual GyroSettings ReadValue(stringstream &in) override
	{
		GyroSettings value;
		value.always_off = _always_off; // Added line from DefaultParser
		in >> value;
		return value;
	}

	virtual void DisplayNewValue(const GyroSettings &value) override
	{
		if (_chordButton > ButtonID::NONE)
		{
			COUT << _chordButton << ',';
		}
		COUT << (value.always_off ? string("GYRO_ON") : string("GYRO_OFF")) << " has been set to " << value << endl;
	}

public:
	GyroButtonAssignment(in_string name, in_string displayName, JSMVariable<GyroSettings> &setting, bool always_off, ButtonID chord = ButtonID::NONE)
	  : JSMAssignment(name, name, setting, true)
	  , _always_off(always_off)
	  , _chordButton(chord)
	{
	}

	GyroButtonAssignment(SettingID id, bool always_off)
	  : GyroButtonAssignment(magic_enum::enum_name(id).data(), magic_enum::enum_name(id).data(), gyro_settings, always_off)
	{
	}

	GyroButtonAssignment *SetListener()
	{
		_listenerId = _var.AddOnChangeListener(bind(&GyroButtonAssignment::DisplayNewValue, this, placeholders::_1));
		NONAME.push_back(NONAME[0] ^ 0x05);
		NONAME.push_back(NONAME[1] ^ 14);
		return this;
	}

	virtual unique_ptr<JSMCommand> GetModifiedCmd(char op, in_string chord) override
	{
		auto optBtn = magic_enum::enum_cast<ButtonID>(chord);
		auto settingVar = dynamic_cast<JSMSetting<GyroSettings> *>(&_var);
		if (optBtn > ButtonID::NONE && op == ',' && settingVar)
		{
			//Create Modeshift
			string name = chord + op + _displayName;
			unique_ptr<JSMCommand> chordAssignment((new GyroButtonAssignment(_name, name, *settingVar->AtChord(*optBtn), _always_off, *optBtn))->SetListener());
			chordAssignment->SetHelp(_help)->SetParser(bind(&GyroButtonAssignment::ModeshiftParser, *optBtn, settingVar, &_parse, placeholders::_1, placeholders::_2))->SetTaskOnDestruction(bind(&JSMSetting<GyroSettings>::ProcessModeshiftRemoval, settingVar, *optBtn));
			return chordAssignment;
		}
		return JSMCommand::GetModifiedCmd(op, chord);
	}

	virtual ~GyroButtonAssignment()
	{
	}
};

class HelpCmd : public JSMMacro
{
private:
	// HELP runs the macro for each argument given to it.
	string arg; // parsed argument

	bool Parser(in_string arguments)
	{
		stringstream ss(arguments);
		ss >> arg;
		do
		{ // Run at least once with an empty arg string if there's no argument.
			_macro(this, arguments);
			ss >> arg;
		} while (!ss.fail());
		arg.clear();
		return true;
	}

	// The run function is nothing like the delegate. See how I use the bind function
	// below to hard-code the pointer parameter and the instance pointer 'this'.
	bool RunHelp(CmdRegistry *registry)
	{
		if (arg.empty())
		{
			// Show all commands
			COUT << "Here's the list of all commands." << endl;
			vector<string> list;
			registry->GetCommandList(list);
			for (auto cmd : list)
			{
				COUT_INFO << "    " << cmd << endl;
			}
			COUT << "Enter HELP [cmd1] [cmd2] ... for details on specific commands." << endl;
		}
		else if (registry->hasCommand(arg))
		{
			auto help = registry->GetHelp(arg);
			if (!help.empty())
			{
				COUT << arg << " :" << endl
				     << "    " << help << endl;
			}
			else
			{
				COUT << arg << " is not a recognized command" << endl;
			}
		}
		else
		{
			// Show all commands that include ARG
			COUT << "\"" << arg << "\" is not a command, but the following are:" << endl;
			vector<string> list;
			registry->GetCommandList(list);
			for (auto cmd : list)
			{
				auto pos = cmd.find(arg);
				if (pos != string::npos)
					COUT_INFO << "    " << cmd << endl;
			}
			COUT << "Enter HELP [cmd1] [cmd2] ... for details on specific commands." << endl;
		}
		return true;
	}

public:
	HelpCmd(CmdRegistry &reg)
	  : JSMMacro("HELP")
	{
		// Bind allows me to use instance function by hardcoding the invisible "this" parameter, and the registry pointer
		SetMacro(bind(&HelpCmd::RunHelp, this, &reg));

		// The placeholder parameter says to pass 2nd parameter of call to _parse to the 1st argument of the call to HelpCmd::Parser.
		// The first parameter is the command pointer which is not required because Parser is an instance function rather than a static one.
		SetParser(bind(&HelpCmd::Parser, this, ::placeholders::_2));
		NONAME.push_back(NONAME[2] - 1);
		NONAME.push_back(NONAME[1] & ~0x06);
	}
};

#ifdef _WIN32
int __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPWSTR cmdLine, int cmdShow)
{
	auto trayIconData = hInstance;
	int argc = 0;
	wchar_t **argv = CommandLineToArgvW(cmdLine, &argc);
	unsigned long length = 256;
	wstring wmodule(length, '\0');
	auto handle = GetCurrentProcess();
	QueryFullProcessImageNameW(handle, 0, &wmodule[0], &length);
	string module(wmodule.begin(), wmodule.begin() + length);

#else
int main(int argc, char *argv[])
{
	static_cast<void>(argc);
	static_cast<void>(argv);
	void *trayIconData = nullptr;
	string module(argv[0]);
#endif // _WIN32
	jsl.reset(JslWrapper::getNew());
	whitelister.reset(Whitelister::getNew(false));
	currentWorkingDir = GetCWD();

	grid_mappings.reserve(int(ButtonID::T25) - FIRST_TOUCH_BUTTON); // This makes sure the items will never get copied and cause crashes
	mappings.reserve(MAPPING_SIZE);
	for (int id = 0; id < MAPPING_SIZE; ++id)
	{
		JSMButton newButton(ButtonID(id), Mapping::NO_MAPPING);
		newButton.SetFilter(&filterMapping);
		mappings.push_back(newButton);
	}
	// console
	initConsole();
	COUT_BOLD << "Welcome to JoyShockMapper version " << version << '!' << endl;
	//if (whitelister) COUT << "JoyShockMapper was successfully whitelisted!" << endl;
	// Threads need to be created before listeners
	CmdRegistry commandRegistry;
	minimizeThread.reset(new PollingThread("Minimize thread", &MinimizePoll, nullptr, 1000, hide_minimized.get() == Switch::ON));          // Start by default
	autoLoadThread.reset(new PollingThread("AutoLoad thread", &AutoLoadPoll, &commandRegistry, 1000, autoloadSwitch.get() == Switch::ON)); // Start by default

	left_stick_mode.SetFilter(&filterStickMode)->AddOnChangeListener(bind(&UpdateRingModeFromStickMode, &left_ring_mode, ::placeholders::_1));
	right_stick_mode.SetFilter(&filterStickMode)->AddOnChangeListener(bind(&UpdateRingModeFromStickMode, &right_ring_mode, ::placeholders::_1));
	motion_stick_mode.SetFilter(&filterMotionStickMode)->AddOnChangeListener(bind(&UpdateRingModeFromStickMode, &motion_ring_mode, ::placeholders::_1));
	left_ring_mode.SetFilter(&filterInvalidValue<RingMode, RingMode::INVALID>);
	right_ring_mode.SetFilter(&filterInvalidValue<RingMode, RingMode::INVALID>);
	motion_ring_mode.SetFilter(&filterInvalidValue<RingMode, RingMode::INVALID>);
	mouse_x_from_gyro.SetFilter(&filterInvalidValue<GyroAxisMask, GyroAxisMask::INVALID>);
	mouse_y_from_gyro.SetFilter(&filterInvalidValue<GyroAxisMask, GyroAxisMask::INVALID>);
	gyro_settings.SetFilter([](GyroSettings current, GyroSettings next) {
		return next.ignore_mode != GyroIgnoreMode::INVALID ? next : current;
	});
	joycon_gyro_mask.SetFilter(&filterInvalidValue<JoyconMask, JoyconMask::INVALID>);
	joycon_motion_mask.SetFilter(&filterInvalidValue<JoyconMask, JoyconMask::INVALID>);
	controller_orientation.SetFilter(&filterInvalidValue<ControllerOrientation, ControllerOrientation::INVALID>);
	gyro_space.SetFilter(&filterInvalidValue<GyroSpace, GyroSpace::INVALID>);
	zlMode.SetFilter(&filterTriggerMode);
	zrMode.SetFilter(&filterTriggerMode);
	flick_snap_mode.SetFilter(&filterInvalidValue<FlickSnapMode, FlickSnapMode::INVALID>);
	min_gyro_sens.SetFilter(&filterFloatPair);
	max_gyro_sens.SetFilter(&filterFloatPair);
	min_gyro_threshold.SetFilter(&filterFloat);
	max_gyro_threshold.SetFilter(&filterFloat);
	stick_power.SetFilter(&filterFloat);
	real_world_calibration.SetFilter(&filterFloat);
	virtual_stick_calibration.SetFilter(&filterFloat);
	in_game_sens.SetFilter(bind(&fmaxf, 0.0001f, ::placeholders::_2));
	trigger_threshold.SetFilter(&filterFloat);
	left_stick_axis.SetFilter(&filterSignPair);
	right_stick_axis.SetFilter(&filterSignPair);
	motion_stick_axis.SetFilter(&filterSignPair);
	touch_stick_axis.SetFilter(&filterSignPair);
	aim_x_sign.SetFilter(&filterInvalidValue<AxisMode, AxisMode::INVALID>)->AddOnChangeListener(bind(OnNewStickAxis, placeholders::_1, false));
	aim_y_sign.SetFilter(&filterInvalidValue<AxisMode, AxisMode::INVALID>)->AddOnChangeListener(bind(OnNewStickAxis, placeholders::_1, true));
	gyro_x_sign.SetFilter(&filterInvalidValue<AxisMode, AxisMode::INVALID>);
	gyro_y_sign.SetFilter(&filterInvalidValue<AxisMode, AxisMode::INVALID>);
	flick_time.SetFilter(bind(&fmaxf, 0.0001f, ::placeholders::_2));
	flick_time_exponent.SetFilter(&filterFloat);
	gyro_smooth_time.SetFilter(bind(&fmaxf, 0.0001f, ::placeholders::_2));
	gyro_smooth_threshold.SetFilter(&filterPositive);
	gyro_cutoff_speed.SetFilter(&filterPositive);
	gyro_cutoff_recovery.SetFilter(&filterPositive);
	stick_acceleration_rate.SetFilter(&filterPositive);
	stick_acceleration_cap.SetFilter(bind(&fmaxf, 1.0f, ::placeholders::_2));
	left_stick_deadzone_inner.SetFilter(&filterClamp01);
	left_stick_deadzone_outer.SetFilter(&filterClamp01);
	flick_deadzone_angle.SetFilter(&filterPositive);
	right_stick_deadzone_inner.SetFilter(&filterClamp01);
	right_stick_deadzone_outer.SetFilter(&filterClamp01);
	motion_deadzone_inner.SetFilter(&filterPositive);
	motion_deadzone_outer.SetFilter(&filterPositive);
	angle_to_axis_deadzone_inner.SetFilter(&filterPositive);
	angle_to_axis_deadzone_outer.SetFilter(&filterPositive);
	lean_threshold.SetFilter(&filterPositive);
	mouse_ring_radius.SetFilter([](float c, float n) { return n <= screen_resolution_y ? floorf(n) : c; });
	trackball_decay.SetFilter(&filterPositive);
	screen_resolution_x.SetFilter(&filterPositive);
	screen_resolution_y.SetFilter(&filterPositive);
	// no filtering for rotate_smooth_override
	flick_snap_strength.SetFilter(&filterClamp01);
	trigger_skip_delay.SetFilter(&filterPositive);
	turbo_period.SetFilter(&filterPositive);
	sim_press_window.SetFilter(&filterPositive);
	dbl_press_window.SetFilter(&filterPositive);
	hold_press_time.SetFilter(&filterHoldPressDelay);
	tick_time.SetFilter(&filterTickTime);
	currentWorkingDir.SetFilter([](PathString current, PathString next) {
		return SetCWD(string(next)) ? next : current;
	});
	autoloadSwitch.SetFilter(&filterInvalidValue<Switch, Switch::INVALID>)->AddOnChangeListener(bind(&UpdateThread, autoLoadThread.get(), placeholders::_1));
	hide_minimized.SetFilter(&filterInvalidValue<Switch, Switch::INVALID>)->AddOnChangeListener(bind(&UpdateThread, minimizeThread.get(), placeholders::_1));
	grid_size.SetFilter([](auto current, auto next) {
		float floorX = floorf(next.x());
		float floorY = floorf(next.y());
		return floorX * floorY >= 1 && floorX * floorY <= 25 ? FloatXY{ floorX, floorY } : current;
	});
	grid_size.AddOnChangeListener(bind(&OnNewGridDimensions, &commandRegistry, placeholders::_1), true); // Call the listener now
	touchpad_mode.SetFilter(&filterInvalidValue<TouchpadMode, TouchpadMode::INVALID>);
	touch_stick_mode.SetFilter(&filterInvalidValue<StickMode, StickMode::INVALID>)->AddOnChangeListener(bind(&UpdateRingModeFromStickMode, &touch_ring_mode, ::placeholders::_1));
	touch_deadzone_inner.SetFilter(&filterPositive);
	touch_ring_mode.SetFilter(&filterInvalidValue<RingMode, RingMode::INVALID>);
	touchpad_sens.SetFilter(filterFloatPair);
	touch_stick_radius.SetFilter([](auto current, auto next) {
		return filterPositive(current, floorf(next));
	});
	virtual_controller.SetFilter(&UpdateVirtualController)->AddOnChangeListener(&OnVirtualControllerChange);
	rumble_enable.SetFilter(&filterInvalidValue<Switch, Switch::INVALID>);
    adaptive_trigger.SetFilter(&filterInvalidValue<Switch, Switch::INVALID>);
    left_trigger_effect.SetFilter(static_cast<AdaptiveTriggerSetting(*)(AdaptiveTriggerSetting, AdaptiveTriggerSetting)>(&filterInvalidValue));
    right_trigger_effect.SetFilter(static_cast<AdaptiveTriggerSetting(*)(AdaptiveTriggerSetting, AdaptiveTriggerSetting)>(&filterInvalidValue));
	scroll_sens.SetFilter(&filterFloatPair);
	touch_ds_mode.SetFilter(&filterTouchpadDualStageMode);
	right_trigger_offset.SetFilter(&filterClampByte);
	left_trigger_offset.SetFilter(&filterClampByte);
	right_trigger_range.SetFilter(&filterClampByte);
	left_trigger_range.SetFilter(&filterClampByte);
	auto_calibrate_gyro.SetFilter(&filterInvalidValue<Switch, Switch::INVALID>);
	left_stick_undeadzone_inner.SetFilter(&filterClamp01);
	left_stick_undeadzone_outer.SetFilter(&filterClamp01);
	left_stick_unpower.SetFilter(&filterFloat);
	right_stick_undeadzone_inner.SetFilter(&filterClamp01);
	right_stick_undeadzone_outer.SetFilter(&filterClamp01);
	right_stick_unpower.SetFilter(&filterFloat);
	left_stick_virtual_scale.SetFilter(&filterFloat);
	right_stick_virtual_scale.SetFilter(&filterFloat);
	wind_stick_range.SetFilter(&filterPositive);
	wind_stick_power.SetFilter(&filterPositive);
	unwind_rate.SetFilter(&filterPositive);
	gyro_output.SetFilter(&filterGyroOutput);
	flick_stick_output.SetFilter(&filterInvalidValue<GyroOutput, GyroOutput::INVALID>);

	// light_bar needs no filter or listener. The callback polls and updates the color.
	for (int i = argc - 1; i >= 0; --i)
	{
#if _WIN32
		string arg(&argv[i][0], &argv[i][wcslen(argv[i])]);
#else
		string arg = string(argv[0]);
#endif
		if (filesystem::is_directory(filesystem::status(arg)) &&
		  (currentWorkingDir = arg).compare(arg) == 0)
		{
			break;
		}
	}

	if (autoLoadThread && autoLoadThread->isRunning())
	{
		COUT << "AUTOLOAD is available. Files in ";
		COUT_INFO << AUTOLOAD_FOLDER();
		COUT << " folder will get loaded automatically when a matching application is in focus." << endl;
	}
	else
	{
		CERR << "AutoLoad is unavailable" << endl;
	}

	assert(MAPPING_SIZE == buttonHelpMap.size() && "Please update the button help map in ButtonHelp.cpp");
	for (auto &mapping : mappings) // Add all button mappings as commands
	{
		commandRegistry.Add((new JSMAssignment<Mapping>(mapping.getName(), mapping))->SetHelp(buttonHelpMap.at(mapping._id)));
	}
	// SL and SR are shorthand for two different mappings
	commandRegistry.Add(new JSMAssignment<Mapping>("SL", "LSL", mappings[(int)ButtonID::LSL], true));
	commandRegistry.Add(new JSMAssignment<Mapping>("SL", "RSL", mappings[(int)ButtonID::RSL], true));
	commandRegistry.Add(new JSMAssignment<Mapping>("SR", "LSR", mappings[(int)ButtonID::LSR], true));
	commandRegistry.Add(new JSMAssignment<Mapping>("SR", "RSR", mappings[(int)ButtonID::RSR], true));

	commandRegistry.Add((new JSMAssignment<FloatXY>(min_gyro_sens))
	                      ->SetHelp("Minimum gyro sensitivity when turning controller at or below MIN_GYRO_THRESHOLD.\nYou can assign a second value as a different vertical sensitivity."));
	commandRegistry.Add((new JSMAssignment<FloatXY>(max_gyro_sens))
	                      ->SetHelp("Maximum gyro sensitivity when turning controller at or above MAX_GYRO_THRESHOLD.\nYou can assign a second value as a different vertical sensitivity."));
	commandRegistry.Add((new JSMAssignment<float>(min_gyro_threshold))
	                      ->SetHelp("Degrees per second at and below which to apply minimum gyro sensitivity."));
	commandRegistry.Add((new JSMAssignment<float>(max_gyro_threshold))
	                      ->SetHelp("Degrees per second at and above which to apply maximum gyro sensitivity."));
	commandRegistry.Add((new JSMAssignment<float>(stick_power))
	                      ->SetHelp("Power curve for stick input when in AIM mode. 1 for linear, 0 for no curve (full strength once out of deadzone). Higher numbers make more of the stick's range appear like a very slight tilt."));
	commandRegistry.Add((new JSMAssignment<FloatXY>(stick_sens))
	                      ->SetHelp("Stick sensitivity when using classic AIM mode."));
	commandRegistry.Add((new JSMAssignment<float>(real_world_calibration))
	                      ->SetHelp("Calibration value mapping mouse values to in game degrees. This value is used for FLICK mode, and to make GYRO and stick AIM sensitivities use real world values."));
	commandRegistry.Add((new JSMAssignment<float>(virtual_stick_calibration))
	                      ->SetHelp("With a virtual controller, how fast a full tilt of the stick will turn the controller, in degrees per second. This value is used for FLICK mode with virtual controllers and to make GYRO sensitivities use real world values."));
	commandRegistry.Add((new JSMAssignment<float>(in_game_sens))
	                      ->SetHelp("Set this value to the sensitivity you use in game. It is used by stick FLICK and AIM modes as well as GYRO aiming."));
	commandRegistry.Add((new JSMAssignment<float>(trigger_threshold))
	                      ->SetHelp("Set this to a value between 0 and 1. This is the threshold at which a soft press binding is triggered. Or set the value to -1 to use hair trigger mode"));
	commandRegistry.Add((new JSMMacro("RESET_MAPPINGS"))->SetMacro(bind(&do_RESET_MAPPINGS, &commandRegistry))->SetHelp("Delete all custom bindings and reset to default.\nHOME and CAPTURE are set to CALIBRATE on both tap and hold by default."));
	commandRegistry.Add((new JSMMacro("NO_GYRO_BUTTON"))->SetMacro(bind(&do_NO_GYRO_BUTTON))->SetHelp("Enable gyro at all times, without any GYRO_OFF binding."));
	commandRegistry.Add((new JSMAssignment<StickMode>(left_stick_mode))
	                      ->SetHelp("Set a mouse mode for the left stick. Valid values are the following:\nNO_MOUSE, AIM, FLICK, FLICK_ONLY, ROTATE_ONLY, MOUSE_RING, MOUSE_AREA, OUTER_RING, INNER_RING, SCROLL_WHEEL, LEFT_STICK, RIGHT_STICK"));
	commandRegistry.Add((new JSMAssignment<StickMode>(right_stick_mode))
	                      ->SetHelp("Set a mouse mode for the right stick. Valid values are the following:\nNO_MOUSE, AIM, FLICK, FLICK_ONLY, ROTATE_ONLY, MOUSE_RING, MOUSE_AREA, OUTER_RING, INNER_RING LEFT_STICK, RIGHT_STICK"));
	commandRegistry.Add((new JSMAssignment<StickMode>(motion_stick_mode))
	                      ->SetHelp("Set a mouse mode for the motion-stick -- the whole controller is treated as a stick. Valid values are the following:\nNO_MOUSE, AIM, FLICK, FLICK_ONLY, ROTATE_ONLY, MOUSE_RING, MOUSE_AREA, OUTER_RING, INNER_RING LEFT_STICK, RIGHT_STICK"));
	commandRegistry.Add((new GyroButtonAssignment(SettingID::GYRO_OFF, false))
	                      ->SetHelp("Assign a controller button to disable the gyro when pressed."));
	commandRegistry.Add((new GyroButtonAssignment(SettingID::GYRO_ON, true))->SetListener() // Set only one listener
	                      ->SetHelp("Assign a controller button to enable the gyro when pressed."));
	commandRegistry.Add((new JSMAssignment<AxisMode>(gyro_x_sign))
	                      ->SetHelp("Set gyro X axis inversion. Valid values are the following:\nSTANDARD or 1, and INVERTED or -1"));
	commandRegistry.Add((new JSMAssignment<AxisMode>(gyro_y_sign))
	                      ->SetHelp("Set gyro Y axis inversion. Valid values are the following:\nSTANDARD or 1, and INVERTED or -1"));
	commandRegistry.Add((new JSMMacro("RECONNECT_CONTROLLERS"))->SetMacro(bind(&do_RECONNECT_CONTROLLERS, placeholders::_2))->SetHelp("Look for newly connected controllers. Specify MERGE (default) or SPLIT whether you want to consider joycons as a single or separate controllers."));
	commandRegistry.Add((new JSMMacro("COUNTER_OS_MOUSE_SPEED"))->SetMacro(bind(do_COUNTER_OS_MOUSE_SPEED))->SetHelp("JoyShockMapper will load the user's OS mouse sensitivity value to consider it in its calculations."));
	commandRegistry.Add((new JSMMacro("IGNORE_OS_MOUSE_SPEED"))->SetMacro(bind(do_IGNORE_OS_MOUSE_SPEED))->SetHelp("Disable JoyShockMapper's consideration of the the user's OS mouse sensitivity value."));
	commandRegistry.Add((new JSMAssignment<JoyconMask>(joycon_gyro_mask))
	                      ->SetHelp("When using two Joycons, select which one will be used for gyro. Valid values are the following:\nUSE_BOTH, IGNORE_LEFT, IGNORE_RIGHT, IGNORE_BOTH"));
	commandRegistry.Add((new JSMAssignment<JoyconMask>(joycon_motion_mask))
	                      ->SetHelp("When using two Joycons, select which one will be used for non-gyro motion. Valid values are the following:\nUSE_BOTH, IGNORE_LEFT, IGNORE_RIGHT, IGNORE_BOTH"));
	commandRegistry.Add((new GyroSensAssignment(SettingID::GYRO_SENS, min_gyro_sens))
	                      ->SetHelp("Sets a gyro sensitivity to use. This sets both MIN_GYRO_SENS and MAX_GYRO_SENS to the same values. You can assign a second value as a different vertical sensitivity."));
	commandRegistry.Add((new GyroSensAssignment(SettingID::GYRO_SENS, max_gyro_sens))->SetHelp(""));
	commandRegistry.Add((new JSMAssignment<float>(flick_time))
	                      ->SetHelp("Sets how long a flick takes in seconds. This value is used by stick FLICK mode."));
	commandRegistry.Add((new JSMAssignment<float>(flick_time_exponent))
	                      ->SetHelp("Applies a delta exponent to flick_time, effectively making flick speed depend on the flick angle: use 0 for no effect and 1 for linear. This value is used by stick FLICK mode."));
	commandRegistry.Add((new JSMAssignment<float>(gyro_smooth_threshold))
	                      ->SetHelp("When the controller's angular velocity is below this threshold (in degrees per second), smoothing will be applied."));
	commandRegistry.Add((new JSMAssignment<float>(gyro_smooth_time))
	                      ->SetHelp("This length of the smoothing window in seconds. Smoothing is only applied below the GYRO_SMOOTH_THRESHOLD, with a smooth transition to full smoothing."));
	commandRegistry.Add((new JSMAssignment<float>(gyro_cutoff_speed))
	                      ->SetHelp("Gyro deadzone. Gyro input will be ignored when below this angular velocity (in degrees per second). This should be a last-resort stability option."));
	commandRegistry.Add((new JSMAssignment<float>(gyro_cutoff_recovery))
	                      ->SetHelp("Below this threshold (in degrees per second), gyro sensitivity is pushed down towards zero. This can tighten and steady aim without a deadzone."));
	commandRegistry.Add((new JSMAssignment<float>(stick_acceleration_rate))
	                      ->SetHelp("When in AIM mode and the stick is fully tilted, stick sensitivity increases over time. This is a multiplier starting at 1x and increasing this by this value per second."));
	commandRegistry.Add((new JSMAssignment<float>(stick_acceleration_cap))
	                      ->SetHelp("When in AIM mode and the stick is fully tilted, stick sensitivity increases over time. This value is the maximum sensitivity multiplier."));
	commandRegistry.Add((new JSMAssignment<float>(left_stick_deadzone_inner))
	                      ->SetHelp("Defines a radius of the left stick within which all values will be ignored. This value can only be between 0 and 1 but it should be small. Stick input out of this radius will be adjusted."));
	commandRegistry.Add((new JSMAssignment<float>(left_stick_deadzone_outer))
	                      ->SetHelp("Defines a distance from the left stick's outer edge for which the stick will be considered fully tilted. This value can only be between 0 and 1 but it should be small. Stick input out of this deadzone will be adjusted."));
	commandRegistry.Add((new JSMAssignment<float>(flick_deadzone_angle))
	                      ->SetHelp("Defines a minimum angle (in degrees) for the flick to be considered a flick. Helps ignore unintentional turns when tilting the stick straight forward."));
	commandRegistry.Add((new JSMAssignment<float>(right_stick_deadzone_inner))
	                      ->SetHelp("Defines a radius of the right stick within which all values will be ignored. This value can only be between 0 and 1 but it should be small. Stick input out of this radius will be adjusted."));
	commandRegistry.Add((new JSMAssignment<float>(right_stick_deadzone_outer))
	                      ->SetHelp("Defines a distance from the right stick's outer edge for which the stick will be considered fully tilted. This value can only be between 0 and 1 but it should be small. Stick input out of this deadzone will be adjusted."));
	commandRegistry.Add((new StickDeadzoneAssignment(SettingID::STICK_DEADZONE_INNER, left_stick_deadzone_inner))
	                      ->SetHelp("Defines a radius of the both left and right sticks within which all values will be ignored. This value can only be between 0 and 1 but it should be small. Stick input out of this radius will be adjusted."));
	commandRegistry.Add((new StickDeadzoneAssignment(SettingID::STICK_DEADZONE_INNER, right_stick_deadzone_inner))->SetHelp(""));
	commandRegistry.Add((new StickDeadzoneAssignment(SettingID::STICK_DEADZONE_OUTER, left_stick_deadzone_outer))
	                      ->SetHelp("Defines a distance from both sticks' outer edge for which the stick will be considered fully tilted. This value can only be between 0 and 1 but it should be small. Stick input out of this deadzone will be adjusted."));
	commandRegistry.Add((new StickDeadzoneAssignment(SettingID::STICK_DEADZONE_OUTER, right_stick_deadzone_outer))->SetHelp(""));
	commandRegistry.Add((new JSMAssignment<float>(motion_deadzone_inner))
	                      ->SetHelp("Defines a radius of the motion-stick within which all values will be ignored. This value can only be between 0 and 1 but it should be small. Stick input out of this radius will be adjusted."));
	commandRegistry.Add((new JSMAssignment<float>(motion_deadzone_outer))
	                      ->SetHelp("Defines a distance from the motion-stick's outer edge for which the stick will be considered fully tilted. Stick input out of this deadzone will be adjusted."));
	commandRegistry.Add((new JSMAssignment<float>(angle_to_axis_deadzone_inner))
	                      ->SetHelp("Defines an angle within which _ANGLE_TO_X and _ANGLE_TO_Y stick modes will be ignored (in degrees). Since a circular deadzone is already used for deciding whether the stick is engaged at all, it's recommended not to use an inner angular deadzone, which is why the default value is 0."));
	commandRegistry.Add((new JSMAssignment<float>(angle_to_axis_deadzone_outer))
	                      ->SetHelp("Defines an angle from max or min rotation that will be treated as max or min rotation, respectively, for _ANGLE_TO_X and _ANGLE_TO_Y stick modes. Since players intending to point the stick perfectly up/down or perfectly left/right will usually be off by a few degrees, this enables players to more easily hit their intended min/max values, so the default value is 10 degrees."));
	commandRegistry.Add((new JSMAssignment<float>(lean_threshold))
	                      ->SetHelp("How far the controller must be leaned left or right to trigger a LEAN_LEFT or LEAN_RIGHT binding."));
	commandRegistry.Add((new JSMMacro("CALCULATE_REAL_WORLD_CALIBRATION"))->SetMacro(bind(&do_CALCULATE_REAL_WORLD_CALIBRATION, placeholders::_2))->SetHelp("Get JoyShockMapper to recommend you a REAL_WORLD_CALIBRATION value after performing the calibration sequence. Visit GyroWiki for details:\nhttp://gyrowiki.jibbsmart.com/blog:joyshockmapper-guide#calibrating"));
	commandRegistry.Add((new JSMMacro("SLEEP"))->SetMacro(bind(&do_SLEEP, placeholders::_2))->SetHelp("Sleep for the given number of seconds, or one second if no number is given. Can't sleep more than 10 seconds per command."));
	commandRegistry.Add((new JSMMacro("FINISH_GYRO_CALIBRATION"))->SetMacro(bind(&do_FINISH_GYRO_CALIBRATION))->SetHelp("Finish calibrating the gyro in all controllers."));
	commandRegistry.Add((new JSMMacro("RESTART_GYRO_CALIBRATION"))->SetMacro(bind(&do_RESTART_GYRO_CALIBRATION))->SetHelp("Start calibrating the gyro in all controllers."));
	commandRegistry.Add((new JSMMacro("SET_MOTION_STICK_NEUTRAL"))->SetMacro(bind(&do_SET_MOTION_STICK_NEUTRAL))->SetHelp("Set the neutral orientation for motion stick to whatever the orientation of the controller is."));
	commandRegistry.Add((new JSMAssignment<GyroAxisMask>(mouse_x_from_gyro))
	                      ->SetHelp("Pick a gyro axis to operate on the mouse's X axis. Valid values are the following: X, Y and Z."));
	commandRegistry.Add((new JSMAssignment<GyroAxisMask>(mouse_y_from_gyro))
	                      ->SetHelp("Pick a gyro axis to operate on the mouse's Y axis. Valid values are the following: X, Y and Z."));
	commandRegistry.Add((new JSMAssignment<ControllerOrientation>(controller_orientation))
	                      ->SetHelp("Let the stick modes account for how you're holding the controller:\nFORWARD, LEFT, RIGHT, BACKWARD"));
	commandRegistry.Add((new JSMAssignment<GyroSpace>(gyro_space))
	                      ->SetHelp("How gyro input is converted to 2D input. With LOCAL, your MOUSE_X_FROM_GYRO_AXIS and MOUSE_Y_FROM_GYRO_AXIS settings decide which local angular axis maps to which 2D mouse axis.\nYour other options are PLAYER_TURN and PLAYER_LEAN. These both take gravity into account to combine your axes more reliably.\n\tUse PLAYER_TURN if you like to turn your camera or move your cursor by turning your controller side to side.\n\tUse PLAYER_LEAN if you'd rather lean your controller to turn the camera."));
	commandRegistry.Add((new JSMAssignment<TriggerMode>(zlMode))
	                      ->SetHelp("Controllers with a right analog trigger can use one of the following dual stage trigger modes:\nNO_FULL, NO_SKIP, MAY_SKIP, MUST_SKIP, MAY_SKIP_R, MUST_SKIP_R, NO_SKIP_EXCLUSIVE, X_LT, X_RT, PS_L2, PS_R2"));
	commandRegistry.Add((new JSMAssignment<TriggerMode>(zrMode))
	                      ->SetHelp("Controllers with a left analog trigger can use one of the following dual stage trigger modes:\nNO_FULL, NO_SKIP, MAY_SKIP, MUST_SKIP, MAY_SKIP_R, MUST_SKIP_R, NO_SKIP_EXCLUSIVE, X_LT, X_RT, PS_L2, PS_R2"));
	auto *autoloadCmd = new JSMAssignment<Switch>("AUTOLOAD", autoloadSwitch);
	commandRegistry.Add(autoloadCmd);
	currentWorkingDir.AddOnChangeListener(bind(&RefreshAutoLoadHelp, autoloadCmd), true);
	commandRegistry.Add((new JSMMacro("README"))->SetMacro(bind(&do_README))->SetHelp("Open the latest JoyShockMapper README in your browser."));
	commandRegistry.Add((new JSMMacro("WHITELIST_SHOW"))->SetMacro(bind(&do_WHITELIST_SHOW))->SetHelp("Open the whitelister application"));
	commandRegistry.Add((new JSMMacro("WHITELIST_ADD"))->SetMacro(bind(&do_WHITELIST_ADD))->SetHelp("Add JoyShockMapper to the whitelisted applications."));
	commandRegistry.Add((new JSMMacro("WHITELIST_REMOVE"))->SetMacro(bind(&do_WHITELIST_REMOVE))->SetHelp("Remove JoyShockMapper from whitelisted applications."));
	commandRegistry.Add((new JSMAssignment<RingMode>(left_ring_mode))
	                      ->SetHelp("Pick a ring where to apply the LEFT_RING binding. Valid values are the following: INNER and OUTER."));
	commandRegistry.Add((new JSMAssignment<RingMode>(right_ring_mode))
	                      ->SetHelp("Pick a ring where to apply the RIGHT_RING binding. Valid values are the following: INNER and OUTER."));
	commandRegistry.Add((new JSMAssignment<RingMode>(motion_ring_mode))
	                      ->SetHelp("Pick a ring where to apply the MOTION_RING binding. Valid values are the following: INNER and OUTER."));
	commandRegistry.Add((new JSMAssignment<float>(mouse_ring_radius))
	                      ->SetHelp("Pick a radius on which the cursor will be allowed to move. This value is used for stick mode MOUSE_RING and MOUSE_AREA."));
	commandRegistry.Add((new JSMAssignment<float>(trackball_decay))
	                      ->SetHelp("Choose the rate at which trackball gyro slows down. 0 means no decay, 1 means it'll halve each second, 2 to halve each 1/2 seconds, etc."));
	commandRegistry.Add((new JSMAssignment<float>(screen_resolution_x))
	                      ->SetHelp("Indicate your monitor's horizontal resolution when using the stick mode MOUSE_RING."));
	commandRegistry.Add((new JSMAssignment<float>(screen_resolution_y))
	                      ->SetHelp("Indicate your monitor's vertical resolution when using the stick mode MOUSE_RING."));
	commandRegistry.Add((new JSMAssignment<float>(rotate_smooth_override))
	                      ->SetHelp("Some smoothing is applied to flick stick rotations to account for the controller's stick resolution. This value overrides the smoothing threshold."));
	commandRegistry.Add((new JSMAssignment<FlickSnapMode>(flick_snap_mode))
	                      ->SetHelp("Snap flicks to cardinal directions. Valid values are the following: NONE or 0, FOUR or 4 and EIGHT or 8."));
	commandRegistry.Add((new JSMAssignment<float>(flick_snap_strength))
	                      ->SetHelp("If FLICK_SNAP_MODE is set to something other than NONE, this sets the degree of snapping -- 0 for none, 1 for full snapping to the nearest direction, and values in between will bias you towards the nearest direction instead of snapping."));
	commandRegistry.Add((new JSMAssignment<float>(trigger_skip_delay))
	                      ->SetHelp("Sets the amount of time in milliseconds within which the user needs to reach the full press to skip the soft pull binding of the trigger."));
	commandRegistry.Add((new JSMAssignment<float>(turbo_period))
	                      ->SetHelp("Sets the time in milliseconds to wait between each turbo activation."));
	commandRegistry.Add((new JSMAssignment<float>(hold_press_time))
	                      ->SetHelp("Sets the amount of time in milliseconds to hold a button before the hold press is enabled. Releasing the button before this time will trigger the tap press. Turbo press only starts after this delay."));
	commandRegistry.Add((new JSMAssignment<float>("SIM_PRESS_WINDOW", sim_press_window))
	                      ->SetHelp("Sets the amount of time in milliseconds within which both buttons of a simultaneous press needs to be pressed before enabling the sim press mappings. This setting does not support modeshift."));
	commandRegistry.Add((new JSMAssignment<float>("DBL_PRESS_WINDOW", dbl_press_window))
	                      ->SetHelp("Sets the amount of time in milliseconds within which the user needs to press a button twice before enabling the double press mappings. This setting does not support modeshift."));
	commandRegistry.Add((new JSMAssignment<float>("TICK_TIME", tick_time))
	                      ->SetHelp("Sets the time in milliseconds that JoyShockMaper waits before reading from each controller again."));
	commandRegistry.Add((new JSMAssignment<PathString>("JSM_DIRECTORY", currentWorkingDir))
	                      ->SetHelp("If AUTOLOAD doesn't work properly, set this value to the path to the directory holding the JoyShockMapper.exe file. Make sure a folder named \"AutoLoad\" exists there."));
	commandRegistry.Add((new JSMAssignment<TouchpadMode>("TOUCHPAD_MODE", touchpad_mode))
	                      ->SetHelp("Assign a mode to the touchpad. Valid values are GRID_AND_STICK or MOUSE."));
	commandRegistry.Add((new JSMAssignment<FloatXY>("GRID_SIZE", grid_size))
	                      ->SetHelp("When TOUCHPAD_MODE is set to GRID_AND_STICK, this variable sets the number of rows and columns in the grid. The product of the two numbers need to be between 1 and 25."));
	commandRegistry.Add((new JSMAssignment<StickMode>(touch_stick_mode))
	                      ->SetHelp("Set a mouse mode for the touchpad stick. Valid values are the following:\nNO_MOUSE, AIM, FLICK, FLICK_ONLY, ROTATE_ONLY, MOUSE_RING, MOUSE_AREA, OUTER_RING, INNER_RING"));
	commandRegistry.Add((new JSMAssignment<float>(touch_stick_radius))
	                      ->SetHelp("Set the radius of the touchpad stick. The center of the stick is always the first point of contact. Use a very large value (ex: 800) to use it as swipe gesture."));
	commandRegistry.Add((new JSMAssignment<float>(touch_deadzone_inner))
	                      ->SetHelp("Sets the radius of the circle in which a touch stick input sends no output."));
	commandRegistry.Add((new JSMAssignment<RingMode>(touch_ring_mode))
	                      ->SetHelp("Sets the ring mode for the touch stick. Valid values are INNER and OUTER"));
	commandRegistry.Add((new JSMAssignment<Color>(light_bar))
	                      ->SetHelp("Changes the color bar of the DS4. Either enter as a hex code (xRRGGBB), as three decimal values between 0 and 255 (RRR GGG BBB), or as a common color name in all caps and underscores."));
	commandRegistry.Add((new JSMAssignment<FloatXY>(touchpad_sens))
	                      ->SetHelp("Changes the sensitivity of the touchpad when set as a mouse. Enter a second value for a different vertical sensitivity."));
	commandRegistry.Add(new HelpCmd(commandRegistry));
	commandRegistry.Add((new JSMAssignment<ControllerScheme>(magic_enum::enum_name(SettingID::VIRTUAL_CONTROLLER).data(), virtual_controller))
	                      ->SetHelp("Sets the vigem virtual controller type. Can be NONE (default), XBOX (360) or DS4 (PS4)."));
	commandRegistry.Add((new JSMAssignment<Switch>("HIDE_MINIMIZED", hide_minimized))
	                      ->SetHelp("JSM will be hidden in the notification area when minimized if this setting is ON. Otherwise it stays in the taskbar."));
	commandRegistry.Add((new JSMAssignment<FloatXY>(scroll_sens))
	                      ->SetHelp("Scrolling sensitivity for sticks."));
	commandRegistry.Add((new JSMAssignment<Switch>(rumble_enable))
	                      ->SetHelp("Disable the rumbling feature from vigem. Valid values are ON and OFF."));
	commandRegistry.Add((new JSMAssignment<Switch>(adaptive_trigger))
	                      ->SetHelp("Control the adaptive trigger feature of the DualSense. Valid values are ON and OFF."));
	commandRegistry.Add((new JSMAssignment<AdaptiveTriggerSetting>(left_trigger_effect))
	                      ->SetHelp("Sets the adaptive trigger effect on the left trigger:\n"\
									"OFF: No effect\n"\
									"ON: Use effect generated by JSM depending on ZL_MODE\n"\
									"RESISTANCE start[0 9] force[0 8]: Some resistance starting at point\n"\
									"BOW start[0 8] end[0 8] forceStart[0 8] forceEnd[0 8]: increasingly strong resistance\n"\
									"GALLOPING start[0 8] end[0 9] foot1[0 6] foot2[0 7] freq[Hz]: Two pulses repeated periodically\n"\
									"SEMI_AUTOMATIC start[2 7] end[0 8] force[0 8]: Trigger effect\n"\
									"AUTOMATIC start[0 9] strength[0 8] freq[Hz]: Regular pulse effect\n"\
									"MACHINE start[0 9] end[0 9] force1[0 7] force2[0 7] freq[Hz] period: Irregular pulsing"));
	commandRegistry.Add((new JSMAssignment<AdaptiveTriggerSetting>(right_trigger_effect))
	                      ->SetHelp("Sets the adaptive trigger effect on the right trigger:\n"\
									"OFF: No effect\n"\
									"ON: Use effect generated by JSM depending on ZR_MODE\n"\
									"RESISTANCE start[0 9] force[0 8]: Some resistance starting at point\n"\
									"BOW start[0 8] end[0 8] forceStart[0 8] forceEnd[0 8]: increasingly strong resistance\n"\
									"GALLOPING start[0 8] end[0 9] foot1[0 6] foot2[0 7] freq[Hz]: Two pulses repeated periodically\n"\
									"SEMI_AUTOMATIC start[2 7] end[0 8] force[0 8]: Trigger effect\n"\
									"AUTOMATIC start[0 9] strength[0 8] freq[Hz]: Regular pulse effect\n"\
									"MACHINE start[0 9] end[0 9] force1[0 7] force2[0 7] freq[Hz] period: Irregular pulsing"));
	commandRegistry.Add((new JSMAssignment<TriggerMode>(touch_ds_mode))
	                      ->SetHelp("Dual stage mode for the touchpad TOUCH and CAPTURE (i.e. click) bindings."));
	commandRegistry.Add((new JSMMacro("CLEAR"))->SetMacro(bind(&ClearConsole))->SetHelp("Removes all text in the console screen"));
	commandRegistry.Add((new JSMMacro("CALIBRATE_TRIGGERS"))->SetMacro([](JSMMacro *, in_string) {
		                                                        triggerCalibrationStep = 1;
		                                                        return true;
	                                                        })
	                      ->SetHelp("Starts the trigger calibration procedure for the dualsense triggers."));
	commandRegistry.Add((new JSMAssignment<int>(magic_enum::enum_name(SettingID::LEFT_TRIGGER_OFFSET).data(), left_trigger_offset)));
	commandRegistry.Add((new JSMAssignment<int>(magic_enum::enum_name(SettingID::RIGHT_TRIGGER_OFFSET).data(), right_trigger_offset)));
	commandRegistry.Add((new JSMAssignment<int>(magic_enum::enum_name(SettingID::LEFT_TRIGGER_RANGE).data(), left_trigger_range)));
	commandRegistry.Add((new JSMAssignment<int>(magic_enum::enum_name(SettingID::RIGHT_TRIGGER_RANGE).data(), right_trigger_range)));
	commandRegistry.Add((new JSMAssignment<Switch>("AUTO_CALIBRATE_GYRO", auto_calibrate_gyro))
	                      ->SetHelp("Gyro calibration happens automatically when this setting is ON. Otherwise you'll need to calibrate the gyro manually when using gyro aiming."));
	commandRegistry.Add((new JSMAssignment<AxisSignPair>(left_stick_axis))
	                      ->SetHelp("When in AIM mode, set stick X axis inversion. Valid values are the following:\nSTANDARD or 1, and INVERTED or -1"));
	commandRegistry.Add((new JSMAssignment<AxisSignPair>(right_stick_axis))
	                      ->SetHelp("When in AIM mode, set stick X axis inversion. Valid values are the following:\nSTANDARD or 1, and INVERTED or -1"));
	commandRegistry.Add((new JSMAssignment<AxisSignPair>(motion_stick_axis))
	                      ->SetHelp("When in AIM mode, set stick X axis inversion. Valid values are the following:\nSTANDARD or 1, and INVERTED or -1"));
	commandRegistry.Add((new JSMAssignment<AxisSignPair>(touch_stick_axis))
	                      ->SetHelp("When in AIM mode, set stick X axis inversion. Valid values are the following:\nSTANDARD or 1, and INVERTED or -1"));

	commandRegistry.Add(new JSMAssignment<AxisMode>(aim_x_sign, true));
	commandRegistry.Add(new JSMAssignment<AxisMode>(aim_y_sign, true));

	commandRegistry.Add((new JSMAssignment<float>(left_stick_undeadzone_inner))
	                      ->SetHelp("When outputting as a virtual controller, account for this much inner deadzone being applied in the target game. This value can only be between 0 and 1 but it should be small."));
	commandRegistry.Add((new JSMAssignment<float>(left_stick_undeadzone_outer))
	                      ->SetHelp("When outputting as a virtual controller, account for this much outer deadzone being applied in the target game. This value can only be between 0 and 1 but it should be small."));
	commandRegistry.Add((new JSMAssignment<float>(left_stick_unpower))
	                      ->SetHelp("When outputting as a virtual controller, account for this power curve being applied in the target game."));
	commandRegistry.Add((new JSMAssignment<float>(right_stick_undeadzone_inner))
	                      ->SetHelp("When outputting as a virtual controller, account for this much inner deadzone being applied in the target game. This value can only be between 0 and 1 but it should be small."));
	commandRegistry.Add((new JSMAssignment<float>(right_stick_undeadzone_outer))
	                      ->SetHelp("When outputting as a virtual controller, account for this much outer deadzone being applied in the target game. This value can only be between 0 and 1 but it should be small."));
	commandRegistry.Add((new JSMAssignment<float>(right_stick_unpower))
	                      ->SetHelp("When outputting as a virtual controller, account for this power curve being applied in the target game."));
	commandRegistry.Add((new JSMAssignment<float>(left_stick_virtual_scale))
	                      ->SetHelp("When outputting as a virtual controller, use this to adjust the scale of the left stick output. This does not affect the gyro->stick conversion."));
	commandRegistry.Add((new JSMAssignment<float>(right_stick_virtual_scale))
	                      ->SetHelp("When outputting as a virtual controller, use this to adjust the scale of the right stick output. This does not affect the gyro->stick conversion."));
	commandRegistry.Add((new JSMAssignment<float>(wind_stick_range))
	                      ->SetHelp("When using the WIND stick modes, this is how many degrees the stick has to be wound to cover the full range of the ouptut, from minimum value to maximum value."));
	commandRegistry.Add((new JSMAssignment<float>(wind_stick_power))
	                      ->SetHelp("Power curve for WIND stick modes, letting you have more or less sensitivity towards the neutral position."));
	commandRegistry.Add((new JSMAssignment<float>(unwind_rate))
	                      ->SetHelp("How quickly the WIND sticks unwind on their own when the relevant stick isn't engaged (in degrees per second)."));
	commandRegistry.Add((new JSMAssignment<GyroOutput>(gyro_output))
	                      ->SetHelp("Whether gyro should be converted to mouse, left stick, or right stick movement. If you don't want to use gyro aiming, simply leave GYRO_SENS set to 0."));
	commandRegistry.Add((new JSMAssignment<GyroOutput>(flick_stick_output))
	                      ->SetHelp("Whether flick stick should be converted to a mouse, left stick, or right stick movement."));
	
	bool quit = false;
	commandRegistry.Add((new JSMMacro("QUIT"))
	                      ->SetMacro([&quit](JSMMacro *, in_string) {
		                      quit = true;
		                      WriteToConsole(""); // If ran from autoload thread, you need to send RETURN to resume the main loop and check the quit flag.
		                      return true;
	                      })
	                      ->SetHelp("Close the application."));

	Mapping::_isCommandValid = bind(&CmdRegistry::isCommandValid, &commandRegistry, placeholders::_1);

	connectDevices();
	jsl->SetCallback(&joyShockPollCallback);
	jsl->SetTouchCallback(&TouchCallback);
	tray.reset(TrayIcon::getNew(trayIconData, &beforeShowTrayMenu));
	if (tray)
	{
		tray->Show();
	}

	do_RESET_MAPPINGS(&commandRegistry); // OnReset.txt
	if (commandRegistry.loadConfigFile("OnStartup.txt"))
	{
		COUT << "Finished executing startup file." << endl;
	}
	else
	{
		COUT << "There is no ";
		COUT_INFO << "OnStartup.txt";
		COUT << " file to load." << endl;
	}

	for (int i = 0; i < argc; ++i)
	{
#if _WIN32
		string arg(&argv[i][0], &argv[i][wcslen(argv[i])]);
#else
		string arg = string(argv[0]);
#endif
		if (filesystem::is_regular_file(filesystem::status(arg)) && arg != module)
		{
			commandRegistry.loadConfigFile(arg);
			autoloadSwitch = Switch::OFF;
		}
	}

	// The main loop is simple and reads like pseudocode
	string enteredCommand;
	while (!quit)
	{
		getline(cin, enteredCommand);
		loading_lock.lock();
		commandRegistry.processLine(enteredCommand);
		loading_lock.unlock();
	}
#ifdef _WIN32
	LocalFree(argv);
#endif
	CleanUp();
	return 0;
}
