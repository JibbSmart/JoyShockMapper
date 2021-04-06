#include "JoyShockMapper.h"
#include "JoyShockLibrary.h"
#include "GamepadMotion.hpp"
#include "InputHelpers.h"
#include "Whitelister.h"
#include "TrayIcon.h"
#include "JSMAssignment.hpp"
#include "quatMaths.cpp"
#include "Gamepad.h"

#include <mutex>
#include <deque>
#include <iomanip>

#pragma warning(disable : 4996) // Disable deprecated API warnings

#define PI 3.14159265359f

const KeyCode KeyCode::EMPTY = KeyCode();
const Mapping Mapping::NO_MAPPING = Mapping("NONE");
function<bool(in_string)> Mapping::_isCommandValid = function<bool(in_string)>();

class JoyShock;
void joyShockPollCallback(int jcHandle, JOY_SHOCK_STATE state, JOY_SHOCK_STATE lastState, IMU_STATE imuState, IMU_STATE lastImuState, float deltaTime);

// Contains all settings that can be modeshifted. They should be accessed only via Joyshock::getSetting
JSMSetting<StickMode> left_stick_mode = JSMSetting<StickMode>(SettingID::LEFT_STICK_MODE, StickMode::NO_MOUSE);
JSMSetting<StickMode> right_stick_mode = JSMSetting<StickMode>(SettingID::RIGHT_STICK_MODE, StickMode::NO_MOUSE);
JSMSetting<StickMode> motion_stick_mode = JSMSetting<StickMode>(SettingID::MOTION_STICK_MODE, StickMode::NO_MOUSE);
JSMSetting<RingMode> left_ring_mode = JSMSetting<RingMode>(SettingID::LEFT_RING_MODE, RingMode::OUTER);
JSMSetting<RingMode> right_ring_mode = JSMSetting<RingMode>(SettingID::LEFT_RING_MODE, RingMode::OUTER);
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
JSMSetting<float> in_game_sens = JSMSetting<float>(SettingID::IN_GAME_SENS, 1.0f);
JSMSetting<float> trigger_threshold = JSMSetting<float>(SettingID::TRIGGER_THRESHOLD, 0.0f);
JSMSetting<AxisMode> aim_x_sign = JSMSetting<AxisMode>(SettingID::STICK_AXIS_X, AxisMode::STANDARD);
JSMSetting<AxisMode> aim_y_sign = JSMSetting<AxisMode>(SettingID::STICK_AXIS_Y, AxisMode::STANDARD);
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
JSMSetting<float> lean_threshold = JSMSetting<float>(SettingID::LEAN_THRESHOLD, 15.f);
JSMSetting<ControllerOrientation> controller_orientation = JSMSetting<ControllerOrientation>(SettingID::CONTROLLER_ORIENTATION, ControllerOrientation::FORWARD);
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
JSMVariable<float> dbl_press_window = JSMVariable<float>(200.0f);
JSMVariable<float> tick_time = JSMSetting<float>(SettingID::TICK_TIME, 3);
JSMSetting<Color> light_bar = JSMSetting<Color>(SettingID::LIGHT_BAR, 0xFFFFFF);
JSMSetting<FloatXY> scroll_sens = JSMSetting<FloatXY>(SettingID::SCROLL_SENS, { 30.f, 30.f });
JSMVariable<Switch> autoloadSwitch = JSMVariable<Switch>(Switch::ON);
JSMVariable<Switch> hide_minimized = JSMVariable<Switch>(Switch::OFF);
JSMVariable<ControllerScheme> virtual_controller = JSMVariable<ControllerScheme>(ControllerScheme::NONE);
JSMSetting<TriggerMode> touch_ds_mode = JSMSetting<TriggerMode>(SettingID::TOUCHPAD_DUAL_STAGE_MODE, TriggerMode::NO_SKIP);
JSMSetting<Switch> rumble_enable = JSMSetting<Switch>(SettingID::RUMBLE, Switch::ON);

JSMVariable<PathString> currentWorkingDir = JSMVariable<PathString>(GetCWD());
vector<JSMButton> mappings; // array enables use of for each loop and other i/f
mutex loading_lock;

float os_mouse_speed = 1.0;
float last_flick_and_rotation = 0.0;
unique_ptr<PollingThread> autoLoadThread;
unique_ptr<PollingThread> minimizeThread;
unique_ptr<TrayIcon> tray;
bool devicesCalibrating = false;
Whitelister whitelister(false);
unordered_map<int, shared_ptr<JoyShock>> handle_to_joyshock;
string currentWindowName;

// This class holds all the logic related to a single digital button. It does not hold the mapping but only a reference
// to it. It also contains it's various states, flags and data.
class DigitalButton
{
public:
	// All digital buttons need a reference to the same instance of a the common structure within the same controller.
	// It enables the buttons to synchronize and be aware of the state of the whole controller.
	struct Common
	{
		Common(Gamepad::Callback virtualControllerCallback)
		{
			chordStack.push_front(ButtonID::NONE); //Always hold mapping none at the end to handle modeshifts and chords
			if (virtual_controller.get() != ControllerScheme::NONE)
			{
				_vigemController.reset(new Gamepad(virtual_controller.get(), virtualControllerCallback));
			}
		}
		deque<pair<ButtonID, KeyCode>> gyroActionQueue; // Queue of gyro control actions currently in effect
		deque<pair<ButtonID, KeyCode>> activeTogglesQueue;
		deque<ButtonID> chordStack; // Represents the current active buttons in order from most recent to latest
		unique_ptr<Gamepad> _vigemController;
		function<DigitalButton *(ButtonID)> _getMatchingSimBtn;
		mutex callback_lock; // Needs to be in the common struct for both joycons to use the same
		function<void(int small, int big)> _rumble;
	};

	static bool findQueueItem(pair<ButtonID, KeyCode> &pair, ButtonID btn)
	{
		return btn == pair.first;
	}

	DigitalButton(shared_ptr<DigitalButton::Common> btnCommon, ButtonID id, int deviceHandle, GamepadMotion *gamepadMotion)
	  : _id(id)
	  , _btnState(BtnState::NoPress)
	  , _common(btnCommon)
	  , _mapping(mappings[int(_id)])
	  , _press_times()
	  , _keyToRelease()
	  , _turboCount(0)
	  , _simPressMaster(nullptr)
	  , _instantReleaseQueue()
	  , _gamepadMotion(gamepadMotion)
	{
		_instantReleaseQueue.reserve(2);
	}

	const ButtonID _id; // Always ID first for easy debugging
	BtnState _btnState = BtnState::NoPress;
	shared_ptr<Common> _common;
	const JSMButton &_mapping;
	chrono::steady_clock::time_point _press_times;
	unique_ptr<Mapping> _keyToRelease; // At key press, remember what to release
	string _nameToRelease;
	unsigned int _turboCount;
	DigitalButton *_simPressMaster;
	vector<BtnEvent> _instantReleaseQueue;
	GamepadMotion *_gamepadMotion;

	bool CheckInstantRelease(BtnEvent instantEvent)
	{
		auto instant = find(_instantReleaseQueue.begin(), _instantReleaseQueue.end(), instantEvent);
		if (instant != _instantReleaseQueue.end())
		{
			//COUT << "Button " << _id << " releases instant " << instantEvent << endl;
			_keyToRelease->ProcessEvent(BtnEvent::OnInstantRelease, *this, _nameToRelease);
			_instantReleaseQueue.erase(instant);
			return true;
		}
		return false;
	}

	void ProcessButtonPress(bool pressed, chrono::steady_clock::time_point time_now, float turbo_ms, float hold_ms)
	{
		auto elapsed_time = GetPressDurationMS(time_now);
		if (pressed)
		{
			if (_turboCount == 0)
			{
				if (elapsed_time > MAGIC_INSTANT_DURATION)
				{
					CheckInstantRelease(BtnEvent::OnPress);
				}
				if (elapsed_time > hold_ms)
				{
					_keyToRelease->ProcessEvent(BtnEvent::OnHold, *this, _nameToRelease);
					_keyToRelease->ProcessEvent(BtnEvent::OnTurbo, *this, _nameToRelease);
					_turboCount++;
				}
			}
			else
			{
				if (elapsed_time > hold_ms + MAGIC_INSTANT_DURATION)
				{
					CheckInstantRelease(BtnEvent::OnHold);
				}
				if (floorf((elapsed_time - hold_ms) / turbo_ms) >= _turboCount)
				{
					_keyToRelease->ProcessEvent(BtnEvent::OnTurbo, *this, _nameToRelease);
					_turboCount++;
				}
				if (elapsed_time > hold_ms + _turboCount * turbo_ms + MAGIC_INSTANT_DURATION)
				{
					CheckInstantRelease(BtnEvent::OnTurbo);
				}
			}
		}
		else // not pressed
		{
			_keyToRelease->ProcessEvent(BtnEvent::OnRelease, *this, _nameToRelease);
			if (_turboCount == 0)
			{
				_keyToRelease->ProcessEvent(BtnEvent::OnTap, *this, _nameToRelease);
				_btnState = BtnState::TapRelease;
				_press_times = time_now; // Start counting tap duration
			}
			else
			{
				_keyToRelease->ProcessEvent(BtnEvent::OnHoldRelease, *this, _nameToRelease);
				if (_instantReleaseQueue.empty())
				{
					_btnState = BtnState::NoPress;
					ClearKey();
				}
				else
				{
					_btnState = BtnState::InstRelease;
					_press_times = time_now; // Start counting tap duration
				}
			}
		}
	}

	Mapping *GetPressMapping()
	{
		if (!_keyToRelease)
		{
			// Look at active chord mappings starting with the latest activates chord
			for (auto activeChord = _common->chordStack.cbegin(); activeChord != _common->chordStack.cend(); activeChord++)
			{
				auto binding = _mapping.get(*activeChord);
				if (binding && *activeChord != _id)
				{
					_keyToRelease.reset(new Mapping(*binding));
					_nameToRelease = _mapping.getName(*activeChord);
					return _keyToRelease.get();
				}
			}
			// Chord stack should always include NONE which will provide a value in the loop above
			throw runtime_error("ChordStack should always include ButtonID::NONE, for the chorded variable to return the base value.");
		}
		return _keyToRelease.get();
	}

	void StartCalibration()
	{
		COUT << "Starting continuous calibration" << endl;
		_gamepadMotion->ResetContinuousCalibration();
		_gamepadMotion->StartContinuousCalibration();
	}

	void FinishCalibration()
	{
		_gamepadMotion->PauseContinuousCalibration();
		COUT << "Gyro calibration set" << endl;
		ClearAllActiveToggle(KeyCode("CALIBRATE"));
	}

	void ApplyGyroAction(KeyCode gyroAction)
	{
		_common->gyroActionQueue.push_back({ _id, gyroAction });
	}

	void RemoveGyroAction()
	{
		auto gyroAction = find_if(_common->gyroActionQueue.begin(), _common->gyroActionQueue.end(),
		  [this](auto pair) {
			  // On a sim press, release the master button (the one who triggered the press)
			  return pair.first == (_simPressMaster ? _simPressMaster->_id : _id);
		  });
		if (gyroAction != _common->gyroActionQueue.end())
		{
			ClearAllActiveToggle(gyroAction->second);
			_common->gyroActionQueue.erase(gyroAction);
		}
	}

	void SetRumble(int smallRumble, int bigRumble)
	{
		_common->_rumble(smallRumble, bigRumble);
	}

	void ApplyBtnPress(KeyCode key)
	{
		if (key.code >= X_UP && key.code <= X_START || key.code == PS_HOME || key.code == PS_PAD_CLICK)
		{
			if (_common->_vigemController)
				_common->_vigemController->setButton(key, true);
		}
		else if (key.code != NO_HOLD_MAPPED && currentWindowName.compare("JoyShockMapper.exe") != 0)
		{
			// Don't send key presses to JSM console. It's annoying and useless.
			pressKey(key, true);
		}
	}

	void ApplyBtnRelease(KeyCode key)
	{
		if (key.code >= X_UP && key.code <= X_START || key.code == PS_HOME || key.code == PS_PAD_CLICK)
		{
			if (_common->_vigemController)
				_common->_vigemController->setButton(key, false);
		}
		else if (key.code != NO_HOLD_MAPPED)
		{
			pressKey(key, false);
			ClearAllActiveToggle(key);
		}
	}

	void ApplyButtonToggle(KeyCode key, function<void(DigitalButton *)> apply, function<void(DigitalButton *)> release)
	{
		auto currentlyActive = find_if(_common->activeTogglesQueue.begin(), _common->activeTogglesQueue.end(),
		  [this, key](pair<ButtonID, KeyCode> pair) {
			  return pair.first == _id && pair.second == key;
		  });
		if (currentlyActive == _common->activeTogglesQueue.end())
		{
			apply(this);
			_common->activeTogglesQueue.push_front({ _id, key });
		}
		else
		{
			release(this); // The bound action here should always erase the active toggle from the queue
		}
	}

	void RegisterInstant(BtnEvent evt)
	{
		//COUT << "Button " << _id << " registers instant " << evt << endl;
		_instantReleaseQueue.push_back(evt);
	}

	void ClearAllActiveToggle(KeyCode key)
	{
		std::function<bool(pair<ButtonID, KeyCode>)> isSameKey = [key](pair<ButtonID, KeyCode> pair) {
			return pair.second == key;
		};

		for (auto currentlyActive = find_if(_common->activeTogglesQueue.begin(), _common->activeTogglesQueue.end(), isSameKey);
		     currentlyActive != _common->activeTogglesQueue.end();
		     currentlyActive = find_if(_common->activeTogglesQueue.begin(), _common->activeTogglesQueue.end(), isSameKey))
		{
			_common->activeTogglesQueue.erase(currentlyActive);
		}
	}

	void SyncSimPress(DigitalButton &btn)
	{
		_keyToRelease.reset(new Mapping(*btn._keyToRelease));
		_nameToRelease = btn._nameToRelease;
		_simPressMaster = &btn;
		//COUT << btn << " is the master button" << endl;
	}

	void ClearKey()
	{
		_keyToRelease.reset();
		_instantReleaseQueue.clear();
		_nameToRelease.clear();
		_turboCount = 0;
	}

	// Pretty wrapper
	inline float GetPressDurationMS(chrono::steady_clock::time_point time_now)
	{
		return static_cast<float>(chrono::duration_cast<chrono::milliseconds>(time_now - _press_times).count());
	}

	void updateButtonState(bool pressed, chrono::steady_clock::time_point time_now, float turboTime, float holdTime)
	{
		if (_id < ButtonID::SIZE)
		{
			auto foundChord = find(_common->chordStack.begin(), _common->chordStack.end(), _id);
			if (!pressed)
			{
				if (foundChord != _common->chordStack.end())
				{
					//COUT << "Button " << index << " is released!" << endl;
					_common->chordStack.erase(foundChord); // The chord is released
				}
			}
			else if (foundChord == _common->chordStack.end())
			{
				//COUT << "Button " << index << " is pressed!" << endl;
				_common->chordStack.push_front(_id); // Always push at the fromt to make it a stack
			}
		}

		switch (_btnState)
		{
		case BtnState::NoPress:
			if (pressed)
			{
				_press_times = time_now;
				if (_mapping.HasSimMappings())
				{
					_btnState = BtnState::WaitSim;
				}
				else if (_mapping.getDblPressMap())
				{
					// Start counting time between two start presses
					_btnState = BtnState::DblPressStart;
				}
				else
				{
					_btnState = BtnState::BtnPress;
					GetPressMapping()->ProcessEvent(BtnEvent::OnPress, *this, _nameToRelease);
				}
			}
			break;
		case BtnState::BtnPress:
			ProcessButtonPress(pressed, time_now, turboTime, holdTime);
			break;
		case BtnState::TapRelease:
		{
			if (pressed || GetPressDurationMS(time_now) > MAGIC_INSTANT_DURATION)
			{
				CheckInstantRelease(BtnEvent::OnRelease);
				CheckInstantRelease(BtnEvent::OnTap);
			}
			if (pressed || GetPressDurationMS(time_now) > _keyToRelease->getTapDuration())
			{
				GetPressMapping()->ProcessEvent(BtnEvent::OnTapRelease, *this, _nameToRelease);
				_btnState = BtnState::NoPress;
				ClearKey();
			}
			break;
		}
		case BtnState::WaitSim:
		{
			// Is there a sim mapping on this button where the other button is in WaitSim state too?
			auto simBtn = _common->_getMatchingSimBtn(_id);
			if (pressed && simBtn)
			{
				_btnState = BtnState::SimPress;
				_press_times = time_now;                                                   // Reset Timer
				_keyToRelease.reset(new Mapping(_mapping.AtSimPress(simBtn->_id)->get())); // Make a copy
				_nameToRelease = _mapping.getSimPressName(simBtn->_id);

				simBtn->_btnState = BtnState::SimPress;
				simBtn->_press_times = time_now;
				simBtn->SyncSimPress(*this);

				_keyToRelease->ProcessEvent(BtnEvent::OnPress, *this, _nameToRelease);
			}
			else if (!pressed || GetPressDurationMS(time_now) > sim_press_window)
			{
				// Button was released before sim delay expired OR
				// Button is still pressed but Sim delay did expire
				if (_mapping.getDblPressMap())
				{
					// Start counting time between two start presses
					_btnState = BtnState::DblPressStart;
				}
				else
				{
					_btnState = BtnState::BtnPress;
					GetPressMapping()->ProcessEvent(BtnEvent::OnPress, *this, _nameToRelease);
					//_press_times = time_now;
				}
			}
			// Else let time flow, stay in this state, no output.
			break;
		}
		case BtnState::SimPress:
			if (_simPressMaster && _simPressMaster->_btnState != BtnState::SimPress)
			{
				// The master button has released! change state now!
				_btnState = BtnState::SimRelease;
				_simPressMaster = nullptr;
			}
			else if (!pressed || !_simPressMaster) // Both slave and master handle release, but only the master handles the press
			{
				ProcessButtonPress(pressed, time_now, turboTime, holdTime);
				if (_simPressMaster && _btnState != BtnState::SimPress)
				{
					// The slave button has released! Change master state now!
					_simPressMaster->_btnState = BtnState::SimRelease;
					_simPressMaster = nullptr;
				}
			}
			break;
		case BtnState::SimRelease:
			if (!pressed)
			{
				_btnState = BtnState::NoPress;
				ClearKey();
			}
			break;
		case BtnState::DblPressStart:
			if (GetPressDurationMS(time_now) > dbl_press_window)
			{
				GetPressMapping()->ProcessEvent(BtnEvent::OnPress, *this, _nameToRelease);
				_btnState = BtnState::BtnPress;
				//_press_times = time_now; // Reset Timer
			}
			else if (!pressed)
			{
				if (GetPressDurationMS(time_now) > holdTime)
				{
					_btnState = BtnState::DblPressNoPressHold;
				}
				else
				{
					_btnState = BtnState::DblPressNoPressTap;
				}
			}
			break;
		case BtnState::DblPressNoPressTap:
			if (GetPressDurationMS(time_now) > dbl_press_window)
			{
				_btnState = BtnState::BtnPress;
				_press_times = time_now; // Reset Timer to raise a tap
				GetPressMapping()->ProcessEvent(BtnEvent::OnPress, *this, _nameToRelease);
			}
			else if (pressed)
			{
				_btnState = BtnState::DblPressPress;
				_press_times = time_now;
				_keyToRelease.reset(new Mapping(_mapping.getDblPressMap()->second));
				_nameToRelease = _mapping.getName(_id);
				_mapping.getDblPressMap()->second.get().ProcessEvent(BtnEvent::OnPress, *this, _nameToRelease);
			}
			break;
		case BtnState::DblPressNoPressHold:
			if (GetPressDurationMS(time_now) > dbl_press_window)
			{
				_btnState = BtnState::BtnPress;
				// Don't reset timer to preserve hold press behaviour
				GetPressMapping()->ProcessEvent(BtnEvent::OnPress, *this, _nameToRelease);
			}
			else if (pressed)
			{
				_btnState = BtnState::DblPressPress;
				_press_times = time_now;
				_keyToRelease.reset(new Mapping(_mapping.getDblPressMap()->second));
				_nameToRelease = _mapping.getName(_id);
				_mapping.getDblPressMap()->second.get().ProcessEvent(BtnEvent::OnPress, *this, _nameToRelease);
			}
			break;
		case BtnState::DblPressPress:
			ProcessButtonPress(pressed, time_now, turboTime, holdTime);
			break;
		case BtnState::InstRelease:
		{
			if (GetPressDurationMS(time_now) > MAGIC_INSTANT_DURATION)
			{
				CheckInstantRelease(BtnEvent::OnRelease);
				_btnState = BtnState::NoPress;
				ClearKey();
			}
			break;
		}
		default:
			CERR << "Invalid button state " << _btnState << ": Resetting to NoPress" << endl;
			_btnState = BtnState::NoPress;
			break;
		}
	}
};

ostream &operator<<(ostream &out, Mapping mapping)
{
	out << mapping._command;
	return out;
}

istream &operator>>(istream &in, Mapping &mapping)
{
	string valueName(128, '\0');
	in.getline(&valueName[0], valueName.size());
	valueName.resize(strlen(valueName.c_str()));
	smatch results;
	int count = 0;

	mapping._command = valueName;
	stringstream ss;
	const char *rgx = R"(\s*([!\^]?)((\".*?\")|\w*[0-9A-Z]|\W)([\\\/+'_]?)\s*(.*))";
	while (regex_match(valueName, results, regex(rgx)) && !results[0].str().empty())
	{
		if (count > 0)
			ss << " and ";
		Mapping::ActionModifier actMod = results[1].str().empty() ? Mapping::ActionModifier::None :
		  results[1].str()[0] == '!'                              ? Mapping::ActionModifier::Instant :
		  results[1].str()[0] == '^'                              ? Mapping::ActionModifier::Toggle :
                                                                    Mapping::ActionModifier::INVALID;

		string keyStr(results[2]);

		Mapping::EventModifier evtMod = results[4].str().empty() ? Mapping::EventModifier::None :
		  results[4].str()[0] == '\\'                            ? Mapping::EventModifier::StartPress :
		  results[4].str()[0] == '+'                             ? Mapping::EventModifier::TurboPress :
		  results[4].str()[0] == '/'                             ? Mapping::EventModifier::ReleasePress :
		  results[4].str()[0] == '\''                            ? Mapping::EventModifier::TapPress :
		  results[4].str()[0] == '_'                             ? Mapping::EventModifier::HoldPress :
                                                                   Mapping::EventModifier::INVALID;

		string leftovers(results[5]);

		KeyCode key(keyStr);
		if (evtMod == Mapping::EventModifier::None)
		{
			evtMod = count == 0 ? (leftovers.empty() ? Mapping::EventModifier::StartPress : Mapping::EventModifier::TapPress) :
                                  (count == 1 ? Mapping::EventModifier::HoldPress : Mapping::EventModifier::None);
		}

		// Some exceptions :(
		if (key.code == COMMAND_ACTION && actMod == Mapping::ActionModifier::None)
		{
			// Any command actions are instant by default
			actMod = Mapping::ActionModifier::Instant;
		}
		else if (key.code == CALIBRATE && actMod == Mapping::ActionModifier::None &&
		  (evtMod == Mapping::EventModifier::TapPress || evtMod == Mapping::EventModifier::ReleasePress))
		{
			// Calibrate only makes sense on tap or release if it toggles. Also preserves legacy.
			actMod = Mapping::ActionModifier::Toggle;
		}

		if (key.code == 0 ||
		  key.code == COMMAND_ACTION && actMod != Mapping::ActionModifier::Instant ||
		  actMod == Mapping::ActionModifier::INVALID ||
		  evtMod == Mapping::EventModifier::INVALID ||
		  evtMod == Mapping::EventModifier::None && count >= 2 ||
		  evtMod == Mapping::EventModifier::ReleasePress && actMod == Mapping::ActionModifier::None ||
		  !mapping.AddMapping(key, evtMod, actMod))
		{
			//error!!!
			in.setstate(in.failbit);
			break;
		}
		else
		{
			// build string rep
			if (actMod != Mapping::ActionModifier::None)
			{
				ss << actMod << " ";
			}
			ss << key.name;
			if (count != 0 || !leftovers.empty() || evtMod != Mapping::EventModifier::StartPress) // Don't display event modifier when using default binding on single key
			{
				ss << " on " << evtMod;
			}
		}
		valueName = leftovers;
		count++;
	} // Next item

	mapping._description = ss.str();

	return in;
}

bool operator==(const Mapping &lhs, const Mapping &rhs)
{
	// Very flawfull :(
	return lhs._command == rhs._command;
}

Mapping::Mapping(in_string mapping)
{
	stringstream ss(mapping);
	ss >> *this;
	if (ss.fail())
	{
		clear();
	}
}

void Mapping::ProcessEvent(BtnEvent evt, DigitalButton &button, in_string displayName) const
{
	// COUT << button._id << " processes event " << evt << endl;
	auto entry = _eventMapping.find(evt);
	if (entry != _eventMapping.end() && entry->second) // Skip over empty entries
	{
		switch (evt)
		{
		case BtnEvent::OnPress:
			COUT << displayName << ": true" << endl;
			break;
		case BtnEvent::OnRelease:
		case BtnEvent::OnHoldRelease:
			COUT << displayName << ": false" << endl;
			break;
		case BtnEvent::OnTap:
			COUT << displayName << ": tapped" << endl;
			break;
		case BtnEvent::OnHold:
			COUT << displayName << ": held" << endl;
			break;
		case BtnEvent::OnTurbo:
			COUT << displayName << ": turbo" << endl;
			break;
		}
		//COUT << button._id << " processes event " << evt << endl;
		if (entry->second)
			entry->second(&button);
	}
}

void Mapping::InsertEventMapping(BtnEvent evt, OnEventAction action)
{
	auto existingActions = _eventMapping.find(evt);
	_eventMapping[evt] = existingActions == _eventMapping.end() ? action :
                                                                  bind(&RunBothActions, placeholders::_1, existingActions->second, action); // Chain with already existing mapping, if any
}

bool Mapping::AddMapping(KeyCode key, EventModifier evtMod, ActionModifier actMod)
{
	OnEventAction apply, release;
	if (key.code == CALIBRATE)
	{
		apply = bind(&DigitalButton::StartCalibration, placeholders::_1);
		release = bind(&DigitalButton::FinishCalibration, placeholders::_1);
		_tapDurationMs = MAGIC_EXTENDED_TAP_DURATION; // Unused in regular press
	}
	else if (key.code >= GYRO_INV_X && key.code <= GYRO_TRACKBALL)
	{
		apply = bind(&DigitalButton::ApplyGyroAction, placeholders::_1, key);
		release = bind(&DigitalButton::RemoveGyroAction, placeholders::_1);
		_tapDurationMs = MAGIC_EXTENDED_TAP_DURATION; // Unused in regular press
	}
	else if (key.code == COMMAND_ACTION)
	{
		_ASSERT_EXPR(Mapping::_isCommandValid, "You need to assign a function to this field. It should be a function that validates the command line.");
		if (!Mapping::_isCommandValid(key.name))
		{
			COUT << "Error: \"" << key.name << "\" is not a valid command" << endl;
			return false;
		}
		apply = bind(&WriteToConsole, key.name);
		release = OnEventAction();
	}
	else if (key.code == RUMBLE)
	{
		union Rumble
		{
			int raw;
			array<UCHAR, 2> bytes;
		} rumble;
		rumble.raw = stoi(key.name.substr(1, 4), nullptr, 16);
		apply = bind(&DigitalButton::SetRumble, placeholders::_1, rumble.bytes[1], rumble.bytes[0]);
		release = bind(&DigitalButton::SetRumble, placeholders::_1, 0, 0);
		_tapDurationMs = MAGIC_EXTENDED_TAP_DURATION; // Unused in regular press
	}
	else // if (key.code != NO_HOLD_MAPPED)
	{
		_hasViGEmBtn |= (key.code >= X_UP && key.code <= X_START) || key.code == PS_HOME || key.code == PS_PAD_CLICK; // Set flag if vigem button
		apply = bind(&DigitalButton::ApplyBtnPress, placeholders::_1, key);
		release = bind(&DigitalButton::ApplyBtnRelease, placeholders::_1, key);
	}

	BtnEvent applyEvt, releaseEvt;
	switch (actMod)
	{
	case ActionModifier::Toggle:
		apply = bind(&DigitalButton::ApplyButtonToggle, placeholders::_1, key, apply, release);
		release = OnEventAction();
		break;
	case ActionModifier::Instant:
	{
		OnEventAction action2 = bind(&DigitalButton::RegisterInstant, placeholders::_1, applyEvt);
		apply = bind(&Mapping::RunBothActions, placeholders::_1, apply, action2);
		releaseEvt = BtnEvent::OnInstantRelease;
	}
	break;
	case ActionModifier::INVALID:
		return false;
		// None applies no modification... Hey!
	}

	switch (evtMod)
	{
	case EventModifier::StartPress:
		applyEvt = BtnEvent::OnPress;
		releaseEvt = BtnEvent::OnRelease;
		break;
	case EventModifier::TapPress:
		applyEvt = BtnEvent::OnTap;
		releaseEvt = BtnEvent::OnTapRelease;
		break;
	case EventModifier::HoldPress:
		applyEvt = BtnEvent::OnHold;
		releaseEvt = BtnEvent::OnHoldRelease;
		break;
	case EventModifier::ReleasePress:
		// Acttion Modifier is required
		applyEvt = BtnEvent::OnRelease;
		releaseEvt = BtnEvent::INVALID;
		break;
	case EventModifier::TurboPress:
		applyEvt = BtnEvent::OnTurbo;
		releaseEvt = BtnEvent::OnTurbo;
		InsertEventMapping(BtnEvent::OnRelease, release); // On turbo you also need to clear the turbo on release
		break;
	default: // EventModifier::INVALID or None
		return false;
	}

	// Insert release first because in turbo's case apply and release are the same but we want release to happen first
	InsertEventMapping(releaseEvt, release);
	InsertEventMapping(applyEvt, apply);
	return true;
}

void Mapping::RunBothActions(DigitalButton *btn, OnEventAction action1, OnEventAction action2)
{
	if (action1)
		action1(btn);
	if (action2)
		action2(btn);
}

class ScrollAxis
{
protected:
	float _leftovers;
	ButtonID _negativeId;
	ButtonID _positiveId;
	JoyShock *_js;

	bool _pressed;

public:
	static function<void(JoyShock *, ButtonID, bool)> _handleButtonChange;

	ScrollAxis(JoyShock *js, ButtonID negativeId, ButtonID positiveId)
	  : _leftovers(0.f)
	  , _negativeId(negativeId)
	  , _positiveId(positiveId)
	  , _pressed(false)
	  , _js(js)
	{
	}

	void ProcessScroll(float distance, float sens)
	{
		_leftovers += distance;
		//if(distance != 0) COUT << "[" << _negativeId << "," << _positiveId << "] moved " << distance << " so that leftover is now " << _leftovers << endl;

		if (!_pressed && fabsf(_leftovers) > sens)
		{
			_handleButtonChange(_js, _negativeId, _leftovers > 0);
			_handleButtonChange(_js, _positiveId, _leftovers < 0);
			_leftovers = _leftovers > 0 ? _leftovers - sens : _leftovers + sens;
			_pressed = true;
		}
		else
		{
			_handleButtonChange(_js, _negativeId, false);
			_handleButtonChange(_js, _positiveId, false);
			_pressed = false;
		}
	}

	void Reset()
	{
		_leftovers = 0;
		_handleButtonChange(_js, _negativeId, false);
		_handleButtonChange(_js, _positiveId, false);
		_pressed = false;
	}
};

// An instance of this class represents a single controller device that JSM is listening to.
class JoyShock
{
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

	bool isSoftPullPressed(int triggerIndex, float triggerPosition)
	{
		float threshold = getSetting(SettingID::TRIGGER_THRESHOLD);
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
			isPressed = true;
		}
		else if (avg_t0 < avg_tm1 && avg_tm1 < avg_tm2 && avg_tm2 < avg_tm3)
		{
			isPressed = false;
		}
		else
		{
			isPressed = triggerState[triggerIndex] != DstState::NoPress;
		}
		prevTriggerPosition[triggerIndex].pop_front();
		prevTriggerPosition[triggerIndex].push_back(triggerPosition);
		return isPressed;
	}

public:
	const int MaxGyroSamples = 64;
	const int NumSamples = 64;
	int handle;
	GamepadMotion motion;
	int platform_controller_type;

	vector<DigitalButton> buttons;
	chrono::steady_clock::time_point started_flick;
	chrono::steady_clock::time_point time_now;
	// tap_release_queue has been replaced with button states *TapRelease. The hold time of the tap is effectively quantized to the polling period of the device.
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

	int controller_split_type = 0;

	float left_acceleration = 1.0;
	float right_acceleration = 1.0;
	float motion_stick_acceleration = 1.0;
	vector<DstState> triggerState; // State of analog triggers when skip mode is active
	vector<deque<float>> prevTriggerPosition;
	shared_ptr<DigitalButton::Common> btnCommon;

	// Modeshifting the stick mode can create quirky behaviours on transition. These flags
	// will be set upon returning to standard mode and ignore stick inputs until the stick
	// returns to neutral
	bool ignore_left_stick_mode = false;
	bool ignore_right_stick_mode = false;
	bool ignore_motion_stick_mode = false;

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

	JoyShock(int uniqueHandle, int controllerSplitType, shared_ptr<DigitalButton::Common> sharedButtonCommon = nullptr)
	  : handle(uniqueHandle)
	  , controller_split_type(controllerSplitType)
	  , triggerState(NUM_ANALOG_TRIGGERS, DstState::NoPress)
	  , buttons()
	  , prevTriggerPosition(NUM_ANALOG_TRIGGERS, deque<float>(MAGIC_TRIGGER_SMOOTHING, 0.f))
	  , right_scroll(this, ButtonID::RLEFT, ButtonID::RRIGHT)
	  , left_scroll(this, ButtonID::LLEFT, ButtonID::LRIGHT)
	  , _light_bar()
	  , btnCommon(sharedButtonCommon)
	{
		if (!sharedButtonCommon)
		{
			btnCommon = shared_ptr<DigitalButton::Common>(new DigitalButton::Common(
			  bind(&JoyShock::handleViGEmNotification, this, placeholders::_1, placeholders::_2, placeholders::_3)));
		}
		_light_bar = getSetting<Color>(SettingID::LIGHT_BAR);

		platform_controller_type = JslGetControllerType(handle);
		btnCommon->_getMatchingSimBtn = bind(&JoyShock::GetMatchingSimBtn, this, placeholders::_1);
		btnCommon->_rumble = bind(&JoyShock::Rumble, this, placeholders::_1, placeholders::_2);

		buttons.reserve(MAPPING_SIZE);
		for (int i = 0; i < MAPPING_SIZE; ++i)
		{
			buttons.push_back(DigitalButton(btnCommon, ButtonID(i), uniqueHandle, &motion));
		}
		ResetSmoothSample();
		if (!CheckVigemState())
		{
			virtual_controller = ControllerScheme::NONE;
		}
		JslSetLightColour(handle, _light_bar.raw);
	}

	~JoyShock()
	{
	}

	void Rumble(int smallRumble, int bigRumble)
	{
		if (getSetting<Switch>(SettingID::RUMBLE) == Switch::ON)
		{
			//COUT << "Rumbling at " << smallRumble << " and " << bigRumble << endl;
			JslSetRumble(handle, smallRumble, bigRumble);
		}
	}

	bool CheckVigemState()
	{
		if (virtual_controller.get() != ControllerScheme::NONE)
		{
			string error = "There is no controller object";
			if (!btnCommon->_vigemController || btnCommon->_vigemController->isInitialized(&error) == false)
			{
				CERR << "[ViGEm Client] " << error << endl;
				return false;
			}
			else if (btnCommon->_vigemController->getType() != virtual_controller.get())
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
		lock_guard guard(this->btnCommon->callback_lock);
		switch (platform_controller_type)
		{
		case JS_TYPE_DS4:
		case JS_TYPE_DS:
			JslSetLightColour(handle, _light_bar.raw);
			break;
		default:
			JslSetPlayerNumber(handle, indicator.led);
			break;
		}
		Rumble(smallMotor, largeMotor);
	}

	template<typename E>
	E getSetting(SettingID index)
	{
		static_assert(is_enum<E>::value, "Parameter of JoyShock::getSetting<E> has to be an enum type");
		// Look at active chord mappings starting with the latest activates chord
		for (auto activeChord = btnCommon->chordStack.begin(); activeChord != btnCommon->chordStack.end(); activeChord++)
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
			case SettingID::ZR_MODE:
				opt = GetOptionalSetting<E>(zrMode, *activeChord);
				break;
			case SettingID::ZL_MODE:
				opt = GetOptionalSetting<E>(zlMode, *activeChord);
				break;
			case SettingID::FLICK_SNAP_MODE:
				opt = GetOptionalSetting<E>(flick_snap_mode, *activeChord);
				break;
			case SettingID::TOUCHPAD_DUAL_STAGE_MODE:
				opt = GetOptionalSetting<E>(touch_ds_mode, *activeChord);
				break;
			case SettingID::RUMBLE:
				opt = GetOptionalSetting<E>(rumble_enable, *activeChord);
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
		for (auto activeChord = btnCommon->chordStack.begin(); activeChord != btnCommon->chordStack.end(); activeChord++)
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
				// SIM_PRESS_WINDOW and DBL_PRESS_WINDOW are not chorded, they can be accessed as is.
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
		for (auto activeChord = btnCommon->chordStack.begin(); activeChord != btnCommon->chordStack.end(); activeChord++)
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
			for (auto activeChord = btnCommon->chordStack.begin(); activeChord != btnCommon->chordStack.end(); activeChord++)
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
			for (auto activeChord = btnCommon->chordStack.begin(); activeChord != btnCommon->chordStack.end(); activeChord++)
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

public:
	DigitalButton *GetMatchingSimBtn(ButtonID index)
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

	inline DigitalButton *GetButton(ButtonID index)
	{
		return &buttons[int(index)];
	}

	void handleButtonChange(ButtonID id, bool pressed)
	{
		GetButton(id)->updateButtonState(pressed, time_now, getSetting(SettingID::TURBO_PERIOD), getSetting(SettingID::HOLD_PRESS_TIME));
	}

	void handleTriggerChange(ButtonID softIndex, ButtonID fullIndex, TriggerMode mode, float position)
	{
		if (mode != TriggerMode::X_LT && mode != TriggerMode::X_RT && (platform_controller_type == JS_TYPE_PRO_CONTROLLER || platform_controller_type == JS_TYPE_JOYCON_LEFT || platform_controller_type == JS_TYPE_JOYCON_RIGHT))
		{
			// Override local variable because the controller has digital triggers. Effectively ignore Full Pull binding.
			mode = TriggerMode::NO_FULL;
		}

		if (mode == TriggerMode::X_LT)
		{
			if (btnCommon->_vigemController)
				btnCommon->_vigemController->setLeftTrigger(position);
			return;
		}
		else if (mode == TriggerMode::X_RT)
		{
			if (btnCommon->_vigemController)
				btnCommon->_vigemController->setRightTrigger(position);
			return;
		}

		auto idxState = int(fullIndex) - FIRST_ANALOG_TRIGGER; // Get analog trigger index
		if (idxState < 0 || idxState >= (int)triggerState.size())
		{
			COUT << "Error: Trigger " << fullIndex << " does not exist in state map. Dual Stage Trigger not possible." << endl;
			return;
		}

		// if either trigger is waiting to be tap released, give it a go
		if (buttons[int(softIndex)]._btnState == BtnState::TapRelease)
		{
			// keep triggering until the tap release is complete
			handleButtonChange(softIndex, false);
		}
		if (buttons[int(fullIndex)]._btnState == BtnState::TapRelease)
		{
			// keep triggering until the tap release is complete
			handleButtonChange(fullIndex, false);
		}

		switch (triggerState[idxState])
		{
		case DstState::NoPress:
			// It actually doesn't matter what the last Press is. Theoretically, we could have missed the edge.
			if (isSoftPullPressed(idxState, position))
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
			else if (buttons[int(softIndex)].GetPressDurationMS(time_now) >= getSetting(SettingID::TRIGGER_SKIP_DELAY))
			{
				triggerState[idxState] = DstState::SoftPress;
				// Reset the time for hold soft press purposes.
				buttons[int(softIndex)]._press_times = time_now;
				handleButtonChange(softIndex, true);
			}
			// Else, time passes as soft press is being held, waiting to see if the soft binding should be skipped
			break;
		case DstState::PressStartResp:
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
				if (buttons[int(softIndex)].GetPressDurationMS(time_now) >= getSetting(SettingID::TRIGGER_SKIP_DELAY))
				{
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
				triggerState[idxState] = DstState::NoPress;
				handleButtonChange(softIndex, false);
			}
			else // Soft Press is being held
			{

				if ((mode == TriggerMode::MAY_SKIP || mode == TriggerMode::NO_SKIP || mode == TriggerMode::MAY_SKIP_R) && position == 1.0)
				{
					// Full press is allowed in addition to soft press
					triggerState[idxState] = DstState::DelayFullPress;
					handleButtonChange(fullIndex, true);
				}
				else if (mode == TriggerMode::NO_SKIP_EXCLUSIVE && position == 1.0)
				{
					handleButtonChange(softIndex, false);
					triggerState[idxState] = DstState::ExclFullPress;
					handleButtonChange(fullIndex, true);
				}
				else
				{
					handleButtonChange(softIndex, true);
				}
				// else ignore full press on NO_FULL and MUST_SKIP
			}
			break;
		case DstState::DelayFullPress:
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
	}

	bool IsPressed(ButtonID btn)
	{
		// Use chord stack to know if a button is pressed, because the state from the callback
		// only holds half the information when it comes to a joycon pair.
		// Also, NONE is always part of the stack (for chord handling) but NONE is never pressed.
		return btn != ButtonID::NONE && find(btnCommon->chordStack.begin(), btnCommon->chordStack.end(), btn) != btnCommon->chordStack.end();
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
};

function<void(JoyShock *, ButtonID, bool)> ScrollAxis::_handleButtonChange = [](JoyShock *js, ButtonID id, bool pressed) {
	js->handleButtonChange(id, pressed);
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
	aim_y_sign.Reset();
	aim_x_sign.Reset();
	gyro_y_sign.Reset();
	gyro_x_sign.Reset();
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
	tick_time.Reset();
	light_bar.Reset();
	scroll_sens.Reset();
	autoloadSwitch.Reset();
	hide_minimized.Reset();
	virtual_controller.Reset();
	rumble_enable.Reset();
	touch_ds_mode.Reset();

	os_mouse_speed = 1.0f;
	last_flick_and_rotation = 0.0f;
}

// Convert number to bitmap
constexpr int PlayerNumber(size_t n)
{
	int i = 0;
	while (n-- > 0)
		i = (i << 1) | 1;
	return i;
}

void connectDevices(bool mergeJoycons = true)
{
	handle_to_joyshock.clear();
	int numConnected = JslConnectDevices();
	vector<int> deviceHandles(numConnected, 0);
	if (numConnected > 0)
	{
		JslGetConnectedDeviceHandles(&deviceHandles[0], numConnected);

		for (auto handle : deviceHandles)
		{
			auto type = JslGetControllerSplitType(handle);
			auto otherJoyCon = find_if(handle_to_joyshock.begin(), handle_to_joyshock.end(),
			  [type](auto &pair) {
				  return type == JS_SPLIT_TYPE_LEFT && pair.second->controller_split_type == JS_SPLIT_TYPE_RIGHT ||
				    type == JS_SPLIT_TYPE_RIGHT && pair.second->controller_split_type == JS_SPLIT_TYPE_LEFT;
			  });
			shared_ptr<JoyShock> js = nullptr;
			if (mergeJoycons && otherJoyCon != handle_to_joyshock.end())
			{
				// The second JC points to the same common buttons as the other one.
				js.reset(new JoyShock(handle, type, otherJoyCon->second->btnCommon));
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
	//	tray->SendToast(wstring(msg.begin(), msg.end()));
	//}

	//if (numConnected != 0) {
	//	COUT << "All devices have started continuous gyro calibration" << endl;
	//}
}

void SimPressCrossUpdate(ButtonID sim, ButtonID origin, Mapping newVal)
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
		JslDisconnectAndDisposeAll();
		connectDevices(mergeJoycons);
		JslSetCallback(&joyShockPollCallback);
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

void UpdateThread(PollingThread *thread, Switch newValue)
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
		iter->second->motion.PauseContinuousCalibration();
	}
	devicesCalibrating = false;
	return true;
}

bool do_RESTART_GYRO_CALIBRATION()
{
	COUT << "Restarting continuous calibration for all devices" << endl;
	for (auto iter = handle_to_joyshock.begin(); iter != handle_to_joyshock.end(); ++iter)
	{
		iter->second->motion.ResetContinuousCalibration();
		iter->second->motion.StartContinuousCalibration();
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
		;
	}
	return true;
}

bool do_WHITELIST_SHOW()
{
	COUT << "Your PID is " << GetCurrentProcessId() << endl;
	Whitelister::ShowHIDCerberus();
	return true;
}

bool do_WHITELIST_ADD()
{
	whitelister.Add();
	if (whitelister)
	{
		COUT << "JoyShockMapper was successfully whitelisted" << endl;
	}
	else
	{
		COUT << "Whitelisting failed!" << endl;
	}
	return true;
}

bool do_WHITELIST_REMOVE()
{
	whitelister.Remove();
	COUT << "JoyShockMapper removed from whitelist" << endl;
	return true;
}

static float handleFlickStick(float calX, float calY, float lastCalX, float lastCalY, float stickLength, bool &isFlicking, shared_ptr<JoyShock> jc, float mouseCalibrationFactor, bool FLICK_ONLY, bool ROTATE_ONLY)
{
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
		float stickAngle = atan2(-offsetX, offsetY);
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
				// TODO: All these printfs should be hidden behind a setting. User might not want them.
				COUT << "Flick: " << setprecision(3) << stickAngle * (180.0f / (float)PI) << " degrees" << endl;
			}
		}
		else
		{
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
	// do the flicking
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

// https://stackoverflow.com/questions/25144887/map-unordered-map-prefer-find-and-then-at-or-try-at-catch-out-of-range
JoyShock *getJoyShockFromHandle(int handle)
{
	auto iter = handle_to_joyshock.find(handle);

	if (iter != handle_to_joyshock.end())
	{
		return iter->second.get();
		// iter is item pair in the map. The value will be accessible as `iter->second`.
	}
	return nullptr;
}

void processStick(shared_ptr<JoyShock> jc, float stickX, float stickY, float lastX, float lastY, float innerDeadzone, float outerDeadzone,
  RingMode ringMode, StickMode stickMode, ButtonID ringId, ButtonID leftId, ButtonID rightId, ButtonID upId, ButtonID downId,
  ControllerOrientation controllerOrientation, float mouseCalibrationFactor, float deltaTime, float &acceleration, FloatXY &lastAreaCal,
  bool &isFlicking, bool &ignoreStickMode, bool &anyStickInput, bool &lockMouse, float &camSpeedX, float &camSpeedY, ScrollAxis *scroll)
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

	// Stick inversion
	stickX *= jc->getSetting(SettingID::STICK_AXIS_X);
	stickY *= jc->getSetting(SettingID::STICK_AXIS_Y);
	lastX *= jc->getSetting(SettingID::STICK_AXIS_X);
	lastY *= jc->getSetting(SettingID::STICK_AXIS_Y);

	outerDeadzone = 1.0f - outerDeadzone;
	jc->processDeadZones(lastX, lastY, innerDeadzone, outerDeadzone);
	bool pegged = jc->processDeadZones(stickX, stickY, innerDeadzone, outerDeadzone);
	float absX = abs(stickX);
	float absY = abs(stickY);
	bool left = stickX < -0.5f * absY;
	bool right = stickX > 0.5f * absY;
	bool down = stickY < -0.5f * absX;
	bool up = stickY > 0.5f * absX;
	float stickLength = sqrt(stickX * stickX + stickY * stickY);
	bool ring = ringMode == RingMode::INNER && stickLength > 0.0f && stickLength < 0.7f ||
	  ringMode == RingMode::OUTER && stickLength > 0.7f;
	jc->handleButtonChange(ringId, ring);

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
				scroll->Reset();
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
				scroll->ProcessScroll(angle - lastAngle, jc->getSetting<FloatXY>(SettingID::SCROLL_SENS).x());
			}
		}
	}
	else if (stickMode == StickMode::NO_MOUSE)
	{ // Do not do if invalid
		// left!
		jc->handleButtonChange(leftId, left);
		// right!
		jc->handleButtonChange(rightId, right);
		// up!
		jc->handleButtonChange(upId, up);
		// down!
		jc->handleButtonChange(downId, down);

		anyStickInput = left | right | up | down; // ring doesn't count
	}
	else if (stickMode == StickMode::LEFT_STICK)
	{
		if (jc->btnCommon->_vigemController)
		{
			jc->btnCommon->_vigemController->setLeftStick(stickX, stickY);
		}
	}
	else if (stickMode == StickMode::RIGHT_STICK)
	{
		if (jc->btnCommon->_vigemController)
		{
			jc->btnCommon->_vigemController->setRightStick(stickX, stickY);
		}
	}
}

void joyShockPollCallback(int jcHandle, JOY_SHOCK_STATE state, JOY_SHOCK_STATE lastState, IMU_STATE imuState, IMU_STATE lastImuState, float deltaTime)
{

	shared_ptr<JoyShock> jc = handle_to_joyshock[jcHandle];
	if (jc == nullptr)
		return;
	jc->btnCommon->callback_lock.lock();
	string windowTitle;
	tie(currentWindowName, windowTitle) = GetActiveWindowName();

	auto timeNow = chrono::steady_clock::now();
	deltaTime = ((float)chrono::duration_cast<chrono::microseconds>(timeNow - jc->time_now).count()) / 1000000.0f;
	jc->time_now = timeNow;

	GamepadMotion &motion = jc->motion;

	IMU_STATE imu = JslGetIMUState(jc->handle);

	motion.ProcessMotion(imu.gyroX, imu.gyroY, imu.gyroZ, imu.accelX, imu.accelY, imu.accelZ, deltaTime);

	float inGyroX, inGyroY, inGyroZ;
	motion.GetCalibratedGyro(inGyroX, inGyroY, inGyroZ);

	float inGravX, inGravY, inGravZ;
	motion.GetGravity(inGravX, inGravY, inGravZ);
	inGravX *= 1.f / 9.8f; // to Gs
	inGravY *= 1.f / 9.8f;
	inGravZ *= 1.f / 9.8f;

	float inQuatW, inQuatX, inQuatY, inQuatZ;
	motion.GetOrientation(inQuatW, inQuatX, inQuatY, inQuatZ);

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
		jc->neutralQuatW = inQuatW;
		jc->neutralQuatX = inQuatX;
		jc->neutralQuatY = inQuatY;
		jc->neutralQuatZ = inQuatZ;
		jc->set_neutral_quat = false;
		COUT << "Neutral orientation for device " << jc->handle << " set..." << endl;
	}

	float gyroX = 0.0;
	float gyroY = 0.0;
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
	float gyroLength = sqrt(gyroX * gyroX + gyroY * gyroY);
	// do gyro smoothing
	// convert gyro smooth time to number of samples
	auto numGyroSamples = 1.f / tick_time * jc->getSetting(SettingID::GYRO_SMOOTH_TIME); // samples per second * seconds = samples
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

	jc->time_now = std::chrono::steady_clock::now();

	// sticks!
	ControllerOrientation controllerOrientation = jc->getSetting<ControllerOrientation>(SettingID::CONTROLLER_ORIENTATION);
	float camSpeedX = 0.0f;
	float camSpeedY = 0.0f;
	// account for os mouse speed and convert from radians to degrees because gyro reports in degrees per second
	float mouseCalibrationFactor = 180.0f / PI / os_mouse_speed;
	if (jc->controller_split_type != JS_SPLIT_TYPE_RIGHT)
	{
		// let's do these sticks... don't want to constantly send input, so we need to compare them to last time
		float lastCalX = jc->lastLX;
		float lastCalY = jc->lastLY;
		float calX = JslGetLeftX(jc->handle);
		float calY = JslGetLeftY(jc->handle);

		jc->lastLX = calX;
		jc->lastLY = calY;

		processStick(jc, calX, calY, lastCalX, lastCalY, jc->getSetting(SettingID::LEFT_STICK_DEADZONE_INNER), jc->getSetting(SettingID::LEFT_STICK_DEADZONE_OUTER),
		  jc->getSetting<RingMode>(SettingID::LEFT_RING_MODE), jc->getSetting<StickMode>(SettingID::LEFT_STICK_MODE),
		  ButtonID::LRING, ButtonID::LLEFT, ButtonID::LRIGHT, ButtonID::LUP, ButtonID::LDOWN, controllerOrientation,
		  mouseCalibrationFactor, deltaTime, jc->left_acceleration, jc->left_last_cal, jc->is_flicking_left, jc->ignore_left_stick_mode, leftAny, lockMouse, camSpeedX, camSpeedY, &jc->left_scroll);
	}

	if (jc->controller_split_type != JS_SPLIT_TYPE_LEFT)
	{
		float lastCalX = jc->lastRX;
		float lastCalY = jc->lastRY;
		float calX = JslGetRightX(jc->handle);
		float calY = JslGetRightY(jc->handle);

		jc->lastRX = calX;
		jc->lastRY = calY;

		processStick(jc, calX, calY, lastCalX, lastCalY, jc->getSetting(SettingID::RIGHT_STICK_DEADZONE_INNER), jc->getSetting(SettingID::RIGHT_STICK_DEADZONE_OUTER),
		  jc->getSetting<RingMode>(SettingID::RIGHT_RING_MODE), jc->getSetting<StickMode>(SettingID::RIGHT_STICK_MODE),
		  ButtonID::RRING, ButtonID::RLEFT, ButtonID::RRIGHT, ButtonID::RUP, ButtonID::RDOWN, controllerOrientation,
		  mouseCalibrationFactor, deltaTime, jc->right_acceleration, jc->right_last_cal, jc->is_flicking_right, jc->ignore_right_stick_mode, rightAny, lockMouse, camSpeedX, camSpeedY, &jc->right_scroll);
	}

	if (jc->controller_split_type == JS_SPLIT_TYPE_FULL ||
	  (jc->controller_split_type & (int)jc->getSetting<JoyconMask>(SettingID::JOYCON_MOTION_MASK)) == 0)
	{
		Quat neutralQuat = Quat(jc->neutralQuatW, jc->neutralQuatX, jc->neutralQuatY, jc->neutralQuatZ);
		Vec grav = Vec(inGravX, inGravY, inGravZ) * neutralQuat;

		float lastCalX = jc->lastMotionStickX;
		float lastCalY = jc->lastMotionStickY;
		// use gravity vector deflection
		float calX = grav.x;
		float calY = -grav.z;
		float gravLength2D = sqrtf(grav.x * grav.x + grav.z * grav.z);
		float gravStickDeflection = atan2f(gravLength2D, -grav.y) / PI;
		if (gravLength2D > 0)
		{
			calX *= gravStickDeflection / gravLength2D;
			calY *= gravStickDeflection / gravLength2D;
		}

		jc->lastMotionStickX = calX;
		jc->lastMotionStickY = calY;

		processStick(jc, calX, calY, lastCalX, lastCalY, jc->getSetting(SettingID::MOTION_DEADZONE_INNER) / 180.f, jc->getSetting(SettingID::MOTION_DEADZONE_OUTER) / 180.f,
		  jc->getSetting<RingMode>(SettingID::MOTION_RING_MODE), jc->getSetting<StickMode>(SettingID::MOTION_STICK_MODE),
		  ButtonID::MRING, ButtonID::MLEFT, ButtonID::MRIGHT, ButtonID::MUP, ButtonID::MDOWN, controllerOrientation,
		  mouseCalibrationFactor, deltaTime, jc->motion_stick_acceleration, jc->motion_last_cal, jc->is_flicking_motion, jc->ignore_motion_stick_mode, motionAny, lockMouse, camSpeedX, camSpeedY, nullptr);

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
		}
	}

	int buttons = JslGetButtons(jc->handle);

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
		// SL and SR are mapped to back paddle positions:
		jc->handleButtonChange(ButtonID::LSL, buttons & (1 << JSOFFSET_SL));
		jc->handleButtonChange(ButtonID::LSR, buttons & (1 << JSOFFSET_SR));

		float lTrigger = JslGetLeftTrigger(jc->handle);
		jc->handleTriggerChange(ButtonID::ZL, ButtonID::ZLF, jc->getSetting<TriggerMode>(SettingID::ZL_MODE), lTrigger);

		bool touch = JslGetTouchDown(jc->handle, false) || JslGetTouchDown(jc->handle, true);
		switch (jc->platform_controller_type)
		{
		case JS_TYPE_DS4:
		case JS_TYPE_DS:
		{
			float triggerpos = buttons & (1 << JSOFFSET_CAPTURE) ? 1.f :
			  touch                                              ? 0.99f :
                                                                   0.f;
			jc->handleTriggerChange(ButtonID::TOUCH, ButtonID::CAPTURE, jc->getSetting<TriggerMode>(SettingID::TOUCHPAD_DUAL_STAGE_MODE), triggerpos);
		}
		break;
		default:
		{
			jc->handleButtonChange(ButtonID::TOUCH, touch);
			jc->handleButtonChange(ButtonID::CAPTURE, buttons & (1 << JSOFFSET_CAPTURE));
		}
		break;
		}
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
		// SL and SR are mapped to back paddle positions:
		jc->handleButtonChange(ButtonID::RSL, buttons & (1 << JSOFFSET_SL));
		jc->handleButtonChange(ButtonID::RSR, buttons & (1 << JSOFFSET_SR));

		float rTrigger = JslGetRightTrigger(jc->handle);
		jc->handleTriggerChange(ButtonID::ZR, ButtonID::ZRF, jc->getSetting<TriggerMode>(SettingID::ZR_MODE), rTrigger);
	}

	// Handle buttons before GYRO because some of them may affect the value of blockGyro
	auto gyro = jc->getSetting<GyroSettings>(SettingID::GYRO_ON); // same result as getting GYRO_OFF
	switch (gyro.ignore_mode)
	{
	case GyroIgnoreMode::BUTTON:
		blockGyro = gyro.always_off ^ jc->IsPressed(gyro.button);
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

	bool trackball_x_pressed = false;
	bool trackball_y_pressed = false;

	// Apply gyro modifiers in the queue from oldest to newest (thus giving priority to most recent)
	for (auto pair : jc->btnCommon->gyroActionQueue)
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
	// optionally ignore the gyro of one of the joycons
	if (!lockMouse &&
	  (jc->controller_split_type == JS_SPLIT_TYPE_FULL ||
	    (jc->controller_split_type & (int)jc->getSetting<JoyconMask>(SettingID::JOYCON_GYRO_MASK)) == 0))
	{
		//COUT << "GX: %0.4f GY: %0.4f GZ: %0.4f\n", imuState.gyroX, imuState.gyroY, imuState.gyroZ);
		float mouseCalibration = jc->getSetting(SettingID::REAL_WORLD_CALIBRATION) / os_mouse_speed / jc->getSetting(SettingID::IN_GAME_SENS);
		shapedSensitivityMoveMouse(gyroX * gyro_x_sign_to_use, gyroY * gyro_y_sign_to_use, jc->getSetting<FloatXY>(SettingID::MIN_GYRO_SENS), jc->getSetting<FloatXY>(SettingID::MAX_GYRO_SENS),
		  jc->getSetting(SettingID::MIN_GYRO_THRESHOLD), jc->getSetting(SettingID::MAX_GYRO_THRESHOLD), deltaTime,
		  camSpeedX * jc->getSetting(SettingID::STICK_AXIS_X), -camSpeedY * jc->getSetting(SettingID::STICK_AXIS_Y), mouseCalibration);
	}
	if (jc->btnCommon->_vigemController)
	{
		jc->btnCommon->_vigemController->update(); // Check for initialized built-in
	}
	auto newColor = jc->getSetting<Color>(SettingID::LIGHT_BAR);
	if (jc->_light_bar != newColor)
	{
		JslSetLightColour(jc->handle, newColor.raw);
		jc->_light_bar = newColor;
	}
	jc->btnCommon->callback_lock.unlock();
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
	static string lastModuleName;

	auto registry = reinterpret_cast<CmdRegistry *>(param);
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

		if (Whitelister::IsHIDCerberusRunning())
		{
			tray->AddMenuItem(
			  U("Whitelist"), [](bool isChecked) {
				  isChecked ?
                    whitelister.Add() :
                    whitelister.Remove();
			  },
			  bind(&Whitelister::operator bool, &whitelister));
		}
		tray->AddMenuItem(
		  U("Calibrate all devices"), [](bool isChecked) { isChecked ?
                                                             WriteToConsole("RESTART_GYRO_CALIBRATION") :
                                                             WriteToConsole("FINISH_GYRO_CALIBRATION"); }, []() { return devicesCalibrating; });

		string autoloadFolder{ AUTOLOAD_FOLDER() };
		for (auto file : ListDirectory(autoloadFolder.c_str()))
		{
			string fullPathName = autoloadFolder + file;
			auto noext = file.substr(0, file.find_last_of('.'));
			tray->AddMenuItem(U("AutoLoad folder"), UnicodeString(noext.begin(), noext.end()), [fullPathName] {
				WriteToConsole(string(fullPathName.begin(), fullPathName.end()));
				autoLoadThread->Stop();
			});
		}
		std::string gyroConfigsFolder{ GYRO_CONFIGS_FOLDER() };
		for (auto file : ListDirectory(gyroConfigsFolder.c_str()))
		{
			string fullPathName = gyroConfigsFolder + file;
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
	tray->Hide();
	HideConsole();
	JslDisconnectAndDisposeAll();
	handle_to_joyshock.clear(); // Destroy Vigem Gamepads
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
      next :
      current;
}

template<typename E, E invalid>
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
      next :
      current;
}

float filterHoldPressDelay(float c, float next)
{
	if (next <= sim_press_window || next >= dbl_press_window)
	{
		CERR << SettingID::HOLD_PRESS_TIME << " can only be set to a value between those of " << SettingID::SIM_PRESS_WINDOW << " (" << sim_press_window << "ms) and " << SettingID::DBL_PRESS_WINDOW << " (" << dbl_press_window << "ms) exclusive." << endl;
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
		if (JslGetControllerType(js.first) != JS_TYPE_DS4 && next != TriggerMode::NO_FULL)
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

StickMode filterStickMode(StickMode current, StickMode next)
{
	if (next == StickMode::LEFT_STICK || next == StickMode::RIGHT_STICK)
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

ControllerScheme UpdateVirtualController(ControllerScheme prevScheme, ControllerScheme nextScheme)
{
	bool success = true;
	for (auto &js : handle_to_joyshock)
	{
		if (!js.second->btnCommon->_vigemController ||
		  js.second->btnCommon->_vigemController->getType() != nextScheme)
		{
			if (nextScheme == ControllerScheme::NONE)
			{
				js.second->btnCommon->_vigemController.reset(nullptr);
			}
			else
			{
				js.second->btnCommon->_vigemController.reset(new Gamepad(nextScheme, bind(&JoyShock::handleViGEmNotification, js.second.get(), placeholders::_1, placeholders::_2, placeholders::_3)));
				success &= js.second->btnCommon->_vigemController->isInitialized();
			}
		}
	}
	return success ? nextScheme : prevScheme;
}

void OnVirtualControllerChange(ControllerScheme newScheme)
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

	virtual void DisplayNewValue(GyroSettings value) override
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
	}
};

#ifdef _WIN32
int __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPWSTR cmdLine, int cmdShow)
{
	auto trayIconData = hInstance;
#else
int main(int argc, char *argv[])
{
	static_cast<void>(argc);
	static_cast<void>(argv);
	void *trayIconData = nullptr;
#endif // _WIN32
	mappings.reserve(MAPPING_SIZE);
	for (int id = 0; id < MAPPING_SIZE; ++id)
	{
		JSMButton newButton(ButtonID(id), Mapping::NO_MAPPING);
		newButton.SetFilter(&filterMapping);
		mappings.push_back(newButton);
	}
	// console
	initConsole();
	ColorStream<&cout, FOREGROUND_GREEN | FOREGROUND_INTENSITY>() << "Welcome to JoyShockMapper version " << version << '!' << endl;
	//if (whitelister) COUT << "JoyShockMapper was successfully whitelisted!" << endl;
	// Threads need to be created before listeners
	CmdRegistry commandRegistry;
	minimizeThread.reset(new PollingThread("Minimize thread", &MinimizePoll, nullptr, 1000, hide_minimized.get() == Switch::ON));          // Start by default
	autoLoadThread.reset(new PollingThread("AutoLoad thread", &AutoLoadPoll, &commandRegistry, 1000, autoloadSwitch.get() == Switch::ON)); // Start by default

	if (autoLoadThread && autoLoadThread->isRunning())
	{
		COUT << "AUTOLOAD is enabled. Files in ";
		COUT_INFO << AUTOLOAD_FOLDER();
		COUT << " folder will get loaded automatically when a matching application is in focus." << endl;
	}
	else
	{
		CERR << "AutoLoad is unavailable" << endl;
	}

	left_stick_mode.SetFilter(&filterStickMode)->AddOnChangeListener(bind(&UpdateRingModeFromStickMode, &left_ring_mode, ::placeholders::_1));
	right_stick_mode.SetFilter(&filterStickMode)->AddOnChangeListener(bind(&UpdateRingModeFromStickMode, &right_ring_mode, ::placeholders::_1));
	motion_stick_mode.SetFilter(&filterStickMode)->AddOnChangeListener(bind(&UpdateRingModeFromStickMode, &motion_ring_mode, ::placeholders::_1));
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
	zlMode.SetFilter(&filterTriggerMode);
	zrMode.SetFilter(&filterTriggerMode);
	flick_snap_mode.SetFilter(&filterInvalidValue<FlickSnapMode, FlickSnapMode::INVALID>);
	min_gyro_sens.SetFilter(&filterFloatPair);
	max_gyro_sens.SetFilter(&filterFloatPair);
	min_gyro_threshold.SetFilter(&filterFloat);
	max_gyro_threshold.SetFilter(&filterFloat);
	stick_power.SetFilter(&filterFloat);
	real_world_calibration.SetFilter(&filterFloat);
	in_game_sens.SetFilter(bind(&fmaxf, 0.0001f, ::placeholders::_2));
	trigger_threshold.SetFilter(&filterFloat);
	aim_x_sign.SetFilter(&filterInvalidValue<AxisMode, AxisMode::INVALID>);
	aim_y_sign.SetFilter(&filterInvalidValue<AxisMode, AxisMode::INVALID>);
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
	virtual_controller.SetFilter(&UpdateVirtualController)->AddOnChangeListener(&OnVirtualControllerChange);
	rumble_enable.SetFilter(&filterInvalidValue<Switch, Switch::INVALID>);
	scroll_sens.SetFilter(&filterFloatPair);
	touch_ds_mode.SetFilter(&filterTouchpadDualStageMode);
	// light_bar needs no filter or listener. The callback polls and updates the color.
#if _WIN32
	currentWorkingDir = string(&cmdLine[0], &cmdLine[wcslen(cmdLine)]);
#else
	currentWorkingDir = string(argv[0]);
#endif
	for (auto &mapping : mappings) // Add all button mappings as commands
	{
		commandRegistry.Add((new JSMAssignment<Mapping>(mapping.getName(), mapping))->SetHelp(buttonHelpMap.at(mapping._id)));
	}
	// SL and SR are shorthand for two different mappings
	commandRegistry.Add(new JSMAssignment<Mapping>("SL", "", mappings[(int)ButtonID::LSL], true));
	commandRegistry.Add(new JSMAssignment<Mapping>("SL", "", mappings[(int)ButtonID::RSL], true));
	commandRegistry.Add(new JSMAssignment<Mapping>("SR", "", mappings[(int)ButtonID::LSR], true));
	commandRegistry.Add(new JSMAssignment<Mapping>("SR", "", mappings[(int)ButtonID::RSR], true));

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
	commandRegistry.Add((new JSMAssignment<AxisMode>(aim_x_sign))
	                      ->SetHelp("When in AIM mode, set stick X axis inversion. Valid values are the following:\nSTANDARD or 1, and INVERTED or -1"));
	commandRegistry.Add((new JSMAssignment<AxisMode>(aim_y_sign))
	                      ->SetHelp("When in AIM mode, set stick Y axis inversion. Valid values are the following:\nSTANDARD or 1, and INVERTED or -1"));
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
	commandRegistry.Add((new JSMAssignment<TriggerMode>(zlMode))
	                      ->SetHelp("Controllers with a right analog trigger can use one of the following dual stage trigger modes:\nNO_FULL, NO_SKIP, MAY_SKIP, MUST_SKIP, MAY_SKIP_R, MUST_SKIP_R, NO_SKIP_EXCLUSIVE, X_LT, X_RT, PS_L2, PS_R2"));
	commandRegistry.Add((new JSMAssignment<TriggerMode>(zrMode))
	                      ->SetHelp("Controllers with a left analog trigger can use one of the following dual stage trigger modes:\nNO_FULL, NO_SKIP, MAY_SKIP, MUST_SKIP, MAY_SKIP_R, MUST_SKIP_R, NO_SKIP_EXCLUSIVE, X_LT, X_RT, PS_L2, PS_R2"));
	auto *autoloadCmd = new JSMAssignment<Switch>("AUTOLOAD", autoloadSwitch);
	commandRegistry.Add(autoloadCmd);
	currentWorkingDir.AddOnChangeListener(bind(&RefreshAutoLoadHelp, autoloadCmd), true);
	commandRegistry.Add((new JSMMacro("README"))->SetMacro(bind(&do_README))->SetHelp("Open the latest JoyShockMapper README in your browser."));
	commandRegistry.Add((new JSMMacro("WHITELIST_SHOW"))->SetMacro(bind(&do_WHITELIST_SHOW))->SetHelp("Open HIDCerberus configuration page in your browser."));
	commandRegistry.Add((new JSMMacro("WHITELIST_ADD"))->SetMacro(bind(&do_WHITELIST_ADD))->SetHelp("Add JoyShockMapper to HIDGuardian whitelisted applications."));
	commandRegistry.Add((new JSMMacro("WHITELIST_REMOVE"))->SetMacro(bind(&do_WHITELIST_REMOVE))->SetHelp("Remove JoyShockMapper from HIDGuardian whitelisted applications."));
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
	commandRegistry.Add((new JSMAssignment<Switch>("HIDE_MINIMIZED", hide_minimized))
	                      ->SetHelp("When enabled, JoyShockMapper disappears from the taskbar when minimized, leaving only the try icon to access it."));
	commandRegistry.Add((new JSMAssignment<Color>(light_bar))
	                      ->SetHelp("Changes the color bar of the DS4. Either enter as a hex code (xRRGGBB), as three decimal values between 0 and 255 (RRR GGG BBB), or as a common color name in all caps and underscores."));
	commandRegistry.Add(new HelpCmd(commandRegistry));
	commandRegistry.Add((new JSMAssignment<ControllerScheme>(magic_enum::enum_name(SettingID::VIRTUAL_CONTROLLER).data(), virtual_controller))
	                      ->SetHelp("Sets the vigem virtual controller type. Can be NONE (default), XBOX (360) or DS4 (PS4)."));
	commandRegistry.Add((new JSMAssignment<FloatXY>(scroll_sens))
	                      ->SetHelp("Scrolling sensitivity for sticks."));
	commandRegistry.Add((new JSMAssignment<Switch>(rumble_enable))
	                      ->SetHelp("Disable the rumbling feature from vigem. Valid values are ON and OFF."));
	commandRegistry.Add((new JSMAssignment<TriggerMode>(touch_ds_mode))
	                      ->SetHelp("Dual stage mode for the touchpad TOUCH and CAPTURE (i.e. click) bindings."));
	commandRegistry.Add((new JSMMacro("CLEAR"))->SetMacro(bind(&ClearConsole))->SetHelp("Removes all text in the console screen"));

	bool quit = false;
	commandRegistry.Add((new JSMMacro("QUIT"))
	                      ->SetMacro([&quit](JSMMacro *, in_string) {
		                      quit = true;
		                      return true;
	                      })
	                      ->SetHelp("Close the application."));

	Mapping::_isCommandValid = bind(&CmdRegistry::isCommandValid, &commandRegistry, placeholders::_1);

	connectDevices();
	JslSetCallback(&joyShockPollCallback);
	tray.reset(new TrayIcon(trayIconData, &beforeShowTrayMenu));
	tray->Show();

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
