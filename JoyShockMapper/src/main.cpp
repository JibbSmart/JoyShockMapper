
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

#include "JoyShockLibrary.h"
#include "Whitelister.h"
#include "inputHelpers.cpp"
#include "TrayIcon.h"

#pragma warning(disable:4996)

// versions will be in the format A.B.C
// C increases when all that's happened is some bugs have been fixed.
// B increases and C resets to 0 when new features have been added.
// A increases and B and C reset to 0 when major new features have been added that warrant a new major version, or replacing older features with better ones that require the user to interact with them differently
const char* version = "1.4.1";

#define PI 3.14159265359f

#define MAPPING_ERROR -2 // Represents an error in user input
#define MAPPING_NONE -1 // Represents no button when explicitely stated by the user. Not to be confused with NO_HOLD_MAPPED which is no action bound.
#define MAPPING_UP 0
#define MAPPING_DOWN 1
#define MAPPING_LEFT 2
#define MAPPING_RIGHT 3
#define MAPPING_L 4
#define MAPPING_ZL 5
#define MAPPING_MINUS 6
#define MAPPING_CAPTURE 7
#define MAPPING_E 8
#define MAPPING_S 9
#define MAPPING_N 10
#define MAPPING_W 11
#define MAPPING_R 12
#define MAPPING_ZR 13
#define MAPPING_PLUS 14
#define MAPPING_HOME 15
#define MAPPING_SL 16
#define MAPPING_SR 17
#define MAPPING_L3 18
#define MAPPING_R3 19
#define MAPPING_LUP 20
#define MAPPING_LDOWN 21
#define MAPPING_LLEFT 22
#define MAPPING_LRIGHT 23
#define MAPPING_LRING 24
#define MAPPING_RUP 25
#define MAPPING_RDOWN 26
#define MAPPING_RLEFT 27
#define MAPPING_RRIGHT 28
#define MAPPING_RRING 29
#define MAPPING_ZLF 30 // FIRST
// insert more analog triggers here
#define MAPPING_ZRF 31 // LAST
#define MAPPING_SIZE 32

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

#define MAGIC_DST_DELAY 150.0f // in milliseconds
#define MAGIC_TAP_DURATION 40.0f // in milliseconds
#define MAGIC_GYRO_TAP_DURATION 500.0f // in milliseconds
#define MAGIC_HOLD_TIME 150.0f // in milliseconds
#define MAGIC_SIM_DELAY 50.0f // in milliseconds
static_assert(MAGIC_SIM_DELAY < MAGIC_HOLD_TIME, "Simultaneous press delay has to be smaller than hold delay!");

enum class StickMode { none, aim, flick, innerRing, outerRing, invalid };
enum       AxisMode { standard=1, inverted=-1, invalid=0 }; // valid values are true!
enum class TriggerMode { noFull, noSkip, maySkip, mustSkip, maySkipResp, mustSkipResp, invalid };
enum class GyroAxisMask { none = 0, x = 1, y = 2, z = 4, invalid = 8 };
enum class JoyconMask { useBoth = 0, ignoreLeft = 1, ignoreRight = 2, ignoreBoth = 3, invalid = 4 };
enum class GyroIgnoreMode { button, left, right };
enum class DstState { NoPress = 0, PressStart, QuickSoftTap, QuickFullPress, QuickFullRelease, SoftPress, DelayFullPress, PressStartResp, invalid };
enum class BtnState { NoPress = 0, BtnPress, WaitSim, WaitHold, SimPress, HoldPress, WaitSimHold, SimHold, SimRelease, SimTapRelease, TapRelease, invalid};

// Mapping for a press combination, whether chorded or simultaneous
struct ComboMap
{
	std::string name;
	int btn;
	WORD pressBind = 0;
	WORD holdBind = 0;
};

std::mutex loading_lock;

StickMode left_stick_mode = StickMode::none;
StickMode right_stick_mode = StickMode::none;
GyroAxisMask mouse_x_from_gyro = GyroAxisMask::y;
GyroAxisMask mouse_y_from_gyro = GyroAxisMask::x;
JoyconMask joycon_gyro_mask = JoyconMask::ignoreLeft;
GyroIgnoreMode gyro_ignore_mode = GyroIgnoreMode::button; // Ignore mode none means no GYRO_OFF button
TriggerMode zlMode = TriggerMode::noFull;
TriggerMode zrMode = TriggerMode::noFull;
WORD mappings[MAPPING_SIZE];
WORD hold_mappings[MAPPING_SIZE];
std::unordered_map<int, std::vector<ComboMap>> sim_mappings;
std::unordered_map<int, std::vector<ComboMap>> chord_mappings; // Binds a chord button to one or many remappings
float min_gyro_sens = 0.0;
float max_gyro_sens = 0.0;
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
std::unique_ptr<PollingThread> autoLoadThread;
std::unique_ptr<PollingThread> consoleMonitor;
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
	float _weightsRemaining[16];
	float _flickSamples[16];
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
	{

	}

	const int MaxGyroSamples = 64;
	const int NumSamples = 16;
	int intHandle;

	std::chrono::steady_clock::time_point press_times[MAPPING_SIZE];
	BtnState btnState[MAPPING_SIZE];
	WORD keyToRelease[MAPPING_SIZE]; // At key press, remember what to release
	std::deque<int> chordStack; // Represents the remapping layers active. Each item needs to have an entry in chord_mappings.
	std::deque<std::pair<int, WORD>> gyroActionQueue; // Queue of gyro control actions currently in effect
	std::chrono::steady_clock::time_point started_flick;
	std::chrono::steady_clock::time_point time_now;
	// tap_release_queue has been replaced with button states *TapRelease. The hold time of the tap is effectively quantized to the polling period of the device.
	bool is_flicking_left = false;
	bool is_flicking_right = false;
	float delta_flick = 0.0;
	float flick_percent_done = 0.0;
	float flick_rotation_counter = 0.0;
	bool toggleContinuous = false;

	float poll_rate;
	int controller_type = 0;
	float stick_step_size;

	float left_acceleration = 1.0;
	float right_acceleration = 1.0;
	std::vector<DstState> triggerState; // State of analog triggers when skip mode is active

	WORD GetPressMapping(int index)
	{
		// Look at active chord mappings starting with the latest activates chord
		for (auto activeChord = chordStack.begin(); activeChord != chordStack.end(); activeChord++)
		{
			for (auto chordPress : chord_mappings[*activeChord])
			{
				if (chordPress.btn == index)
					return chordPress.pressBind;
			}
		}
		return mappings[index];
	}

	WORD GetHoldMapping(int index)
	{
		// Look at active chord mappings starting with the latest activates chord
		for (auto activeChord = chordStack.begin(); activeChord != chordStack.end(); activeChord++)
		{
			for (auto chordPress : chord_mappings[*activeChord])
			{
				if (chordPress.btn == index)
					return chordPress.holdBind;
			}
		}
		return hold_mappings[index];
	}

	void ApplyBtnPress(int index, bool tap = false)
	{
		auto key = GetPressMapping(index);
		if (key == CALIBRATE)
		{
			toggleContinuous ^= tap; //Toggle on tap
			if (!tap || toggleContinuous) {
				printf("Starting continuous calibration\n");
				JslResetContinuousCalibration(intHandle);
				JslStartContinuousCalibration(intHandle);
			}
		}
		else if (key == GYRO_INV_X || key == GYRO_INV_Y || key == GYRO_INVERT || key == GYRO_ON_BIND || key == GYRO_OFF_BIND)
		{
			gyroActionQueue.push_back({ index, key });
		}
		else
		{
			pressKey(key, true);
		}
		keyToRelease[index] = key;
		if (chord_mappings.find(index) != chord_mappings.cend())
		{
			chordStack.push_front(index); // Always push at the fromt to make it a stack
		}
	}

	void ApplyBtnHold(int index)
	{
		auto key = GetHoldMapping(index);
		if (key == CALIBRATE)
		{
			printf("Starting continuous calibration\n");
			JslResetContinuousCalibration(intHandle);
			JslStartContinuousCalibration(intHandle);
		}
		else if (key == GYRO_INV_X || key == GYRO_INV_Y || key == GYRO_INVERT || key == GYRO_ON_BIND || key == GYRO_OFF_BIND)
		{
			gyroActionQueue.push_back({ index, key });
		}
		else if (key != NO_HOLD_MAPPED)
		{
			pressKey(key, true);
		}
		keyToRelease[index] = key;
		if (chord_mappings.find(index) != chord_mappings.cend())
		{
			chordStack.push_front(index); // Always push at the front
		}
	}

	void ApplyBtnRelease(int index, bool tap = false)
	{
		if (keyToRelease[index] == CALIBRATE)
		{
			if (!tap || !toggleContinuous) 
			{
				JslPauseContinuousCalibration(intHandle);
				toggleContinuous = false; // if we've held the calibration button, we're disabling continuous calibration
				printf("Gyro calibration set\n");
			}
		}
		else if (keyToRelease[index] >= GYRO_INV_X && keyToRelease[index] <= GYRO_ON_BIND)
		{
			gyroActionQueue.erase(std::find_if(gyroActionQueue.begin(), gyroActionQueue.end(), 
				[index](auto pair)
				{
					return pair.first == index;
				}));
		}
		else if (keyToRelease[index] != NO_HOLD_MAPPED)
		{
			pressKey(keyToRelease[index], false);
		}
		auto foundChord = std::find(chordStack.begin(), chordStack.end(), index);
		if (foundChord != chordStack.end())
		{
			// The chord is released
			chordStack.erase(foundChord);
		}
	}

	void ApplyBtnPress(const ComboMap &simPress, int index, bool tap = false)
	{
		if (simPress.pressBind == CALIBRATE)
		{
			toggleContinuous ^= tap; //Toggle on tap
			if (!tap || toggleContinuous) {
				printf("Starting continuous calibration\n");
				JslResetContinuousCalibration(intHandle);
				JslStartContinuousCalibration(intHandle);
			}
		}
		else if (simPress.pressBind == GYRO_INV_X || simPress.pressBind == GYRO_INV_Y || simPress.pressBind == GYRO_INVERT || simPress.pressBind == GYRO_ON_BIND || simPress.pressBind == GYRO_OFF_BIND)
		{
			// I know I don't handle multiple inversion. Otherwise GYRO_INV_X on sim press would do nothing
			gyroActionQueue.push_back({ index, simPress.pressBind });
			gyroActionQueue.push_back({ simPress.btn, simPress.pressBind });
		}
		else
		{
			pressKey(simPress.pressBind, true);
		}
		keyToRelease[simPress.btn] = simPress.pressBind;
		keyToRelease[index] = simPress.pressBind;
		// Combo presses don't enable chords
	}

	void ApplyBtnHold(const ComboMap &simPress, int index)
	{
		if (simPress.holdBind == CALIBRATE)
		{
			printf("Starting continuous calibration\n");
			JslResetContinuousCalibration(intHandle);
			JslStartContinuousCalibration(intHandle);
		}
		else if (simPress.holdBind == GYRO_INV_X || simPress.holdBind == GYRO_ON_BIND || simPress.holdBind == GYRO_OFF_BIND)
		{
			// I know I don't handle multiple inversion. Otherwise GYRO_INV_X on sim press would do nothing
			gyroActionQueue.push_back({ index, simPress.holdBind });
			gyroActionQueue.push_back({ simPress.btn, simPress.holdBind });
		}
		else if (simPress.holdBind != NO_HOLD_MAPPED)
		{
			pressKey(simPress.holdBind, true);
		}
		keyToRelease[simPress.btn] = simPress.holdBind;
		keyToRelease[index] = simPress.holdBind;
		// Combo presses don't enable chords
	}

	void ApplyBtnRelease(const ComboMap &simPress, int index, bool tap = false)
	{
		if (keyToRelease[index] == CALIBRATE)
		{
			if (!tap || !toggleContinuous)
			{
				JslPauseContinuousCalibration(intHandle);
				toggleContinuous = false; // if we've held the calibration button, we're disabling continuous calibration
				printf("Gyro calibration set\n");
			}
		}
		else if (keyToRelease[index] >= GYRO_INV_X && keyToRelease[index] <= GYRO_ON_BIND)
		{
			gyroActionQueue.erase(std::find_if(gyroActionQueue.begin(), gyroActionQueue.end(),
				[index](auto pair)
				{
					return pair.first == index;
				}));
			gyroActionQueue.erase(std::find_if(gyroActionQueue.begin(), gyroActionQueue.end(),
				[simPress](auto pair)
				{
					return pair.first == simPress.btn;
				}));
		}
		else if (keyToRelease[index] != NO_HOLD_MAPPED)
		{
			pressKey(keyToRelease[index], false);
		}
		auto foundChord = std::find(chordStack.begin(), chordStack.end(), index);
		if (foundChord != chordStack.end())
		{
			// The chord is released
			chordStack.erase(foundChord);
		}
	}

	// Pretty wrapper
	inline float GetPressDurationMS(int index)
	{
		return static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(time_now - press_times[index]).count());
	}

	// Indicate if the button is currently sending an assigned mapping.
	bool IsActive(int mappingIndex)
	{
		if (mappingIndex >= 0 && mappingIndex < MAPPING_SIZE)
		{
			auto state = btnState[mappingIndex];
			return state == BtnState::BtnPress || state == BtnState::HoldPress; // Add Sim Press State? Only with Setting?
		}
		return false;
	}

	inline const ComboMap* GetMatchingSimMap(int index)
	{
		// Find the simMapping where the other btn is in the same state as this btn.
		// POTENTIAL FLAW: The mapping you find may not necessarily be the one that got you in a 
		// Simultaneous state in the first place if there is a second SimPress going on where one
		// of the buttons has a third SimMap with this one. I don't know if it's worth solving though...
		if (sim_mappings.find(index) != sim_mappings.cend())
		{
			auto match = std::find_if(sim_mappings[index].cbegin(), sim_mappings[index].cend(),
				[this, index](const auto& simMap)
				{
					return btnState[simMap.btn] == btnState[index];
				});
			return match == sim_mappings[index].cend() ? nullptr : &*match;
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
			immediateFactor = 0.0f;
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
	if (name.compare("INNER_RING") == 0) {
		if (print) printf("Inner Ring");
		return StickMode::innerRing;
	}
	if (name.compare("OUTER_RING") == 0) {
		if (print) printf("Outer Ring");
		return StickMode::outerRing;
	}
	if (print) printf("\"%s\" invalid", name.c_str());
	return StickMode::invalid;
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
	memset(mappings, 0, sizeof(mappings));
	memset(hold_mappings, 0, sizeof(hold_mappings));
	mappings[MAPPING_HOME] = mappings[MAPPING_CAPTURE] = hold_mappings[MAPPING_HOME] = hold_mappings[MAPPING_CAPTURE] = CALIBRATE;
	min_gyro_sens = 0.0f;
	max_gyro_sens = 0.0f;
	min_gyro_threshold = 0.0f;
	max_gyro_threshold = 0.0f;
	stick_power = 1.0f;
	stick_sens = 360.0f;
	real_world_calibration = 40.0f;
	in_game_sens = 1.0f;
	left_stick_mode = StickMode::none;
	right_stick_mode = StickMode::none;
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
		int index = keyToMappingIndex(std::string(key));
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
					try {
						min_gyro_sens = std::stof(value);
						printf("Min gyro sensitivity set to %sx\n", value);
					}
					catch (std::invalid_argument ia) {
						printf("Can't convert \"%s\" to a number\n", value);
					}
					return;
				case MAX_GYRO_SENS:
					try {
						max_gyro_sens = std::stof(value);
						printf("Max gyro sensitivity set to %sx\n", value);
					}
					catch (std::invalid_argument ia) {
						printf("Can't convert \"%s\" to a number\n", value);
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
						printf("Stick power set to %s\n", value);
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
						right_stick_mode = temp;
					}
					else {
						printf("Valid settings for RIGHT_STICK_MODE are NO_MOUSE, AIM, FLICK, INNER_RING or OUTER_RING\n");
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
				{
					try {
						float sens = std::stof(value);
						min_gyro_sens = max_gyro_sens = sens;
						printf("Gyro sensitivity set to %sx\n", value);
					}
					catch (std::invalid_argument ia) {
						printf("Can't convert \"%s\" to a number\n", value);
					}
					return;
				}
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
				int index2 = keyToMappingIndex(keystr.substr(keystr.find_first_of('+') + 1, keystr.size()));
				if (index >= 0 && index < MAPPING_SIZE && index2 >= 0 && index2 < MAPPING_SIZE)
				{
					if (sim_mappings.find(index) != sim_mappings.end())
					{
						auto existingBinding = std::find_if(sim_mappings[index].begin(), sim_mappings[index].end(),
							[index2] (const auto &mapping) {
							return mapping.btn == index2;
						});
						if (existingBinding != sim_mappings[index].end())
						{
							// Remove any existing mapping
							sim_mappings[index].erase(existingBinding);
							// Is it fair to assume the opposing pair exists?
							existingBinding = std::find_if(sim_mappings[index2].begin(), sim_mappings[index2].end(),
								[index] (const auto &mapping) {
								return mapping.btn == index;
							});
							sim_mappings[index2].erase(existingBinding);
							if (sim_mappings[index2].size() == 0)
							{
								sim_mappings.erase(sim_mappings.find(index));
							}
						}
					}
					sim_mappings[index].push_back({ keystr, index2 });
					simMap = &sim_mappings[index].back();
				}
				else
				{
					// Try parsing a chorded press
					index = keyToMappingIndex(keystr.substr(0, keystr.find_first_of(',')));
					int index2 = keyToMappingIndex(keystr.substr(keystr.find_first_of(',') + 1, keystr.size()));
					if (index >= 0 && index < MAPPING_SIZE && index2 >= 0 && index2 < MAPPING_SIZE)
					{
						auto existingBinding = std::find_if(chord_mappings[index].cbegin(), chord_mappings[index].cend(),
							[index2](const auto& chordMap) {
								return chordMap.btn == index2;
							});
						if (existingBinding != chord_mappings[index].end())
						{
							// Remove any existing mapping
							chord_mappings[index].erase(existingBinding);
							if (chord_mappings[index2].size() == 0)
							{
								chord_mappings.erase(chord_mappings.find(index));
							}
						}
						chord_mappings[index].push_back( { keystr, index2 } );
						chordMap = &chord_mappings[index].back(); // point to vector item
					}
					else
					{
						printf("Warning: Input %s not recognized\n", key);
						return;
					}
				}
			}
			// we have a valid single button input, so clear old hold_mapping for this input
			if(index >= 0 && !simMap && !chordMap) hold_mappings[index] = 0;
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
							hold_mappings[index] = holdOutput;
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
				mappings[index] = output;
			if(simMap)
			{
				// Sim Press commands are added as twins
				sim_mappings[simMap->btn].push_back( {simMap->name, index, simMap->pressBind, simMap->holdBind} );
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

void handleButtonChange(int index, bool pressed, const char* name, JoyShock* jc) {
	// for tap duration, we need to know if the key in question is a gyro-related mapping or not
	WORD mapping = mappings[index];
	float magicTapDuration = (mapping >= GYRO_INV_X && mapping <= GYRO_ON_BIND) ? MAGIC_GYRO_TAP_DURATION : MAGIC_TAP_DURATION;
	switch (jc->btnState[index])
	{
	case BtnState::NoPress:
		if (pressed)
		{
			if (sim_mappings.find(index) != sim_mappings.end() && sim_mappings[index].size() > 0)
			{
				jc->btnState[index] = BtnState::WaitSim;
				jc->press_times[index] = jc->time_now;
			}
			else if (jc->GetHoldMapping(index))
			{
				jc->btnState[index] = BtnState::WaitHold;
				jc->press_times[index] = jc->time_now;
			}
			else
			{
				jc->btnState[index] = BtnState::BtnPress;
				jc->ApplyBtnPress(index);
				printf("%s: true\n", name);
			}
		}
		break;
	case BtnState::BtnPress:
		if (!pressed)
		{
			jc->btnState[index] = BtnState::NoPress;
			jc->ApplyBtnRelease(index);
			printf("%s: false\n", name);
		}
		break;
	case BtnState::WaitSim:
		if (!pressed)
		{
			jc->btnState[index] = BtnState::TapRelease;
			jc->press_times[index] = jc->time_now;
			jc->ApplyBtnPress(index, true);
			printf("%s: tapped\n", name);
		}
		else
		{
			// Is there a sim mapping on this button where the other button is in WaitSim state too?
			auto simMap = jc->GetMatchingSimMap(index);
			if (simMap)
			{
				// We have a simultaneous press!
				if (simMap->holdBind)
				{
					jc->btnState[index] = BtnState::WaitSimHold;
					jc->btnState[simMap->btn] = BtnState::WaitSimHold;
					jc->press_times[index] = jc->time_now; // Reset Timer
				}
				else
				{
					jc->btnState[index] = BtnState::SimPress;
					jc->btnState[simMap->btn] = BtnState::SimPress;
					jc->ApplyBtnPress(*simMap, index);
					printf("%s: true\n", simMap->name.c_str());
				}
			}
			else if (jc->GetPressDurationMS(index) > MAGIC_SIM_DELAY)
			{
				// Sim delay expired!
				if (jc->GetHoldMapping(index))
				{
					jc->btnState[index] = BtnState::WaitHold;
					// Don't reset time
				}
				else
				{
					jc->btnState[index] = BtnState::BtnPress;
					jc->ApplyBtnPress(index);
					printf("%s: true\n", name);
				}
			}
			// Else let time flow, stay in this state, no output.
		}
		break;
	case BtnState::WaitHold:
		if (!pressed)
		{
			jc->btnState[index] = BtnState::TapRelease;
			jc->press_times[index] = jc->time_now;
			jc->ApplyBtnPress(index, true);
			printf("%s: tapped\n", name);
		}
		else if (jc->GetPressDurationMS(index) > MAGIC_HOLD_TIME)
		{
			jc->btnState[index] = BtnState::HoldPress;
			jc->ApplyBtnHold(index);
			printf("%s: held\n", name);
		}
		// Else let time flow, stay in this state, no output.
		break;
	case BtnState::SimPress:
	{
		// Which is the sim mapping where the other button is in SimPress state too?
		auto simMap = jc->GetMatchingSimMap(index);
		if (!simMap)
		{
			// Should never happen but added for robustness.
			printf("Error: lost track of matching sim press for %s! Resetting to NoPress.\n", name);
			jc->btnState[index] = BtnState::NoPress;
		}
		else if (!pressed)
		{
			jc->btnState[index] = BtnState::SimRelease;
			jc->btnState[simMap->btn] = BtnState::SimRelease;
			jc->ApplyBtnRelease(*simMap, index);
			printf("%s: false\n", simMap->name.c_str());
		}
		// else sim press is being held, as far as this button is concerned.
		break;
	}
	case BtnState::HoldPress:
		if (!pressed)
		{
			jc->btnState[index] = BtnState::NoPress;
			jc->ApplyBtnRelease(index);
			printf("%s: hold released\n", name);
		}
		break;
	case BtnState::WaitSimHold:
	{
		// Which is the sim mapping where the other button is in WaitSimHold state too?
		auto simMap = jc->GetMatchingSimMap(index);
		if (!simMap)
		{
			// Should never happen but added for robustness.
			printf("Error: lost track of matching sim press for %s! Resetting to NoPress.\n", name);
			jc->btnState[index] = BtnState::NoPress;
		}
		else if (!pressed)
		{
			jc->btnState[index] = BtnState::SimTapRelease;
			jc->btnState[simMap->btn] = BtnState::SimTapRelease;
			jc->press_times[index] = jc->time_now;
			jc->press_times[simMap->btn] = jc->time_now;
			jc->ApplyBtnPress(*simMap, index, true);
			printf("%s: tapped\n", simMap->name.c_str());
		}
		else if (jc->GetPressDurationMS(index) > MAGIC_HOLD_TIME)
		{
			jc->btnState[index] = BtnState::SimHold;
			jc->btnState[simMap->btn] = BtnState::SimHold;
			jc->ApplyBtnHold(*simMap, index);
			printf("%s: held\n", simMap->name.c_str());
			// Else let time flow, stay in this state, no output.
		}
		break;
	}
	case BtnState::SimHold:
	{
		// Which is the sim mapping where the other button is in SimHold state too?
		auto simMap = jc->GetMatchingSimMap(index);
		if (!simMap)
		{
			// Should never happen but added for robustness.
			printf("Error: lost track of matching sim press for %s! Resetting to NoPress.\n", name);
			jc->btnState[index] = BtnState::NoPress;
		}
		else if (!pressed)
		{
			jc->btnState[index] = BtnState::SimRelease;
			jc->btnState[simMap->btn] = BtnState::SimRelease;
			jc->ApplyBtnRelease(*simMap, index);
			printf("%s: hold released\n", simMap->name.c_str());
		}
		break;
	}
	case BtnState::SimRelease:
		if (!pressed)
		{
			jc->btnState[index] = BtnState::NoPress;
		}
		break;
	case BtnState::SimTapRelease:
	{
		// Which is the sim mapping where the other button is in SimTapRelease state too?
		auto simMap = jc->GetMatchingSimMap(index);
		if (!simMap)
		{
			// Should never happen but added for robustness.
			printf("Error: lost track of matching sim press for %s! Resetting to NoPress.\n", name);
			jc->btnState[index] = BtnState::NoPress;
		}
		else if (pressed)
		{
			jc->ApplyBtnRelease(*simMap, index, true);
			jc->btnState[index] = BtnState::NoPress;
			jc->btnState[simMap->btn] = BtnState::SimRelease;
		}
		else if (jc->GetPressDurationMS(index) > magicTapDuration)
		{
			jc->ApplyBtnRelease(*simMap, index, true);
			jc->btnState[index] = BtnState::SimRelease;
			jc->btnState[simMap->btn] = BtnState::SimRelease;
		}
		break;
	}
	case BtnState::TapRelease:
		if (pressed || jc->GetPressDurationMS(index) > magicTapDuration)
		{
			jc->ApplyBtnRelease(index, true);
			jc->btnState[index] = BtnState::NoPress;
		}
		break;
	default:
		printf("Invalid button state %d: Resetting to NoPress\n", jc->btnState[index]);
		jc->btnState[index] = BtnState::NoPress;
		break;
		
	}
}

void handleTriggerChange(int softIndex, int fullIndex, TriggerMode mode, float pressed, char* softName, JoyShock* jc) {
	std::string fullName(softName);
	fullName.append("F");

	if (JslGetControllerType(jc->intHandle) != JS_TYPE_DS4)
	{
		// Override local variable because the controller has digital triggers. Effectively ignore Full Pull binding.
		mode = TriggerMode::noFull;
	}

	auto idxState = fullIndex - FIRST_ANALOG_TRIGGER; // Get analog trigger index
	if (idxState < 0 || idxState >= jc->triggerState.size())
	{
		printf("Error: Trigger %s does not exist in state map. Dual Stage Trigger not possible.\n", fullName.c_str());
		return;
	}

	// if either trigger is waiting to be tap released, give it a go
	if (jc->btnState[softIndex] == BtnState::TapRelease || jc->btnState[softIndex] == BtnState::SimTapRelease)
	{
		// keep triggering until the tap release is complete
		handleButtonChange(softIndex, false, softName, jc);
	}
	if (jc->btnState[fullIndex] == BtnState::TapRelease || jc->btnState[fullIndex] == BtnState::SimTapRelease)
	{
		// keep triggering until the tap release is complete
		handleButtonChange(fullIndex, false, fullName.c_str(), jc);
	}

	switch (jc->triggerState[idxState])
	{
	case DstState::NoPress:
		// It actually doesn't matter what the last Press is. Theoretically, we could have missed the edge.
		if (pressed > trigger_threshold)
		{
			if (mode == TriggerMode::maySkip || mode == TriggerMode::mustSkip)
			{
				// Start counting press time to see if soft binding should be skipped
				jc->triggerState[idxState] = DstState::PressStart;
				jc->press_times[softIndex] = jc->time_now;
			}
			else if (mode == TriggerMode::maySkipResp || mode == TriggerMode::mustSkipResp)
			{
				jc->triggerState[idxState] = DstState::PressStartResp;
				jc->press_times[softIndex] = jc->time_now;
				handleButtonChange(softIndex, true, softName, jc);
			}
			else // mode == NO_FULL or NO_SKIP
			{
				jc->triggerState[idxState] = DstState::SoftPress;
				handleButtonChange(softIndex, true, softName, jc);
			}
		}
		break;
	case DstState::PressStart:
		if (pressed <= trigger_threshold) {
			// Trigger has been quickly tapped on the soft press
			jc->triggerState[idxState] = DstState::QuickSoftTap;
			handleButtonChange(softIndex, true, softName, jc);
		}
		else if (pressed == 1.0)
		{
			// Trigger has been full pressed quickly
			jc->triggerState[idxState] = DstState::QuickFullPress;
			handleButtonChange(fullIndex, true, fullName.c_str(), jc);
		}
		else if (jc->GetPressDurationMS(softIndex) >= MAGIC_DST_DELAY) { // todo: get rid of magic number -- make this a user setting )
			jc->triggerState[idxState] = DstState::SoftPress;
			// Reset the time for hold soft press purposes.
			jc->press_times[softIndex] = jc->time_now;
			handleButtonChange(softIndex, true, softName, jc);
		}
		// Else, time passes as soft press is being held, waiting to see if the soft binding should be skipped
		break;
	case DstState::PressStartResp:
		if (pressed <= trigger_threshold) {
			// Soft press is being released
			jc->triggerState[idxState] = DstState::NoPress;
			handleButtonChange(softIndex, false, softName, jc);
		}
		else if (pressed == 1.0)
		{
			// Trigger has been full pressed quickly
			jc->triggerState[idxState] = DstState::QuickFullPress;
			handleButtonChange(softIndex, false, softName, jc); // Remove soft press
			handleButtonChange(fullIndex, true, fullName.c_str(), jc);
		}
		else
		{
			if (jc->GetPressDurationMS(softIndex) >= MAGIC_DST_DELAY) { // todo: get rid of magic number -- make this a user setting )
				jc->triggerState[idxState] = DstState::SoftPress;
			}
			handleButtonChange(softIndex, true, softName, jc);
		}
		break;
	case DstState::QuickSoftTap:
		// Soft trigger is already released. Send release now!
		jc->triggerState[idxState] = DstState::NoPress;
		handleButtonChange(softIndex, false, softName, jc);
		break;
	case DstState::QuickFullPress:
		if (pressed < 1.0f) {
			// Full press is being release
			jc->triggerState[idxState] = DstState::QuickFullRelease;
			handleButtonChange(fullIndex, false, fullName.c_str(), jc);
		}
		else {
			// Full press is being held
			handleButtonChange(fullIndex, true, fullName.c_str(), jc);
		}
		break;
	case DstState::QuickFullRelease:
		if (pressed <= trigger_threshold) {
			jc->triggerState[idxState] = DstState::NoPress;
		}
		else if (pressed == 1.0f)
		{
			// Trigger is being full pressed again
			jc->triggerState[idxState] = DstState::QuickFullPress;
			handleButtonChange(fullIndex, true, fullName.c_str(), jc);
		}
		// else wait for the the trigger to be fully released
		break;
	case DstState::SoftPress:
		if (pressed <= trigger_threshold) {
			// Soft press is being released
			jc->triggerState[idxState] = DstState::NoPress;
			handleButtonChange(softIndex, false, softName, jc);
		}
		else // Soft Press is being held
		{
			handleButtonChange(softIndex, true, softName, jc);

			if ((mode == TriggerMode::maySkip || mode == TriggerMode::noSkip || mode == TriggerMode::maySkipResp)
				&& pressed == 1.0)
			{
				// Full press is allowed in addition to soft press
				jc->triggerState[idxState] = DstState::DelayFullPress;
				handleButtonChange(fullIndex, true, fullName.c_str(), jc);
			}
			// else ignore full press on NO_FULL and MUST_SKIP
		}
		break;
	case DstState::DelayFullPress:
		if (pressed < 1.0)
		{
			// Full Press is being released
			jc->triggerState[idxState] = DstState::SoftPress;
			handleButtonChange(fullIndex, false, fullName.c_str(), jc);
		}
		else // Full press is being held
		{
			handleButtonChange(fullIndex, true, fullName.c_str(), jc);
		}
		// Soft press is always held regardless
		handleButtonChange(softIndex, true, fullName.c_str(), jc);
		break;
	default:
		// TODO: use magic enum to translate enum # to str
		printf("Error: Trigger %s has invalid state #%d. Reset to NoPress.\n", softName, jc->triggerState[idxState]);
		jc->triggerState[idxState] = DstState::NoPress;
		break;
	}
}

static float handleFlickStick(float calX, float calY, float lastCalX, float lastCalY, float stickLength, bool& isFlicking, JoyShock * jc, float mouseCalibrationFactor) {
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
			jc->started_flick = std::chrono::steady_clock::now();
			jc->delta_flick = stickAngle;
			jc->flick_percent_done = 0.0f;
			jc->ResetSmoothSample();
			jc->flick_rotation_counter = stickAngle; // track all rotation for this flick
			// TODO: All these printfs should be hidden behind a setting. User might not want them.
			printf("Flick: %.3f degrees\n", stickAngle * (180.0f / PI));
		}
		else {
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
			camSpeedX = jc->GetSmoothedStickRotation(flickSpeed, flickSpeedConstant * stepSize * 8.0f, flickSpeedConstant * stepSize * 16.0f, maxSmoothingSamples);
		}
	}
	else if (isFlicking) {
		// was a flick! how much was the flick and rotation?
		last_flick_and_rotation = abs(jc->flick_rotation_counter) / (2.0 * PI);
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
	bool left = calX < -0.2f;
	bool right = calX > 0.2f;
	bool down = calY < -0.2f;
	bool up = calY > 0.2f;
	float stickLength = sqrt(calX * calX + calY * calY);
	bool ring = left_stick_mode == StickMode::innerRing && stickLength < 0.7f ||
		left_stick_mode == StickMode::outerRing && stickLength > 0.7f;
	if (left_stick_mode == StickMode::flick) {
		camSpeedX += handleFlickStick(calX, calY, lastCalX, lastCalY, stickLength, jc->is_flicking_left, jc, mouseCalibrationFactor);
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
	else {
		// ring!
		handleButtonChange(MAPPING_LRING, ring, "LRING", jc);
		// left!
		handleButtonChange(MAPPING_LLEFT, left, "LLEFT", jc);
		// right!
		handleButtonChange(MAPPING_LRIGHT, right, "LRIGHT", jc);
		// up!
		handleButtonChange(MAPPING_LUP, up, "LUP", jc);
		// down!
		handleButtonChange(MAPPING_LDOWN, down, "LDOWN", jc);

		leftAny = left | right | up | down; // ring doesn't count
	}
	lastCalX = lastState.stickRX;
	lastCalY = lastState.stickRY;
	calX = state.stickRX;
	calY = state.stickRY;
	float rightPegged = processDeadZones(calX, calY);
	absX = abs(calX);
	absY = abs(calY);
	left = calX < -0.2f;
	right = calX > 0.2f;
	down = calY < -0.2f;
	up = calY > 0.2f;
	stickLength = sqrt(calX * calX + calY * calY);
	ring = right_stick_mode == StickMode::innerRing && stickLength < 0.7f ||
		   right_stick_mode == StickMode::outerRing && stickLength > 0.7f;
	if (right_stick_mode == StickMode::flick) {
		camSpeedX += handleFlickStick(calX, calY, lastCalX, lastCalY, stickLength, jc->is_flicking_right, jc, mouseCalibrationFactor);
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
	else {
		// ring!
		handleButtonChange(MAPPING_RRING, ring, "RRING", jc);
		// left!
		handleButtonChange(MAPPING_RLEFT, left, "RLEFT", jc);
		// right!
		handleButtonChange(MAPPING_RRIGHT, right, "RRIGHT", jc);
		// up!
		handleButtonChange(MAPPING_RUP, up, "RUP", jc);
		// down!
		handleButtonChange(MAPPING_RDOWN, down, "RDOWN", jc);

		rightAny = left | right | up | down; // ring doesn't count
	}

	// button mappings
	handleButtonChange(MAPPING_UP, (state.buttons & JSMASK_UP) > 0, "UP", jc);
	handleButtonChange(MAPPING_DOWN, (state.buttons & JSMASK_DOWN) > 0, "DOWN", jc);
	handleButtonChange(MAPPING_LEFT, (state.buttons & JSMASK_LEFT) > 0, "LEFT", jc);
	handleButtonChange(MAPPING_RIGHT, (state.buttons & JSMASK_RIGHT) > 0, "RIGHT", jc);
	handleButtonChange(MAPPING_L, (state.buttons & JSMASK_L) > 0, "L", jc);
	handleTriggerChange(MAPPING_ZL, MAPPING_ZLF, zlMode, state.lTrigger, "ZL", jc);
	handleButtonChange(MAPPING_MINUS, (state.buttons & JSMASK_MINUS) > 0, "MINUS", jc);
	handleButtonChange(MAPPING_CAPTURE, (state.buttons & JSMASK_CAPTURE) > 0, "CAPTURE", jc);
	handleButtonChange(MAPPING_E, (state.buttons & JSMASK_E) > 0, "E", jc);
	handleButtonChange(MAPPING_S, (state.buttons & JSMASK_S) > 0, "S", jc);
	handleButtonChange(MAPPING_N, (state.buttons & JSMASK_N) > 0, "N", jc);
	handleButtonChange(MAPPING_W, (state.buttons & JSMASK_W) > 0, "W", jc);
	handleButtonChange(MAPPING_R, (state.buttons & JSMASK_R) > 0, "R", jc);
	handleTriggerChange(MAPPING_ZR, MAPPING_ZRF, zrMode, state.rTrigger, "ZR", jc);
	handleButtonChange(MAPPING_PLUS, (state.buttons & JSMASK_PLUS) > 0, "PLUS", jc);
	handleButtonChange(MAPPING_HOME, (state.buttons & JSMASK_HOME) > 0, "HOME", jc);
	handleButtonChange(MAPPING_SL, (state.buttons & JSMASK_SL) > 0, "SL", jc);
	handleButtonChange(MAPPING_SR, (state.buttons & JSMASK_SR) > 0, "SR", jc);
	handleButtonChange(MAPPING_L3, (state.buttons & JSMASK_LCLICK) > 0, "L3", jc);
	handleButtonChange(MAPPING_R3, (state.buttons & JSMASK_RCLICK) > 0, "R3", jc);

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
	for (auto pair : jc->gyroActionQueue)
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
	if (jc->controller_type == JS_SPLIT_TYPE_FULL ||
		(jc->controller_type & (int)joycon_gyro_mask) == 0)
	{
		//printf("GX: %0.4f GY: %0.4f GZ: %0.4f\n", imuState.gyroX, imuState.gyroY, imuState.gyroZ);
		float mouseCalibration = real_world_calibration / os_mouse_speed / in_game_sens;
		shapedSensitivityMoveMouse(gyroX * gyro_x_sign_to_use, gyroY * gyro_y_sign_to_use, min_gyro_sens, max_gyro_sens, min_gyro_threshold, max_gyro_threshold, deltaTime, camSpeedX * aim_x_sign, -camSpeedY * aim_y_sign, mouseCalibration);
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

bool MonitorConsolePoll(void *param)
{
	static bool firstToast = true;
	if (isConsoleMinimized())
	{
		static_cast<TrayIcon*>(param)->Show();
		HideConsole();
		//if (firstToast)
		//{
		//	firstToast = !tray->SendToast(L"JoyShockMapper will keep running in the system tray.");
		//}
		return false;
	}
	return true;
}

void OnShowConsole()
{
	tray->Hide();
	ShowConsole();
	consoleMonitor->Start();
}

//int main(int argc, char *argv[]) {
int wWinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPWSTR cmdLine, int cmdShow) {
	tray.reset(new TrayIcon(hInstance, prevInstance, cmdLine, cmdShow));
	// console
	initConsole();
	printf("Welcome to JoyShockMapper version %s!\n", version);
	//if (whitelister) printf("JoyShockMapper was successfully whitelisted!\n");
	// prepare for input
	resetAllMappings();
	connectDevices();
	JslSetCallback(&joyShockPollCallback);
	autoLoadThread.reset(new PollingThread(&AutoLoadPoll, nullptr, 1000, true)); // Start by default
	consoleMonitor.reset(new PollingThread(&MonitorConsolePoll, tray.get(), 500, true));
	if (autoLoadThread && *autoLoadThread) printf("AutoLoad is enabled. Configurations in \"AutoLoad\" folder will get loaded when matching application is in focus.\n");
	else printf("[AUTOLOAD] AutoLoad is unavailable\n");

	if (!tray || !*tray) printf("ERROR: Cannot create tray item.\n");
	else
	{
		tray->AddMenuItem(L"Show Console", &OnShowConsole);
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
		tray->AddMenuItem(L"Calibrate all devices", [](bool isChecked)
			{
				isChecked ? 
					WriteToConsole("RESTART_GYRO_CALIBRATION") : 
					WriteToConsole("FINISH_GYRO_CALIBRATION");
			}, []() 
			{
				return devicesCalibrating; 
			});

		std::string cwd(GetCWD());
		std::string autoloadFolder = cwd + "\\AutoLoad\\";
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
		std::string gyroConfigsFolder = cwd + "\\GyroConfigs\\";
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
				OnShowConsole();
			});
		tray->AddMenuItem(L"Quit", [] ()
			{
				WriteToConsole("QUIT");
			});
	}

	// poll joycons:
	while (true) {
		fgets(tempConfigName, 128, stdin);
		removeNewLine(tempConfigName);
		if (strcmp(tempConfigName, "QUIT") == 0) {
			// Hide while cleanup and quit.
			HideConsole();
			break;
		}
		else {
			loading_lock.lock();
			parseCommand(tempConfigName);
			loading_lock.unlock();
		}
		//pollLoop();
	}
	JslDisconnectAndDisposeAll();
	FreeConsole();
	whitelister.Remove();
	return 0;
}
