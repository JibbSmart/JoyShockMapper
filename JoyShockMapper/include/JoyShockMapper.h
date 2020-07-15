#pragma once

#include <string>

// versions will be in the format A.B.C
// C increases when all that's happened is some bugs have been fixed.
// B increases and C resets to 0 when new features have been added.
// A increases and B and C reset to 0 when major new features have been added that warrant a new major version, or replacing older features with better ones that require the user to interact with them differently
static const char* version = "1.6.0";

using namespace std;

typedef const string &in_string; // input string parameters should be const references.

// Reused OS typedefs
typedef unsigned short      WORD;
typedef unsigned long       DWORD;

// https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
// Only use undefined keys from the above list for JSM custom commands
#define V_WHEEL_UP 0x03 // Here I intentionally overwride VK_CANCEL because breaking a process with a keybind is not something we want to do
#define V_WHEEL_DOWN 0x07 // I want all mouse bindings to be contiguous IDs for ease to distinguish
#define NO_HOLD_MAPPED 0x0A
#define CALIBRATE 0x0B
#define GYRO_INV_X 0x88
#define GYRO_INV_Y 0x89
#define GYRO_INVERT 0x8A
#define GYRO_OFF_BIND 0x8B // Not to be confused with settings GYRO_ON and GYRO_OFF
#define GYRO_ON_BIND 0x8C  // Those here are bindings

enum class ButtonID
{
	ERR = -2, // Represents an error in user input
	NONE = -1, // Represents no button when explicitely stated by the user. Not to be confused with NO_HOLD_MAPPED which is no action bound.
	UP = 0,
	DOWN = 1,
	LEFT = 2,
	RIGHT = 3,
	L = 4,
	ZL = 5,
	MINUS = 6,
	CAPTURE = 7,
	E = 8,
	S = 9,
	N = 10,
	W = 11,
	R = 12,
	ZR = 13,
	PLUS = 14,
	HOME = 15,
	SL = 16,
	SR = 17,
	L3 = 18,
	R3 = 19,
	LUP = 20,
	LDOWN = 21,
	LLEFT = 22,
	LRIGHT = 23,
	LRING = 24,
	RUP = 25,
	RDOWN = 26,
	RLEFT = 27,
	RRIGHT = 28,
	RRING = 29,
	ZLF = 30, // FIRST
	FIRST_ANALOG_TRIGGER = ZLF,
	// insert more analo,g triggers here
	ZRF = 31, // LAST
	LAST_ANALOG_TRIGGER = ZRF,
	SIZE = 32,
};

enum class SettingID
{
	ERR = -2, // Represents an error in user input
	MIN_GYRO_SENS = int(ButtonID::SIZE) + 1,
	MAX_GYRO_SENS = 34,
	MIN_GYRO_THRESHOLD = 35,
	MAX_GYRO_THRESHOLD = 36,
	STICK_POWER = 37,
	STICK_SENS = 38,
	REAL_WORLD_CALIBRATION = 39,
	IN_GAME_SENS = 40,
	TRIGGER_THRESHOLD = 41,
	RESET_MAPPINGS = 42,
	NO_GYRO_BUTTON = 43,
	LEFT_STICK_MODE = 44,
	RIGHT_STICK_MODE = 45,
	GYRO_OFF = 46,
	GYRO_ON = 47,
	STICK_AXIS_X = 48,
	STICK_AXIS_Y = 49,
	GYRO_AXIS_X = 50,
	GYRO_AXIS_Y = 51,
	RECONNECT_CONTROLLERS = 52,
	COUNTER_OS_MOUSE_SPEED = 53,
	IGNORE_OS_MOUSE_SPEED = 54,
	JOYCON_GYRO_MASK = 55,
	GYRO_SENS = 56,
	FLICK_TIME = 57,
	GYRO_SMOOTH_THRESHOLD = 58,
	GYRO_SMOOTH_TIME = 59,
	GYRO_CUTOFF_SPEED = 60,
	GYRO_CUTOFF_RECOVERY = 61,
	STICK_ACCELERATION_RATE = 62,
	STICK_ACCELERATION_CAP = 63,
	STICK_DEADZONE_INNER = 64,
	STICK_DEADZONE_OUTER = 65,
	CALCULATE_REAL_WORLD_CALIBRATION = 66,
	FINISH_GYRO_CALIBRATION = 67,
	RESTART_GYRO_CALIBRATION = 68,
	MOUSE_X_FROM_GYRO_AXIS = 69,
	MOUSE_Y_FROM_GYRO_AXIS = 70,
	ZR_DUAL_STAGE_MODE = 71,
	ZL_DUAL_STAGE_MODE = 72,
	AUTOLOAD = 73,
	HELP = 74,
	WHITELIST_SHOW = 75,
	WHITELIST_ADD = 76,
	WHITELIST_REMOVE = 77,
	LEFT_RING_MODE = 78,
	RIGHT_RING_MODE = 79,
	MOUSE_RING_RADIUS = 80,
	SCREEN_RESOLUTION_X = 81,
	SCREEN_RESOLUTION_Y = 82,
	ROTATE_SMOOTH_OVERRIDE = 83,
	FLICK_SNAP_MODE = 84,
	FLICK_SNAP_STRENGTH = 85,
};

// constexpr > #define
constexpr int MAPPING_SIZE = int(ButtonID::SIZE);
constexpr int NUM_ANALOG_TRIGGERS = int(ButtonID::LAST_ANALOG_TRIGGER) - int(ButtonID::FIRST_ANALOG_TRIGGER) + 1;
constexpr float MAGIC_DST_DELAY = 150.0f; // in milliseconds
constexpr float MAGIC_TAP_DURATION = 40.0f; // in milliseconds
constexpr float MAGIC_GYRO_TAP_DURATION = 500.0f; // in milliseconds
constexpr float MAGIC_HOLD_TIME = 150.0f; // in milliseconds
constexpr float MAGIC_SIM_DELAY = 50.0f; // in milliseconds
constexpr float MAGIC_DBL_PRESS_WINDOW = 200.0f; // in milliseconds
static_assert(MAGIC_SIM_DELAY < MAGIC_HOLD_TIME, "Simultaneous press delay has to be smaller than hold delay!");
static_assert(MAGIC_HOLD_TIME < MAGIC_DBL_PRESS_WINDOW, "Hold delay has to be smaller than double press window!");

enum class RingMode { outer, inner, invalid };
enum class StickMode { none, aim, flick, flickOnly, rotateOnly, mouseRing, mouseArea, outer, inner, invalid };
enum class FlickSnapMode { none, four, eight, invalid };
enum       AxisMode { standard = 1, inverted = -1, invalid = 0 }; // valid values are true!
enum class TriggerMode { noFull, noSkip, maySkip, mustSkip, maySkipResp, mustSkipResp, invalid };
enum class GyroAxisMask { none = 0, x = 1, y = 2, z = 4, invalid = 8 };
enum class JoyconMask { useBoth = 0, ignoreLeft = 1, ignoreRight = 2, ignoreBoth = 3, invalid = 4 };
enum class GyroIgnoreMode { button, left, right, invalid };
enum class DstState { NoPress = 0, PressStart, QuickSoftTap, QuickFullPress, QuickFullRelease, SoftPress, DelayFullPress, PressStartResp, invalid };
enum class BtnState {
	NoPress = 0, BtnPress, WaitHold, HoldPress, TapRelease,
	WaitSim, SimPress, WaitSimHold, SimHold, SimTapRelease, SimRelease,
	DblPressStart, DblPressNoPress, DblPressPress, DblPressWaitHold, DblPressHold, invalid
};

// Used for XY pair values such as sensitivity or GyroSample
// that includes a nicer accessor
struct FloatXY : public pair<float, float>
{
	FloatXY(float x = 0, float y = 0)
		: pair(x, y)
	{}

	inline float x() {
		return first;
	}

	inline float y() {
		return second;
	}

};

static ostream &operator << (ostream &out, FloatXY fxy)
{
	out << fxy.first << " " << fxy.second;
	return out;
}

static istream &operator >> (istream &in, FloatXY fxy)
{
	in >> fxy.first >> fxy.second;
	return in;
}

// Set of gyro control settings bundled in one structure
struct GyroSettings {
	bool always_off = false;
	ButtonID button = ButtonID::NONE;
	GyroIgnoreMode ignore_mode = GyroIgnoreMode::button;

	GyroSettings() = default;

	// This constructor is required to make use of the default value of JSMVariable's constructor
	GyroSettings(int dummy) : GyroSettings() {}
};

// Structure representing any kind of combination action, such as chords and modeshifts
// The location of this element in the overarching data structure identifies to what button
// or setting the binding is bound.
struct ComboMap
{
	string name; // Display name of the command
	ButtonID btn;           // ID of the key this commands is combined with
	WORD pressBind = 0;
	WORD holdBind = 0;
};

// This structure holds information about a simple button binding that can possibly be held.
// It also contains all alternate values it can provide via button combination.
// The location of this element in the overarching data structure identifies to what button
// the binding is bound.
struct Mapping
{
	WORD pressBind = 0; // Press or tap binding
	WORD holdBind = 0; // Hold binding if any.
	
	Mapping(WORD press = 0, WORD hold = 0)
		: pressBind(press)
		, holdBind(hold)
	{}
	// This constructor is required to make use of the default value of JSMVariable's constructor
	Mapping(int dummy) : Mapping() {}
};


// Custom Optional
template<typename T>
class Optional
{
private:
	bool valid = false;
	T value;

public:
	Optional() {} // nullopt

	Optional(T val)
		: value(val)
		, valid(true)
	{}

	inline operator bool() const
	{
		return valid;
	}

	inline T &operator *()
	{
		return value;
	}

	inline const T &operator *() const
	{
		return value;
	}

	inline T *operator ->()
	{
		return &value;
	}

	inline const T *operator ->() const
	{
		return &value;
	}

	inline T operator =(T newVal)
	{
		valid = true;
		value = newVal;
		return value;
	}

};

istream &operator >>(istream &in, ButtonID &button);
ostream &operator <<(ostream &out, ButtonID button);

istream &operator >>(istream &in, SettingID &setting);
ostream &operator <<(ostream &out, SettingID setting);

istream &operator >>(istream &in, StickMode &stickMode);
ostream &operator <<(ostream &out, StickMode stickMode);

istream &operator >>(istream &in, RingMode &ringMode);
ostream &operator <<(ostream &out, RingMode ringMode);

istream &operator >>(istream &in, FlickSnapMode &fsm);
ostream &operator <<(ostream &out, FlickSnapMode fsm);

istream &operator >>(istream &in, AxisMode &axisMode);
ostream &operator <<(ostream &out, AxisMode axisMode);

istream &operator >>(istream &in, TriggerMode &triggerMode);
ostream &operator <<(ostream &out, TriggerMode triggerMode);

istream &operator >>(istream &in, GyroAxisMask &gyroMask);
ostream &operator <<(ostream &out, GyroAxisMask gyroMask);

istream &operator >>(istream &in, JoyconMask &joyconMask);
ostream &operator <<(ostream &out, JoyconMask joyconMask);

istream &operator >>(istream &in, GyroSettings &gyro_settings);
ostream &operator <<(ostream &out, GyroSettings gyro_settings);

istream &operator >>(istream &in, Mapping &mapping);
ostream &operator <<(ostream &out, Mapping mapping);
