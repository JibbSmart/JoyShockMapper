#include "JoyShockMapper.h"
#include "JoyShockLibrary.h"
#include "InputHelpers.h"
#include "Whitelister.h"
#include "TrayIcon.h"
#include "JSMAssignment.hpp"
#include "quatMaths.cpp"

#include <mutex>
#include <deque>

#pragma warning(disable:4996) // Disable deprecated API warnings

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
JSMSetting<JoyconMask> joycon_motion_mask = JSMSetting<JoyconMask>(SettingID::JOYCON_GYRO_MASK, JoyconMask::IGNORE_RIGHT);
JSMSetting<TriggerMode> zlMode = JSMSetting<TriggerMode>(SettingID::ZL_MODE, TriggerMode::NO_FULL);
JSMSetting<TriggerMode> zrMode = JSMSetting<TriggerMode>(SettingID::ZR_MODE, TriggerMode::NO_FULL);;
JSMSetting<FlickSnapMode> flick_snap_mode = JSMSetting<FlickSnapMode>(SettingID::FLICK_SNAP_MODE, FlickSnapMode::NONE);
JSMSetting<FloatXY> min_gyro_sens = JSMSetting<FloatXY>(SettingID::MIN_GYRO_SENS, { 0.0f, 0.0f });
JSMSetting<FloatXY> max_gyro_sens = JSMSetting<FloatXY>(SettingID::MAX_GYRO_SENS, { 0.0f, 0.0f });
JSMSetting<float> min_gyro_threshold = JSMSetting<float>(SettingID::MIN_GYRO_THRESHOLD, 0.0f);
JSMSetting<float> max_gyro_threshold = JSMSetting<float>(SettingID::MAX_GYRO_THRESHOLD, 0.0f);
JSMSetting<float> stick_power = JSMSetting<float>(SettingID::STICK_POWER, 1.0f);
JSMSetting<FloatXY> stick_sens = JSMSetting<FloatXY>(SettingID::STICK_SENS, {360.0f, 360.0f});
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
JSMVariable<PathString> currentWorkingDir = JSMVariable<PathString>(GetCWD());
JSMVariable<Switch> autoloadSwitch = JSMVariable<Switch>(Switch::ON);
vector<JSMButton> mappings; // array enables use of for each loop and other i/f

mutex loading_lock;

float os_mouse_speed = 1.0;
float last_flick_and_rotation = 0.0;
unique_ptr<PollingThread> autoLoadThread;
unique_ptr<TrayIcon> tray;
bool devicesCalibrating = false;
Whitelister whitelister(false);
unordered_map<int, shared_ptr<JoyShock>> handle_to_joyshock;

// This class holds all the logic related to a single digital button. It does not hold the mapping but only a reference
// to it. It also contains it's various states, flags and data.
class DigitalButton
{
public:
	// All digital buttons need a reference to the same instance of a the common structure within the same controller.
	// It enables the buttons to synchronize and be aware of the state of the whole controller.
	struct Common {
		Common()
		{
			chordStack.push_front(ButtonID::NONE); //Always hold mapping none at the end to handle modeshifts and chords
			_referenceCount++;
		}
		deque<pair<ButtonID, KeyCode>> gyroActionQueue; // Queue of gyro control actions currently in effect
		deque<pair<ButtonID, KeyCode>> activeTogglesQueue;
		deque<ButtonID> chordStack; // Represents the current active buttons in order from most recent to latest

		private:
		int _referenceCount;

		public:
		void IncrementReferenceCounter()
		{
			_referenceCount++;
		}

		bool DecrementReferenceCounter()
		{
			_referenceCount--;
			if (_referenceCount == 0)
			{
				delete this;
				return false;
			}
			return true;
		}
	};

	static bool findQueueItem(pair<ButtonID, KeyCode> &pair, ButtonID btn)
	{
		return btn == pair.first;
	}


	DigitalButton(DigitalButton::Common* btnCommon, ButtonID id, int deviceHandle)
		: _id(id)
		, _common(btnCommon)
		, _mapping(mappings[int(_id)])
		, _press_times()
		, _btnState(BtnState::NoPress)
		, _keyToRelease()
		, _turboCount(0)
		, _simPressMaster(ButtonID::NONE)
		, _instantReleaseQueue()
		, _deviceHandle(deviceHandle)
	{
		_instantReleaseQueue.reserve(2);
	}

	const ButtonID _id; // Always ID first for easy debugging
	Common* _common;
	const JSMButton &_mapping;
	chrono::steady_clock::time_point _press_times;
	BtnState _btnState = BtnState::NoPress;
	unique_ptr<Mapping> _keyToRelease; // At key press, remember what to release
	string _nameToRelease;
	unsigned int _turboCount;
	ButtonID _simPressMaster;
	vector<BtnEvent> _instantReleaseQueue;
	int _deviceHandle;

	bool CheckInstantRelease(BtnEvent instantEvent)
	{
		auto instant = find(_instantReleaseQueue.begin(), _instantReleaseQueue.end(), instantEvent);
		if (instant != _instantReleaseQueue.end())
		{
			//cout << "Button " << _id << " releases instant " << instantEvent << endl;
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
					_keyToRelease.reset( new Mapping(*binding));
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
		printf("Starting continuous calibration\n");
		JslResetContinuousCalibration(_deviceHandle);
		JslStartContinuousCalibration(_deviceHandle);
	}

	void FinishCalibration()
	{
		JslPauseContinuousCalibration(_deviceHandle);
		printf("Gyro calibration set\n");
		ClearAllActiveToggle(KeyCode("CALIBRATE"));
	}

	void ApplyGyroAction(KeyCode gyroAction) // TODO: Keycode should be WORD here
	{
		_common->gyroActionQueue.push_back({ _id, gyroAction });
	}

	void RemoveGyroAction()
	{
		auto gyroAction = find_if(_common->gyroActionQueue.begin(), _common->gyroActionQueue.end(),
			[this](auto pair)
			{
				// On a sim press, release the master button (the one who triggered the press)
				return pair.first == (_simPressMaster != ButtonID::NONE ? _simPressMaster : _id);
			});
		if (gyroAction != _common->gyroActionQueue.end())
		{
			ClearAllActiveToggle(gyroAction->second);
			_common->gyroActionQueue.erase(gyroAction);
		}
	}

	void ApplyBtnPress(KeyCode keyCode)
	{
		if(keyCode.code != NO_HOLD_MAPPED)
		{
			pressKey(keyCode, true);
		}
	}

	void ApplyBtnRelease(KeyCode key)
	{
		if (key.code != NO_HOLD_MAPPED)
		{
			pressKey(key, false);
			ClearAllActiveToggle(key);
		}
	}

	void ApplyButtonToggle(KeyCode key, function<void(DigitalButton *)> apply, function<void(DigitalButton *)> release)
	{
		auto currentlyActive = find_if(_common->activeTogglesQueue.begin(), _common->activeTogglesQueue.end(),
			[this, key](pair<ButtonID, KeyCode> pair)
			{
				return pair.first == _id && pair.second == key;
			});
		if (currentlyActive == _common->activeTogglesQueue.end())
		{
			apply(this);
			_common->activeTogglesQueue.push_front( { _id, key } );
		}
		else
		{
			release(this); // The bound action here should always erase the active toggle from the queue
		}
	}

	void RegisterInstant(BtnEvent evt)
	{
		//cout << "Button " << _id << " registers instant " << evt << endl;
		_instantReleaseQueue.push_back(evt);
	}

	void ClearAllActiveToggle(KeyCode key)
	{
		std::function<bool(pair<ButtonID, KeyCode>)> isSameKey = [key](pair<ButtonID, KeyCode> pair)
		{
			return pair.second == key;
		};

		for(auto currentlyActive = find_if(_common->activeTogglesQueue.begin(), _common->activeTogglesQueue.end(), isSameKey);
			currentlyActive != _common->activeTogglesQueue.end();
			currentlyActive = find_if(_common->activeTogglesQueue.begin(), _common->activeTogglesQueue.end(), isSameKey))
		{
			_common->activeTogglesQueue.erase(currentlyActive);
		}
	}

	void SyncSimPress(ButtonID btn, const ComboMap &map)
	{
		_keyToRelease.reset(new Mapping(*_mapping.AtSimPress(btn)));
		_nameToRelease = _mapping.getSimPressName(btn);
		_simPressMaster = btn;
		//cout << btn << " is the master button" << endl;
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
};

ostream &operator << (ostream &out, Mapping mapping)
{
	out << mapping.command;
	return out;
}

istream &operator >> (istream &in, Mapping &mapping)
{
	string valueName(128, '\0');
	in.getline(&valueName[0], valueName.size());
	valueName.resize(strlen(valueName.c_str()));
	smatch results;
	int count = 0;

	mapping.command = valueName;
	stringstream ss;

	while (regex_match(valueName, results, regex(R"(\s*([!\^]?)((\".*?\")|\w*[0-9A-Z]|\W)([\\\/+'_]?)\s*(.*))")) && !results[0].str().empty())
	{
		if (count > 0) ss << " and ";
		Mapping::ActionModifier actMod = results[1].str().empty() ? Mapping::ActionModifier::None :
			results[1].str()[0] == '!' ? Mapping::ActionModifier::Instant :
			results[1].str()[0] == '^' ? Mapping::ActionModifier::Toggle :
			Mapping::ActionModifier::INVALID;

		string keyStr(results[2]);

		Mapping::EventModifier evtMod = results[4].str().empty() ? Mapping::EventModifier::None :
			results[4].str()[0] == '\\' ? Mapping::EventModifier::StartPress :
			results[4].str()[0] == '+'  ? Mapping::EventModifier::TurboPress :
			results[4].str()[0] == '/'  ? Mapping::EventModifier::ReleasePress :
			results[4].str()[0] == '\'' ? Mapping::EventModifier::TapPress :
			results[4].str()[0] == '_' ? Mapping::EventModifier::HoldPress :
			Mapping::EventModifier::INVALID;

		string leftovers(results[5]);

		KeyCode key(keyStr);
		if (evtMod == Mapping::EventModifier::None)
		{
			evtMod = count == 0 ? (leftovers.empty() ? Mapping::EventModifier::StartPress : Mapping::EventModifier::TapPress) :
				                         (count == 1 ? Mapping::EventModifier::HoldPress  : Mapping::EventModifier::None);
		}

		// Some exceptions :(
		if (key.code == COMMAND_ACTION && actMod == Mapping::ActionModifier::None)
		{
			// Any command actions are instant by default
			actMod = Mapping::ActionModifier::Instant;
		}
		else if (key.code == CALIBRATE && actMod == Mapping::ActionModifier::None &&
			(evtMod == Mapping::EventModifier::TapPress || evtMod == Mapping::EventModifier::ReleasePress) )
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

	mapping.description = ss.str();

	return in;
}

bool operator ==(const Mapping &lhs, const Mapping &rhs)
{
	// Very flawfull :(
	return lhs.command == rhs.command;
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
	// cout << button._id << " processes event " << evt << endl;
	auto entry = eventMapping.find(evt);
	if (entry != eventMapping.end() && entry->second) // Skip over empty entries
	{
		switch (evt)
		{
		case BtnEvent::OnPress:
			cout << displayName << ": true" << endl;
			break;
		case BtnEvent::OnRelease:
		case BtnEvent::OnHoldRelease:
			cout << displayName << ": false" << endl;
			break;
		case BtnEvent::OnTap:
			cout << displayName << ": tapped" << endl;
			break;
		case BtnEvent::OnHold:
			cout << displayName << ": held" << endl;
			break;
		}
		//cout << button._id << " processes event " << evt << endl;
		if(entry->second)
			entry->second(&button);
	}
}

void Mapping::InsertEventMapping(BtnEvent evt, OnEventAction action) {
    auto existingActions = eventMapping.find(evt);
    //eventMapping[evt] = existingActions == eventMapping.end() ? action :
    //bind(&RunAllActions, placeholders::_1, 2, existingActions->second,
    //     action); // Chain with already existing mapping, if any

    if (existingActions == eventMapping.end()) {
        eventMapping[evt] = action;
    } else {
        eventMapping[evt] = [=](DigitalButton* btn) {
            if(existingActions->second) existingActions->second(btn);
            if(action) action(btn);
        };
    }
}

bool Mapping::AddMapping(KeyCode key, EventModifier evtMod, ActionModifier actMod)
{
	OnEventAction apply, release;
	if (key.code == CALIBRATE)
	{
		apply = bind(&DigitalButton::StartCalibration, placeholders::_1);
		release = bind(&DigitalButton::FinishCalibration, placeholders::_1);
		tapDurationMs = MAGIC_EXTENDED_TAP_DURATION; // Unused in regular press
	}
	else if (key.code >= GYRO_INV_X && key.code <= GYRO_TRACKBALL)
	{
		apply = bind(&DigitalButton::ApplyGyroAction, placeholders::_1, key);
		release = bind(&DigitalButton::RemoveGyroAction, placeholders::_1);
		tapDurationMs = MAGIC_EXTENDED_TAP_DURATION; // Unused in regular press
	}
	else if (key.code == COMMAND_ACTION)
	{
		_ASSERT_EXPR(Mapping::_isCommandValid, "You need to assign a function to this field. It should be a function that validates the command line.");
		if (!Mapping::_isCommandValid(key.name))
		{
			cout << "Error: \"" << key.name << "\" is not a valid command" << endl;
			return false;
		}
		apply = bind(&WriteToConsole, key.name);
		release = OnEventAction();
	}
	else // if (key.code != NO_HOLD_MAPPED)
	{
		apply = bind(&DigitalButton::ApplyBtnPress, placeholders::_1, key);
		release = bind(&DigitalButton::ApplyBtnRelease, placeholders::_1, key);
	}

	BtnEvent applyEvt, releaseEvt;
	switch(evtMod)
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
		break;
	default: // EventModifier::INVALID or None
		return false;
	}

	switch (actMod)
	{
	case ActionModifier::Toggle:
		apply = bind(&DigitalButton::ApplyButtonToggle, placeholders::_1, key, apply, release);
		release = OnEventAction();
		break;
	case ActionModifier::Instant:
	{
		OnEventAction action2 = bind(&DigitalButton::RegisterInstant, placeholders::_1, applyEvt);
		//apply = bind(&Mapping::RunAllActions, placeholders::_1, 2, apply, action2);
		apply = [=](DigitalButton* btn) {
		    if(apply) apply(btn);
		    if(action2) action2(btn);
		};
		releaseEvt = BtnEvent::OnInstantRelease;
	} break;
	case ActionModifier::INVALID:
		return false;
	// None applies no modification... Hey!
	}

	// Insert release first because in turbo's case apply and release are the same but we want release to apply first
	InsertEventMapping(releaseEvt, release);
	InsertEventMapping(applyEvt, apply);
	return true;
}


void Mapping::RunAllActions(DigitalButton *btn, int numEventActions, ...)
{
    /*
	va_list arguments;
	va_start(arguments, numEventActions);
	for (int x = 0; x < numEventActions; x++)
	{
		auto action = va_arg(arguments, OnEventAction);
		if(action)
			action(btn);
	}
	va_end(arguments);
	return;
     */
}

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
		throw invalid_argument(ss.str().c_str());
	}

public:
	const int MaxGyroSamples = 64;
	const int NumSamples = 64;
	int intHandle;
	mutex callback_lock;

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

	float poll_rate;
	int controller_type = 0;
	float stick_step_size;

	float left_acceleration = 1.0;
	float right_acceleration = 1.0;
	float motion_stick_acceleration = 1.0;
	vector<DstState> triggerState; // State of analog triggers when skip mode is active
	DigitalButton::Common* btnCommon;

	// Modeshifting the stick mode can create quirky behaviours on transition. These flags
	// will be set upon returning to standard mode and ignore stick inputs until the stick
	// returns to neutral
	bool ignore_left_stick_mode = false;
	bool ignore_right_stick_mode = false;
	bool ignore_motion_stick_mode = false;

	float lastMotionStickX = 0.0f;
	float lastMotionStickY = 0.0f;

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

	JoyShock(int uniqueHandle, float pollRate, int controllerSplitType, float stickStepSize)
		: intHandle(uniqueHandle)
		, poll_rate(pollRate)
		, controller_type(controllerSplitType)
		, stick_step_size(stickStepSize)
		, triggerState(NUM_ANALOG_TRIGGERS, DstState::NoPress)
		, buttons()
	{
		btnCommon = new DigitalButton::Common();
		buttons.reserve(MAPPING_SIZE);
		for (int i = 0; i < MAPPING_SIZE; ++i)
		{
			buttons.push_back( DigitalButton(btnCommon, ButtonID(i), uniqueHandle) );
		}
		ResetSmoothSample();
	}

	JoyShock(int uniqueHandle, float pollRate, int controllerSplitType, float stickStepSize, DigitalButton::Common* sharedButtonCommon)
	  : intHandle(uniqueHandle)
	  , poll_rate(pollRate)
	  , controller_type(controllerSplitType)
	  , stick_step_size(stickStepSize)
	  , triggerState(NUM_ANALOG_TRIGGERS, DstState::NoPress)
	  , btnCommon(sharedButtonCommon)
	  , buttons()
	{
		btnCommon->IncrementReferenceCounter();
		buttons.reserve(MAPPING_SIZE);
		for (int i = 0; i < MAPPING_SIZE; ++i)
		{
			buttons.push_back(DigitalButton(btnCommon, ButtonID(i), uniqueHandle));
		}
		ResetSmoothSample();
	}

	~JoyShock()
	{
		btnCommon->DecrementReferenceCounter();
	}

	template<typename E>
	E getSetting(SettingID index)
	{
		static_assert(is_enum<E>::value, "Parameter of JoyShock::getSetting<E> has to be an enum type");
		// Look at active chord mappings starting with the latest activates chord
		for (auto activeChord = btnCommon->chordStack.begin(); activeChord != btnCommon->chordStack.end(); activeChord++)
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
			}
			if (opt) return *opt;
		}// Check next Chord

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
				if (opt) return *opt;
			}
		}
		stringstream ss;
		ss << "Index " << index << " is not a valid GyroSetting";
		throw invalid_argument(ss.str().c_str());
	}

public:

	const ComboMap *GetMatchingSimMap(ButtonID index)
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
		auto foundChord = find(btnCommon->chordStack.begin(), btnCommon->chordStack.end(), index);
		if (!pressed)
		{
			if (foundChord != btnCommon->chordStack.end())
			{
				//cout << "Button " << index << " is released!" << endl;
				btnCommon->chordStack.erase(foundChord); // The chord is released
			}
		}
		else if (foundChord == btnCommon->chordStack.end()) {
			//cout << "Button " << index << " is pressed!" << endl;
			btnCommon->chordStack.push_front(index); // Always push at the fromt to make it a stack
		}

		DigitalButton &button = buttons[int(index)];

		switch (button._btnState)
		{
		case BtnState::NoPress:
			if (pressed)
			{
				button._press_times = time_now;
				if (button._mapping.HasSimMappings())
				{
					button._btnState = BtnState::WaitSim;
				}
				else if (button._mapping.getDblPressMap())
				{
					// Start counting time between two start presses
					button._btnState = BtnState::DblPressStart;
				}
				else
				{
					button._btnState = BtnState::BtnPress;
					button.GetPressMapping()->ProcessEvent(BtnEvent::OnPress, button, button._nameToRelease);
				}
			}
			break;
		case BtnState::BtnPress:
			button.ProcessButtonPress(pressed, time_now, getSetting(SettingID::TURBO_PERIOD), getSetting(SettingID::HOLD_PRESS_TIME));
			break;
		case BtnState::TapRelease:
		{
			if (pressed || button.GetPressDurationMS(time_now) > MAGIC_INSTANT_DURATION)
			{
				button.CheckInstantRelease(BtnEvent::OnRelease);
				button.CheckInstantRelease(BtnEvent::OnTap);
			}
			if (pressed || button.GetPressDurationMS(time_now) > button._keyToRelease->getTapDuration())
			{
				button.GetPressMapping()->ProcessEvent(BtnEvent::OnTapRelease, button, button._nameToRelease);
				button._btnState = BtnState::NoPress;
				button.ClearKey();
			}
			break;
		}
		case BtnState::WaitSim:
		{
			// Is there a sim mapping on this button where the other button is in WaitSim state too?
			auto simMap = GetMatchingSimMap(index);
			if (pressed && simMap)
			{
				button._btnState = BtnState::SimPress;
				button._press_times = time_now; // Reset Timer
				button._keyToRelease.reset(new Mapping(simMap->second)); // Make a copy
				button._nameToRelease = button._mapping.getSimPressName(simMap->first);

				buttons[int(simMap->first)]._btnState = BtnState::SimPress;
				buttons[int(simMap->first)]._press_times = time_now;
				buttons[int(simMap->first)].SyncSimPress(index, *simMap);

				simMap->second.get().ProcessEvent(BtnEvent::OnPress, button, button._nameToRelease);
			}
			else if (!pressed || button.GetPressDurationMS(time_now) > sim_press_window)
			{
				// Button was released before sim delay expired OR
				// Button is still pressed but Sim delay did expire
				if (button._mapping.getDblPressMap())
				{
					// Start counting time between two start presses
					button._btnState = BtnState::DblPressStart;
				}
				else
				{
					button._btnState = BtnState::BtnPress;
					button.GetPressMapping()->ProcessEvent(BtnEvent::OnPress, button, button._nameToRelease);
					//button._press_times = time_now;
				}
			}
			// Else let time flow, stay in this state, no output.
			break;
		}
		case BtnState::SimPress:
			if (button._simPressMaster != ButtonID::NONE && buttons[int(button._simPressMaster)]._btnState != BtnState::SimPress)
			{
				// The master button has released! change state now!
				button._btnState = BtnState::SimRelease;
				button._simPressMaster = ButtonID::NONE;
			}
			else if (!pressed || button._simPressMaster == ButtonID::NONE) // Both slave and master handle release, but only the master handles the press
			{
				button.ProcessButtonPress(pressed, time_now, getSetting(SettingID::TURBO_PERIOD), getSetting(SettingID::HOLD_PRESS_TIME));
				if (button._simPressMaster != ButtonID::NONE && button._btnState != BtnState::SimPress)
				{
					// The slave button has released! Change master state now!
					buttons[int(button._simPressMaster)]._btnState = BtnState::SimRelease;
					button._simPressMaster = ButtonID::NONE;
				}
			}
			break;
		case BtnState::SimRelease:
			if (!pressed)
			{
				button._btnState = BtnState::NoPress;
				button.ClearKey();
			}
			break;
		case BtnState::DblPressStart:
			if (button.GetPressDurationMS(time_now) > dbl_press_window)
			{
				button.GetPressMapping()->ProcessEvent(BtnEvent::OnPress, button, button._nameToRelease);
				button._btnState = BtnState::BtnPress;
				//button._press_times = time_now; // Reset Timer
			}
			else if (!pressed)
			{
				if (button.GetPressDurationMS(time_now) > getSetting(SettingID::HOLD_PRESS_TIME))
				{
					button._btnState = BtnState::DblPressNoPressHold;
				}
				else
				{
					button._btnState = BtnState::DblPressNoPressTap;
				}
			}
			break;
		case BtnState::DblPressNoPressTap:
			if (button.GetPressDurationMS(time_now) > dbl_press_window)
			{
				button._btnState = BtnState::BtnPress;
				button._press_times = time_now; // Reset Timer to raise a tap
				button.GetPressMapping()->ProcessEvent(BtnEvent::OnPress, button, button._nameToRelease);
			}
			else if (pressed)
			{
				button._btnState = BtnState::DblPressPress;
				button._press_times = time_now;
				button._keyToRelease.reset(new Mapping(button._mapping.getDblPressMap()->second));
				button._nameToRelease = button._mapping.getName(index);
				button._mapping.getDblPressMap()->second.get().ProcessEvent(BtnEvent::OnPress, button, button._nameToRelease);
			}
			break;
		case BtnState::DblPressNoPressHold:
			if (button.GetPressDurationMS(time_now) > dbl_press_window)
			{
				button._btnState = BtnState::BtnPress;
				// Don't reset timer to preserve hold press behaviour
				button.GetPressMapping()->ProcessEvent(BtnEvent::OnPress, button, button._nameToRelease);
			}
			else if (pressed)
			{
				button._btnState = BtnState::DblPressPress;
				button._press_times = time_now;
				button._keyToRelease.reset(new Mapping(button._mapping.getDblPressMap()->second));
				button._nameToRelease = button._mapping.getName(index);
				button._mapping.getDblPressMap()->second.get().ProcessEvent(BtnEvent::OnPress, button, button._nameToRelease);
			}
			break;
		case BtnState::DblPressPress:
			button.ProcessButtonPress(pressed, time_now, getSetting(SettingID::TURBO_PERIOD), getSetting(SettingID::HOLD_PRESS_TIME));
			break;
		case BtnState::InstRelease:
		{
			if (button.GetPressDurationMS(time_now) > MAGIC_INSTANT_DURATION)
			{
				button.CheckInstantRelease(BtnEvent::OnRelease);
				button._btnState = BtnState::NoPress;
				button.ClearKey();
			}
			break;
		}
		default:
			cout << "Invalid button state " << button._btnState << ": Resetting to NoPress" << endl;
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
			cout << "Error: Trigger " << fullIndex << " does not exist in state map. Dual Stage Trigger not possible." << endl;
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
			else if (buttons[int(softIndex)].GetPressDurationMS(time_now) >= getSetting(SettingID::TRIGGER_SKIP_DELAY)) {
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
				if (buttons[int(softIndex)].GetPressDurationMS(time_now) >= getSetting(SettingID::TRIGGER_SKIP_DELAY)) {
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
			cout << "Error: Trigger " << softIndex << " has invalid state " << triggerState[idxState] << ". Reset to NoPress." << endl;
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
	bool processDeadZones(float& x, float& y, float innerDeadzone, float outerDeadzone) {
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

	os_mouse_speed = 1.0f;
	last_flick_and_rotation = 0.0f;
}

void connectDevices() {
	handle_to_joyshock.clear();
	int numConnected = JslConnectDevices();
	int* deviceHandles = new int[numConnected];

	JslGetConnectedDeviceHandles(deviceHandles, numConnected);

	for (int i = 0; i < numConnected; i++) {

		// map handles to extra local data
		int handle = deviceHandles[i];
		auto type = JslGetControllerSplitType(handle);
		if (type != JS_SPLIT_TYPE_FULL)
		{
			auto otherJoyCon = find_if(handle_to_joyshock.begin(), handle_to_joyshock.end(),
				[type](auto pair)
				{
					return type == JS_SPLIT_TYPE_LEFT && pair.second->controller_type == JS_SPLIT_TYPE_RIGHT ||
						   type == JS_SPLIT_TYPE_RIGHT && pair.second->controller_type == JS_SPLIT_TYPE_LEFT;
				});
			if (otherJoyCon != handle_to_joyshock.end())
			{
				// The second JC points to the same common buttons as the other one.
				JoyShock *js = new JoyShock(handle,
					JslGetPollRate(handle),
					JslGetControllerSplitType(handle),
					JslGetStickStep(handle),
					otherJoyCon->second->btnCommon);
				handle_to_joyshock.emplace(deviceHandles[i], js);
				continue;
			}
		}
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
	mappings[int(sim)].AtSimPress(origin)->operator= (newVal);
}

bool do_NO_GYRO_BUTTON() {
	// TODO: handle chords
	gyro_settings.Reset();
	return true;
}

bool do_RESET_MAPPINGS(CmdRegistry *registry) {
	printf("Resetting all mappings to defaults\n");
	resetAllMappings();
	if (registry)
	{
		if (!registry->loadConfigFile("onreset.txt"))
		{
			cout << "There is no onreset.txt file to load." << endl;
		}
	}
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

void UpdateAutoload(Switch newValue)
{
	if (autoLoadThread)
	{
		if (newValue == Switch::ON)
		{
			autoLoadThread->Start();
		}
		else if (newValue == Switch::OFF)
		{
			autoLoadThread->Stop();
		}
	}
	else
	{
		cout << "AutoLoad is unavailable" << endl;
	}
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
	for (auto iter = handle_to_joyshock.begin(); iter != handle_to_joyshock.end(); ++iter) {
		JslPauseContinuousCalibration(iter->second->intHandle);
	}
	devicesCalibrating = false;
	return true;
}

bool do_RESTART_GYRO_CALIBRATION() {
	printf("Restarting continuous calibration for all devices\n");
	for (auto iter = handle_to_joyshock.begin(); iter != handle_to_joyshock.end(); ++iter) {
		JslResetContinuousCalibration(iter->second->intHandle);
		JslStartContinuousCalibration(iter->second->intHandle);
	}
	devicesCalibrating = true;
	return true;
}

bool do_SET_MOTION_STICK_NEUTRAL()
{
	printf("Setting neutral motion stick orientation...\n");
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
			printf("Can't convert \"%s\" to a number\n", argument.c_str());
			return false;
		}
	}

	if (sleepTime <= 0)
	{
		printf("Sleep time must be greater than 0 and less than or equal to 10\n");
		return false;
	}

	if (sleepTime > 10)
	{
		printf("Sleep is capped at 10s per command\n");
		sleepTime = 10.f;
	}
	printf("Sleeping for %.3f second(s)...\n", sleepTime);
	std::this_thread::sleep_for(std::chrono::milliseconds((int)(sleepTime * 1000)));
	printf("Finished sleeping.\n");

	return true;
}

bool do_README() {
	printf("Opening online help in your browser\n");
	auto err = ShowOnlineHelp();
	if (err != 0)
	{
		printf("Could not open online help. Error #%d\n", err);
	}
	return true;
}

bool do_WHITELIST_SHOW() {
	printf("Your PID is %lu\n", GetCurrentProcessId()); // WinAPI call!
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
	float flickStickThreshold = 0.995f;
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
					float snapInterval = PI;
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
				if (abs(stickAngle) * (180.0f / PI) < jc->getSetting(SettingID::FLICK_DEADZONE_ANGLE)) {
					stickAngle = 0.0f;
				}

				jc->started_flick = chrono::steady_clock::now();
				jc->delta_flick = stickAngle;
				jc->flick_percent_done = 0.0f;
				jc->ResetSmoothSample();
				jc->flick_rotation_counter = stickAngle; // track all rotation for this flick
				// TODO: All these printfs should be hidden behind a setting. User might not want them.
				printf("Flick: %.3f degrees\n", stickAngle * (180.0f / (float)PI));
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

	// don't divide by zero
	if (abs(jc->delta_flick) > 0.0f) {
		newPercent = newPercent / pow(abs(jc->delta_flick) / PI, jc->getSetting(SettingID::FLICK_TIME_EXPONENT));
	}

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
		return iter->second.get();
		// iter is item pair in the map. The value will be accessible as `iter->second`.
	}
	return nullptr;
}

void processStick(JoyShock* jc, float stickX, float stickY, float lastX, float lastY, float innerDeadzone, float outerDeadzone, RingMode ringMode, StickMode stickMode,
	ButtonID ringId, ButtonID leftId, ButtonID rightId, ButtonID upId, ButtonID downId, ControllerOrientation controllerOrientation,
	float mouseCalibrationFactor, float deltaTime, float &acceleration, FloatXY &lastAreaCal, bool& isFlicking, bool &ignoreStickMode,
	bool &anyStickInput, bool &lockMouse, float &camSpeedX, float &camSpeedY)
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
}

void joyShockPollCallback(int jcHandle, JOY_SHOCK_STATE state, JOY_SHOCK_STATE lastState, IMU_STATE imuState, IMU_STATE lastImuState, float deltaTime) {

	//printf("DS4 accel: %.4f, %.4f, %.4f\n", imuState.accelX, imuState.accelY, imuState.accelZ);
	//printf("\tDS4 gyro: %.4f, %.4f, %.4f\n", imuState.gyroX, imuState.gyroY, imuState.gyroZ);
	MOTION_STATE motion = JslGetMotionState(jcHandle);
	//printf("\tDS4 quat: %.4f, %.4f, %.4f, %.4f | accel: %.4f, %.4f, %.4f | grav: %.4f, %.4f, %.4f\n",
	//	motion.quatW, motion.quatX, motion.quatY, motion.quatZ,
	//	motion.accelX, motion.accelY, motion.accelZ,
	//	motion.gravX, motion.gravY, motion.gravZ);

	bool blockGyro = false;
	bool lockMouse = false;
	bool leftAny = false;
	bool rightAny = false;
	bool motionAny = false;
	// get jc from handle
	JoyShock* jc = getJoyShockFromHandle(jcHandle);
	//printf("Controller %d\n", jcHandle);
	if (jc == nullptr) return;
	jc->callback_lock.lock();
	if (jc->set_neutral_quat)
	{
		jc->neutralQuatW = motion.quatW;
		jc->neutralQuatX = motion.quatX;
		jc->neutralQuatY = motion.quatY;
		jc->neutralQuatZ = motion.quatZ;
		jc->set_neutral_quat = false;
		printf("Neutral orientation for device %d set...\n", jc->intHandle);
	}
	jc->controller_type = JslGetControllerSplitType(jcHandle); // Reassign at each call? :( Low impact function
	//printf("Found a match for %d\n", jcHandle);
	float gyroX = 0.0;
	float gyroY = 0.0;
	int mouse_x_flag = (int)jc->getSetting<GyroAxisMask>(SettingID::MOUSE_X_FROM_GYRO_AXIS);
	if ((mouse_x_flag & (int)GyroAxisMask::X) > 0) {
		gyroX += imuState.gyroX;
	}
	if ((mouse_x_flag & (int)GyroAxisMask::Y) > 0) {
		gyroX -= imuState.gyroY;
	}
	if ((mouse_x_flag & (int)GyroAxisMask::Z) > 0) {
		gyroX -= imuState.gyroZ;
	}
	int mouse_y_flag = (int)jc->getSetting<GyroAxisMask>(SettingID::MOUSE_Y_FROM_GYRO_AXIS);
	if ((mouse_y_flag & (int)GyroAxisMask::X) > 0) {
		gyroY -= imuState.gyroX;
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
	ControllerOrientation controllerOrientation = jc->getSetting<ControllerOrientation>(SettingID::CONTROLLER_ORIENTATION);
	float camSpeedX = 0.0f;
	float camSpeedY = 0.0f;
	// account for os mouse speed and convert from radians to degrees because gyro reports in degrees per second
	float mouseCalibrationFactor = 180.0f / PI / os_mouse_speed;
	if (jc->controller_type != JS_SPLIT_TYPE_RIGHT)
	{
		// let's do these sticks... don't want to constantly send input, so we need to compare them to last time
		float lastCalX = lastState.stickLX;
		float lastCalY = lastState.stickLY;
		float calX = state.stickLX;
		float calY = state.stickLY;

		processStick(jc, calX, calY, lastCalX, lastCalY, jc->getSetting(SettingID::LEFT_STICK_DEADZONE_INNER), jc->getSetting(SettingID::LEFT_STICK_DEADZONE_OUTER),
			jc->getSetting<RingMode>(SettingID::LEFT_RING_MODE), jc->getSetting<StickMode>(SettingID::LEFT_STICK_MODE),
			ButtonID::LRING, ButtonID::LLEFT, ButtonID::LRIGHT, ButtonID::LUP, ButtonID::LDOWN, controllerOrientation,
			mouseCalibrationFactor, deltaTime, jc->left_acceleration, jc->left_last_cal, jc->is_flicking_left, jc->ignore_left_stick_mode, leftAny, lockMouse, camSpeedX, camSpeedY);
	}

	if (jc->controller_type != JS_SPLIT_TYPE_LEFT)
	{
		float lastCalX = lastState.stickRX;
		float lastCalY = lastState.stickRY;
		float calX = state.stickRX;
		float calY = state.stickRY;

		processStick(jc, calX, calY, lastCalX, lastCalY, jc->getSetting(SettingID::RIGHT_STICK_DEADZONE_INNER), jc->getSetting(SettingID::RIGHT_STICK_DEADZONE_OUTER),
			jc->getSetting<RingMode>(SettingID::RIGHT_RING_MODE), jc->getSetting<StickMode>(SettingID::RIGHT_STICK_MODE),
			ButtonID::RRING, ButtonID::RLEFT, ButtonID::RRIGHT, ButtonID::RUP, ButtonID::RDOWN, controllerOrientation,
			mouseCalibrationFactor, deltaTime, jc->right_acceleration, jc->right_last_cal, jc->is_flicking_right, jc->ignore_right_stick_mode, rightAny, lockMouse, camSpeedX, camSpeedY);
	}

	if (jc->controller_type == JS_SPLIT_TYPE_FULL ||
		(jc->controller_type & (int)jc->getSetting<JoyconMask>(SettingID::JOYCON_MOTION_MASK)) == 0)
	{
		Quat neutralQuat = Quat(jc->neutralQuatW, jc->neutralQuatX, jc->neutralQuatY, jc->neutralQuatZ);
		Vec grav = Vec(motion.gravX, motion.gravY, motion.gravZ) * neutralQuat;

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
			mouseCalibrationFactor, deltaTime, jc->motion_stick_acceleration, jc->motion_last_cal, jc->is_flicking_motion, jc->ignore_motion_stick_mode, motionAny, lockMouse, camSpeedX, camSpeedY);

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

	// button mappings
	if (jc->controller_type != JS_SPLIT_TYPE_RIGHT)
	{
		jc->handleButtonChange(ButtonID::UP, (state.buttons & JSMASK_UP) > 0);
		jc->handleButtonChange(ButtonID::DOWN, (state.buttons & JSMASK_DOWN) > 0);
		jc->handleButtonChange(ButtonID::LEFT, (state.buttons & JSMASK_LEFT) > 0);
		jc->handleButtonChange(ButtonID::RIGHT, (state.buttons & JSMASK_RIGHT) > 0);
		jc->handleButtonChange(ButtonID::L, (state.buttons & JSMASK_L) > 0);
		jc->handleButtonChange(ButtonID::MINUS, (state.buttons & JSMASK_MINUS) > 0);
		jc->handleButtonChange(ButtonID::CAPTURE, (state.buttons & JSMASK_CAPTURE) > 0);
		jc->handleButtonChange(ButtonID::L3, (state.buttons & JSMASK_LCLICK) > 0);
		jc->handleTriggerChange(ButtonID::ZL, ButtonID::ZLF, jc->getSetting<TriggerMode>(SettingID::ZL_MODE), state.lTrigger);
	}
	if (jc->controller_type != JS_SPLIT_TYPE_LEFT)
	{
		jc->handleButtonChange(ButtonID::E, (state.buttons & JSMASK_E) > 0);
		jc->handleButtonChange(ButtonID::S, (state.buttons & JSMASK_S) > 0);
		jc->handleButtonChange(ButtonID::N, (state.buttons & JSMASK_N) > 0);
		jc->handleButtonChange(ButtonID::W, (state.buttons & JSMASK_W) > 0);
		jc->handleButtonChange(ButtonID::R, (state.buttons & JSMASK_R) > 0);
		jc->handleButtonChange(ButtonID::PLUS, (state.buttons & JSMASK_PLUS) > 0);
		jc->handleButtonChange(ButtonID::HOME, (state.buttons & JSMASK_HOME) > 0);
		jc->handleButtonChange(ButtonID::R3, (state.buttons & JSMASK_RCLICK) > 0);
		jc->handleTriggerChange(ButtonID::ZR, ButtonID::ZRF, jc->getSetting<TriggerMode>(SettingID::ZR_MODE), state.rTrigger);
	}
	jc->handleButtonChange(ButtonID::SL, (state.buttons & JSMASK_SL) > 0);
	jc->handleButtonChange(ButtonID::SR, (state.buttons & JSMASK_SR) > 0);

	// Handle buttons before GYRO because some of them may affect the value of blockGyro
	auto gyro = jc->getSetting<GyroSettings>(SettingID::GYRO_ON); // same result as getting GYRO_OFF
	switch (gyro.ignore_mode) {
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
		// TODO: logic optimization
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
	jc->callback_lock.unlock();
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
		string path(AUTOLOAD_FOLDER());
		auto files = ListDirectory(path);
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
				registry->processLine(path + file);
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

		string autoloadFolder{ AUTOLOAD_FOLDER() };
		for (auto file : ListDirectory(autoloadFolder.c_str()))
		{
			string fullPathName = autoloadFolder + file;
			auto noext = file.substr(0, file.find_last_of('.'));
			tray->AddMenuItem(U("AutoLoad folder"), UnicodeString(noext.begin(), noext.end()), [fullPathName]
			{
				WriteToConsole(string(fullPathName.begin(), fullPathName.end()));
				autoLoadThread->Stop();
			});
		}
		std::string gyroConfigsFolder{ GYRO_CONFIGS_FOLDER() };
		for (auto file : ListDirectory(gyroConfigsFolder.c_str()))
		{
			string fullPathName = gyroConfigsFolder + file;
			auto noext = file.substr(0, file.find_last_of('.'));
			tray->AddMenuItem(U("GyroConfigs folder"), UnicodeString(noext.begin(), noext.end()), [fullPathName]
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

float filterHoldPressDelay(float c, float next)
{
	if (next <= sim_press_window || next >= dbl_press_window)
	{
		cout << SettingID::HOLD_PRESS_TIME << " can only be set to a value between those of " <<
			SettingID::SIM_PRESS_WINDOW << " (" << sim_press_window << "ms) and " <<
			SettingID::DBL_PRESS_WINDOW << " (" << dbl_press_window << "ms) exclusive." << endl;
		return c;
	}
	return next;
}

Mapping filterMapping(Mapping current, Mapping next)
{
	return next.isValid() ? next : current;
}

TriggerMode triggerModeNotification(TriggerMode current, TriggerMode next)
{
	for (auto js : handle_to_joyshock)
	{
		if (JslGetControllerType(js.first) != JS_TYPE_DS4 && next != TriggerMode::NO_FULL)
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

void RefreshAutoloadHelp(JSMAssignment<Switch> *autoloadCmd)
{
	stringstream ss;
	ss << "AUTOLOAD will attempt load a file from the following folder when a window with a matching executable name enters focus:" << endl << AUTOLOAD_FOLDER();
	autoloadCmd->SetHelp(ss.str());
}

class GyroSensAssignment : public JSMAssignment<FloatXY>
{
public:
	GyroSensAssignment(in_string name, JSMSetting<FloatXY>& gyroSens)
		: JSMAssignment(name, string(magic_enum::enum_name(gyroSens._id)), gyroSens)
	{
		// min and max gyro sens already have a listener
		gyroSens.RemoveOnChangeListener(_listenerId);
	}
};

class StickDeadzoneAssignment : public JSMAssignment<float>
{
public:
	StickDeadzoneAssignment(in_string name, JSMSetting<float> &stickDeadzone)
	  : JSMAssignment(name, string(magic_enum::enum_name(stickDeadzone._id)), stickDeadzone)
	{
		// min and max gyro sens already have a listener
		stickDeadzone.RemoveOnChangeListener(_listenerId);
	}
};

class GyroButtonAssignment : public JSMAssignment<GyroSettings>
{
private:
	const bool _always_off;

	bool GyroParser(in_string data)
	{
		if (data.empty())
		{
			GyroSettings value(_var);
			//No assignment? Display current assignment
			cout << (value.always_off ? string("GYRO_ON") : string("GYRO_OFF")) << " = " << value << endl;;
		}
		else
		{
			stringstream ss(data);
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
	GyroButtonAssignment(in_string name, bool always_off)
		: JSMAssignment(name, gyro_settings)
		, _always_off(always_off)
	{
		SetParser(bind(&GyroButtonAssignment::GyroParser, this, placeholders::_2));
		_var.RemoveOnChangeListener(_listenerId);
	}

	GyroButtonAssignment *SetListener()
	{
		_listenerId = _var.AddOnChangeListener(bind(&GyroButtonAssignment::DisplayGyroSettingValue, this, placeholders::_1));
		return this;
	}

	virtual ~GyroButtonAssignment() = default;
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
		do { // Run at least once with an empty arg string if there's no argument.
			_macro(this, arguments);
			ss >> arg;
		} while (!ss.fail());
		arg.clear();
		return true;
	}

	// The run function is nothing like the delegate. See how I use the bind function
	// below to hard-code the pointer parameter and the instance pointer 'this'.
	void RunHelp(CmdRegistry *registry)
	{
		if (arg.empty())
		{
			// Show all commands
			cout << "Here's the list of all commands." << endl;
			vector<string> list;
			registry->GetCommandList(list);
			for (auto cmd : list)
			{
				cout << "    " << cmd << endl;
			}
			cout << "Enter HELP [cmd1] [cmd2] ... for details on specific commands." << endl;
		}
		else
		{
			auto help = registry->GetHelp(arg);
			if (!help.empty())
			{
				cout << arg << " :" << endl <<
					"    " << help << endl;
			}
			else
			{
				cout << arg << " is not a recognized command" << endl;
			}
		}
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
	for (int id = 0; id < MAPPING_SIZE; ++id)
	{
		JSMButton newButton(ButtonID(id), Mapping::NO_MAPPING);
		newButton.SetFilter(&filterMapping);
		mappings.push_back(newButton);
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
	motion_stick_mode.SetFilter(&filterInvalidValue<StickMode, StickMode::INVALID>)->
		AddOnChangeListener(bind(&UpdateRingModeFromStickMode, &motion_ring_mode, ::placeholders::_1));
	left_ring_mode.SetFilter(&filterInvalidValue<RingMode, RingMode::INVALID>);
	right_ring_mode.SetFilter(&filterInvalidValue<RingMode, RingMode::INVALID>);
	motion_ring_mode.SetFilter(&filterInvalidValue<RingMode, RingMode::INVALID>);
	mouse_x_from_gyro.SetFilter(&filterInvalidValue<GyroAxisMask, GyroAxisMask::INVALID>);
	mouse_y_from_gyro.SetFilter(&filterInvalidValue<GyroAxisMask, GyroAxisMask::INVALID>);
	gyro_settings.SetFilter( [] (GyroSettings current, GyroSettings next)
		{
			return next.ignore_mode != GyroIgnoreMode::INVALID ? next : current;
		});
	joycon_gyro_mask.SetFilter(&filterInvalidValue<JoyconMask, JoyconMask::INVALID>);
	joycon_motion_mask.SetFilter(&filterInvalidValue<JoyconMask, JoyconMask::INVALID>);
	controller_orientation.SetFilter(&filterInvalidValue<ControllerOrientation, ControllerOrientation::INVALID>);
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
	currentWorkingDir.SetFilter( [] (PathString current, PathString next) { return SetCWD(string(next)) ? next : current; });
	autoloadSwitch.SetFilter(&filterInvalidValue<Switch, Switch::INVALID>)->AddOnChangeListener(&UpdateAutoload);

#if _WIN32
    currentWorkingDir = string(&cmdLine[0], &cmdLine[wcslen(cmdLine)]);
#else
    currentWorkingDir = string(argv[0]);
#endif
	CmdRegistry commandRegistry;

	autoLoadThread.reset(new PollingThread(&AutoLoadPoll, &commandRegistry, 1000, true)); // Start by default
	if (autoLoadThread && autoLoadThread->isRunning())
	{
		printf("AutoLoad is enabled. Configurations in \"%s\" folder will get loaded when matching application is in focus.\n", AUTOLOAD_FOLDER());
	}
	else printf("[AUTOLOAD] AutoLoad is unavailable\n");


	for (auto &mapping : mappings) // Add all button mappings as commands
	{
		commandRegistry.Add(new JSMAssignment<Mapping>(mapping.getName(), mapping));
	}
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
		->SetHelp("Set this to a value between 0 and 1. This is the threshold at which a soft press binding is triggered."));
	commandRegistry.Add((new JSMMacro("RESET_MAPPINGS"))->SetMacro(bind(&do_RESET_MAPPINGS, &commandRegistry))
		->SetHelp("Delete all custom bindings and reset to default.\nHOME and CAPTURE are set to CALIBRATE on both tap and hold by default."));
	commandRegistry.Add((new JSMMacro("NO_GYRO_BUTTON"))->SetMacro(bind(&do_NO_GYRO_BUTTON))
		->SetHelp("Enable gyro at all times, without any GYRO_OFF binding."));
	commandRegistry.Add((new JSMAssignment<StickMode>(left_stick_mode))
		->SetHelp("Set a mouse mode for the left stick. Valid values are the following:\nNO_MOUSE, AIM, FLICK, FLICK_ONLY, ROTATE_ONLY, MOUSE_RING, MOUSE_AREA, OUTER_RING, INNER_RING"));
	commandRegistry.Add((new JSMAssignment<StickMode>(right_stick_mode))
		->SetHelp("Set a mouse mode for the right stick. Valid values are the following:\nNO_MOUSE, AIM, FLICK, FLICK_ONLY, ROTATE_ONLY, MOUSE_RING, MOUSE_AREA, OUTER_RING, INNER_RING"));
	commandRegistry.Add((new JSMAssignment<StickMode>(motion_stick_mode))
	    ->SetHelp("Set a mouse mode for the motion-stick -- the whole controller is treated as a stick. Valid values are the following:\nNO_MOUSE, AIM, FLICK, FLICK_ONLY, ROTATE_ONLY, MOUSE_RING, MOUSE_AREA, OUTER_RING, INNER_RING"));
	commandRegistry.Add((new GyroButtonAssignment("GYRO_OFF", false))
		->SetHelp("Assign a controller button to disable the gyro when pressed."));
	commandRegistry.Add((new GyroButtonAssignment("GYRO_ON", true))->SetListener() // Set only one listener
		->SetHelp("Assign a controller button to enable the gyro when pressed."));
	commandRegistry.Add((new JSMAssignment<AxisMode>(aim_x_sign))
		->SetHelp("When in AIM mode, set stick X axis inversion. Valid values are the following:\nSTANDARD or 1, and INVERTED or -1"));
	commandRegistry.Add((new JSMAssignment<AxisMode>(aim_y_sign))
		->SetHelp("When in AIM mode, set stick Y axis inversion. Valid values are the following:\nSTANDARD or 1, and INVERTED or -1"));
	commandRegistry.Add((new JSMAssignment<AxisMode>(gyro_x_sign))
		->SetHelp("Set gyro X axis inversion. Valid values are the following:\nSTANDARD or 1, and INVERTED or -1"));
	commandRegistry.Add((new JSMAssignment<AxisMode>(gyro_y_sign))
		->SetHelp("Set gyro Y axis inversion. Valid values are the following:\nSTANDARD or 1, and INVERTED or -1"));
	commandRegistry.Add((new JSMMacro("RECONNECT_CONTROLLERS"))->SetMacro(bind(&do_RECONNECT_CONTROLLERS))
		->SetHelp("Look for newly connected controllers."));
	commandRegistry.Add((new JSMMacro("COUNTER_OS_MOUSE_SPEED"))->SetMacro(bind(do_COUNTER_OS_MOUSE_SPEED))
		->SetHelp("JoyShockMapper will load the user's OS mouse sensitivity value to consider it in its calculations."));
	commandRegistry.Add((new JSMMacro("IGNORE_OS_MOUSE_SPEED"))->SetMacro(bind(do_IGNORE_OS_MOUSE_SPEED))
		->SetHelp("Disable JoyShockMapper's consideration of the the user's OS mouse sensitivity value."));
	commandRegistry.Add((new JSMAssignment<JoyconMask>("JOYCON_GYRO_MASK", joycon_gyro_mask))
		->SetHelp("When using two Joycons, select which one will be used for gyro. Valid values are the following:\nUSE_BOTH, IGNORE_LEFT, IGNORE_RIGHT, IGNORE_BOTH"));
	commandRegistry.Add((new JSMAssignment<JoyconMask>("JOYCON_MOTION_MASK", joycon_motion_mask))
	    ->SetHelp("When using two Joycons, select which one will be used for non-gyro motion. Valid values are the following:\nUSE_BOTH, IGNORE_LEFT, IGNORE_RIGHT, IGNORE_BOTH"));
	commandRegistry.Add((new GyroSensAssignment("GYRO_SENS", min_gyro_sens))
		->SetHelp("Sets a gyro sensitivity to use. This sets both MIN_GYRO_SENS and MAX_GYRO_SENS to the same values. You can assign a second value as a different vertical sensitivity."));
	commandRegistry.Add((new GyroSensAssignment("GYRO_SENS", max_gyro_sens))->SetHelp(""));
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
	commandRegistry.Add((new StickDeadzoneAssignment("STICK_DEADZONE_INNER", left_stick_deadzone_inner))
		->SetHelp("Defines a radius of the both left and right sticks within which all values will be ignored. This value can only be between 0 and 1 but it should be small. Stick input out of this radius will be adjusted."));
	commandRegistry.Add((new StickDeadzoneAssignment("STICK_DEADZONE_INNER", right_stick_deadzone_inner))->SetHelp(""));
	commandRegistry.Add((new StickDeadzoneAssignment("STICK_DEADZONE_OUTER", left_stick_deadzone_outer))
		->SetHelp("Defines a distance from both sticks' outer edge for which the stick will be considered fully tilted. This value can only be between 0 and 1 but it should be small. Stick input out of this deadzone will be adjusted."));
	commandRegistry.Add((new StickDeadzoneAssignment("STICK_DEADZONE_OUTER", right_stick_deadzone_outer))->SetHelp(""));
	commandRegistry.Add((new JSMAssignment<float>(motion_deadzone_inner))
		->SetHelp("Defines a radius of the motion-stick within which all values will be ignored. This value can only be between 0 and 1 but it should be small. Stick input out of this radius will be adjusted."));
	commandRegistry.Add((new JSMAssignment<float>(motion_deadzone_outer))
		->SetHelp("Defines a distance from the motion-stick's outer edge for which the stick will be considered fully tilted. Stick input out of this deadzone will be adjusted."));
	commandRegistry.Add((new JSMAssignment<float>(lean_threshold))
		->SetHelp("How far the controller must be leaned left or right to trigger a LEAN_LEFT or LEAN_RIGHT binding."));
	commandRegistry.Add((new JSMMacro("CALCULATE_REAL_WORLD_CALIBRATION"))->SetMacro(bind(&do_CALCULATE_REAL_WORLD_CALIBRATION, placeholders::_2))
		->SetHelp("Get JoyShockMapper to recommend you a REAL_WORLD_CALIBRATION value after performing the calibration sequence. Visit GyroWiki for details:\nhttp://gyrowiki.jibbsmart.com/blog:joyshockmapper-guide#calibrating"));
	commandRegistry.Add((new JSMMacro("SLEEP"))->SetMacro(bind(&do_SLEEP, placeholders::_2))
		->SetHelp("Sleep for the given number of seconds, or one second if no number is given. Can't sleep more than 10 seconds per command."));
	commandRegistry.Add((new JSMMacro("FINISH_GYRO_CALIBRATION"))->SetMacro(bind(&do_FINISH_GYRO_CALIBRATION))
		->SetHelp("Finish calibrating the gyro in all controllers."));
	commandRegistry.Add((new JSMMacro("RESTART_GYRO_CALIBRATION"))->SetMacro(bind(&do_RESTART_GYRO_CALIBRATION))
		->SetHelp("Start calibrating the gyro in all controllers."));
	commandRegistry.Add((new JSMMacro("SET_MOTION_STICK_NEUTRAL"))->SetMacro(bind(&do_SET_MOTION_STICK_NEUTRAL))
		->SetHelp("Set the neutral orientation for motion stick to whatever the orientation of the controller is."));
	commandRegistry.Add((new JSMAssignment<GyroAxisMask>(mouse_x_from_gyro))
		->SetHelp("Pick a gyro axis to operate on the mouse's X axis. Valid values are the following: X, Y and Z."));
	commandRegistry.Add((new JSMAssignment<GyroAxisMask>(mouse_y_from_gyro))
		->SetHelp("Pick a gyro axis to operate on the mouse's Y axis. Valid values are the following: X, Y and Z."));
	commandRegistry.Add((new JSMAssignment<ControllerOrientation>(controller_orientation))
	    ->SetHelp("Let the stick modes account for how you're holding the controller:\nFORWARD, LEFT, RIGHT, BACKWARD"));
	commandRegistry.Add((new JSMAssignment<TriggerMode>(zlMode))
		->SetHelp("Controllers with a right analog trigger can use one of the following dual stage trigger modes:\nNO_FULL, NO_SKIP, MAY_SKIP, MUST_SKIP, MAY_SKIP_R, MUST_SKIP_R"));
	commandRegistry.Add((new JSMAssignment<TriggerMode>(zrMode))
		->SetHelp("Controllers with a left analog trigger can use one of the following dual stage trigger modes:\nNO_FULL, NO_SKIP, MAY_SKIP, MUST_SKIP, MAY_SKIP_R, MUST_SKIP_R"));
	auto *autoloadCmd = new JSMAssignment<Switch>("AUTOLOAD", autoloadSwitch);
	currentWorkingDir.AddOnChangeListener(bind(&RefreshAutoloadHelp, autoloadCmd));
    commandRegistry.Add(autoloadCmd);
	RefreshAutoloadHelp(autoloadCmd);
	commandRegistry.Add((new JSMMacro("README"))->SetMacro(bind(&do_README))
		->SetHelp("Open the latest JoyShockMapper README in your browser."));
	commandRegistry.Add((new JSMMacro("WHITELIST_SHOW"))->SetMacro(bind(&do_WHITELIST_SHOW))
		->SetHelp("Open HIDCerberus configuration page in your browser."));
	commandRegistry.Add((new JSMMacro("WHITELIST_ADD"))->SetMacro(bind(&do_WHITELIST_ADD))
		->SetHelp("Add JoyShockMapper to HIDGuardian whitelisted applications."));
	commandRegistry.Add((new JSMMacro("WHITELIST_REMOVE"))->SetMacro(bind(&do_WHITELIST_REMOVE))
		->SetHelp("Remove JoyShockMapper from HIDGuardian whitelisted applications."));
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
	commandRegistry.Add((new JSMAssignment<PathString>("JSM_DIRECTORY", currentWorkingDir))
		->SetHelp("If AUTOLOAD doesn't work properly, set this value to the path to the directory holding the JoyShockMapper.exe file. Make sure a folder named \"AutoLoad\" exists there."));
	commandRegistry.Add(new HelpCmd(commandRegistry));

	bool quit = false;
	commandRegistry.Add((new JSMMacro("QUIT"))
		->SetMacro( [&quit] (JSMMacro *, in_string)
			{
				quit = true;
			})
		->SetHelp("Close the application.")
	);

	Mapping::_isCommandValid = bind(&CmdRegistry::isCommandValid, &commandRegistry, placeholders::_1);

	do_RESET_MAPPINGS(&commandRegistry);
	if (commandRegistry.loadConfigFile("onstartup.txt"))
	{
		printf("Finished executing startup file.\n");
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
