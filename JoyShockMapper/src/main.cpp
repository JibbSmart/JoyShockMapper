
#include <chrono>
#include <sstream>
#include <algorithm>
#include <string.h>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <deque>
#include <memory>
#include <mutex>
#include <array>
#include <optional>

#include "JoyShockLibrary.h"
#include "Whitelister.h"
#include "inputHelpers.cpp"
#include "TrayIcon.h"

#pragma warning(disable:4996)

// versions will be in the format A.B.C
// C increases when all that's happened is some bugs have been fixed.
// B increases and C resets to 0 when new features have been added.
// A increases and B and C reset to 0 when major new features have been added that warrant a new major version, or replacing older features with better ones that require the user to interact with them differently
const char* version = "1.5.0";

#define PI 3.14159265359f

#define MAPPING_ERROR	-2 // Represents an error in user input
#define MAPPING_NONE	-1 // Represents no button when explicitely stated by the user. Not to be confused with NO_HOLD_MAPPED which is no action bound.
#define MAPPING_UP		0
#define MAPPING_DOWN	1
#define MAPPING_LEFT	2
#define MAPPING_RIGHT	3
#define MAPPING_L		4
#define MAPPING_ZL		5
#define MAPPING_MINUS	6
#define MAPPING_CAPTURE 7
#define MAPPING_E		8
#define MAPPING_S		9
#define MAPPING_N		10
#define MAPPING_W		11
#define MAPPING_R		12
#define MAPPING_ZR		13
#define MAPPING_PLUS	14
#define MAPPING_HOME	15
#define MAPPING_SL		16
#define MAPPING_SR		17
#define MAPPING_L3		18
#define MAPPING_R3		19
#define MAPPING_LUP		20
#define MAPPING_LDOWN	21
#define MAPPING_LLEFT	22
#define MAPPING_LRIGHT	23
#define MAPPING_LRING	24
#define MAPPING_RUP		25
#define MAPPING_RDOWN	26
#define MAPPING_RLEFT	27
#define MAPPING_RRIGHT	28
#define MAPPING_RRING	29
#define MAPPING_ZLF		30 // FIRST
// insert more analog triggers here
#define MAPPING_ZRF		31 // LAST
#define MAPPING_SIZE	32

#define FIRST_ANALOG_TRIGGER MAPPING_ZLF
#define LAST_ANALOG_TRIGGER MAPPING_ZRF

#define MIN_GYRO_SENS 33
#define MAX_GYRO_SENS 34
#define MIN_GYRO_THRESHOLD 35
#define MAX_GYRO_THRESHOLD 36
#define STICK_POWER 37
#define STICK_SENS 38
#define REAL_WORLD_CALIBRATION 39
#define IN_GAME_SENS 40
#define TRIGGER_THRESHOLD 41
#define RESET_MAPPINGS 42
#define NO_GYRO_BUTTON 43
#define LEFT_STICK_MODE 44
#define RIGHT_STICK_MODE 45
#define GYRO_OFF 46
#define GYRO_ON 47
#define STICK_AXIS_X 48
#define STICK_AXIS_Y 49
#define GYRO_AXIS_X 50
#define GYRO_AXIS_Y 51
#define RECONNECT_CONTROLLERS 52
#define COUNTER_OS_MOUSE_SPEED 53
#define IGNORE_OS_MOUSE_SPEED 54
#define JOYCON_GYRO_MASK 55
#define GYRO_SENS 56
#define FLICK_TIME 57
#define GYRO_SMOOTH_THRESHOLD 58
#define GYRO_SMOOTH_TIME 59
#define GYRO_CUTOFF_SPEED 60
#define GYRO_CUTOFF_RECOVERY 61
#define STICK_ACCELERATION_RATE 62
#define STICK_ACCELERATION_CAP 63
#define STICK_DEADZONE_INNER 64
#define STICK_DEADZONE_OUTER 65
#define CALCULATE_REAL_WORLD_CALIBRATION 66
#define FINISH_GYRO_CALIBRATION 67
#define RESTART_GYRO_CALIBRATION 68
#define MOUSE_X_FROM_GYRO_AXIS 69
#define MOUSE_Y_FROM_GYRO_AXIS 70
#define ZR_DUAL_STAGE_MODE 71
#define ZL_DUAL_STAGE_MODE 72
#define AUTOLOAD 73
#define HELP 74
#define WHITELIST_SHOW 75
#define WHITELIST_ADD 76
#define WHITELIST_REMOVE 77
#define LEFT_RING_MODE 78
#define RIGHT_RING_MODE 79
#define MOUSE_RING_RADIUS 80
#define SCREEN_RESOLUTION_X 81
#define SCREEN_RESOLUTION_Y 82
#define ROTATE_SMOOTH_OVERRIDE 83
#define FLICK_SNAP_MODE 84
#define FLICK_SNAP_STRENGTH 85

#define MAGIC_DST_DELAY 150.0f // in milliseconds
#define MAGIC_TAP_DURATION 40.0f // in milliseconds
#define MAGIC_GYRO_TAP_DURATION 500.0f // in milliseconds
#define MAGIC_HOLD_TIME 150.0f // in milliseconds
#define MAGIC_SIM_DELAY 50.0f // in milliseconds
#define MAGIC_DBL_PRESS_WINDOW 200.0f // in milliseconds
static_assert(MAGIC_SIM_DELAY < MAGIC_HOLD_TIME, "Simultaneous press delay has to be smaller than hold delay!");
static_assert(MAGIC_HOLD_TIME < MAGIC_DBL_PRESS_WINDOW, "Hold delay has to be smaller than double press window!");

enum class RingMode { outer, inner, invalid };
enum class StickMode { none, aim, flick, flickOnly, rotateOnly, mouseRing, outer, inner, invalid };
enum class FlickSnapMode { none, four, eight, invalid };
enum       AxisMode { standard=1, inverted=-1, invalid=0 }; // valid values are true!
enum class TriggerMode { noFull, noSkip, maySkip, mustSkip, maySkipResp, mustSkipResp, invalid };
enum class GyroAxisMask { none = 0, x = 1, y = 2, z = 4, invalid = 8 };
enum class JoyconMask { useBoth = 0, ignoreLeft = 1, ignoreRight = 2, ignoreBoth = 3, invalid = 4 };
enum class GyroIgnoreMode { button, left, right };
enum class DstState { NoPress = 0, PressStart, QuickSoftTap, QuickFullPress, QuickFullRelease, SoftPress, DelayFullPress, PressStartResp, invalid };
enum class BtnState { NoPress = 0, BtnPress, WaitHold, HoldPress, TapRelease,
					  WaitSim, SimPress, WaitSimHold, SimHold, SimTapRelease, SimRelease, 
					  DblPressStart, DblPressNoPress, DblPressPress, DblPressWaitHold, DblPressHold, invalid};

struct ComboMap
{
	std::string name; // Display name of the mapping
	int btn; // Combination button
	WORD pressBind = 0; // Press or tap binding
	WORD holdBind = 0; // Hold binding if any.
};

// This structure holds information about a simple button binding that can possibly be held.
// The location of this element in the overarching data structure identifies to what button
// the binding is bound.
struct Mapping
{
	WORD pressBind = 0; // Press or tap binding
	WORD holdBind = 0; // Hold binding if any.
	std::vector<ComboMap> sim_mappings;
	std::vector<ComboMap> chord_mappings; // Binds a chord button to one or many remappings

	void reset()
	{
		pressBind = 0;
		holdBind = 0;
		sim_mappings.clear();
		chord_mappings.clear();
	}
};

class DigitalButton
{
public:
	struct Common {
		Common(int handle = 0)
			: intHandle(handle)
		{}
		bool toggleContinuous = false;
		std::deque<std::pair<int, WORD>> gyroActionQueue; // Queue of gyro control actions currently in effect
		std::deque<int> chordStack; // Represents the current active buttons in order from most recent to latest
		int intHandle;
	};

	DigitalButton(DigitalButton::Common &btnCommon, int id, const char *name, const Mapping &mapping)
		: _common(btnCommon)
		, _id(id)
		, _name(name)
		, _mapping(mapping)
		, _press_times()
		, _btnState(BtnState::NoPress)
		, _keyToRelease(0)
		, _nameToRelease(nullptr)
	{

	}

	Common &_common;
	int _id;
	const char *_name; // Display name of the mapping
	const Mapping &_mapping;
	std::chrono::steady_clock::time_point _press_times;
	BtnState _btnState = BtnState::NoPress;
	WORD _keyToRelease = 0; // At key press, remember what to release
	const char * _nameToRelease = nullptr;

	float GetTapDuration()
	{
		// for tap duration, we need to know if the key in question is a gyro-related mapping or not
		WORD mapping = GetPressMapping(); //Consider chords
		return (mapping >= GYRO_INV_X && mapping <= GYRO_ON_BIND) ? MAGIC_GYRO_TAP_DURATION : MAGIC_TAP_DURATION;
	}

	WORD GetPressMapping()
	{
		// Look at active chord mappings starting with the latest activates chord
		for (auto activeChord = _common.chordStack.begin(); activeChord != _common.chordStack.end(); activeChord++)
		{
			for (auto &chordPress : _mapping.chord_mappings)
			{
				if (chordPress.btn == *activeChord && chordPress.btn != _id)
				{
					_keyToRelease = chordPress.pressBind;
					_nameToRelease = chordPress.name.c_str();
					return chordPress.pressBind;
				}
			}
		}
		_keyToRelease = _mapping.pressBind;
		_nameToRelease = _name;
		return _mapping.pressBind;
	}

	bool HasHoldMapping()
	{
		// Look at active chord mappings starting with the latest activates chord
		for (auto activeChord = _common.chordStack.begin(); activeChord != _common.chordStack.end(); activeChord++)
		{
			for (auto &chordPress : _mapping.chord_mappings)
			{
				if (chordPress.btn == *activeChord)
				{
					return chordPress.holdBind != 0;
				}
			}
		}
		return _mapping.holdBind != 0;
	}

	inline bool HasSimMapping()
	{
		return !_mapping.sim_mappings.empty();
	}

	inline bool HasDblPressMapping()
	{
		return GetDblPressMapping() != nullptr;
	}

	const ComboMap *GetDblPressMapping()
	{
		auto maps = std::find_if(_mapping.chord_mappings.begin(), _mapping.chord_mappings.end(),
			[this](ComboMap map)
			{
				return map.btn == _id;
			});
		return maps != _mapping.chord_mappings.end() ? &*maps : nullptr;
	}

	WORD GetHoldMapping()
	{
		// Look at active chord mappings starting with the latest activates chord
		for (auto activeChord = _common.chordStack.begin(); activeChord != _common.chordStack.end(); activeChord++)
		{
			for (auto &chordPress : _mapping.chord_mappings)
			{
				if (chordPress.btn == *activeChord && chordPress.btn != _id)
				{
					_keyToRelease = chordPress.holdBind;
					_nameToRelease = chordPress.name.c_str();
					return chordPress.holdBind;
				}
			}
		}
		_keyToRelease = _mapping.holdBind;
		_nameToRelease = _name;
		return _mapping.holdBind;
	}

	void ApplyBtnPress(bool tap = false)
	{
		auto key = GetPressMapping();
		if (key == CALIBRATE)
		{
			_common.toggleContinuous ^= tap; //Toggle on tap
			if (!tap || _common.toggleContinuous) {
				printf("Starting continuous calibration\n");
				JslResetContinuousCalibration(_common.intHandle);
				JslStartContinuousCalibration(_common.intHandle);
			}
		}
		else if (key >= GYRO_INV_X && key <= GYRO_ON_BIND)
		{
			_common.gyroActionQueue.push_back({ _id, key });
		}
		else
		{
			printf("%s: %s\n", _nameToRelease, tap ? "tapped" : "true");
			pressKey(key, true);
		}
		_keyToRelease = key;
		//if (std::find(_common.chordStack.begin(), _common.chordStack.end(), _id) == _common.chordStack.end())
		//{
		//	_common.chordStack.push_front(_id); // Always push at the fromt to make it a stack
		//}
	}

	void ApplyBtnHold()
	{
		auto key = GetHoldMapping();
		if (key == CALIBRATE)
		{
			printf("Starting continuous calibration\n");
			JslResetContinuousCalibration(_common.intHandle);
			JslStartContinuousCalibration(_common.intHandle);
		}
		else if (key >= GYRO_INV_X && key <= GYRO_ON_BIND)
		{
			_common.gyroActionQueue.push_back({ _id, key });
		}
		else if (key != NO_HOLD_MAPPED)
		{
			printf("%s: held\n", _nameToRelease);
			pressKey(key, true);
		}
		_keyToRelease = key;
		//if (std::find(_common.chordStack.begin(), _common.chordStack.end(), _id) == _common.chordStack.end())
		//{
		//	_common.chordStack.push_front(_id); // Always push at the fromt to make it a stack
		//}
	}

	void ApplyBtnRelease(bool tap = false)
	{
		if (_keyToRelease == CALIBRATE)
		{
			if (!tap || !_common.toggleContinuous)
			{
				JslPauseContinuousCalibration(_common.intHandle);
				_common.toggleContinuous = false; // if we've held the calibration button, we're disabling continuous calibration
				printf("Gyro calibration set\n");
			}
		}
		else if (_keyToRelease >= GYRO_INV_X && _keyToRelease <= GYRO_ON_BIND)
		{
			_common.gyroActionQueue.erase(std::find_if(_common.gyroActionQueue.begin(), _common.gyroActionQueue.end(),
				[this](auto pair)
				{
					return pair.first == _id;
				}));
		}
		else if (_keyToRelease != NO_HOLD_MAPPED)
		{
			printf(tap ? "" : "%s: false\n", _nameToRelease); // Is this good coding? [Insert meme]
			pressKey(_keyToRelease, false);
		}
		//auto foundChord = std::find(_common.chordStack.begin(), _common.chordStack.end(), _id);
		//if (foundChord != _common.chordStack.end())
		//{
		//	// The chord is released
		//	_common.chordStack.erase(foundChord);
		//}
	}

	void ApplyBtnPress(const ComboMap &map, bool tap = false)
	{
		if (map.pressBind == CALIBRATE)
		{
			_common.toggleContinuous ^= tap; //Toggle on tap
			if (!tap || _common.toggleContinuous) {
				printf("Starting continuous calibration\n");
				JslResetContinuousCalibration(_common.intHandle);
				JslStartContinuousCalibration(_common.intHandle);
			}
		}
		else if (map.pressBind >= GYRO_INV_X && map.pressBind <= GYRO_ON_BIND)
		{
			// I know I don't handle multiple inversion. Otherwise GYRO_INV_X on sim press would do nothing
			_common.gyroActionQueue.push_back({ _id, map.pressBind });
		}
		else
		{
			printf("%s: %s\n", map.name.c_str(), tap ? "tapped" : "true");
			pressKey(map.pressBind, true);
		}
		_keyToRelease = map.pressBind;
		_nameToRelease = map.name.c_str();
	}


	void ApplyBtnHold(const ComboMap &map)
	{
		if (map.holdBind == CALIBRATE)
		{
			printf("Starting continuous calibration\n");
			JslResetContinuousCalibration(_common.intHandle);
			JslStartContinuousCalibration(_common.intHandle);
		}
		else if (map.holdBind >= GYRO_INV_X && map.holdBind <= GYRO_ON_BIND)
		{
			// I know I don't handle multiple inversion. Otherwise GYRO_INV_X on sim press would do nothing
			_common.gyroActionQueue.push_back({ _id, map.holdBind });
		}
		else if (map.holdBind != NO_HOLD_MAPPED)
		{
			printf("%s: held\n", map.name.c_str());
			pressKey(map.holdBind, true);
		}
		_keyToRelease = map.holdBind;
		_nameToRelease = map.name.c_str();
	}

	void ApplyBtnRelease(const ComboMap &map, bool tap = false)
	{
		if (_keyToRelease == CALIBRATE)
		{
			if (!tap || !_common.toggleContinuous)
			{
				JslPauseContinuousCalibration(_common.intHandle);
				_common.toggleContinuous = false; // if we've held the calibration button, we're disabling continuous calibration
				printf("Gyro calibration set\n");
			}
		}
		else if (_keyToRelease >= GYRO_INV_X && _keyToRelease <= GYRO_ON_BIND)
		{
			_common.gyroActionQueue.erase(std::find_if(_common.gyroActionQueue.begin(), _common.gyroActionQueue.end(),
				[this] (auto pair)
				{
					return pair.first == _id;
				}));
			_common.gyroActionQueue.erase(std::find_if(_common.gyroActionQueue.begin(), _common.gyroActionQueue.end(),
				[map](auto pair)
				{
					return pair.first == map.btn;
				}));
		}
		else if (_keyToRelease != NO_HOLD_MAPPED)
		{
			printf(tap ? "" : "%s: false\n", _nameToRelease);
			pressKey(_keyToRelease, false);
		}
		auto foundChord = std::find(_common.chordStack.begin(), _common.chordStack.end(), _id);
		if (foundChord != _common.chordStack.end())
		{
			// The chord is released
			_common.chordStack.erase(foundChord);
		}
	}

	void SyncSimPress(const ComboMap &map)
	{
		_keyToRelease = map.pressBind;
		_nameToRelease = map.name.c_str();
		if (map.holdBind >= GYRO_INV_X && map.holdBind <= GYRO_ON_BIND)
		{
			_common.gyroActionQueue.push_back({ _id, map.holdBind });
		}
	}

	void SyncSimHold(const ComboMap &map)
	{
		_keyToRelease = map.holdBind;
		_nameToRelease = map.name.c_str();
		if (map.holdBind >= GYRO_INV_X && map.holdBind <= GYRO_ON_BIND)
		{
			// I know I don't handle multiple inversion. Otherwise GYRO_INV_X on sim press would do nothing
			_common.gyroActionQueue.push_back({ _id, map.holdBind });
		}
	}

	// Pretty wrapper
	inline float GetPressDurationMS(std::chrono::steady_clock::time_point time_now)
	{
		return static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(time_now - _press_times).count());
	}

	// Indicate if the button is currently sending an assigned mapping.
	bool IsActive()
	{
		return _btnState == BtnState::BtnPress || _btnState == BtnState::HoldPress; // Add Sim Press State? Only with Setting?
	}
};

std::mutex loading_lock;

StickMode left_stick_mode = StickMode::none;
StickMode right_stick_mode = StickMode::none;
RingMode left_ring_mode = RingMode::outer;
RingMode right_ring_mode = RingMode::outer;
GyroAxisMask mouse_x_from_gyro = GyroAxisMask::y;
GyroAxisMask mouse_y_from_gyro = GyroAxisMask::x;
JoyconMask joycon_gyro_mask = JoyconMask::ignoreLeft;
GyroIgnoreMode gyro_ignore_mode = GyroIgnoreMode::button; // Ignore mode none means no GYRO_OFF button
TriggerMode zlMode = TriggerMode::noFull;
TriggerMode zrMode = TriggerMode::noFull;
std::array<Mapping, MAPPING_SIZE> mappings; // std::array enables use of for each loop and other i/f
float min_gyro_sens_x = 0.0;
float min_gyro_sens_y = 0.0;
float max_gyro_sens_x = 0.0;
float max_gyro_sens_y = 0.0;
float min_gyro_threshold = 0.0;
float max_gyro_threshold = 0.0;
float stick_power = 0.0;
float stick_sens = 0.0;
int gyro_button = MAPPING_NONE; // No gyro button. The value is the btn index rather than the mask
bool gyro_always_off = false; // gyro_button disables when gyro is always on and vice versa
float real_world_calibration = 0.0;
float in_game_sens = 1.0;
float trigger_threshold = 0.0;
float os_mouse_speed = 1.0;
float aim_y_sign = 1.0;
float aim_x_sign = 1.0;
float gyro_y_sign = 1.0;
float gyro_x_sign = 1.0;
float flick_time = 0.0;
float gyro_smooth_time = 0.0;
float gyro_smooth_threshold = 0.0;
float gyro_cutoff_speed = 0.0;
float gyro_cutoff_recovery = 0.0;
float stick_acceleration_rate = 0.0;
float stick_acceleration_cap = 1.0;
float stick_deadzone_inner = 0.0;
float stick_deadzone_outer = 0.0;
float last_flick_and_rotation = 0.0;
float mouse_ring_radius = 128.0f;
int screen_resolution_x = 1920;
int screen_resolution_y = 1080;
float rotate_smooth_override = -1.0f;
float flick_snap_strength = 1.0f;
FlickSnapMode flick_snap_mode = FlickSnapMode::none;
std::unique_ptr<PollingThread> autoLoadThread;
std::unique_ptr<TrayIcon> tray;
bool devicesCalibrating = false;
Whitelister whitelister(false);

char tempConfigName[128];

typedef struct GyroSample {
	float x;
	float y;
} GyroSample;

class JoyShock {
private:
	float _weightsRemaining[64];
	float _flickSamples[64];
	int _frontSample = 0;

	GyroSample _gyroSamples[64];
	int _frontGyroSample = 0;

public:
	JoyShock(int uniqueHandle, float pollRate, int controllerSplitType, float stickStepSize)
		: intHandle(uniqueHandle)
		, poll_rate(pollRate)
		, controller_type(controllerSplitType)
		, stick_step_size(stickStepSize)
		, triggerState(LAST_ANALOG_TRIGGER - FIRST_ANALOG_TRIGGER + 1, DstState::NoPress)
		, btnCommon(intHandle)
		, buttons{
			DigitalButton(btnCommon, MAPPING_UP, "UP", mappings[MAPPING_UP]),
			DigitalButton(btnCommon, MAPPING_DOWN, "DOWN", mappings[MAPPING_DOWN]),
			DigitalButton(btnCommon, MAPPING_LEFT, "LEFT", mappings[MAPPING_LEFT]),
			DigitalButton(btnCommon, MAPPING_RIGHT, "RIGHT", mappings[MAPPING_RIGHT]),
			DigitalButton(btnCommon, MAPPING_L, "L", mappings[MAPPING_L]),
			DigitalButton(btnCommon, MAPPING_ZL, "ZL", mappings[MAPPING_ZL]),
			DigitalButton(btnCommon, MAPPING_MINUS, "MINUS", mappings[MAPPING_MINUS]),
			DigitalButton(btnCommon, MAPPING_CAPTURE, "CAPTURE", mappings[MAPPING_CAPTURE]),
			DigitalButton(btnCommon, MAPPING_E, "E", mappings[MAPPING_E]),
			DigitalButton(btnCommon, MAPPING_S, "S", mappings[MAPPING_S]),
			DigitalButton(btnCommon, MAPPING_N, "N", mappings[MAPPING_N]),
			DigitalButton(btnCommon, MAPPING_W, "W", mappings[MAPPING_W]),
			DigitalButton(btnCommon, MAPPING_R, "R", mappings[MAPPING_R]),
			DigitalButton(btnCommon, MAPPING_ZR, "ZR", mappings[MAPPING_ZR]),
			DigitalButton(btnCommon, MAPPING_PLUS, "PLUS", mappings[MAPPING_PLUS]),
			DigitalButton(btnCommon, MAPPING_HOME, "HOME", mappings[MAPPING_HOME]),
			DigitalButton(btnCommon, MAPPING_SL, "SL", mappings[MAPPING_SL]),
			DigitalButton(btnCommon, MAPPING_SR, "SR", mappings[MAPPING_SR]),
			DigitalButton(btnCommon, MAPPING_L3, "L3", mappings[MAPPING_L3]),
			DigitalButton(btnCommon, MAPPING_R3, "R3", mappings[MAPPING_R3]),
			DigitalButton(btnCommon, MAPPING_LUP, "LUP", mappings[MAPPING_LUP]),
			DigitalButton(btnCommon, MAPPING_LDOWN, "LDOWN", mappings[MAPPING_LDOWN]),
			DigitalButton(btnCommon, MAPPING_LLEFT, "LLEFT", mappings[MAPPING_LLEFT]),
			DigitalButton(btnCommon, MAPPING_LRIGHT, "LRIGHT", mappings[MAPPING_LRIGHT]),
			DigitalButton(btnCommon, MAPPING_LRING, "LRING", mappings[MAPPING_LRING]),
			DigitalButton(btnCommon, MAPPING_RUP, "RUP", mappings[MAPPING_RUP]),
			DigitalButton(btnCommon, MAPPING_RDOWN, "RDOWN", mappings[MAPPING_RDOWN]),
			DigitalButton(btnCommon, MAPPING_RLEFT, "RLEFT", mappings[MAPPING_RLEFT]),
			DigitalButton(btnCommon, MAPPING_RRIGHT, "RRIGHT", mappings[MAPPING_RRIGHT]),
			DigitalButton(btnCommon, MAPPING_RRING, "RRING", mappings[MAPPING_RRING]),
			DigitalButton(btnCommon, MAPPING_ZLF, "ZLF", mappings[MAPPING_ZLF]),
			DigitalButton(btnCommon, MAPPING_ZRF, "ZRF", mappings[MAPPING_ZRF]),
		}
	{
	}



	const int MaxGyroSamples = 64;
	const int NumSamples = 64;
	int intHandle;

	std::array<DigitalButton, MAPPING_SIZE> buttons;
	std::chrono::steady_clock::time_point started_flick;
	std::chrono::steady_clock::time_point time_now;
	// tap_release_queue has been replaced with button states *TapRelease. The hold time of the tap is effectively quantized to the polling period of the device.
	bool is_flicking_left = false;
	bool is_flicking_right = false;
	float delta_flick = 0.0;
	float flick_percent_done = 0.0;
	float flick_rotation_counter = 0.0;

	float poll_rate;
	int controller_type = 0;
	float stick_step_size;

	float left_acceleration = 1.0;
	float right_acceleration = 1.0;
	std::vector<DstState> triggerState; // State of analog triggers when skip mode is active
	DigitalButton::Common btnCommon;

	inline const ComboMap* GetMatchingSimMap(int index)
	{
		// Find the simMapping where the other btn is in the same state as this btn.
		// POTENTIAL FLAW: The mapping you find may not necessarily be the one that got you in a 
		// Simultaneous state in the first place if there is a second SimPress going on where one
		// of the buttons has a third SimMap with this one. I don't know if it's worth solving though...

		auto match = std::find_if(mappings[index].sim_mappings.cbegin(), mappings[index].sim_mappings.cend(),
			[this, index] (const ComboMap& simMap)
			{
				return buttons[simMap.btn]._btnState == buttons[index]._btnState && index != simMap.btn;
			});
		return match == mappings[index].sim_mappings.cend() ? nullptr : &*match;
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
		GyroSample frontSample = _gyroSamples[_frontGyroSample] = { x * smoothFactor, y * smoothFactor };
		// and now calculate smoothed result
		float xResult = frontSample.x / maxSamples;
		float yResult = frontSample.y / maxSamples;
		for (int i = 1; i < maxSamples; i++) {
			int rotatedIndex = (_frontGyroSample + i) % MaxGyroSamples;
			frontSample = _gyroSamples[rotatedIndex];
			xResult += frontSample.x / maxSamples;
			yResult += frontSample.y / maxSamples;
		}
		// finally, add immediate portion
		outX = xResult + x * immediateFactor;
		outY = yResult + y * immediateFactor;
	}

	~JoyShock() {
	}

	void handleButtonChange(int index, bool pressed) {
		auto foundChord = std::find(btnCommon.chordStack.begin(), btnCommon.chordStack.end(), index);
		if (!pressed)
		{
			if (foundChord != btnCommon.chordStack.end())
			{
				// The chord is released
				btnCommon.chordStack.erase(foundChord);
			}
		}
		else if (foundChord == btnCommon.chordStack.end()) {
			btnCommon.chordStack.push_front(index); // Always push at the fromt to make it a stack
		}

		switch (buttons[index]._btnState)
		{
		case BtnState::NoPress:
		if (pressed)
		{
			if (buttons[index].HasSimMapping())
			{
				buttons[index]._btnState = BtnState::WaitSim;
				buttons[index]._press_times = time_now;
			}
			else if (buttons[index].HasHoldMapping())
			{
				buttons[index]._btnState = BtnState::WaitHold;
				buttons[index]._press_times = time_now;
			}
			else if (buttons[index].HasDblPressMapping())
			{
				// Start counting time between two start presses
				buttons[index]._btnState = BtnState::DblPressStart;
				buttons[index]._press_times = time_now;
			}
			else
			{
				buttons[index]._btnState = BtnState::BtnPress;
				buttons[index].ApplyBtnPress();
			}
		}
		break;
		case BtnState::BtnPress:
		if (!pressed)
		{
			buttons[index]._btnState = BtnState::NoPress;
			buttons[index].ApplyBtnRelease();
		}
		break;
		case BtnState::WaitSim:
		if (!pressed)
		{
			buttons[index]._btnState = BtnState::TapRelease;
			buttons[index]._press_times = time_now;
			buttons[index].ApplyBtnPress(true);
		}
		else
		{
			// Is there a sim mapping on this button where the other button is in WaitSim state too?
			auto simMap = GetMatchingSimMap(index);
			if (simMap)
			{
				// We have a simultaneous press!
				if (simMap->holdBind)
				{
					buttons[index]._btnState = BtnState::WaitSimHold;
					buttons[simMap->btn]._btnState = BtnState::WaitSimHold;
					buttons[index]._press_times = time_now; // Reset Timer
				}
				else
				{
					buttons[index]._btnState = BtnState::SimPress;
					buttons[simMap->btn]._btnState = BtnState::SimPress;
					buttons[index].ApplyBtnPress(*simMap);
					buttons[simMap->btn].SyncSimPress(*simMap);
				}
			}
			else if (buttons[index].GetPressDurationMS(time_now) > MAGIC_SIM_DELAY)
			{
				// Sim delay expired!
				if (buttons[index].HasHoldMapping())
				{
					buttons[index]._btnState = BtnState::WaitHold;
					// Don't reset time
				}
				else if (buttons[index].HasDblPressMapping())
				{
					// Start counting time between two start presses
					buttons[index]._btnState = BtnState::DblPressStart;
				}
				else
				{
					buttons[index]._btnState = BtnState::BtnPress;
					buttons[index].ApplyBtnPress();
				}
			}
			// Else let time flow, stay in this state, no output.
		}
		break;
		case BtnState::WaitHold:
		if (!pressed)
		{
			if (buttons[index].HasDblPressMapping())
			{
				buttons[index]._btnState = BtnState::DblPressStart;
			}
			else
			{
				buttons[index]._btnState = BtnState::TapRelease;
				buttons[index]._press_times = time_now;
				buttons[index].ApplyBtnPress(true);
			}
		}
		else if (buttons[index].GetPressDurationMS(time_now) > MAGIC_HOLD_TIME)
		{
			buttons[index]._btnState = BtnState::HoldPress;
			buttons[index].ApplyBtnHold();
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
				printf("Error: lost track of matching sim press for %s! Resetting to NoPress.\n", buttons[index]._name);
				buttons[index]._btnState = BtnState::NoPress;
			}
			else if (!pressed)
			{
				buttons[index]._btnState = BtnState::SimRelease;
				buttons[simMap->btn]._btnState = BtnState::SimRelease;
				buttons[index].ApplyBtnRelease(*simMap, index);
			}
			// else sim press is being held, as far as this button is concerned.
			break;
		}
		case BtnState::HoldPress:
		if (!pressed)
		{
			buttons[index]._btnState = BtnState::NoPress;
			buttons[index].ApplyBtnRelease();
		}
		break;
		case BtnState::WaitSimHold:
		{
			// Which is the sim mapping where the other button is in WaitSimHold state too?
			auto simMap = GetMatchingSimMap(index);
			if (!simMap)
			{
				// Should never happen but added for robustness.
				printf("Error: lost track of matching sim press for %s! Resetting to NoPress.\n", buttons[index]._name);
				buttons[index]._btnState = BtnState::NoPress;
			}
			else if (!pressed)
			{
				buttons[index]._btnState = BtnState::SimTapRelease;
				buttons[simMap->btn]._btnState = BtnState::SimTapRelease;
				buttons[index]._press_times = time_now;
				buttons[simMap->btn]._press_times = time_now;
				buttons[index].ApplyBtnPress(*simMap, true);
				buttons[simMap->btn].SyncSimPress(*simMap);
			}
			else if (buttons[index].GetPressDurationMS(time_now) > MAGIC_HOLD_TIME)
			{
				buttons[index]._btnState = BtnState::SimHold;
				buttons[simMap->btn]._btnState = BtnState::SimHold;
				buttons[index].ApplyBtnHold(*simMap);
				buttons[simMap->btn].SyncSimHold(*simMap);
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
				printf("Error: lost track of matching sim press for %s! Resetting to NoPress.\n", buttons[index]._name);
				buttons[index]._btnState = BtnState::NoPress;
			}
			else if (!pressed)
			{
				buttons[index]._btnState = BtnState::SimRelease;
				buttons[simMap->btn]._btnState = BtnState::SimRelease;
				buttons[index].ApplyBtnRelease(*simMap);
			}
			break;
		}
		case BtnState::SimRelease:
		if (!pressed)
		{
			buttons[index]._btnState = BtnState::NoPress;
		}
		break;
		case BtnState::SimTapRelease:
		{
			// Which is the sim mapping where the other button is in SimTapRelease state too?
			auto simMap = GetMatchingSimMap(index);
			if (!simMap)
			{
				// Should never happen but added for robustness.
				printf("Error: lost track of matching sim press for %s! Resetting to NoPress.\n", buttons[index]._name);
				buttons[index]._btnState = BtnState::NoPress;
			}
			else if (pressed)
			{
				buttons[index].ApplyBtnRelease(*simMap, true);
				buttons[index]._btnState = BtnState::NoPress;
				buttons[simMap->btn]._btnState = BtnState::SimRelease;
			}
			else if (buttons[index].GetPressDurationMS(time_now) > buttons[index].GetTapDuration())
			{
				buttons[index].ApplyBtnRelease(*simMap, true);
				buttons[index]._btnState = BtnState::SimRelease;
				buttons[simMap->btn]._btnState = BtnState::SimRelease;
			}
			break;
		}
		case BtnState::TapRelease:
		if (pressed || buttons[index].GetPressDurationMS(time_now) > buttons[index].GetTapDuration())
		{
			buttons[index].ApplyBtnRelease(true);
			buttons[index]._btnState = BtnState::NoPress;
		}
		break;
		case BtnState::DblPressStart:
		if (buttons[index].GetPressDurationMS(time_now) > MAGIC_DBL_PRESS_WINDOW)
		{
			buttons[index]._btnState = BtnState::BtnPress;
			buttons[index].ApplyBtnPress();
		}
		else if (!pressed)
		{
			buttons[index]._btnState = BtnState::DblPressNoPress;
		}
		break;
		case BtnState::DblPressNoPress:
		if (buttons[index].GetPressDurationMS(time_now) > MAGIC_DBL_PRESS_WINDOW)
		{
			buttons[index]._btnState = BtnState::TapRelease;
			buttons[index].ApplyBtnPress(true);
		}
		else if (pressed)
		{
			// dblPress will be valid because HasDblPressMapping already returned true.
			if (buttons[index].GetDblPressMapping()->holdBind)
			{
				buttons[index]._btnState = BtnState::DblPressWaitHold;
				buttons[index]._press_times = time_now;
			}
			else
			{
				buttons[index]._btnState = BtnState::DblPressPress;
				buttons[index].ApplyBtnPress(*buttons[index].GetDblPressMapping());
			}
		}
		break;
		case BtnState::DblPressPress:
		if (!pressed)
		{
			buttons[index]._btnState = BtnState::NoPress;
			buttons[index].ApplyBtnRelease(*buttons[index].GetDblPressMapping());
		}
		break;
		case BtnState::DblPressWaitHold:
		if (!pressed)
		{
			buttons[index]._btnState = BtnState::TapRelease;
			buttons[index]._press_times = time_now;
			buttons[index].ApplyBtnPress(*buttons[index].GetDblPressMapping(), true);
		}
		else if (buttons[index].GetPressDurationMS(time_now) > MAGIC_HOLD_TIME)
		{
			buttons[index]._btnState = BtnState::DblPressHold;
			buttons[index].ApplyBtnHold(*buttons[index].GetDblPressMapping());
		}
		break;
		case BtnState::DblPressHold:
		if (!pressed)
		{
			buttons[index]._btnState = BtnState::NoPress;
			buttons[index].ApplyBtnRelease(*buttons[index].GetDblPressMapping());
		}
		break;
		default:
		printf("Invalid button state %d: Resetting to NoPress\n", buttons[index]._btnState);
		buttons[index]._btnState = BtnState::NoPress;
		break;

		}
	}

	void handleTriggerChange(int softIndex, int fullIndex, TriggerMode mode, float pressed)
	{
		if (JslGetControllerType(intHandle) != JS_TYPE_DS4)
		{
			// Override local variable because the controller has digital triggers. Effectively ignore Full Pull binding.
			mode = TriggerMode::noFull;
		}

		auto idxState = fullIndex - FIRST_ANALOG_TRIGGER; // Get analog trigger index
		if (idxState < 0 || idxState >= triggerState.size())
		{
			printf("Error: Trigger %s does not exist in state map. Dual Stage Trigger not possible.\n", buttons[fullIndex]._name);
			return;
		}

		// if either trigger is waiting to be tap released, give it a go
		if (buttons[softIndex]._btnState == BtnState::TapRelease || buttons[softIndex]._btnState == BtnState::SimTapRelease)
		{
			// keep triggering until the tap release is complete
			handleButtonChange(softIndex, false);
		}
		if (buttons[fullIndex]._btnState == BtnState::TapRelease || buttons[fullIndex]._btnState == BtnState::SimTapRelease)
		{
			// keep triggering until the tap release is complete
			handleButtonChange(fullIndex, false);
		}

		switch (triggerState[idxState])
		{
		case DstState::NoPress:
		// It actually doesn't matter what the last Press is. Theoretically, we could have missed the edge.
		if (pressed > trigger_threshold)
		{
			if (mode == TriggerMode::maySkip || mode == TriggerMode::mustSkip)
			{
				// Start counting press time to see if soft binding should be skipped
				triggerState[idxState] = DstState::PressStart;
				buttons[softIndex]._press_times = time_now;
			}
			else if (mode == TriggerMode::maySkipResp || mode == TriggerMode::mustSkipResp)
			{
				triggerState[idxState] = DstState::PressStartResp;
				buttons[softIndex]._press_times = time_now;
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
		if (pressed <= trigger_threshold) {
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
		else if (buttons[softIndex].GetPressDurationMS(time_now) >= MAGIC_DST_DELAY) { // todo: get rid of magic number -- make this a user setting )
			triggerState[idxState] = DstState::SoftPress;
			// Reset the time for hold soft press purposes.
			buttons[softIndex]._press_times = time_now;
			handleButtonChange(softIndex, true);
		}
		// Else, time passes as soft press is being held, waiting to see if the soft binding should be skipped
		break;
		case DstState::PressStartResp:
		if (pressed <= trigger_threshold) {
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
			if (buttons[softIndex].GetPressDurationMS(time_now) >= MAGIC_DST_DELAY) { // todo: get rid of magic number -- make this a user setting )
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
		if (pressed <= trigger_threshold) {
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
		if (pressed <= trigger_threshold) {
			// Soft press is being released
			triggerState[idxState] = DstState::NoPress;
			handleButtonChange(softIndex, false);
		}
		else // Soft Press is being held
		{
			handleButtonChange(softIndex, true);

			if ((mode == TriggerMode::maySkip || mode == TriggerMode::noSkip || mode == TriggerMode::maySkipResp)
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
		printf("Error: Trigger %s has invalid state #%d. Reset to NoPress.\n", buttons[softIndex]._name, triggerState[idxState]);
		triggerState[idxState] = DstState::NoPress;
		break;
		}
	}

};

// https://stackoverflow.com/a/4119881/1130520 gives us case insensitive equality
static bool iequals(const std::string& a, const std::string& b)
{
	return std::equal(a.begin(), a.end(),
		b.begin(), b.end(),
		[](char a, char b) {
		return tolower(a) == tolower(b);
	});
}

std::optional<float> getFloat(const std::string &str, size_t *newpos = nullptr)
{
	try
	{
		float f = std::stof(str, newpos);
		return f;
	}
	catch (std::invalid_argument)
	{
		return std::nullopt;
	}
}

std::unordered_map<int, JoyShock*> handle_to_joyshock;

static int keyToBitOffset(WORD index) {
	switch (index) {
	case MAPPING_UP:
		return JSOFFSET_UP;
	case MAPPING_DOWN:
		return JSOFFSET_DOWN;
	case MAPPING_LEFT:
		return JSOFFSET_LEFT;
	case MAPPING_RIGHT:
		return JSOFFSET_RIGHT;
	case MAPPING_L:
		return JSOFFSET_L;
	case MAPPING_ZL:
		return JSOFFSET_ZL;
	case MAPPING_MINUS:
		return JSOFFSET_MINUS;
	case MAPPING_CAPTURE:
		return JSOFFSET_CAPTURE;
	case MAPPING_E:
		return JSOFFSET_E;
	case MAPPING_S:
		return JSOFFSET_S;
	case MAPPING_N:
		return JSOFFSET_N;
	case MAPPING_W:
		return JSOFFSET_W;
	case MAPPING_R:
		return JSOFFSET_R;
	case MAPPING_ZR:
		return JSOFFSET_ZR;
	case MAPPING_PLUS:
		return JSOFFSET_PLUS;
	case MAPPING_HOME:
		return JSOFFSET_HOME;
	case MAPPING_SL:
		return JSOFFSET_SL;
	case MAPPING_SR:
		return JSOFFSET_SR;
	case MAPPING_L3:
		return JSOFFSET_LCLICK;
	case MAPPING_R3:
		return JSOFFSET_RCLICK;
	}
}

bool IsPressed(const JOY_SHOCK_STATE& state, int mappingIndex)
{
	if (mappingIndex >= 0 && mappingIndex < MAPPING_SIZE)
	{
		if (mappingIndex == MAPPING_ZLF) return state.lTrigger == 1.0;
		else if (mappingIndex == MAPPING_ZRF) return state.rTrigger == 1.0;
		// Is it better to consider trigger_threshold for GYRO_ON/OFF on ZL/ZR?
		else if (mappingIndex == MAPPING_ZL) return state.lTrigger > trigger_threshold;
		else if (mappingIndex == MAPPING_ZR) return state.rTrigger > trigger_threshold;
		else return state.buttons & (1 << keyToBitOffset(mappingIndex));
	}
	return false;
}

/// Yes, this looks slow. But it's only there to help set up mappings more easily
static int keyToMappingIndex(std::string& s) {
	if (s.rfind("NONE", 0) == 0) {
		return MAPPING_NONE;
	}
	if (s.compare("UP") == 0) {
		return MAPPING_UP;
	}
	if (s.compare("DOWN") == 0) {
		return MAPPING_DOWN;
	}
	if (s.compare("LEFT") == 0) {
		return MAPPING_LEFT;
	}
	if (s.compare("RIGHT") == 0) {
		return MAPPING_RIGHT;
	}
	if (s.compare("L") == 0) {
		return MAPPING_L;
	}
	if (s.compare("ZL") == 0) {
		return MAPPING_ZL;
	}
	if (s.compare("-") == 0) {
		return MAPPING_MINUS;
	}
	if (s.compare("CAPTURE") == 0) {
		return MAPPING_CAPTURE;
	}
	if (s.compare("E") == 0) {
		return MAPPING_E;
	}
	if (s.compare("S") == 0) {
		return MAPPING_S;
	}
	if (s.compare("N") == 0) {
		return MAPPING_N;
	}
	if (s.compare("W") == 0) {
		return MAPPING_W;
	}
	if (s.compare("R") == 0) {
		return MAPPING_R;
	}
	if (s.compare("ZR") == 0) {
		return MAPPING_ZR;
	}
	if (s.compare("+") == 0) {
		return MAPPING_PLUS;
	}
	if (s.compare("HOME") == 0) {
		return MAPPING_HOME;
	}
	if (s.compare("SL") == 0) {
		return MAPPING_SL;
	}
	if (s.compare("SR") == 0) {
		return MAPPING_SR;
	}
	if (s.compare("L3") == 0) {
		return MAPPING_L3;
	}
	if (s.compare("R3") == 0) {
		return MAPPING_R3;
	}
	if (s.compare("LUP") == 0) {
		return MAPPING_LUP;
	}
	if (s.compare("LDOWN") == 0) {
		return MAPPING_LDOWN;
	}
	if (s.compare("LLEFT") == 0) {
		return MAPPING_LLEFT;
	}
	if (s.compare("LRIGHT") == 0) {
		return MAPPING_LRIGHT;
	}
	if (s.compare("RUP") == 0) {
		return MAPPING_RUP;
	}
	if (s.compare("RDOWN") == 0) {
		return MAPPING_RDOWN;
	}
	if (s.compare("RLEFT") == 0) {
		return MAPPING_RLEFT;
	}
	if (s.compare("RRIGHT") == 0) {
		return MAPPING_RRIGHT;
	}
	if (s.compare("ZRF") == 0) {
		return MAPPING_ZRF;
	}
	if (s.compare("ZLF") == 0) {
		return MAPPING_ZLF;
	}
	if (s.compare("LRING") == 0) {
		return MAPPING_LRING;
	}
	if (s.compare("RRING") == 0) {
		return MAPPING_RRING;
	}
	if (s.compare("MIN_GYRO_SENS") == 0) {
		return MIN_GYRO_SENS;
	}
	if (s.compare("MAX_GYRO_SENS") == 0) {
		return MAX_GYRO_SENS;
	}
	if (s.compare("MIN_GYRO_THRESHOLD") == 0) {
		return MIN_GYRO_THRESHOLD;
	}
	if (s.compare("MAX_GYRO_THRESHOLD") == 0) {
		return MAX_GYRO_THRESHOLD;
	}
	if (s.compare("SCREEN_RESOLUTION_X") == 0) {
		return SCREEN_RESOLUTION_X;
	}
	if (s.compare("SCREEN_RESOLUTION_Y") == 0) {
		return SCREEN_RESOLUTION_Y;
	}
	if (s.compare("ROTATE_SMOOTH_OVERRIDE") == 0) {
		return ROTATE_SMOOTH_OVERRIDE;
	}
	if (s.compare("MOUSE_RING_RADIUS") == 0) {
		return MOUSE_RING_RADIUS;
	}
	if (s.compare("FLICK_SNAP_MODE") == 0) {
		return FLICK_SNAP_MODE;
	}
	if (s.compare("FLICK_SNAP_STRENGTH") == 0) {
		return FLICK_SNAP_STRENGTH;
	}
	if (s.compare("STICK_POWER") == 0) {
		return STICK_POWER;
	}
	if (s.compare("STICK_SENS") == 0) {
		return STICK_SENS;
	}
	if (s.compare("REAL_WORLD_CALIBRATION") == 0) {
		return REAL_WORLD_CALIBRATION;
	}
	if (s.compare("IN_GAME_SENS") == 0) {
		return IN_GAME_SENS;
	}
	if (s.compare("TRIGGER_THRESHOLD") == 0) {
		return TRIGGER_THRESHOLD;
	}
	if (s.compare("RESET_MAPPINGS") == 0) {
		return RESET_MAPPINGS;
	}
	if (s.compare("NO_GYRO_BUTTON") == 0) {
		return NO_GYRO_BUTTON;
	}
	if (s.compare("LEFT_STICK_MODE") == 0) {
		return LEFT_STICK_MODE;
	}
	if (s.compare("RIGHT_STICK_MODE") == 0) {
		return RIGHT_STICK_MODE;
	}
	if (s.compare("LEFT_RING_MODE") == 0) {
		return LEFT_RING_MODE;
	}
	if (s.compare("RIGHT_RING_MODE") == 0) {
		return RIGHT_RING_MODE;
	}
	if (s.compare("GYRO_OFF") == 0) {
		return GYRO_OFF;
	}
	if (s.compare("GYRO_ON") == 0) {
		return GYRO_ON;
	}
	if (s.compare("STICK_AXIS_X") == 0) {
		return STICK_AXIS_X;
	}
	if (s.compare("STICK_AXIS_Y") == 0) {
		return STICK_AXIS_Y;
	}
	if (s.compare("GYRO_AXIS_X") == 0) {
		return GYRO_AXIS_X;
	}
	if (s.compare("GYRO_AXIS_Y") == 0) {
		return GYRO_AXIS_Y;
	}
	if (s.compare("RECONNECT_CONTROLLERS") == 0) {
		return RECONNECT_CONTROLLERS;
	}
	if (s.compare("COUNTER_OS_MOUSE_SPEED") == 0) {
		return COUNTER_OS_MOUSE_SPEED;
	}
	if (s.compare("IGNORE_OS_MOUSE_SPEED") == 0) {
		return IGNORE_OS_MOUSE_SPEED;
	}
	if (s.compare("JOYCON_GYRO_MASK") == 0) {
		return JOYCON_GYRO_MASK;
	}
	if (s.compare("GYRO_SENS") == 0) {
		return GYRO_SENS;
	}
	if (s.compare("FLICK_TIME") == 0) {
		return FLICK_TIME;
	}
	if (s.compare("GYRO_SMOOTH_THRESHOLD") == 0) {
		return GYRO_SMOOTH_THRESHOLD;
	}
	if (s.compare("GYRO_SMOOTH_TIME") == 0) {
		return GYRO_SMOOTH_TIME;
	}
	if (s.compare("GYRO_CUTOFF_SPEED") == 0) {
		return GYRO_CUTOFF_SPEED;
	}
	if (s.compare("GYRO_CUTOFF_RECOVERY") == 0) {
		return GYRO_CUTOFF_RECOVERY;
	}
	if (s.compare("STICK_ACCELERATION_RATE") == 0) {
		return STICK_ACCELERATION_RATE;
	}
	if (s.compare("STICK_ACCELERATION_CAP") == 0) {
		return STICK_ACCELERATION_CAP;
	}
	if (s.compare("STICK_DEADZONE_INNER") == 0) {
		return STICK_DEADZONE_INNER;
	}
	if (s.compare("STICK_DEADZONE_OUTER") == 0) {
		return STICK_DEADZONE_OUTER;
	}
	if (s.rfind("CALCULATE_REAL_WORLD_CALIBRATION", 0) == 0) {
		return CALCULATE_REAL_WORLD_CALIBRATION;
	}
	if (s.rfind("FINISH_GYRO_CALIBRATION", 0) == 0) {
		return FINISH_GYRO_CALIBRATION;
	}
	if (s.rfind("RESTART_GYRO_CALIBRATION", 0) == 0) {
		return RESTART_GYRO_CALIBRATION;
	}
	if (s.rfind("MOUSE_X_FROM_GYRO_AXIS", 0) == 0) {
		return MOUSE_X_FROM_GYRO_AXIS;
	}
	if (s.rfind("MOUSE_Y_FROM_GYRO_AXIS", 0) == 0) {
		return MOUSE_Y_FROM_GYRO_AXIS;
	}
	if (s.rfind("ZR_MODE", 0) == 0) {
		return ZR_DUAL_STAGE_MODE;
	}
	if (s.rfind("ZL_MODE", 0) == 0) {
		return ZL_DUAL_STAGE_MODE;
	}
	if (s.rfind("AUTOLOAD", 0) == 0) {
		return AUTOLOAD;
	}
	if (s.rfind("HELP", 0) == 0) {
		return HELP;
	}
	if (s.rfind("WHITELIST_SHOW", 0) == 0) {
		return WHITELIST_SHOW;
	}
	if (s.rfind("WHITELIST_ADD", 0) == 0) {
		return WHITELIST_ADD;
	}
	if (s.rfind("WHITELIST_REMOVE", 0) == 0) {
		return WHITELIST_REMOVE;
	}
	return MAPPING_ERROR;
}

static StickMode nameToStickMode(std::string& name, bool print = false) {
	if (name.compare("AIM") == 0) {
		if (print) printf("Aim");
		return StickMode::aim;
	}
	if (name.compare("FLICK") == 0) {
		if (print) printf("Flick");
		return StickMode::flick;
	}
	if (name.compare("NO_MOUSE") == 0) {
		if (print) printf("No mouse");
		return StickMode::none;
	}
	if (name.compare("FLICK_ONLY") == 0) {
		if (print) printf("Flick only");
		return StickMode::flickOnly;
	}
	if (name.compare("ROTATE_ONLY") == 0) {
		if (print) printf("Rotate only");
		return StickMode::rotateOnly;
	}
	if (name.compare("MOUSE_RING") == 0) {
		if (print) printf("Mouse ring");
		return StickMode::mouseRing;
	}
	// just here for backwards compatibility
	if (name.compare("INNER_RING") == 0) {
		if (print) printf("Inner Ring (Legacy)");
		return StickMode::inner;
	}
	if (name.compare("OUTER_RING") == 0) {
		if (print) printf("Outer Ring (Legacy)");
		return StickMode::outer;
	}
	if (print) printf("\"%s\" invalid", name.c_str());
	return StickMode::invalid;
}

static RingMode nameToRingMode(std::string& name, bool print = false) {
	if (name.compare("INNER") == 0) {
		if (print) printf("Inner");
		return RingMode::inner;
	}
	if (name.compare("OUTER") == 0) {
		if (print) printf("Outer");
		return RingMode::outer;
	}
	if (print) printf("\"%s\" invalid", name.c_str());
	return RingMode::invalid;
}

static FlickSnapMode nameToFlickSnapMode(std::string& name, bool print = false) {
	if (name.compare("NONE") == 0) {
		if (print) printf("None");
		return FlickSnapMode::none;
	}
	if (name.compare("4") == 0) {
		if (print) printf("Four");
		return FlickSnapMode::four;
	}
	if (name.compare("8") == 0) {
		if (print) printf("Eight");
		return FlickSnapMode::eight;
	}
	if (print) printf("\"%s\" invalid", name.c_str());
	return FlickSnapMode::invalid;
}

static AxisMode nameToAxisMode(std::string& name, bool print = false) {
	if (name.compare("STANDARD") == 0) {
		if (print) printf("Standard");
		return AxisMode::standard;
	}
	if (name.compare("INVERTED") == 0) {
		if (print) printf("Inverted");
		return AxisMode::inverted;
	}
	if (print) printf("\"%s\" invalid", name.c_str());
	return AxisMode::invalid;
}

static TriggerMode nameToTriggerMode(std::string& name, bool print = false) {
	if (name.compare("NO_FULL") == 0) {
		if (print) printf("Trigger will never apply full pull binding");
		return TriggerMode::noFull;
	}
	if (name.compare("NO_SKIP") == 0) {
		if (print) printf("Trigger will never skip trigger binding to apply full pull binding");
		return TriggerMode::noSkip;
	}
	if (name.compare("MAY_SKIP") == 0) {
		if (print) printf("Trigger can skip trigger binding on a quick full pull");
		return TriggerMode::maySkip;
	}
	if (name.compare("MUST_SKIP") == 0) {
		if (print) printf("Trigger can only apply full pull binding on a quick full pull, skipping trigger binding");
		return TriggerMode::mustSkip;
	}
	if (name.compare("MAY_SKIP_R") == 0) {
		if (print) printf("Trigger can skip trigger binding on a quick full pull, maximizing soft binding responsiveness");
		return TriggerMode::maySkipResp;
	}
	if (name.compare("MUST_SKIP_R") == 0) {
		if (print) printf("Trigger can only apply full pull binding on a quick full pull, skipping trigger binding, maximizing soft binding responsiveness");
		return TriggerMode::mustSkipResp;
	}
	if (print) printf("\"%s\" invalid", name.c_str());
	return TriggerMode::invalid;
}

static GyroAxisMask nameToGyroAxisMask(std::string& name, bool print = false) {
	if (name.compare("X") == 0) {
		if (print) printf("X");
		return GyroAxisMask::x;
	}
	if (name.compare("Y") == 0) {
		if (print) printf("Y");
		return GyroAxisMask::y;
	}
	if (name.compare("Z") == 0) {
		if (print) printf("Z");
		return GyroAxisMask::z;
	}
	if (name.compare("NONE") == 0) {
		if (print) printf("NONE");
		return GyroAxisMask::none;
	}
	if (print) printf("\"%s\" invalid", name.c_str());
	return GyroAxisMask::invalid;
}

static JoyconMask nameToJoyconMask(std::string& name, bool print = false) {
	if (name.compare("USE_BOTH") == 0) {
		if (print) printf("Use both");
		return JoyconMask::useBoth;
	}
	if (name.compare("IGNORE_LEFT") == 0) {
		if (print) printf("Ignore left");
		return JoyconMask::ignoreLeft;
	}
	if (name.compare("IGNORE_RIGHT") == 0) {
		if (print) printf("Ignore right");
		return JoyconMask::ignoreRight;
	}
	if (name.compare("IGNORE_BOTH") == 0) {
		if (print) printf("Ignore both");
		return JoyconMask::ignoreBoth;
	}
	if (print) printf("\"%s\" invalid", name.c_str());
	return JoyconMask::invalid;
}

// https://stackoverflow.com/questions/25345598/c-implementation-to-trim-char-array-of-leading-trailing-white-space-not-workin
static void strtrim(char* str) {
	int start = 0; // number of leading spaces
	char* buffer = str;
	while (*str && *str++ == ' ') ++start;
	while (*str++); // move to end of string
	int end = str - buffer - 1;
	while (end > 0 && buffer[end - 1] == ' ') --end; // backup over trailing spaces
	buffer[end] = 0; // remove trailing spaces
	if (end <= start || start == 0) return; // exit if no leading spaces or string is now empty
	str = buffer + start;
	while ((*buffer++ = *str++));  // remove leading spaces: K&R
}

static void resetAllMappings() {
	std::for_each(mappings.begin(), mappings.end(), [] (auto &map) { map.reset(); });
	// Question: Why is this a default mapping? Shouldn't it be empty? It's always possible to calibrate with RESET_GYRO_CALIBRATION
	mappings[MAPPING_HOME].pressBind = mappings[MAPPING_CAPTURE].pressBind = mappings[MAPPING_HOME].holdBind = mappings[MAPPING_CAPTURE].holdBind = CALIBRATE;
	min_gyro_sens_x = 0.0f;
	min_gyro_sens_y = 0.0f;
	max_gyro_sens_x = 0.0f;
	max_gyro_sens_y = 0.0f;
	min_gyro_threshold = 0.0f;
	max_gyro_threshold = 0.0f;
	stick_power = 1.0f;
	stick_sens = 360.0f;
	real_world_calibration = 40.0f;
	in_game_sens = 1.0f;
	left_stick_mode = StickMode::none;
	right_stick_mode = StickMode::none;
	left_ring_mode = RingMode::outer;
	right_ring_mode = RingMode::outer;
	mouse_x_from_gyro = GyroAxisMask::y;
	mouse_y_from_gyro = GyroAxisMask::x;
	joycon_gyro_mask = JoyconMask::ignoreLeft;
	zlMode = TriggerMode::noFull;
	zrMode = TriggerMode::noFull;
	trigger_threshold = 0.0f;
	os_mouse_speed = 1.0f;
	gyro_button = 0;
	gyro_always_off = false;
	aim_y_sign = 1.0f;
	aim_x_sign = 1.0f;
	gyro_y_sign = 1.0f;
	gyro_x_sign = 1.0f;
	flick_time = 0.1f;
	gyro_smooth_time = 0.125f;
	gyro_smooth_threshold = 0.0f;
	gyro_cutoff_speed = 0.0f;
	gyro_cutoff_recovery = 0.0f;
	stick_acceleration_rate = 0.0f;
	stick_acceleration_cap = 1000000.0f;
	stick_deadzone_inner = 0.15f;
	stick_deadzone_outer = 0.1f;
	last_flick_and_rotation = 0.0f;
	screen_resolution_x = 1920;
	screen_resolution_y = 1080;
	mouse_ring_radius = 128.0f;
	rotate_smooth_override = -1.0f;
	flick_snap_strength = 1.0f;
	flick_snap_mode = FlickSnapMode::none;
}

void joyShockPollCallback(int jcHandle, JOY_SHOCK_STATE state, JOY_SHOCK_STATE lastState, IMU_STATE imuState, IMU_STATE lastImuState, float deltaTime);
void connectDevices();
static void parseCommand(std::string line);

static bool loadMappings(std::string fileName) {
	// https://stackoverflow.com/questions/2602013/read-whole-ascii-file-into-c-stdstring
	std::ifstream t(fileName);
	if (t)
	{
		printf("Loading commands from file %s\n", fileName.c_str());
		// https://stackoverflow.com/questions/6892754/creating-a-simple-configuration-file-and-parser-in-c
		std::string line;
		while (getline(t, line)) {
			parseCommand(line);
		}
		t.close();
		return true;
	}
	else {
		return false;
	}
}

static void parseCommand(std::string line) {
	if (line[0] == 0) {
		return;
	}
	// # for comments
	if (line[0] == '#') {
		return;
	}
	// replace further comment markers with null so that input ends there
	std::replace(line.begin(), line.end(), '#', (char)0);
	if (loadMappings(line)) {
		return;
	}
	//printf("Processing line %s\n", line);
	std::istringstream is_line(line);
	char key[128];
	if (is_line.getline(key, 128, '=')) {
		strtrim(key);
		//printf("Key: %s; ", key);
		int index2, index = keyToMappingIndex(std::string(key));
		ComboMap *simMap = nullptr; // Used when parsing simultaneous press
		ComboMap* chordMap = nullptr; // Used when parsing chorded press
		// unary commands
		switch (index) {
		case NO_GYRO_BUTTON:
			printf("No button disables or enables gyro\n");
			gyro_button = MAPPING_NONE;
			gyro_always_off = false;
			gyro_ignore_mode = GyroIgnoreMode::button;
			return;
		case RESET_MAPPINGS:
			printf("Resetting all mappings to defaults\n");
			resetAllMappings();
			return;
		case RECONNECT_CONTROLLERS:
			printf("Reconnecting controllers\n");
			JslDisconnectAndDisposeAll();
			connectDevices();
			JslSetCallback(&joyShockPollCallback);
			return;
		case COUNTER_OS_MOUSE_SPEED:
			printf("Countering OS mouse speed setting\n");
			os_mouse_speed = getMouseSpeed();
			return;
		case IGNORE_OS_MOUSE_SPEED:
			printf("Ignoring OS mouse speed setting\n");
			os_mouse_speed = 1.0;
			return;
		case CALCULATE_REAL_WORLD_CALIBRATION: {
			// first, check for a parameter
			std::string keyAsString = std::string(key);
			std::string crwcArgument = keyAsString.substr(std::string("CALCULATE_REAL_WORLD_CALIBRATION").length(), 64); // no argument bigger than 64, I assume
			float numRotations = 1.0;
			if (crwcArgument.length() > 0) {
				try {
					numRotations = std::stof(crwcArgument);
				}
				catch (std::invalid_argument ia) {
					printf("Can't convert \"%s\" to a number\n", crwcArgument.c_str());
					return;
				}
			}
			if (numRotations == 0) {
				printf("Can't calculate calibration from zero rotations\n");
			}
			else if (last_flick_and_rotation == 0) {
				printf("Need to use the flick stick at least once before calculating an appropriate calibration value\n");
			}
			else {
				printf("Recommendation: REAL_WORLD_CALIBRATION = %.5g\n", real_world_calibration * last_flick_and_rotation / numRotations);
			}
			return;
		}
		case FINISH_GYRO_CALIBRATION: {
			printf("Finishing continuous calibration for all devices\n");
			for (std::unordered_map<int, JoyShock*>::iterator iter = handle_to_joyshock.begin(); iter != handle_to_joyshock.end(); ++iter) {
				JoyShock* jc = iter->second;
				JslPauseContinuousCalibration(jc->intHandle);
			}
			devicesCalibrating = false;
			return;
		}
		case RESTART_GYRO_CALIBRATION:
			printf("Restarting continuous calibration for all devices\n");
			for (std::unordered_map<int, JoyShock*>::iterator iter = handle_to_joyshock.begin(); iter != handle_to_joyshock.end(); ++iter) {
				JoyShock* jc = iter->second;
				JslResetContinuousCalibration(jc->intHandle);
				JslStartContinuousCalibration(jc->intHandle);
			}
			devicesCalibrating = true;
			return;
		case HELP:
			printf("Opening online help in your browser\n");
			if (auto err = ShowOnlineHelp() != 0)
			{
				printf("Could not open online help. Error #%d\n", err);
			}
			return;
		case WHITELIST_SHOW:
			printf("Launching HIDCerberus in your browser. Your PID is %lu\n", GetCurrentProcessId()); // WinAPI call!
			Whitelister::ShowHIDCerberus();
			return;
		case WHITELIST_ADD:
			whitelister.Add();
			if (whitelister)
			{
				printf("JoyShockMapper was successfully whitelisted\n");
			}
			else
			{
				printf("Whitelisting failed!\n");
			}
			return;
		case WHITELIST_REMOVE:
			whitelister.Remove();
			printf("JoyShockMapper removed from whitelist\n");
			return;
		}

		std::optional<float> sens;
		size_t pos;
		char value[128];
		// other commands
		if (is_line.getline(value, 128)) {
			strtrim(value);
			//printf("Value: %s;\n", value);
			// bam! got our thingy!
			// now, let's map!
			// if index < 0 => handle Combo press after settings below
			if (index == MAPPING_SIZE) {
				printf("Error: Input %s somehow mapped to array size\n", key);
				return;
			}
			// settings
			else if (index > MAPPING_SIZE) {
				switch (index) {
				case MIN_GYRO_SENS:
					sens = getFloat(value, &pos);
					if(sens)
					{
						min_gyro_sens_x = min_gyro_sens_y = *sens;
						sens = getFloat(&value[pos]);
						if (sens) {
							min_gyro_sens_y = *sens;
							value[pos] = 0;
							printf("Min gyro sensitivity set to X:%sx Y:%sx\n", value, &value[pos+1]);
						} else {
							printf("Min gyro sensitivity set to %sx\n", value);
						}
					}
					else {
						printf("Can't convert \"%s\" to one or two numbers\n", value);
					}
					return;
				case MAX_GYRO_SENS:
					sens = getFloat(value, &pos);
					if (sens)
					{
						max_gyro_sens_x = max_gyro_sens_y = *sens;
						sens = getFloat(&value[pos]);
						if (sens) {
							max_gyro_sens_y = *sens;
							value[pos] = 0;
							printf("Max gyro sensitivity set to X:%sx Y:%sx\n", value, &value[pos + 1]);
						} else {
							printf("Max gyro sensitivity set to %sx\n", value);
						}
					}
					else {
						printf("Can't convert \"%s\" to one or two numbers\n", value);
					}
					return;
				case MIN_GYRO_THRESHOLD:
					try {
						min_gyro_threshold = std::stof(value);
						printf("Min gyro sensitivity used at and below %sdps\n", value);
					}
					catch (std::invalid_argument ia) {
						printf("Can't convert \"%s\" to a number\n", value);
					}
					return;
				case MAX_GYRO_THRESHOLD:
					try {
						max_gyro_threshold = std::stof(value);
						printf("Max gyro sensitivity used at and above %sdps\n", value);
					}
					catch (std::invalid_argument ia) {
						printf("Can't convert \"%s\" to a number\n", value);
					}
					return;
				case STICK_POWER:
					try {
						stick_power = max(0.0f, std::stof(value));
						printf("Stick power set to %0.4f\n", stick_power);
					}
					catch (std::invalid_argument ia) {
						printf("Can't convert \"%s\" to a number\n", value);
					}
					return;
				case STICK_SENS:
					try {
						stick_sens = std::stof(value);
						printf("Stick sensitivity set to %sdps\n", value);
					}
					catch (std::invalid_argument ia) {
						printf("Can't convert \"%s\" to a number\n", value);
					}
					return;
				case REAL_WORLD_CALIBRATION:
					try {
						real_world_calibration = std::stof(value);
						printf("Real world calibration set to %s\n", value);
					}
					catch (std::invalid_argument ia) {
						printf("Can't convert \"%s\" to a number\n", value);
					}
					return;
				case IN_GAME_SENS:
				{
					try {
						float old_sens = in_game_sens;
						in_game_sens = std::stof(value);
						if (in_game_sens == 0) {
							in_game_sens = old_sens;
							printf("Can't set IN_GAME_SENS to zero. Reverting to previous setting (%0.4f)\n", in_game_sens);
						}
						else {
							printf("In game sensitivity set to %s\n", value);
						}
					}
					catch (std::invalid_argument ia) {
						printf("Can't convert \"%s\" to a number\n", value);
					}
					return;
				}
				case MOUSE_RING_RADIUS:
					try {
						float temp = std::stof(value);
						mouse_ring_radius = temp;
						printf("Mouse ring radius set to %s\n", value);
					}
					catch (std::invalid_argument ia) {
						printf("Can't convert \"%s\" to a number\n", value);
					}
					return;
				case SCREEN_RESOLUTION_X:
					try {
						int temp = std::stoi(value);
						screen_resolution_x = temp;
						printf("Screen resolution X set to %s\n", value);
					}
					catch (std::invalid_argument ia) {
						printf("Can't convert \"%s\" to a number\n", value);
					}
					return;
				case SCREEN_RESOLUTION_Y:
					try {
						int temp = std::stoi(value);
						screen_resolution_y = temp;
						printf("Screen resolution Y set to %s\n", value);
					}
					catch (std::invalid_argument ia) {
						printf("Can't convert \"%s\" to a number\n", value);
					}
					return;
				case ROTATE_SMOOTH_OVERRIDE:
					try {
						float temp = std::stof(value);
						if (temp < 0.0f) {
							temp = -1.0f;
						}
						rotate_smooth_override = temp;
						printf("Rotate smooth override set to %0.4f\n", temp);
					}
					catch (std::invalid_argument ia) {
						printf("Can't convert \"%s\" to a number\n", value);
					}
					return;
				case FLICK_SNAP_STRENGTH:
					try {
						float temp = std::stof(value);
						if (temp < 0.0f) {
							temp = 0.0f;
						}
						if (temp > 1.0f) {
							temp = 1.0f;
						}
						flick_snap_strength = temp;
						printf("Flick snap strength set to %0.4f\n", temp);
					}
					catch (std::invalid_argument ia) {
						printf("Can't convert \"%s\" to a number\n", value);
					}
					return;
				case FLICK_SNAP_MODE:
				{
					FlickSnapMode temp = nameToFlickSnapMode(std::string(value));
					if (temp != FlickSnapMode::invalid) {
						flick_snap_mode = temp;
						printf("Flick snap mode set to %s\n", value);
					}
					else {
						printf("Valid settings for FLICK_SNAP_MODE are NONE, 4, or 8\n");
					}
					return;
				}
				case TRIGGER_THRESHOLD:
					try {
						trigger_threshold = std::stof(value);
						printf("Trigger ignored below %s\n", value);
					}
					catch (std::invalid_argument ia) {
						printf("Can't convert \"%s\" to a number\n", value);
					}
					return;
				case LEFT_STICK_MODE:
				{
					printf("Left stick: ");
					StickMode temp = nameToStickMode(std::string(value), true);
					printf("\n");
					if (temp != StickMode::invalid) {
						if (temp == StickMode::inner) {
							left_ring_mode = RingMode::inner;
							temp = StickMode::none;
						}
						else if (temp == StickMode::outer) {
							left_ring_mode = RingMode::outer;
							temp = StickMode::none;
						}
						left_stick_mode = temp;
					}
					else {
						printf("Valid settings for LEFT_STICK_MODE are NO_MOUSE, AIM, FLICK, INNER_RING or OUTER_RING\n");
					}
					return;
				}
				case RIGHT_STICK_MODE:
				{
					printf("Right stick: ");
					StickMode temp = nameToStickMode(std::string(value), true);
					printf("\n");
					if (temp != StickMode::invalid) {
						if (temp == StickMode::inner) {
							right_ring_mode = RingMode::inner;
							temp = StickMode::none;
						}
						else if (temp == StickMode::outer) {
							right_ring_mode = RingMode::outer;
							temp = StickMode::none;
						}
						right_stick_mode = temp;
					}
					else {
						printf("Valid settings for RIGHT_STICK_MODE are NO_MOUSE, AIM, FLICK, FLICK_ONLY, ROTATE_ONLY, ABSOLUTE_MOUSOE, INNER_RING or OUTER_RING\n");
					}
					return;
				}
				case LEFT_RING_MODE:
				{
					printf("Left ring: ");
					RingMode temp = nameToRingMode(std::string(value), true);
					printf("\n");
					if (temp != RingMode::invalid) {
						left_ring_mode = temp;
					}
					else {
						printf("Valid settings for LEFT_RING_MODE are INNER or OUTER\n");
					}
					return;
				}
				case RIGHT_RING_MODE:
				{
					printf("Right ring: ");
					RingMode temp = nameToRingMode(std::string(value), true);
					printf("\n");
					if (temp != RingMode::invalid) {
						right_ring_mode = temp;
					}
					else {
						printf("Valid settings for RIGHT_RING_MODE are INNER or OUTER\n");
					}
					return;
				}
				case STICK_AXIS_X:
				{
					printf("Stick aim X axis: ");
					AxisMode sign = nameToAxisMode(std::string(value), true);
					printf("\n");
					if (sign) {
						aim_x_sign = static_cast<float>(sign);
					}
					else {
						printf("Valid settings for STICK_AXIS_X are STANDARD or INVERTED\n");
					}
					return;
				}
				case STICK_AXIS_Y:
				{
					printf("Stick aim Y axis: ");
					AxisMode sign = nameToAxisMode(std::string(value), true);
					printf("\n");
					if (sign) {
						aim_y_sign = static_cast<float>(sign);
					}
					else {
						printf("Valid settings for STICK_AXIS_Y are STANDARD or INVERTED\n");
					}
					return;
				}
				case GYRO_AXIS_X:
				{
					printf("Gyro aim X axis: ");
					AxisMode sign = nameToAxisMode(std::string(value), true);
					printf("\n");
					if (sign) {
						gyro_x_sign = static_cast<float>(sign);
					}
					else {
						printf("Valid settings for GYRO_AXIS_X are STANDARD or INVERTED\n");
					}
					return;
				}
				case GYRO_AXIS_Y:
				{
					printf("Gyro aim Y axis: ");
					AxisMode sign = nameToAxisMode(std::string(value), true);
					printf("\n");
					if (sign) {
						gyro_y_sign = static_cast<float>(sign);
					}
					else {
						printf("Valid settings for GYRO_AXIS_Y are STANDARD or INVERTED\n");
					}
					return;
				}
				case JOYCON_GYRO_MASK:
				{
					printf("Joycon gyro mask: ");
					JoyconMask temp = nameToJoyconMask(std::string(value), true);
					printf("\n");
					if (temp != JoyconMask::invalid) {
						joycon_gyro_mask = temp;
						printf("Joycon gyro mask set to %s\n", value);
					}
					else {
						printf("Valid settings for JOYCON_GYRO_MASK are IGNORE_LEFT, IGNORE_RIGHT, IGNORE_BOTH, or USE_BOTH\n");
					}
					return;
				}
				case MOUSE_X_FROM_GYRO_AXIS:
				{
					printf("Mouse x reads gyro axis: ");
					GyroAxisMask temp = nameToGyroAxisMask(std::string(value), true);
					printf("\n");
					if (temp != GyroAxisMask::invalid) {
						mouse_x_from_gyro = temp;
						if (temp == GyroAxisMask::none) {
							printf("Gyro not used for mouse x axis\no");
						}
						else {
							printf("Mouse x axis set to gyro %s axis\n", value);
						}
					}
					else {
						printf("Valid settings for MOUSE_X_FROM_GYRO_AXIS are X, Y, Z, or NONE\n");
					}
					return;
				}
				case MOUSE_Y_FROM_GYRO_AXIS:
				{
					printf("Mouse y reads gyro axis: ");
					GyroAxisMask temp = nameToGyroAxisMask(std::string(value), true);
					printf("\n");
					if (temp != GyroAxisMask::invalid) {
						mouse_y_from_gyro = temp;
						if (temp == GyroAxisMask::none) {
							printf("Gyro not used for mouse y axis\n");
						}
						else {
							printf("Mouse y axis set to gyro %s axis\n", value);
						}
					}
					else {
						printf("Valid settings for MOUSE_Y_FROM_GYRO_AXIS are X, Y, Z, or NONE\n");
					}
					return;
				}
				case GYRO_OFF:
				{
					std::string valueName = std::string(value);
					int rhsMappingIndex = keyToMappingIndex(valueName);
					if (rhsMappingIndex >= 0 && rhsMappingIndex < MAPPING_SIZE)
					{
						gyro_always_off = false;
						gyro_ignore_mode = GyroIgnoreMode::button;
						gyro_button = rhsMappingIndex;
						printf("Disable gyro with %s\n", value);
					}
					else if (rhsMappingIndex == MAPPING_NONE)
					{
						gyro_always_off = false;
						gyro_ignore_mode = GyroIgnoreMode::button;
						gyro_button = MAPPING_NONE;
						printf("No button disables gyro\n");
					}
					else if (valueName.compare("LEFT_STICK") == 0)
					{
						gyro_always_off = false;
						gyro_ignore_mode = GyroIgnoreMode::left;
						printf("Disable gyro when left stick is used\n");
					}
					else if (valueName.compare("RIGHT_STICK") == 0)
					{
						gyro_always_off = false;
						gyro_ignore_mode = GyroIgnoreMode::right;
						printf("Disable gyro when right stick is used\n");
					}
					else {
						printf("Can't map GYRO_OFF to \"%s\". Gyro button can be unmapped with \"NO_GYRO_BUTTON\"\n", value);
					}
					return;
				}
				case GYRO_ON:
				{
					std::string valueName = std::string(value);
					int rhsMappingIndex = keyToMappingIndex(valueName);
					if (rhsMappingIndex >= 0 && rhsMappingIndex < MAPPING_SIZE)
					{
						gyro_always_off = true;
						gyro_ignore_mode = GyroIgnoreMode::button;
						gyro_button = rhsMappingIndex;
						printf("Enable gyro with %s\n", value);
					}
					else if (rhsMappingIndex == MAPPING_NONE)
					{
						gyro_always_off = true;
						gyro_ignore_mode = GyroIgnoreMode::button;
						gyro_button = MAPPING_NONE;
						printf("No button enables gyro\n");
					}
					else if (valueName.compare("LEFT_STICK") == 0)
					{
						gyro_always_off = true;
						gyro_ignore_mode = GyroIgnoreMode::left;
						printf("Enable gyro when left stick is used\n");
					}
					else if (valueName.compare("RIGHT_STICK") == 0)
					{
						gyro_always_off = true;
						gyro_ignore_mode = GyroIgnoreMode::right;
						printf("Enable gyro when right stick is used\n");
					}
					else {
						printf("Can't map GYRO_ON to \"%s\". Gyro button can be unmapped with \"NO_GYRO_BUTTON\"\n", value);
					}
					return;
				}
				case GYRO_SENS:
					sens = getFloat(value, &pos); // Get sensitivity value
					if (sens)
					{
						min_gyro_sens_x = min_gyro_sens_y = max_gyro_sens_x = max_gyro_sens_y = *sens;
						sens = getFloat(&value[pos]); // Is there a specific vertical value?
						if (sens) {
							min_gyro_sens_y = max_gyro_sens_y = *sens;
							value[pos] = 0;
							printf("Gyro sensitivity set to X:%sx Y:%sx\n", value, &value[pos + 1]);
						}
						else {
							printf("Gyro sensitivity set to %sx\n", value);
						}
					}
					else {
						printf("Can't convert \"%s\" to one or two numbers\n", value);
					}
					return;
				case FLICK_TIME:
				{
					try {
						float val = std::stof(value);
						if (val < 0.0001f) {
							val = 0.0001f; // prevent it being actually zero
						}
						flick_time = val;
						printf("Flick time set to %ss\n", value);
					}
					catch (std::invalid_argument ia) {
						printf("Can't convert \"%s\" to a number\n", value);
					}
					return;
				}
				case GYRO_SMOOTH_THRESHOLD:
				{
					try {
						gyro_smooth_threshold = std::stof(value);
						printf("Gyro smoothing ignored at and above %sdps\n", value);
					}
					catch (std::invalid_argument ia) {
						printf("Can't convert \"%s\" to a number\n", value);
					}
					return;
				}
				case GYRO_SMOOTH_TIME:
				{
					try {
						gyro_smooth_time = std::stof(value);
						printf("GYRO_SMOOTH_TIME set to %ss\n", value);
					}
					catch (std::invalid_argument ia) {
						printf("Can't convert \"%s\" to a number\n", value);
					}
					return;
				}
				case GYRO_CUTOFF_SPEED:
				{
					try {
						gyro_cutoff_speed = std::stof(value);
						printf("Ignoring gyro speed below %sdps\n", value);
					}
					catch (std::invalid_argument ia) {
						printf("Can't convert \"%s\" to a number\n", value);
					}
					return;
				}
				case GYRO_CUTOFF_RECOVERY:
				{
					try {
						gyro_cutoff_recovery = std::stof(value);
						printf("Gyro cutoff recovery set to %sdps\n", value);
					}
					catch (std::invalid_argument ia) {
						printf("Can't convert \"%s\" to a number\n", value);
					}
					return;
				}
				case STICK_ACCELERATION_RATE:
				{
					try {
						stick_acceleration_rate = max(0.0f, std::stof(value));
						printf("Stick speed when fully pressed increases by a factor of %s per second\n", value);
					}
					catch (std::invalid_argument ia) {
						printf("Can't convert \"%s\" to a number\n", value);
					}
					return;
				}
				case STICK_ACCELERATION_CAP:
				{
					try {
						stick_acceleration_cap = max(1.0f, std::stof(value));
						printf("Stick acceleration factor capped at %s\n", value);
					}
					catch (std::invalid_argument ia) {
						printf("Can't convert \"%s\" to a number\n", value);
					}
					return;
				}
				case STICK_DEADZONE_INNER:
				{
					try {
						stick_deadzone_inner = max(0.0f, min(1.0f, std::stof(value)));
						printf("Stick treats any value within %s of the centre as the centre\n", value);
					}
					catch (std::invalid_argument ia) {
						printf("Can't convert \"%s\" to a number\n", value);
					}
					return;
				}
				case STICK_DEADZONE_OUTER:
				{
					try {
						stick_deadzone_outer = max(0.0f, min(1.0f, std::stof(value)));
						printf("Stick treats any value within %s of the edge as the edge\n", value);
					}
					catch (std::invalid_argument ia) {
						printf("Can't convert \"%s\" to a number\n", value);
					}
					return;
				}
				case ZR_DUAL_STAGE_MODE:
				{
					printf("Right trigger: ");
					TriggerMode temp = nameToTriggerMode(std::string(value), true);
					printf("\n");
					if (temp != TriggerMode::invalid) {
						zrMode = temp;
						for (auto js : handle_to_joyshock)
						{
							if (JslGetControllerType(js.first) != JS_TYPE_DS4)
							{
								printf("WARNING: Dual Stage Triggers are only valid on analog triggers. Full pull bindings will be ignored on non DS4 controllers.\n");
								break;
							}
						}
					}
					else {
						printf("Valid settings for ZR_MODE are NO_FULL, NO_SKIP, MAY_SKIP, MAY_SKIP_R, MUST_SKIP and MUST_SKIP_R\n");
					}
					return;
				}
				case ZL_DUAL_STAGE_MODE:
				{
					printf("Left trigger: ");
					TriggerMode temp = nameToTriggerMode(std::string(value), true);
					printf("\n");
					if (temp != TriggerMode::invalid) {
						zlMode = temp;
						for (auto js : handle_to_joyshock)
						{
							if (JslGetControllerType(js.first) != JS_TYPE_DS4)
							{
								printf("WARNING: Dual Stage Triggers are only valid on analog triggers. Full pull bindings will be ignored on non DS4 controllers.\n");
								break;
							}
						}
					}
					else {
						printf("Valid settings for ZR_MODE are NO_FULL, NO_SKIP, MAY_SKIP, MAY_SKIP_R, MUST_SKIP and MUST_SKIP_R\n");
					}
					return;
				}
				case AUTOLOAD:
				{
					if (!autoLoadThread)
					{
						printf("AutoLoad is unavailable\n");
						return;
					}
					std::string valueName(value);
					if (valueName.compare("ON") == 0)
					{
						autoLoadThread->Start();
						printf("AutoLoad is now running.\n");
					}
					else if (valueName.compare("OFF") == 0)
					{
						autoLoadThread->Stop();
						printf("AutoLoad is stopped\n");
					}
					else {
						printf("%s is an invalid option\nAUTOLOAD can only be assigned ON or OFF\n", value);
					}
					return;
				}
				default:
					printf("Error: Input %s somehow mapped to value out of mapping range\n", key);
				}
			}
			else if (index < 0)
			{
				// Try Parsing a simultaneous Press
				std::string keystr(key);
				index = keyToMappingIndex(keystr.substr(0, keystr.find_first_of('+')));
				index2 = keyToMappingIndex(keystr.substr(keystr.find_first_of('+') + 1, keystr.size()));
				if (index >= 0 && index < MAPPING_SIZE && index2 >= 0 && index2 < MAPPING_SIZE)
				{
					auto existingBinding = std::find_if(mappings[index].sim_mappings.begin(), mappings[index].sim_mappings.end(),
						[index2] (const auto &mapping) {
						return mapping.btn == index2;
					});
					if (existingBinding != mappings[index].sim_mappings.end())
					{
						// Remove any existing mapping
						mappings[index].sim_mappings.erase(existingBinding);
						// Is it fair to assume the opposing pair exists?
						existingBinding = std::find_if(mappings[index2].sim_mappings.begin(), mappings[index2].sim_mappings.end(),
							[index] (const auto &mapping) {
							return mapping.btn == index;
						});
						mappings[index2].sim_mappings.erase(existingBinding);
					}
					mappings[index].sim_mappings.push_back({ keystr, index2, 0, 0 });
					simMap = &mappings[index].sim_mappings.back();
				}
				else
				{
					// Try parsing a chorded press. It is stored in index2. 
					index = keyToMappingIndex(keystr.substr(0, keystr.find_first_of(',')));
					index2 = keyToMappingIndex(keystr.substr(keystr.find_first_of(',') + 1, keystr.size()));
					if (index >= 0 && index < MAPPING_SIZE && index2 >= 0 && index2 < MAPPING_SIZE)
					{
						auto existingBinding = std::find_if(mappings[index2].chord_mappings.cbegin(), mappings[index2].chord_mappings.cend(),
							[index](const auto& chordMap) {
								return chordMap.btn == index;
							});
						if (existingBinding != mappings[index2].chord_mappings.end())
						{
							// Remove any existing mapping
							mappings[index2].chord_mappings.erase(existingBinding);
						}
						mappings[index2].chord_mappings.push_back( { keystr, index, 0, 0 } );
						chordMap = &mappings[index2].chord_mappings.back(); // point to vector item
					}
					else
					{
						printf("Warning: Input %s not recognized\n", key);
						return;
					}
				}
			}
			// we have a valid single button input, so clear old hold_mapping for this input
			if(index >= 0 && !simMap && !chordMap) mappings[index].holdBind = 0;
			WORD output = nameToKey(std::string(value));
			if (output == NO_HOLD_MAPPED) {
				output = 0;
				printf("%s mapped to no input\n", key);
			}
			else if (output == 0x00) {
				// try splitting -- it might be a tap and hold
				char* subValue = value;
				while (*subValue && *++subValue != ' '); // find a space
				if (*subValue) {
					// we haven't reached the end, we're good to go
					*subValue = 0; // null terminating. now we maybe have two strings
					subValue++;
				}
				// Else we have reached the end. Keep pointing at the 0
				output = nameToKey(std::string(value));
				WORD holdOutput = nameToKey(std::string(subValue));
				if (output == 0x00) {
					// tap and hold requires the tap to be a regular input. Hold can be none, if only taps should register input
					printf("Tap output %s for %s not recognized\n", value, key);
				}
				else {
					printf("Tap %s mapped to %s\n", key, value);
				}
				if (strlen(subValue) > 0)
				{
					if (holdOutput == 0x00 ) {
						printf("Hold output %s for input %s not recognized\n", subValue, key);
					}

					else
					{
						simMap ? simMap->holdBind = holdOutput : 
							chordMap ? chordMap->holdBind = holdOutput :
							mappings[index].holdBind = holdOutput;
						printf("Hold %s mapped to %s\n", key, subValue);
					}
				}

				// translate a "NONE" to 0 -- only keep it as a NO_HOLD_MAPPING for hold_mappings
				if (output == NO_HOLD_MAPPED) {
					output = 0;
				}
			}
			else {
				printf("%s mapped to %s\n", key, value);
			}
			simMap ? simMap->pressBind = output : 
				chordMap ? chordMap->pressBind = output :
				mappings[index].pressBind = output;
			if (output == 0) // Setting a sim press or chord to NONE erases the bindings
			{
				if (simMap)
				{
					mappings[index].sim_mappings.pop_back();
				}
				else if (chordMap)
				{
					mappings[index2].chord_mappings.pop_back();
				}
			}
			else if(simMap) // and output not NONE
			{
				// Sim Press commands are added as twins
				mappings[simMap->btn].sim_mappings.push_back( {simMap->name, index, simMap->pressBind, simMap->holdBind } );
			}
		}
		else {
			printf("Command \"%s\" not recognized\nEnter HELP to open online documentation\n", line.c_str());
		}
	}
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

// return true if it hits the outer deadzone
bool processDeadZones(float& x, float& y) {
	float innerDeadzone = stick_deadzone_inner;
	float outerDeadzone = 1.0f - stick_deadzone_outer;
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

static float handleFlickStick(float calX, float calY, float lastCalX, float lastCalY, float stickLength, bool& isFlicking, JoyShock * jc, float mouseCalibrationFactor, bool flickOnly, bool rotateOnly) {
	float camSpeedX = 0.0f;
	// let's centre this
	float offsetX = calX;
	float offsetY = calY;
	float lastOffsetX = lastCalX;
	float lastOffsetY = lastCalY;
	float flickStickThreshold = 1.0f - stick_deadzone_outer;
	if (isFlicking)
	{
		flickStickThreshold *= 0.9;
	}
	if (stickLength >= flickStickThreshold) {
		float stickAngle = atan2(-offsetX, offsetY);
		//printf(", %.4f\n", lastOffsetLength);
		if (!isFlicking) {
			// bam! new flick!
			isFlicking = true;
			if (!rotateOnly)
			{
				if (flick_snap_mode != FlickSnapMode::none) {
					// handle snapping
					float snapInterval;
					if (flick_snap_mode == FlickSnapMode::four) {
						snapInterval = PI / 2.0f; // 90 degrees
					}
					else if (flick_snap_mode == FlickSnapMode::eight) {
						snapInterval = PI / 4.0f; // 45 degrees
					}
					float snappedAngle = round(stickAngle / snapInterval) * snapInterval;
					// lerp by snap strength
					stickAngle = stickAngle * (1.0f - flick_snap_strength) + snappedAngle * flick_snap_strength;
				}

				jc->started_flick = std::chrono::steady_clock::now();
				jc->delta_flick = stickAngle;
				jc->flick_percent_done = 0.0f;
				jc->ResetSmoothSample();
				jc->flick_rotation_counter = stickAngle; // track all rotation for this flick
				// TODO: All these printfs should be hidden behind a setting. User might not want them.
				printf("Flick: %.3f degrees\n", stickAngle * (180.0f / PI));
			}
		}
		else {
			if (!flickOnly)
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
				float flickSpeedConstant = real_world_calibration * mouseCalibrationFactor / in_game_sens;
				float flickSpeed = -(angleChange * flickSpeedConstant);
				int maxSmoothingSamples = min(jc->NumSamples, (int)(64.0f * (jc->poll_rate / 1000.0f))); // target a max smoothing window size of 64ms
				float stepSize = jc->stick_step_size; // and we only want full on smoothing when the stick change each time we poll it is approximately the minimum stick resolution
													  // the fact that we're using radians makes this really easy
				
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
		if (!flickOnly && !rotateOnly)
		{
			last_flick_and_rotation = abs(jc->flick_rotation_counter) / (2.0 * PI);
		}
		isFlicking = false;
	}
	// do the flicking
	float secondsSinceFlick = ((float)std::chrono::duration_cast<std::chrono::microseconds>(jc->time_now - jc->started_flick).count()) / 1000000.0f;
	float newPercent = secondsSinceFlick / flick_time;
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
	float camSpeedChange = (newPercent - oldShapedPercent) * jc->delta_flick * real_world_calibration * -mouseCalibrationFactor / in_game_sens;
	camSpeedX += camSpeedChange;

	return camSpeedX;
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
	int mouse_x_flag = (int)mouse_x_from_gyro;
	if ((mouse_x_flag & (int)GyroAxisMask::x) > 0) {
		gyroX -= imuState.gyroX; // x axis is negative because that's what worked before, don't want to mess with definitions of "inverted"
	}
	if ((mouse_x_flag & (int)GyroAxisMask::y) > 0) {
		gyroX -= imuState.gyroY; // x axis is negative because that's what worked before, don't want to mess with definitions of "inverted"
	}
	if ((mouse_x_flag & (int)GyroAxisMask::z) > 0) {
		gyroX -= imuState.gyroZ; // x axis is negative because that's what worked before, don't want to mess with definitions of "inverted"
	}
	int mouse_y_flag = (int)mouse_y_from_gyro;
	if ((mouse_y_flag & (int)GyroAxisMask::x) > 0) {
		gyroY += imuState.gyroX;
	}
	if ((mouse_y_flag & (int)GyroAxisMask::y) > 0) {
		gyroY += imuState.gyroY;
	}
	if ((mouse_y_flag & (int)GyroAxisMask::z) > 0) {
		gyroY += imuState.gyroZ;
	}
	float gyroLength = sqrt(gyroX * gyroX + gyroY * gyroY);
	// do gyro smoothing
	// convert gyro smooth time to number of samples
	int numGyroSamples = jc->poll_rate * gyro_smooth_time; // samples per second * seconds = samples
	if (numGyroSamples < 1) numGyroSamples = 1; // need at least 1 sample
	jc->GetSmoothedGyro(gyroX, gyroY, gyroLength, gyro_smooth_threshold / 2.0f, gyro_smooth_threshold, numGyroSamples, gyroX, gyroY);
	//printf("%d Samples for threshold: %0.4f\n", numGyroSamples, gyro_smooth_threshold * maxSmoothingSamples);

	// now, honour gyro_cutoff_speed
	gyroLength = sqrt(gyroX * gyroX + gyroY * gyroY);
	if (gyro_cutoff_recovery > gyro_cutoff_speed) {
		// we can use gyro_cutoff_speed
		float gyroIgnoreFactor = (gyroLength - gyro_cutoff_speed) / (gyro_cutoff_recovery - gyro_cutoff_speed);
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
	else if (gyro_cutoff_speed > 0.0f && gyroLength < gyro_cutoff_speed) {
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
	bool leftPegged = processDeadZones(calX, calY);
	float absX = abs(calX);
	float absY = abs(calY);
	bool left = calX < -0.5f * absY;
	bool right = calX > 0.5f * absY;
	bool down = calY < -0.5f * absX;
	bool up = calY > 0.5f * absX;
	float stickLength = sqrt(calX * calX + calY * calY);
	bool ring = left_ring_mode == RingMode::inner && stickLength > 0.0f && stickLength < 0.7f ||
		left_ring_mode == RingMode::outer && stickLength > 0.7f;
	jc->handleButtonChange(MAPPING_LRING, ring);

	bool rotateOnly = left_stick_mode == StickMode::rotateOnly;
	bool flickOnly = left_stick_mode == StickMode::flickOnly;
	if (left_stick_mode == StickMode::flick || flickOnly || rotateOnly) {
		camSpeedX += handleFlickStick(calX, calY, lastCalX, lastCalY, stickLength, jc->is_flicking_left, jc, mouseCalibrationFactor, flickOnly, rotateOnly);
		leftAny = leftPegged;
	}
	else if (left_stick_mode == StickMode::aim) {
		// camera movement
		if (!leftPegged) {
			jc->left_acceleration = 1.0f; // reset
		}
		float stickLength = sqrt(calX * calX + calY * calY);
		if (stickLength != 0.0f) {
			leftAny = true;
			float warpedStickLength = pow(stickLength, stick_power);
			warpedStickLength *= stick_sens * real_world_calibration / os_mouse_speed / in_game_sens;
			camSpeedX += calX / stickLength * warpedStickLength * jc->left_acceleration * deltaTime;
			camSpeedY += calY / stickLength * warpedStickLength * jc->left_acceleration * deltaTime;
			if (leftPegged) {
				jc->left_acceleration += stick_acceleration_rate * deltaTime;
				if (jc->left_acceleration > stick_acceleration_cap) {
					jc->left_acceleration = stick_acceleration_cap;
				}
			}
		}
	}
	else if (left_stick_mode == StickMode::mouseRing) {
		if (calX != 0.0f || calY != 0.0f) {
			float stickLength = sqrt(calX * calX + calY * calY);
			float normX = calX / stickLength;
			float normY = calY / stickLength;
			// use screen resolution
			float mouseX = (float)screen_resolution_x * 0.5f + 0.5f + normX * mouse_ring_radius;
			float mouseY = (float)screen_resolution_y * 0.5f + 0.5f - normY * mouse_ring_radius;
			// normalize
			mouseX = mouseX / screen_resolution_x;
			mouseY = mouseY / screen_resolution_y;
			// do it!
			setMouseNorm(mouseX, mouseY);
			lockMouse = true;
		}
	}
	else {
		// left!
		jc->handleButtonChange(MAPPING_LLEFT, left);
		// right!
		jc->handleButtonChange(MAPPING_LRIGHT, right);
		// up!
		jc->handleButtonChange(MAPPING_LUP, up);
		// down!
		jc->handleButtonChange(MAPPING_LDOWN, down);

		leftAny = left | right | up | down; // ring doesn't count
	}
	lastCalX = lastState.stickRX;
	lastCalY = lastState.stickRY;
	calX = state.stickRX;
	calY = state.stickRY;
	bool rightPegged = processDeadZones(calX, calY);
	absX = abs(calX);
	absY = abs(calY);
	left = calX < -0.5f * absY;
	right = calX > 0.5f * absY;
	down = calY < -0.5f * absX;
	up = calY > 0.5f * absX;
	stickLength = sqrt(calX * calX + calY * calY);
	ring = right_ring_mode == RingMode::inner && stickLength > 0.0f && stickLength < 0.7f ||
		   right_ring_mode == RingMode::outer && stickLength > 0.7f;
	jc->handleButtonChange(MAPPING_RRING, ring);

	rotateOnly = right_stick_mode == StickMode::rotateOnly;
	flickOnly = right_stick_mode == StickMode::flickOnly;
	if (right_stick_mode == StickMode::flick || rotateOnly || flickOnly) {
		camSpeedX += handleFlickStick(calX, calY, lastCalX, lastCalY, stickLength, jc->is_flicking_right, jc, mouseCalibrationFactor, flickOnly, rotateOnly);
		rightAny = rightPegged;
	}
	else if (right_stick_mode == StickMode::aim) {
		// camera movement
		if (!rightPegged) {
			jc->right_acceleration = 1.0f; // reset
		}
		float stickLength = sqrt(calX * calX + calY * calY);
		if (stickLength > 0.0f) {
			rightAny = true;
			float warpedStickLength = pow(stickLength, stick_power);
			warpedStickLength *= stick_sens * real_world_calibration / os_mouse_speed / in_game_sens;
			camSpeedX += calX / stickLength * warpedStickLength * jc->right_acceleration * deltaTime;
			camSpeedY += calY / stickLength * warpedStickLength * jc->right_acceleration * deltaTime;
			if (rightPegged) {
				jc->right_acceleration += stick_acceleration_rate * deltaTime;
				if (jc->right_acceleration > stick_acceleration_cap) {
					jc->right_acceleration = stick_acceleration_cap;
				}
			}
		}
	}
	else if (right_stick_mode == StickMode::mouseRing) {
		if (calX != 0.0f || calY != 0.0f) {
			float stickLength = sqrt(calX * calX + calY * calY);
			float normX = calX / stickLength;
			float normY = calY / stickLength;
			// use screen resolution
			float mouseX = (float)screen_resolution_x * 0.5f + 0.5f + normX * mouse_ring_radius;
			float mouseY = (float)screen_resolution_y * 0.5f + 0.5f - normY * mouse_ring_radius;
			// normalize
			mouseX = mouseX / screen_resolution_x;
			mouseY = mouseY / screen_resolution_y;
			// do it!
			setMouseNorm(mouseX, mouseY);
			lockMouse = true;
		}
	}
	else {
		// left!
		jc->handleButtonChange(MAPPING_RLEFT, left);
		// right!
		jc->handleButtonChange(MAPPING_RRIGHT, right);
		// up!
		jc->handleButtonChange(MAPPING_RUP, up);
		// down!
		jc->handleButtonChange(MAPPING_RDOWN, down);

		rightAny = left | right | up | down; // ring doesn't count
	}

	// button mappings
	jc->handleButtonChange(MAPPING_UP, (state.buttons & JSMASK_UP) > 0);
	jc->handleButtonChange(MAPPING_DOWN, (state.buttons & JSMASK_DOWN) > 0);
	jc->handleButtonChange(MAPPING_LEFT, (state.buttons & JSMASK_LEFT) > 0);
	jc->handleButtonChange(MAPPING_RIGHT, (state.buttons & JSMASK_RIGHT) > 0);
	jc->handleButtonChange(MAPPING_L, (state.buttons & JSMASK_L) > 0);
	jc->handleTriggerChange(MAPPING_ZL, MAPPING_ZLF, zlMode, state.lTrigger);
	jc->handleButtonChange(MAPPING_MINUS, (state.buttons & JSMASK_MINUS) > 0);
	jc->handleButtonChange(MAPPING_CAPTURE, (state.buttons & JSMASK_CAPTURE) > 0);
	jc->handleButtonChange(MAPPING_E, (state.buttons & JSMASK_E) > 0);
	jc->handleButtonChange(MAPPING_S, (state.buttons & JSMASK_S) > 0);
	jc->handleButtonChange(MAPPING_N, (state.buttons & JSMASK_N) > 0);
	jc->handleButtonChange(MAPPING_W, (state.buttons & JSMASK_W) > 0);
	jc->handleButtonChange(MAPPING_R, (state.buttons & JSMASK_R) > 0);
	jc->handleTriggerChange(MAPPING_ZR, MAPPING_ZRF, zrMode, state.rTrigger);
	jc->handleButtonChange(MAPPING_PLUS, (state.buttons & JSMASK_PLUS) > 0);
	jc->handleButtonChange(MAPPING_HOME, (state.buttons & JSMASK_HOME) > 0);
	jc->handleButtonChange(MAPPING_SL, (state.buttons & JSMASK_SL) > 0);
	jc->handleButtonChange(MAPPING_SR, (state.buttons & JSMASK_SR) > 0);
	jc->handleButtonChange(MAPPING_L3, (state.buttons & JSMASK_LCLICK) > 0);
	jc->handleButtonChange(MAPPING_R3, (state.buttons & JSMASK_RCLICK) > 0);

	// Handle buttons before GYRO because some of them may affect the value of blockGyro
	switch (gyro_ignore_mode) {
	case GyroIgnoreMode::button:
		blockGyro = gyro_always_off ^ IsPressed(state, gyro_button); // Use jc->IsActive(gyro_button) to consider button state
		break;
	case GyroIgnoreMode::left:
		blockGyro = (gyro_always_off ^ leftAny);
		break;
	case GyroIgnoreMode::right:
		blockGyro = (gyro_always_off ^ rightAny);
		break;
	}
	float gyro_x_sign_to_use = gyro_x_sign;
	float gyro_y_sign_to_use = gyro_y_sign;

	// Apply gyro modifiers in the queue from oldest to newest (thus giving priority to most recent)
	for (auto pair : jc->btnCommon.gyroActionQueue)
	{
		// TODO: logic optimization
		if (pair.second == GYRO_ON_BIND) blockGyro = false;
		else if (pair.second == GYRO_OFF_BIND) blockGyro = true;
		else if (pair.second == GYRO_INV_X) gyro_x_sign_to_use *= -1; // Intentionally don't support multiple inversions
		else if (pair.second == GYRO_INV_Y) gyro_y_sign_to_use *= -1;
		else if (pair.second == GYRO_INVERT)
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
		(jc->controller_type & (int)joycon_gyro_mask) == 0))
	{
		//printf("GX: %0.4f GY: %0.4f GZ: %0.4f\n", imuState.gyroX, imuState.gyroY, imuState.gyroZ);
		float mouseCalibration = real_world_calibration / os_mouse_speed / in_game_sens;
		shapedSensitivityMoveMouse(gyroX * gyro_x_sign_to_use, gyroY * gyro_y_sign_to_use, { min_gyro_sens_x, min_gyro_sens_y }, { max_gyro_sens_x, max_gyro_sens_y },
			min_gyro_threshold, max_gyro_threshold, deltaTime, camSpeedX * aim_x_sign, -camSpeedY * aim_y_sign, mouseCalibration);
	}
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

	std::string msg;
	if (numConnected == 1) {
		msg = "1 device connected\n";
	}
	else {
		std::stringstream ss;
		ss << numConnected << " devices connected" << std::endl;
		msg = ss.str();
	}
	printf(msg.c_str());
	//if (!IsVisible())
	//{
	//	tray->SendToast(std::wstring(msg.begin(), msg.end()));
	//}

	//if (numConnected != 0) {
	//	printf("All devices have started continuous gyro calibration\n");
	//}

	delete[] deviceHandles;
}

// https://stackoverflow.com/a/25311622/1130520 says this is why filenames obtained by fgets don't work
static void removeNewLine(char* string) {
	char *p;
	if ((p = strchr(string, '\n')) != NULL) {
		*p = '\0'; /* remove newline */
	}
}

bool AutoLoadPoll(void *param)
{
	static std::string lastModuleName;
	std::string windowTitle, windowModule;
	std::tie(windowModule, windowTitle) = GetActiveWindowName();
	if (!windowModule.empty() && windowModule != lastModuleName && windowModule.compare("JoyShockMapper.exe") != 0)
	{
		lastModuleName = windowModule;
		std::string cwd(GetCWD());
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
					parseCommand(cwd + file);
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
		tray->AddMenuItem(L"Show Console", &ShowConsole);
		tray->AddMenuItem(L"Reconnect controllers", []()
		{
			WriteToConsole("RECONNECT_CONTROLLERS");
		});
		tray->AddMenuItem(L"AutoLoad", [](bool isChecked)
		{
			isChecked ?
				autoLoadThread->Start() :
				autoLoadThread->Stop();
		}, std::bind(&PollingThread::isRunning, autoLoadThread.get()));
		if (Whitelister::IsHIDCerberusRunning())
		{
			tray->AddMenuItem(L"Whitelist", [](bool isChecked)
			{
				isChecked ?
					whitelister.Add() :
					whitelister.Remove();
			}, std::bind(&Whitelister::operator bool, &whitelister));
		}
		tray->AddMenuItem(L"Calibrate all devices", [](bool isChecked)
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
			std::string fullPathName = autoloadFolder + file;
			auto noext = file.substr(0, file.find_last_of('.'));
			tray->AddMenuItem(L"AutoLoad folder", std::wstring(noext.begin(), noext.end()), [fullPathName]
			{
				WriteToConsole(std::string(fullPathName.begin(), fullPathName.end()));
				autoLoadThread->Stop();
			});
		}
		std::string gyroConfigsFolder = "GyroConfigs\\";
		for (auto file : ListDirectory(gyroConfigsFolder.c_str()))
		{
			std::string fullPathName = gyroConfigsFolder + file;
			auto noext = file.substr(0, file.find_last_of('.'));
			tray->AddMenuItem(L"GyroConfigs folder", std::wstring(noext.begin(), noext.end()), [fullPathName]
			{
				WriteToConsole(std::string(fullPathName.begin(), fullPathName.end()));
				autoLoadThread->Stop();
			});
		}
		tray->AddMenuItem(L"Calculate RWC", []()
		{
			WriteToConsole("CALCULATE_REAL_WORLD_CALIBRATION");
			ShowConsole();
		});
		tray->AddMenuItem(L"Quit", []()
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
	FreeConsole();
	whitelister.Remove();
}


//int main(int argc, char *argv[]) {
int wWinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPWSTR cmdLine, int cmdShow) {
	tray.reset(new TrayIcon(hInstance, prevInstance, cmdLine, cmdShow, &beforeShowTrayMenu));
	// console
	initConsole(&CleanUp);
	printf("Welcome to JoyShockMapper version %s!\n", version);
	//if (whitelister) printf("JoyShockMapper was successfully whitelisted!\n");
	// prepare for input
	resetAllMappings();
	connectDevices();
	JslSetCallback(&joyShockPollCallback);
	autoLoadThread.reset(new PollingThread(&AutoLoadPoll, nullptr, 1000, true)); // Start by default
	if (autoLoadThread && *autoLoadThread) printf("AutoLoad is enabled. Configurations in \"AutoLoad\" folder will get loaded when matching application is in focus.\n");
	else printf("[AUTOLOAD] AutoLoad is unavailable\n");
	tray->Show();
	// poll joycons:
	while (true) {
		fgets(tempConfigName, 128, stdin);
		removeNewLine(tempConfigName);
		if (strcmp(tempConfigName, "QUIT") == 0) {
			break;
		}
		else {
			loading_lock.lock();
			parseCommand(tempConfigName);
			loading_lock.unlock();
		}
		//pollLoop();
	}
	// Exit JSM
	CleanUp();
	return 0;
}
