#pragma once

#include "JSMVersion.h"
#include "magic_enum.hpp"

#include <map>
#include <functional>

// This header file is meant to be included among all core JSM source files
// And as such it should contain only constants, types and functions related to them

using namespace std; // simplify all std calls

// input string parameters should be const references.
typedef const string &in_string;

// Reused OS typedefs
typedef unsigned short WORD;
typedef unsigned long DWORD;

// https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
// Only use undefined keys from the above list for JSM custom commands
constexpr WORD V_WHEEL_UP = 0x03;     // Here I intentionally overwride VK_CANCEL because breaking a process with a keybind is not something we want to do
constexpr WORD V_WHEEL_DOWN = 0x07;   // I want all mouse bindings to be contiguous IDs for ease to distinguish
constexpr WORD NO_HOLD_MAPPED = 0x0A; // Empty mapping, which is different form no mapping for combo presses
constexpr WORD CALIBRATE = 0x0B;
constexpr WORD GYRO_INV_X = 0x88;
constexpr WORD GYRO_INV_Y = 0x89;
constexpr WORD GYRO_INVERT = 0x8A;
constexpr WORD GYRO_OFF_BIND = 0x8B; // Not to be confused with settings GYRO_ON and GYRO_OFF
constexpr WORD GYRO_ON_BIND = 0x8C;  // Those here are bindings
constexpr WORD GYRO_TRACK_X = 0x8D;
constexpr WORD GYRO_TRACK_Y = 0x8E;
constexpr WORD GYRO_TRACKBALL = 0x8F;
constexpr WORD COMMAND_ACTION = 0x97; // Run command
constexpr WORD RUMBLE = 0xE6;

constexpr const char *SMALL_RUMBLE = "R0080";
constexpr const char *BIG_RUMBLE = "RFF00";

// XInput buttons
constexpr WORD X_UP = 0xE8;
constexpr WORD X_DOWN = 0xE9;
constexpr WORD X_LEFT = 0xEA;
constexpr WORD X_RIGHT = 0xEB;
constexpr WORD X_LB = 0xEC;
constexpr WORD X_RB = 0xED;
constexpr WORD X_X = 0xEE;
constexpr WORD X_A = 0xEF;
constexpr WORD X_Y = 0xF0;
constexpr WORD X_B = 0xF1;
constexpr WORD X_LS = 0xF2;
constexpr WORD X_RS = 0xF3;
constexpr WORD X_BACK = 0xF4;
constexpr WORD X_START = 0xF5;
constexpr WORD X_GUIDE = 0xB8;

// DS4 buttons
constexpr WORD PS_UP = 0xE8;
constexpr WORD PS_DOWN = 0xE9;
constexpr WORD PS_LEFT = 0xEA;
constexpr WORD PS_RIGHT = 0xEB;
constexpr WORD PS_L1 = 0xEC;
constexpr WORD PS_R1 = 0xED;
constexpr WORD PS_SQUARE = 0xEE;
constexpr WORD PS_CROSS = 0xEF;
constexpr WORD PS_TRIANGLE = 0xF0;
constexpr WORD PS_CIRCLE = 0xF1;
constexpr WORD PS_L3 = 0xF2;
constexpr WORD PS_R3 = 0xF3;
constexpr WORD PS_SHARE = 0xF4;
constexpr WORD PS_OPTIONS = 0xF5;
constexpr WORD PS_HOME = 0xB8;
constexpr WORD PS_PAD_CLICK = 0xB9;

// All enums should have an INVALID field for proper use with templated << and >> operators

enum class ButtonID
{
	INVALID = -2, // Represents an error in user input
	NONE,         //  Represents no button when explicitely stated by the user. Not to be confused with NO_HOLD_MAPPED which is no action bound.
	UP,           // = 0 as the first index
	DOWN,
	LEFT,
	RIGHT,
	L,
	ZL,
	MINUS,
	E,
	S,
	N,
	W,
	R,
	ZR,
	PLUS,
	HOME,
	LSL,
	LSR,
	RSL,
	RSR,
	L3,
	R3,
	LUP,
	LDOWN,
	LLEFT,
	LRIGHT,
	LRING,
	RUP,
	RDOWN,
	RLEFT,
	RRIGHT,
	RRING,
	MUP,
	MDOWN,
	MLEFT,
	MRIGHT,
	MRING,
	LEAN_LEFT,
	LEAN_RIGHT,
	TOUCH,   // Touch anywhere on the touchpad
	ZLF,     // = FIRST_ANALOG_TRIGGER
	CAPTURE, // Full press of touchpad touch + press
	// insert more analog triggers here
	ZRF,  // =  LAST_ANALOG_TRIGGER
	SIZE, // Not a button
};

// Help strings for each button
extern const map<ButtonID, string> buttonHelpMap;

enum class SettingID
{
	INVALID = -2,                            // Represents an error in user input
	MIN_GYRO_SENS = int(ButtonID::SIZE) + 1, // Legacy but int value not used
	MAX_GYRO_SENS,
	MIN_GYRO_THRESHOLD,
	MAX_GYRO_THRESHOLD,
	STICK_POWER,
	STICK_SENS,
	REAL_WORLD_CALIBRATION,
	IN_GAME_SENS,
	TRIGGER_THRESHOLD,
	RESET_MAPPINGS,
	NO_GYRO_BUTTON,
	LEFT_STICK_MODE,
	RIGHT_STICK_MODE,
	MOTION_STICK_MODE,
	GYRO_OFF,
	GYRO_ON,
	STICK_AXIS_X,
	STICK_AXIS_Y,
	GYRO_AXIS_X,
	GYRO_AXIS_Y,
	RECONNECT_CONTROLLERS,
	COUNTER_OS_MOUSE_SPEED,
	IGNORE_OS_MOUSE_SPEED,
	JOYCON_GYRO_MASK,
	JOYCON_MOTION_MASK,
	GYRO_SENS,
	FLICK_TIME,
	GYRO_SMOOTH_THRESHOLD,
	GYRO_SMOOTH_TIME,
	GYRO_CUTOFF_SPEED,
	GYRO_CUTOFF_RECOVERY,
	STICK_ACCELERATION_RATE,
	STICK_ACCELERATION_CAP,
	LEFT_STICK_DEADZONE_INNER,
	LEFT_STICK_DEADZONE_OUTER,
	STICK_DEADZONE_INNER,
	STICK_DEADZONE_OUTER,
	CALCULATE_REAL_WORLD_CALIBRATION,
	FINISH_GYRO_CALIBRATION,
	RESTART_GYRO_CALIBRATION,
	MOUSE_X_FROM_GYRO_AXIS,
	MOUSE_Y_FROM_GYRO_AXIS,
	ZR_MODE,
	ZL_MODE,
	AUTOLOAD,
	HELP,
	WHITELIST_SHOW,
	WHITELIST_ADD,
	WHITELIST_REMOVE,
	LEFT_RING_MODE,
	RIGHT_RING_MODE,
	MOTION_RING_MODE,
	MOUSE_RING_RADIUS,
	SCREEN_RESOLUTION_X,
	SCREEN_RESOLUTION_Y,
	ROTATE_SMOOTH_OVERRIDE,
	FLICK_SNAP_MODE,
	FLICK_SNAP_STRENGTH,
	MOTION_DEADZONE_INNER,
	MOTION_DEADZONE_OUTER,
	RIGHT_STICK_DEADZONE_INNER,
	RIGHT_STICK_DEADZONE_OUTER,
	LEAN_THRESHOLD,
	FLICK_DEADZONE_ANGLE,
	FLICK_TIME_EXPONENT,
	CONTROLLER_ORIENTATION,
	TRACKBALL_DECAY,
	TRIGGER_SKIP_DELAY,
	TURBO_PERIOD,
	HOLD_PRESS_TIME,
	TICK_TIME,
	SIM_PRESS_WINDOW, // Unchorded setting
	DBL_PRESS_WINDOW, // Unchorded setting
	LIGHT_BAR,
	SCROLL_SENS,
	VIRTUAL_CONTROLLER,
	RUMBLE,
	TOUCHPAD_DUAL_STAGE_MODE,
};

// constexpr are like #define but with respect to typeness
constexpr int MAPPING_SIZE = int(ButtonID::SIZE);
constexpr int FIRST_ANALOG_TRIGGER = int(ButtonID::ZLF);
constexpr int LAST_ANALOG_TRIGGER = int(ButtonID::ZRF);
constexpr int NUM_ANALOG_TRIGGERS = int(LAST_ANALOG_TRIGGER) - int(FIRST_ANALOG_TRIGGER) + 1;
constexpr float MAGIC_TAP_DURATION = 40.0f;           // in milliseconds.
constexpr float MAGIC_INSTANT_DURATION = 40.0f;       // in milliseconds
constexpr float MAGIC_EXTENDED_TAP_DURATION = 500.0f; // in milliseconds
constexpr int MAGIC_TRIGGER_SMOOTHING = 5;            // in samples

enum class ControllerOrientation
{
	FORWARD,
	LEFT,
	RIGHT,
	BACKWARD,
	JOYCON_SIDEWAYS,
	INVALID
};
enum class RingMode
{
	OUTER,
	INNER,
	INVALID
};
enum class StickMode
{
	NO_MOUSE,
	AIM,
	FLICK,
	FLICK_ONLY,
	ROTATE_ONLY,
	MOUSE_RING,
	MOUSE_AREA,
	OUTER_RING,
	INNER_RING,
	SCROLL_WHEEL,
	LEFT_STICK,
	RIGHT_STICK,
	INVALID
};
enum class FlickSnapMode
{
	NONE,
	FOUR,
	EIGHT,
	INVALID
};
enum class AxisMode
{
	STANDARD = 1,
	INVERTED = -1,
	INVALID = 0
}; // valid values are true!
enum class TriggerMode
{
	NO_FULL,
	NO_SKIP,
	MAY_SKIP,
	MUST_SKIP,
	MAY_SKIP_R,
	MUST_SKIP_R,
	NO_SKIP_EXCLUSIVE,
	X_LT,
	X_RT,
	PS_L2 = X_LT,
	PS_R2 = X_RT,
	INVALID
};
enum class GyroAxisMask
{
	NONE = 0,
	X = 1,
	Y = 2,
	Z = 4,
	INVALID = 8
};
enum class JoyconMask
{
	USE_BOTH,
	IGNORE_LEFT,
	IGNORE_RIGHT,
	IGNORE_BOTH,
	INVALID
};
enum class GyroIgnoreMode
{
	BUTTON,
	LEFT_STICK,
	RIGHT_STICK,
	INVALID
};
enum class DstState
{
	NoPress,
	PressStart,
	QuickSoftTap,
	QuickFullPress,
	QuickFullRelease,
	SoftPress,
	DelayFullPress,
	PressStartResp,
	ExclFullPress,
	INVALID
};
enum class BtnState
{
	NoPress,
	BtnPress,
	TapRelease,
	WaitSim,
	SimPress,
	SimRelease,
	DblPressStart,
	DblPressNoPressTap,
	DblPressNoPressHold,
	DblPressPress,
	InstRelease,
	INVALID
};
enum class BtnEvent
{
	OnPress,
	OnTap,
	OnHold,
	OnTurbo,
	OnRelease,
	OnTapRelease,
	OnHoldRelease,
	OnInstantRelease,
	INVALID
};
enum class Switch : char
{
	OFF,
	ON,
	INVALID,
}; // Used to parse autoload assignment
enum class ControllerScheme
{
	NONE,
	XBOX,
	DS4,
	INVALID
};

// Workaround default string streaming operator
class PathString : public string // Should be wstring
{
public:
	PathString() = default;
	PathString(in_string path)
	  : string(path)
	{
	}
};

union Color
{
	Color(uint32_t color = 0x00ffffff)
	  : raw(color)
	{
	}
	uint32_t raw;
	struct RGB_t
	{
		uint8_t b;
		uint8_t g;
		uint8_t r;
		uint8_t a; // unused. animation? (blink, pulse, etc)
	} rgb;
};

// Needs to be accessed publicly
extern WORD nameToKey(const std::string &name);

struct KeyCode
{
	static const KeyCode EMPTY;

	WORD code;
	string name;

	inline KeyCode()
	  : code()
	  , name()
	{
	}

	inline KeyCode(in_string keyName)
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

	inline bool isValid()
	{
		return code != 0;
	}

	inline bool operator==(const KeyCode &rhs)
	{
		return code == rhs.code && name == rhs.name;
	}

	inline bool operator!=(const KeyCode &rhs)
	{
		return !operator==(rhs);
	}
};

// Used for XY pair values such as sensitivity or GyroSample
// that includes a nicer accessor
struct FloatXY : public pair<float, float>
{
	FloatXY(float x = 0, float y = 0)
	  : pair(x, y)
	{
	}

	inline float x() const
	{
		return first;
	}

	inline float y() const
	{
		return second;
	}
};

// Set of gyro control settings bundled in one structure
struct GyroSettings
{
	bool always_off = false;
	ButtonID button = ButtonID::NONE;
	GyroIgnoreMode ignore_mode = GyroIgnoreMode::BUTTON;
};

class DigitalButton;
class JoyShock;

typedef function<void(DigitalButton *)> OnEventAction;

// This structure handles the mapping of a button, buy processing and action
// to be done on tap, hold, turbo and others. It holds a map of actions to perform
// when a specific event happens. This replaces the old Mapping structure.
class Mapping
{
public:
	enum class ActionModifier
	{
		None,
		Toggle,
		Instant,
		INVALID
	};
	enum class EventModifier
	{
		None,
		StartPress,
		ReleasePress,
		TurboPress,
		TapPress,
		HoldPress,
		INVALID
	};

	// Identifies having no binding mapped
	static const Mapping NO_MAPPING;

	// This functor nees to be set to way to validate a command line string;
	static function<bool(in_string)> _isCommandValid;

	string _description = "no input";
	string _command;

private:
	map<BtnEvent, OnEventAction> _eventMapping;
	float _tapDurationMs = MAGIC_TAP_DURATION;
	bool _hasViGEmBtn = false;

	void InsertEventMapping(BtnEvent evt, OnEventAction action);
	static void RunBothActions(DigitalButton *btn, OnEventAction action1, OnEventAction action2);

public:
	Mapping() = default;

	Mapping(in_string mapping);

	Mapping(int dummy)
	  : Mapping()
	{
	}

	void ProcessEvent(BtnEvent evt, DigitalButton &button, in_string displayName) const;

	bool AddMapping(KeyCode key, EventModifier evtMod, ActionModifier actMod = ActionModifier::None);

	inline bool isValid() const
	{
		return !_eventMapping.empty();
	}

	inline float getTapDuration() const
	{
		return _tapDurationMs;
	}

	inline void clear()
	{
		_eventMapping.clear();
		_description.clear();
		_tapDurationMs = MAGIC_TAP_DURATION;
		_hasViGEmBtn = false;
	}

	inline bool hasViGEmBtn() const
	{
		return _hasViGEmBtn;
	}
};

// This function is defined in main.cpp. It enables two sim press variables to
// listen to each other and make sure they both hold the same values.
void SimPressCrossUpdate(ButtonID sim, ButtonID origin, Mapping newVal);

// This operator enables reading any enum from string
template<class E, class = std::enable_if_t<std::is_enum<E>{}>>
istream &operator>>(istream &in, E &rhv)
{
	string s;
	in >> s;
	auto opt = magic_enum::enum_cast<E>(s);
	rhv = opt ? *opt : *magic_enum::enum_cast<E>("INVALID");
	return in;
}

// This operator enables writing any enum to string
template<class E, class = std::enable_if_t<std::is_enum<E>{}>>
ostream &operator<<(ostream &out, E rhv)
{
	out << magic_enum::enum_name(rhv);
	return out;
}

// The following operators enable reading and writing JSM's custom
// types to and from string, or handles exceptions
istream &operator>>(istream &in, ButtonID &rhv);
ostream &operator<<(ostream &out, ButtonID rhv);

istream &operator>>(istream &in, FlickSnapMode &fsm);
ostream &operator<<(ostream &out, FlickSnapMode fsm);

istream &operator>>(istream &in, TriggerMode &tm); // Handle L2 / R2

istream &operator>>(istream &in, GyroSettings &gyro_settings);
ostream &operator<<(ostream &out, GyroSettings gyro_settings);
bool operator==(const GyroSettings &lhs, const GyroSettings &rhs);
inline bool operator!=(const GyroSettings &lhs, const GyroSettings &rhs)
{
	return !(lhs == rhs);
}

istream &operator>>(istream &in, Mapping &mapping);
ostream &operator<<(ostream &out, Mapping mapping);
bool operator==(const Mapping &lhs, const Mapping &rhs);
inline bool operator!=(const Mapping &lhs, const Mapping &rhs)
{
	return !(lhs == rhs);
}

ostream &operator<<(ostream &out, FloatXY fxy);
istream &operator>>(istream &in, FloatXY &fxy);
bool operator==(const FloatXY &lhs, const FloatXY &rhs);
inline bool operator!=(const FloatXY &lhs, const FloatXY &rhs)
{
	return !(lhs == rhs);
}

istream &operator>>(istream &in, Color &color);
ostream &operator<<(ostream &out, Color color);
bool operator==(const Color &lhs, const Color &rhs);
inline bool operator!=(const Color &lhs, const Color &rhs)
{
	return !(lhs == rhs);
}

istream &operator>>(istream &in, AxisMode &am);
// AxisMode can use the templated operator for writing

istream &operator>>(istream &in, PathString &fxy);

// This trickery doesn't work in Linux does it? :(
#define CERR ColorStream<&std::cerr, FOREGROUND_RED | FOREGROUND_INTENSITY>()
#define COUT ColorStream<&std::cout, FOREGROUND_GREEN>()
#define COUT_INFO ColorStream<&cout, FOREGROUND_BLUE | FOREGROUND_INTENSITY>()
#define COUT_WARN ColorStream<&cout, FOREGROUND_YELLOW | FOREGROUND_INTENSITY>()
