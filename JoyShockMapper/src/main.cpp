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
#include "AutoLoad.h"
#include "SettingsManager.h"
#include "Stick.h"

#include <mutex>
#include <deque>
#include <iomanip>
#include <filesystem>
#include <memory>
#include <cfloat>
#include <cuchar>
#include <cstring>
#include <ranges>

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

float radial(float vX, float vY, float X, float Y)
{
	if (X!=0.0f && Y!=0.0f)
		return (vX * X + vY * Y) / sqrt(X * X + Y * Y);
	else
		return 0.0f;
}

float angleBasedDeadzone(float theta, float returnDeadzone, float returnDeadzoneCutoff)
{
	if (theta <= returnDeadzoneCutoff)
	{
		return (theta - returnDeadzone) / returnDeadzoneCutoff;
	}
	return 0.f;
}

class JoyShock;
void joyShockPollCallback(int jcHandle, JOY_SHOCK_STATE state, JOY_SHOCK_STATE lastState, IMU_STATE imuState, IMU_STATE lastImuState, float deltaTime);
void TouchCallback(int jcHandle, TOUCH_STATE newState, TOUCH_STATE prevState, float delta_time);

vector<JSMButton> grid_mappings; // array of virtual buttons on the touchpad grid
vector<JSMButton> mappings;      // array enables use of for each loop and other i/f

float os_mouse_speed = 1.0;
float last_flick_and_rotation = 0.0;
unique_ptr<PollingThread> autoLoadThread;
unique_ptr<PollingThread> minimizeThread;
bool devicesCalibrating = false;
unordered_map<int, shared_ptr<JoyShock>> handle_to_joyshock;
int triggerCalibrationStep = 0;

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

	template<typename E>
	static inline optional<E> GetOptionalSetting(SettingID id, ButtonID chord)
	{
		auto setting = SettingsManager::get<E>(id);
		if (setting)
		{
			auto chordedValue = setting->chordedValue(chord);
			return chordedValue ? optional<E>(setting->chordedValue(chord)) : nullopt;
		}
		return nullopt;
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
	chrono::steady_clock::time_point time_now;
	Stick left_stick, right_stick, motion_stick;
	// ScrollAxis motion_scroll_x;
	// ScrollAxis motion_scroll_y;
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

	bool processed_gyro_stick = false;

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
	  , _light_bar(SettingsManager::get<Color>(SettingID::LIGHT_BAR)->value())
	  , _context(sharedButtonCommon)
	  , motion(MotionIf::getNew())
	  , left_stick(SettingID::LEFT_STICK_DEADZONE_INNER, SettingID::LEFT_STICK_DEADZONE_OUTER, SettingID::LEFT_RING_MODE,
	      SettingID::LEFT_STICK_MODE, ButtonID::LRING, ButtonID::LLEFT, ButtonID::LRIGHT, ButtonID::LUP, ButtonID::LDOWN)
	  , right_stick(SettingID::RIGHT_STICK_DEADZONE_INNER, SettingID::RIGHT_STICK_DEADZONE_OUTER, SettingID::RIGHT_RING_MODE,
	      SettingID::RIGHT_STICK_MODE, ButtonID::RRING, ButtonID::RLEFT, ButtonID::RRIGHT, ButtonID::RUP, ButtonID::RDOWN)
	  , motion_stick(SettingID::MOTION_DEADZONE_INNER, SettingID::MOTION_DEADZONE_OUTER, SettingID::MOTION_RING_MODE,
	      SettingID::MOTION_STICK_MODE, ButtonID::MRING, ButtonID::MLEFT, ButtonID::MRIGHT, ButtonID::MUP, ButtonID::MDOWN)
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
		ResetSmoothSample();
		if (!CheckVigemState())
		{
			SettingsManager::getV<ControllerScheme>(SettingID::VIRTUAL_CONTROLLER)->set(ControllerScheme::NONE);
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
		touchpads[0].scroll.init(touchpads[0].buttons.find(ButtonID::TLEFT)->second, touchpads[0].buttons.find(ButtonID::TRIGHT)->second);
		touchpads[0].verticalScroll.init(touchpads[0].buttons.find(ButtonID::TUP)->second, touchpads[0].buttons.find(ButtonID::TDOWN)->second);
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
	
        // These two large functions are defined further down
	void processStick(float stickX, float stickY, Stick &stick, float mouseCalibrationFactor, float deltaTime, bool &anyStickInput, bool &lockMouse, float &camSpeedX, float &camSpeedY);
	void handleTouchStickChange(TouchStick &ts, bool down, short movX, short movY, float delta_time);

	void Rumble(int smallRumble, int bigRumble)
	{
		if (SettingsManager::getV<Switch>(SettingID::RUMBLE)->value() == Switch::ON)
		{
			// DEBUG_LOG << "Rumbling at " << smallRumble << " and " << bigRumble << endl;
			jsl->SetRumble(handle, smallRumble, bigRumble);
		}
	}

	bool CheckVigemState()
	{
		auto virtual_controller = SettingsManager::getV<ControllerScheme>(SettingID::VIRTUAL_CONTROLLER);
		if (virtual_controller && virtual_controller->value() != ControllerScheme::NONE)
		{
			string error = "There is no controller object";
			if (!_context->_vigemController || _context->_vigemController->isInitialized(&error) == false)
			{
				CERR << "[ViGEm Client] " << error << endl;
				return false;
			}
			else if (_context->_vigemController->getType() != virtual_controller->value())
			{
				CERR << "[ViGEm Client] The controller is of the wrong type!" << endl;
				return false;
			}
		}
		return true;
	}

	void handleViGEmNotification(UCHAR largeMotor, UCHAR smallMotor, Indicator indicator)
	{
		// static chrono::steady_clock::time_point last_call;
		// auto now = chrono::steady_clock::now();
		// auto diff = ((float)chrono::duration_cast<chrono::microseconds>(now - last_call).count()) / 1000000.0f;
		// last_call = now;
		// COUT_INFO << "Time since last vigem rumble is " << diff << " us" << endl;
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
			optional<E> opt = GetOptionalSetting<E>(index, *activeChord);
			if constexpr (std::is_same_v<E, StickMode>)
			{
				switch (index)
				{
				case SettingID::LEFT_STICK_MODE:
					if (left_stick.flick_percent_done < 1.f && opt && (*opt != StickMode::FLICK && *opt != StickMode::FLICK_ONLY))
						opt = std::make_optional(StickMode::FLICK_ONLY);
					else if (left_stick.ignore_stick_mode && *activeChord == ButtonID::NONE)
						opt = StickMode::INVALID;
					else
						left_stick.ignore_stick_mode |= (opt && *activeChord != ButtonID::NONE);
					break;
				case SettingID::RIGHT_STICK_MODE:
					if (right_stick.flick_percent_done < 1.f && opt && (*opt != StickMode::FLICK && *opt != StickMode::FLICK_ONLY))
						opt = std::make_optional(StickMode::FLICK_ONLY);
					else if (right_stick.ignore_stick_mode && *activeChord == ButtonID::NONE)
						opt = std::make_optional(StickMode::INVALID);
					else
						right_stick.ignore_stick_mode |= (opt && *activeChord != ButtonID::NONE);
					break;
				case SettingID::MOTION_STICK_MODE:
					if (motion_stick.flick_percent_done < 1.f && opt && (*opt != StickMode::FLICK && *opt != StickMode::FLICK_ONLY))
						opt = std::make_optional(StickMode::FLICK_ONLY);
					else if (motion_stick.ignore_stick_mode && *activeChord == ButtonID::NONE)
						opt = std::make_optional(StickMode::INVALID);
					else
						motion_stick.ignore_stick_mode |= (opt && *activeChord != ButtonID::NONE);
					break;
				}
			}

			if constexpr (std::is_same_v<E, ControllerOrientation>)
			{
				if (index == SettingID::CONTROLLER_ORIENTATION && opt && 
				    int(*opt) == int(ControllerOrientation::JOYCON_SIDEWAYS))
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
			auto opt = GetOptionalSetting<float>(index, *activeChord);
			switch (index)
			{
			case SettingID::TRIGGER_THRESHOLD:
				if (opt && platform_controller_type == JS_TYPE_DS && getSetting<Switch>(SettingID::ADAPTIVE_TRIGGER) == Switch::ON)
					opt = optional(max(0.f, *opt)); // hair trigger disabled on dual sense when adaptive triggers are active
				break;
			case SettingID::MOTION_DEADZONE_INNER:
			case SettingID::MOTION_DEADZONE_OUTER:
				if (opt)
					opt = *opt / 180.f;
				break;
			case SettingID::ZERO:
				opt = 0.f;
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
			optional<FloatXY> opt = GetOptionalSetting<FloatXY>(index, *activeChord);
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
				auto opt = GetOptionalSetting<GyroSettings>(index, *activeChord);
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
				auto opt = GetOptionalSetting<Color>(index, *activeChord);
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
			optional<AdaptiveTriggerSetting> opt = GetOptionalSetting<AdaptiveTriggerSetting>(index, *activeChord);
			if (opt)
				return *opt;
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
			optional<AxisSignPair> opt = GetOptionalSetting<AxisSignPair>(index, *activeChord);
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
        // this large functions is defined further down
	float handleFlickStick(float stickX, float stickY, Stick& stick, float stickLength, StickMode mode);

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
		for_each(prevTriggerPosition[triggerIndex].begin(), prevTriggerPosition[triggerIndex].begin() + 3, [&sum](auto data)
		  { sum += data; });
		float avg_tm3 = sum / 3.0f;
		sum = sum - *(prevTriggerPosition[triggerIndex].begin()) + *(prevTriggerPosition[triggerIndex].end() - 2);
		float avg_tm2 = sum / 3.0f;
		sum = sum - *(prevTriggerPosition[triggerIndex].begin() + 1) + *(prevTriggerPosition[triggerIndex].end() - 1);
		float avg_tm1 = sum / 3.0f;
		sum = sum - *(prevTriggerPosition[triggerIndex].begin() + 2) + triggerPosition;
		float avg_t0 = sum / 3.0f;
		// if (avg_t0 > 0) COUT << "Trigger: " << avg_t0 << endl;

		// Soft press is pressed if we got three averaged samples in a row that are pressed
		bool isPressed;
		if (avg_t0 > avg_tm1 && avg_tm1 > avg_tm2 && avg_tm2 > avg_tm3)
		{
			// DEBUG_LOG << "Hair Trigger pressed: " << avg_t0 << " > " << avg_tm1 << " > " << avg_tm2 << " > " << avg_tm3 << endl;
			isPressed = true;
		}
		else if (avg_t0 < avg_tm1 && avg_tm1 < avg_tm2 && avg_tm2 < avg_tm3)
		{
			// DEBUG_LOG << "Hair Trigger released: " << avg_t0 << " < " << avg_tm1 << " < " << avg_tm2 << " < " << avg_tm3 << endl;
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
		uint8_t offset = SettingsManager::getV<int>(softIndex == ButtonID::ZL ? SettingID::LEFT_TRIGGER_OFFSET : SettingID::RIGHT_TRIGGER_OFFSET)->value();
		uint8_t range = SettingsManager::getV<int>(softIndex == ButtonID::ZL ? SettingID::LEFT_TRIGGER_RANGE : SettingID::RIGHT_TRIGGER_RANGE)->value();
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
				float tick_time = SettingsManager::get<float>(SettingID::TICK_TIME)->value();
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

function<void(JoyShock *, ButtonID, int, bool)> ScrollAxis::_handleButtonChange = [](JoyShock *js, ButtonID id, int tid, bool pressed)
{
	js->handleButtonChange(id, pressed, tid);
};

static void resetAllMappings()
{
	static constexpr auto callReset = [](JSMButton &map)
	{
		map.reset();
	};
	ranges::for_each(mappings, callReset);
	// It is possible some settings were intentionally omitted (JSM_DIRECTORY?, Whitelister?)
	// TODO: make sure omitted settings don't get reset
	SettingsManager::resetAllSettings();
	std::ranges::for_each(grid_mappings, callReset);

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
			// deviceHandles.resize(numConnected);
		}

		for (auto handle : deviceHandles) // Don't use foreach!
		{
			auto type = jsl->GetControllerSplitType(handle);
			auto otherJoyCon = find_if(handle_to_joyshock.begin(), handle_to_joyshock.end(),
			  [type](auto &pair)
			  {
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
	// if (!IsVisible())
	//{
	//	tray->SendNotification(wstring(msg.begin(), msg.end()));
	// }

	// if (numConnected != 0) {
	//	COUT << "All devices have started continuous gyro calibration" << endl;
	// }
}

void SimPressCrossUpdate(ButtonID sim, ButtonID origin, const Mapping &newVal)
{
	mappings[int(sim)].atSimPress(origin)->set(newVal);
}

bool do_NO_GYRO_BUTTON()
{
	// TODO: handle chords
	SettingsManager::get<GyroSettings>(SettingID::GYRO_ON)->reset();
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
			numRotations = stof(string(argument));
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
		COUT << "Recommendation: REAL_WORLD_CALIBRATION = " << setprecision(5) << (SettingsManager::get<float>(SettingID::REAL_WORLD_CALIBRATION)->value() * last_flick_and_rotation / numRotations) << endl;
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
			sleepTime = stof(argument.data());
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

bool processGyroStick(JoyShock *jc, float stickX, float stickY, float stickLength, StickMode stickMode, bool forceOutput)
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

float JoyShock::handleFlickStick(float stickX, float stickY, Stick& stick, float stickLength, StickMode mode)
{
	GyroOutput flickStickOutput = getSetting<GyroOutput>(SettingID::FLICK_STICK_OUTPUT);
	bool isMouse = flickStickOutput == GyroOutput::MOUSE;
	float mouseCalibrationFactor = 180.0f / PI / os_mouse_speed;

	float camSpeedX = 0.0f;
	// let's centre this
	float offsetX = stickX;
	float offsetY = stickY;
	float lastOffsetX = stick.lastX;
	float lastOffsetY = stick.lastY;
	float flickStickThreshold = 1.0f;
	if (stick.is_flicking)
	{
		flickStickThreshold *= 0.9f;
	}
	if (stickLength >= flickStickThreshold)
	{
		float stickAngle = atan2f(-offsetX, offsetY);
		// COUT << ", %.4f\n", lastOffsetLength);
		if (!stick.is_flicking)
		{
			// bam! new flick!
			stick.is_flicking = true;
			if (mode != StickMode::ROTATE_ONLY)
			{
				auto flick_snap_mode = getSetting<FlickSnapMode>(SettingID::FLICK_SNAP_MODE);
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
					auto flick_snap_strength = getSetting(SettingID::FLICK_SNAP_STRENGTH);
					stickAngle = stickAngle * (1.0f - flick_snap_strength) + snappedAngle * flick_snap_strength;
				}
				if (abs(stickAngle) * (180.0f / PI) < getSetting(SettingID::FLICK_DEADZONE_ANGLE))
				{
					stickAngle = 0.0f;
				}

				stick.started_flick = chrono::steady_clock::now();
				stick.delta_flick = stickAngle;
				stick.flick_percent_done = 0.0f;
				ResetSmoothSample();
				stick.flick_rotation_counter = stickAngle; // track all rotation for this flick
				COUT << "Flick: " << setprecision(3) << stickAngle * (180.0f / (float)PI) << " degrees" << endl;
			}
		}
		else // I am flicking!
		{
			if (mode != StickMode::FLICK_ONLY)
			{
				// not new? turn camera?
				float lastStickAngle = atan2f(-lastOffsetX, lastOffsetY);
				float angleChange = stickAngle - lastStickAngle;
				// https://stackoverflow.com/a/11498248/1130520
				angleChange = fmod(angleChange + PI, 2.0f * PI);
				if (angleChange < 0)
					angleChange += 2.0f * PI;
				angleChange -= PI;
				stick.flick_rotation_counter += angleChange; // track all rotation for this flick
				float flickSpeedConstant = isMouse ? getSetting(SettingID::REAL_WORLD_CALIBRATION) * mouseCalibrationFactor / getSetting(SettingID::IN_GAME_SENS) : 1.f;
				float flickSpeed = -(angleChange * flickSpeedConstant);
				float tick_time = SettingsManager::get<float>(SettingID::TICK_TIME)->value();
				int maxSmoothingSamples = min(NumSamples, (int)ceil(64.0f / tick_time)); // target a max smoothing window size of 64ms
				float stepSize = 0.01f;                                                      // and we only want full on smoothing when the stick change each time we poll it is approximately the minimum stick resolution
				                                                                             // the fact that we're using radians makes this really easy
				auto rotate_smooth_override = getSetting(SettingID::ROTATE_SMOOTH_OVERRIDE);
				if (rotate_smooth_override < 0.0f)
				{
					camSpeedX = GetSmoothedStickRotation(flickSpeed, flickSpeedConstant * stepSize * 2.0f, flickSpeedConstant * stepSize * 4.0f, maxSmoothingSamples);
				}
				else
				{
					camSpeedX = GetSmoothedStickRotation(flickSpeed, flickSpeedConstant * rotate_smooth_override, flickSpeedConstant * rotate_smooth_override * 2.0f, maxSmoothingSamples);
				}

				if (!isMouse)
				{
					// convert to a velocity
					camSpeedX *= 180.0f / (PI * 0.001f * SettingsManager::get<float>(SettingID::TICK_TIME)->value());
				}
			}
		}
	}
	else if (stick.is_flicking)
	{
		// was a flick! how much was the flick and rotation?
		if (mode == StickMode::FLICK) // not only flick or only rotate
		{
			last_flick_and_rotation = abs(stick.flick_rotation_counter) / (2.0f * PI);
		}
		stick.is_flicking = false;
	}
	// do the flicking. this works very differently if it's mouse vs stick
	if (isMouse)
	{
		float secondsSinceFlick = ((float)chrono::duration_cast<chrono::microseconds>(time_now - stick.started_flick).count()) / 1000000.0f;
		float newPercent = secondsSinceFlick / getSetting(SettingID::FLICK_TIME);

		// don't divide by zero
		if (abs(stick.delta_flick) > 0.0f)
		{
			newPercent = newPercent / pow(abs(stick.delta_flick) / PI, getSetting(SettingID::FLICK_TIME_EXPONENT));
		}

		if (newPercent > 1.0f)
			newPercent = 1.0f;
		// warping towards 1.0
		float oldShapedPercent = 1.0f - stick.flick_percent_done;
		oldShapedPercent *= oldShapedPercent;
		oldShapedPercent = 1.0f - oldShapedPercent;
		// float oldShapedPercent = flick_percent_done;
		stick.flick_percent_done = newPercent;
		newPercent = 1.0f - newPercent;
		newPercent *= newPercent;
		newPercent = 1.0f - newPercent;
		float camSpeedChange = (newPercent - oldShapedPercent) * stick.delta_flick * getSetting(SettingID::REAL_WORLD_CALIBRATION) * -mouseCalibrationFactor / getSetting(SettingID::IN_GAME_SENS);
		camSpeedX += camSpeedChange;

		return camSpeedX;
	}
	else
	{
		float secondsSinceFlick = ((float)chrono::duration_cast<chrono::microseconds>(time_now - stick.started_flick).count()) / 1000000.0f;
		float maxStickGameSpeed = getSetting(SettingID::VIRTUAL_STICK_CALIBRATION);
		float flickTime = abs(stick.delta_flick) / (maxStickGameSpeed * PI / 180.f);

		if (secondsSinceFlick <= flickTime)
		{
			camSpeedX -= stick.delta_flick >= 0 ? maxStickGameSpeed : -maxStickGameSpeed;
		}

		// alright, but what happens if we've set gyro to one stick and flick stick to another?
		// Nic: FS is mouse output and gyrostick is stick output. The game handles the merging (or not)
		// Depends on the game, some take simultaneous input better than others. Players are aware of that. -Nic
		GyroOutput gyroOutput = getSetting<GyroOutput>(SettingID::GYRO_OUTPUT);
		if (gyroOutput == flickStickOutput)
		{
			gyroXVelocity += camSpeedX;
			processGyroStick(this, 0.f, 0.f, 0.f, flickStickOutput == GyroOutput::LEFT_STICK ? StickMode::LEFT_STICK : StickMode::RIGHT_STICK, false);
		}
		else
		{
			float tempGyroXVelocity = gyroXVelocity;
			float tempGyroYVelocity = gyroYVelocity;
			gyroXVelocity = camSpeedX;
			gyroYVelocity = 0.f;
			processGyroStick(this, 0.f, 0.f, 0.f, flickStickOutput == GyroOutput::LEFT_STICK ? StickMode::LEFT_STICK : StickMode::RIGHT_STICK, true);
			gyroXVelocity = tempGyroXVelocity;
			gyroYVelocity = tempGyroYVelocity;
		}

		return 0.f;
	}
}

void JoyShock::processStick(float stickX, float stickY, Stick &stick, float mouseCalibrationFactor, float deltaTime, bool &anyStickInput, bool &lockMouse, float &camSpeedX, float &camSpeedY)
{
	float temp;
	auto controllerOrientation = getSetting<ControllerOrientation>(SettingID::CONTROLLER_ORIENTATION);
	switch (controllerOrientation)
	{
	case ControllerOrientation::LEFT:
		temp = stickX;
		stickX = -stickY;
		stickY = temp;
		temp = stick.lastX;
		stick.lastX = -stick.lastY;
		stick.lastY = temp;
		break;
	case ControllerOrientation::RIGHT:
		temp = stickX;
		stickX = stickY;
		stickY = -temp;
		temp = stick.lastX;
		stick.lastX = stick.lastY;
		stick.lastY = -temp;
		break;
	case ControllerOrientation::BACKWARD:
		stickX = -stickX;
		stickY = -stickY;
		stick.lastX = -stick.lastX;
		stick.lastY = -stick.lastY;
		break;
	}
	auto outerDeadzone = getSetting(stick._outerDeadzone);
	auto innerDeadzone = getSetting(stick._innerDeadzone);
	outerDeadzone = 1.0f - outerDeadzone;
	float rawX = stickX;
	float rawY = stickY;
	float rawLength = sqrtf(rawX * rawX + rawY * rawY);
	float rawLastX = stick.lastX;
	float rawLastY = stick.lastY;
	processDeadZones(stick.lastX, stick.lastY, innerDeadzone, outerDeadzone);
	bool pegged = processDeadZones(stickX, stickY, innerDeadzone, outerDeadzone);
	float absX = abs(stickX);
	float absY = abs(stickY);
	bool left = stickX < -0.5f * absY;
	bool right = stickX > 0.5f * absY;
	bool down = stickY < -0.5f * absX;
	bool up = stickY > 0.5f * absX;
	float stickLength = sqrtf(stickX * stickX + stickY * stickY);
	auto ringMode = getSetting<RingMode>(stick._ringMode);
	auto stickMode = getSetting<StickMode>(stick._stickMode);

	bool ring = ringMode == RingMode::INNER && stickLength > 0.0f && stickLength < 0.7f ||
	  ringMode == RingMode::OUTER && stickLength > 0.7f;
	handleButtonChange(stick._ringId, ring, stick._touchpadIndex);

	if (stick.ignore_stick_mode && stickMode == StickMode::INVALID && stickX == 0 && stickY == 0)
	{
		// clear ignore flag when stick is back at neutral
		stick.ignore_stick_mode = false;
	}
	else if (stickMode == StickMode::FLICK || stickMode == StickMode::FLICK_ONLY || stickMode == StickMode::ROTATE_ONLY)
	{
		camSpeedX += handleFlickStick(stickX, stickY, stick, stickLength, stickMode);
		anyStickInput = pegged;
	}
	else if (stickMode == StickMode::AIM)
	{
		// camera movement
		if (!pegged)
		{
			stick.acceleration = 1.0f; // reset
		}
		float stickLength = sqrt(stickX * stickX + stickY * stickY);
		if (stickLength != 0.0f)
		{
			anyStickInput = true;
			float warpedStickLengthX = pow(stickLength, getSetting(SettingID::STICK_POWER));
			float warpedStickLengthY = warpedStickLengthX;
			warpedStickLengthX *= getSetting<FloatXY>(SettingID::STICK_SENS).first * getSetting(SettingID::REAL_WORLD_CALIBRATION) / os_mouse_speed / getSetting(SettingID::IN_GAME_SENS);
			warpedStickLengthY *= getSetting<FloatXY>(SettingID::STICK_SENS).second * getSetting(SettingID::REAL_WORLD_CALIBRATION) / os_mouse_speed / getSetting(SettingID::IN_GAME_SENS);
			camSpeedX += stickX / stickLength * warpedStickLengthX * stick.acceleration * deltaTime;
			camSpeedY += stickY / stickLength * warpedStickLengthY * stick.acceleration * deltaTime;
			if (pegged)
			{
				stick.acceleration += getSetting(SettingID::STICK_ACCELERATION_RATE) * deltaTime;
				auto cap = getSetting(SettingID::STICK_ACCELERATION_CAP);
				if (stick.acceleration > cap)
				{
					stick.acceleration = cap;
				}
			}
		}
	}
	else if (stickMode == StickMode::MOUSE_AREA)
	{
		auto mouse_ring_radius = getSetting(SettingID::MOUSE_RING_RADIUS);		
		
		float mouseX = (rawX - rawLastX) * mouse_ring_radius;
		float mouseY = (rawY - rawLastY) * -1 * mouse_ring_radius;
		// do it!
		moveMouse(mouseX, mouseY);
		
	}
	else if (stickMode == StickMode::MOUSE_RING)
	{
		if (stickX != 0.0f || stickY != 0.0f)
		{
			auto mouse_ring_radius = getSetting(SettingID::MOUSE_RING_RADIUS);
			float stickLength = sqrt(stickX * stickX + stickY * stickY);
			float normX = stickX / stickLength;
			float normY = stickY / stickLength;
			// use screen resolution
			float mouseX = getSetting(SettingID::SCREEN_RESOLUTION_X) * 0.5f + 0.5f + normX * mouse_ring_radius;
			float mouseY = getSetting(SettingID::SCREEN_RESOLUTION_X) * 0.5f + 0.5f - normY * mouse_ring_radius;
			// normalize
			mouseX = mouseX / getSetting(SettingID::SCREEN_RESOLUTION_X);
			mouseY = mouseY / getSetting(SettingID::SCREEN_RESOLUTION_Y);
			// do it!
			setMouseNorm(mouseX, mouseY);
			lockMouse = true;
		}
	}
	else if (stickMode == StickMode::SCROLL_WHEEL)
	{
		if (stick.scroll.isInitialized())
		{
			if (stickX == 0 && stickY == 0)
			{
				stick.scroll.reset(time_now);
			}
			else if (stick.lastX != 0 && stick.lastY != 0)
			{
				float lastAngle = atan2f(stick.lastY, stick.lastX) / PI * 180.f;
				float angle = atan2f(stickY, stickX) / PI * 180.f;
				if (((lastAngle>0) ^ (angle>0)) && fabsf(angle - lastAngle) > 270.f) // Handle loop the loop
				{
					lastAngle = lastAngle > 0 ? lastAngle - 360.f : lastAngle + 360.f;
				}
				// COUT << "Stick moved from " << lastAngle << " to " << angle; // << endl;
				stick.scroll.processScroll(angle - lastAngle, getSetting<FloatXY>(SettingID::SCROLL_SENS).x(), time_now);
			}
		}
	}
	else if (stickMode == StickMode::NO_MOUSE || stickMode == StickMode::INNER_RING || stickMode == StickMode::OUTER_RING)
	{ // Do not do if invalid
		// left!
		handleButtonChange(stick._leftId, left, stick._touchpadIndex);
		// right!
		handleButtonChange(stick._rightId, right, stick._touchpadIndex);
		// up!
		handleButtonChange(stick._upId, up, stick._touchpadIndex);
		// down!
		handleButtonChange(stick._downId, down, stick._touchpadIndex);

		anyStickInput = left || right || up || down; // ring doesn't count
	}
	else if (stickMode == StickMode::LEFT_STICK || stickMode == StickMode::RIGHT_STICK)
	{
		if (_context->_vigemController)
		{
			anyStickInput = processGyroStick(this, stick.lastX, stick.lastY, stickLength, stickMode, false);
		}
	}
	else if (stickMode >= StickMode::LEFT_ANGLE_TO_X && stickMode <= StickMode::RIGHT_ANGLE_TO_Y)
	{
		if (_context->_vigemController && rawLength > innerDeadzone)
		{
			bool isX = stickMode == StickMode::LEFT_ANGLE_TO_X || stickMode == StickMode::RIGHT_ANGLE_TO_X;
			bool isLeft = stickMode == StickMode::LEFT_ANGLE_TO_X || stickMode == StickMode::LEFT_ANGLE_TO_Y;

			float stickAngle = isX ? atan2f(stickX, absY) : atan2f(stickY, absX);
			float absAngle = abs(stickAngle * 180.0f / PI);
			float signAngle = stickAngle < 0.f ? -1.f : 1.f;
			float angleDeadzoneInner = getSetting(SettingID::ANGLE_TO_AXIS_DEADZONE_INNER);
			float angleDeadzoneOuter = getSetting(SettingID::ANGLE_TO_AXIS_DEADZONE_OUTER);
			float absStickValue = clamp((absAngle - angleDeadzoneInner) / (90.f - angleDeadzoneOuter - angleDeadzoneInner), 0.f, 1.f);

			absStickValue *= pow(stickLength, getSetting(SettingID::STICK_POWER));

			// now actually convert to output stick value, taking deadzones and power curve into account
			float undeadzoneInner, undeadzoneOuter, unpower;
			if (isLeft)
			{
				undeadzoneInner = getSetting(SettingID::LEFT_STICK_UNDEADZONE_INNER);
				undeadzoneOuter = getSetting(SettingID::LEFT_STICK_UNDEADZONE_OUTER);
				unpower = getSetting(SettingID::LEFT_STICK_UNPOWER);
			}
			else
			{
				undeadzoneInner = getSetting(SettingID::RIGHT_STICK_UNDEADZONE_INNER);
				undeadzoneOuter = getSetting(SettingID::RIGHT_STICK_UNDEADZONE_OUTER);
				unpower = getSetting(SettingID::RIGHT_STICK_UNPOWER);
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
					_context->_vigemController->setStick(signedStickValue, 0.f, isLeft);
				}
				else
				{
					_context->_vigemController->setStick(0.f, signedStickValue, isLeft);
				}
			}
		}
	}
	else if (stickMode == StickMode::LEFT_WIND_X || stickMode == StickMode::RIGHT_WIND_X)
	{
		if (_context->_vigemController)
		{
			bool isLeft = stickMode == StickMode::LEFT_WIND_X;

			float &currentWindingAngle = isLeft ? windingAngleLeft : windingAngleRight;

			// currently, just use the same hard-coded thresholds we use for flick stick. These are affected by deadzones
			if (stickLength > 0.f && stick.lastX != 0.f && stick.lastY != 0.f)
			{
				// use difference between last stick angle and current
				float stickAngle = atan2f(-stickX, stickY);
				float lastStickAngle = atan2f(-stick.lastX, stick.lastY);
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
				float unwindAmount = getSetting(SettingID::UNWIND_RATE) * (1.f - stickLength) * deltaTime;
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

			float windingRange = getSetting(SettingID::WIND_STICK_RANGE);
			float windingPower = getSetting(SettingID::WIND_STICK_POWER);
			if (windingPower == 0.f)
			{
				windingPower = 1.f;
			}

			float windingRemapped = std::min(pow(newAbsWindingAngle / windingRange * 2.f, windingPower), 1.f);

			// let's account for deadzone!
			float undeadzoneInner, undeadzoneOuter, unpower;
			if (isLeft)
			{
				undeadzoneInner = getSetting(SettingID::LEFT_STICK_UNDEADZONE_INNER);
				undeadzoneOuter = getSetting(SettingID::LEFT_STICK_UNDEADZONE_OUTER);
				unpower = getSetting(SettingID::LEFT_STICK_UNPOWER);
			}
			else
			{
				undeadzoneInner = getSetting(SettingID::RIGHT_STICK_UNDEADZONE_INNER);
				undeadzoneOuter = getSetting(SettingID::RIGHT_STICK_UNDEADZONE_OUTER);
				unpower = getSetting(SettingID::RIGHT_STICK_UNPOWER);
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
				_context->_vigemController->setStick(signedStickValue, 0.f, isLeft);
			}
		}
	}
	else if (stickMode == StickMode::HYBRID_AIM)
	{
		float velocityX = rawX - rawLastX;
		float velocityY = -rawY + rawLastY;
		float velocity = sqrt(velocityX * velocityX + velocityY * velocityY);
		float velocityRadial = radial(velocityX, velocityY, rawX, -rawY);
		float deflection = rawLength;
		float previousDeflection = sqrt(rawLastX * rawLastX + rawLastY * rawLastY);
		float magnitude = 0;
		float angle = atan2f(-rawY, rawX);
		bool inDeadzone = false;

		// check deadzones
		if (deflection > innerDeadzone)
		{
			inDeadzone = false;
			magnitude = (deflection - innerDeadzone) / (outerDeadzone - innerDeadzone);

			// check outer_deadzone
			if (deflection > outerDeadzone)
			{
				// clip outward radial velocity
				if (velocityRadial > 0.0f)
				{
					float dotProduct = velocityX * sin(angle) + velocityY * -cos(angle);
					velocityX = dotProduct * sin(angle);
					velocityY = dotProduct * -cos(angle);
				}
				magnitude = 1.0f;

				// check entering
				if (previousDeflection <= outerDeadzone)
				{
					float averageVelocityX = 0.0f;
					float averageVelocityY = 0.0f;
					int steps = 0;
					int counter = stick.smoothingCounter;
					while (steps < stick.SMOOTHING_STEPS) // sqrt(cumulativeVX * cumulativeVX + cumulativeVY * cumulativeVY) < smoothingDistance && 
					{
						averageVelocityX += stick.previousVelocitiesX[counter];
						averageVelocityY += stick.previousVelocitiesY[counter];
						if (counter == 0)
							counter = stick.SMOOTHING_STEPS - 1;
						else
							counter--;
						steps++;
					}

					if (getSetting<Switch>(SettingID::EDGE_PUSH_IS_ACTIVE) == Switch::ON)
					{
						stick.edgePushAmount *= stick.smallestMagnitude;
						stick.edgePushAmount += radial(averageVelocityX, averageVelocityY, rawX, -rawY) / steps;
						stick.smallestMagnitude = 1.f;
					}
				}
			}
		}
		else
		{
			stick.edgePushAmount = 0.0f;
			inDeadzone = true;			
		}

		if (magnitude < stick.smallestMagnitude)
			stick.smallestMagnitude = magnitude;

		// compute output
		FloatXY sticklikeFactor = getSetting<FloatXY>(SettingID::STICK_SENS);
		FloatXY mouselikeFactor = getSetting<FloatXY>(SettingID::MOUSELIKE_FACTOR);
		float outputX = sticklikeFactor.x() / 2.f * pow(magnitude, getSetting(SettingID::STICK_POWER)) * cos(angle) * deltaTime;
		float outputY = sticklikeFactor.y() / 2.f * pow(magnitude, getSetting(SettingID::STICK_POWER)) * sin(angle) * deltaTime;
		outputX += mouselikeFactor.x() * pow(stick.smallestMagnitude, getSetting(SettingID::STICK_POWER)) * cos(angle) * stick.edgePushAmount;
		outputY += mouselikeFactor.y() * pow(stick.smallestMagnitude, getSetting(SettingID::STICK_POWER)) * sin(angle) * stick.edgePushAmount;
		outputX += mouselikeFactor.x() * velocityX;
		outputY += mouselikeFactor.y() * velocityY;
		
		// for smoothing edgePush and clipping on returning to center
		// probably only needed as deltaTime is faster than the controller's polling rate 
		// (was tested with xbox360 controller which has a polling rate of 125Hz)
		if (stick.smoothingCounter < stick.SMOOTHING_STEPS - 1)
			stick.smoothingCounter++;
		else
			stick.smoothingCounter = 0;

		stick.previousVelocitiesX[stick.smoothingCounter] = velocityX;
		stick.previousVelocitiesY[stick.smoothingCounter] = velocityY;
		float outputRadial = radial(outputX, outputY, rawX, -rawY);
		stick.previousOutputRadial[stick.smoothingCounter] = outputRadial;
		stick.previousOutputX[stick.smoothingCounter] = outputX;
		stick.previousOutputY[stick.smoothingCounter] = outputY;

		if (getSetting<Switch>(SettingID::RETURN_DEADZONE_IS_ACTIVE) == Switch::ON)
		{
			// 0 means deadzone fully active 1 means unaltered output
			float averageOutputX = 0.f;
			float averageOutputY = 0.f;
			for (int i = 0; i < stick.SMOOTHING_STEPS; i++)
			{
				averageOutputX += stick.previousOutputX[i];
				averageOutputY += stick.previousOutputY[i];
			}
			float averageOutput = sqrt(averageOutputX * averageOutputX + averageOutputY * averageOutputY) / stick.SMOOTHING_STEPS;
			averageOutputX /= stick.SMOOTHING_STEPS;
			averageOutputY /= stick.SMOOTHING_STEPS;

			float averageOutputRadial = 0;
			for (int i = 0; i < stick.SMOOTHING_STEPS; i++)
			{
				averageOutputRadial += stick.previousOutputRadial[i];
			}
			averageOutputRadial /= stick.SMOOTHING_STEPS;
			float returnDeadzone1 = 1.f;
			float angleOutputToCenter = 0;
			const float returningDeadzone = getSetting(SettingID::RETURN_DEADZONE_ANGLE) / 180.f * PI;
			const float returningCutoff = getSetting(SettingID::RETURN_DEADZONE_ANGLE_CUTOFF) / 180.f * PI;

			if (averageOutputRadial < 0.f)
			{
				angleOutputToCenter = abs(PI - acosf((averageOutputX * rawLastX + averageOutputY * -rawLastY) / (averageOutput * previousDeflection))); /// STILL WRONG
				returnDeadzone1 = angleBasedDeadzone(angleOutputToCenter, returningDeadzone, returningCutoff);
			}
			float returnDeadzone2 = 1.f;
			if (inDeadzone)
			{
				if (averageOutputRadial < 0.f)
				{
					// if angle inward output deadzone based on tangent concentric circle
					float angleEquivalent = abs(rawLastX * averageOutputY + -rawLastY * -averageOutputX) / averageOutput;
					returnDeadzone2 = angleBasedDeadzone(angleEquivalent, returningDeadzone, returningCutoff);
				}
				else
				{
					// output deadzone based on distance to center
					float angleEquivalent = asinf(previousDeflection / innerDeadzone);
					returnDeadzone2 = angleBasedDeadzone(angleEquivalent, returningDeadzone, returningCutoff);
				}
			}
			float returnDeadzone = std::min({ returnDeadzone1, returnDeadzone2 });
			outputX *= returnDeadzone;
			outputY *= returnDeadzone;
			if (returnDeadzone == 0.f)
				stick.edgePushAmount = 0.f;			
		}
		moveMouse(outputX, outputY);
    }
}

void JoyShock::handleTouchStickChange(TouchStick &ts, bool down, short movX, short movY, float delta_time)
{
	float stickX = down ? clamp<float>((ts._currentLocation.x() + movX) / getSetting(SettingID::TOUCH_STICK_RADIUS), -1.f, 1.f) : 0.f;
	float stickY = down ? clamp<float>((ts._currentLocation.y() - movY) / getSetting(SettingID::TOUCH_STICK_RADIUS), -1.f, 1.f) : 0.f;
	float innerDeadzone = getSetting(SettingID::TOUCH_DEADZONE_INNER);
	RingMode ringMode = getSetting<RingMode>(SettingID::TOUCH_RING_MODE);
	StickMode stickMode = getSetting<StickMode>(SettingID::TOUCH_STICK_MODE);
	ControllerOrientation controllerOrientation = getSetting<ControllerOrientation>(SettingID::CONTROLLER_ORIENTATION);
	float mouseCalibrationFactor = 180.0f / PI / os_mouse_speed;

	bool anyStickInput = false;
	bool lockMouse = false;
	float camSpeedX = 0.f;
	float camSpeedY = 0.f;
	auto axisSign = getSetting<AxisSignPair>(SettingID::TOUCH_STICK_AXIS);

	stickX *= float(axisSign.first);
	stickY *= float(axisSign.second);
	processStick(stickX, stickY, ts, mouseCalibrationFactor, delta_time, anyStickInput, lockMouse, camSpeedX, camSpeedY);
	ts.lastX = stickX;
	ts.lastY = stickY;

	moveMouse(camSpeedX * float(getSetting<AxisSignPair>(SettingID::TOUCH_STICK_AXIS).first), -camSpeedY * float(getSetting<AxisSignPair>(SettingID::TOUCH_STICK_AXIS).second));

	if (!down && ts._prevDown)
	{
		ts._currentLocation = { 0.f, 0.f };

		ts.is_flicking = false;
		ts.acceleration = 1.0;
		ts.ignore_stick_mode = false;
	}
	else
	{
		ts._currentLocation = { ts._currentLocation.x() + movX, ts._currentLocation.y() - movY };
	}

	ts._prevDown = down;
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

	// if (current.t0Down || previous.t0Down)
	//{
	//	DisplayTouchInfo(current.t0Down ? current.t0Id : previous.t0Id,
	//		current.t0Down ? optional<FloatXY>({ current.t0X, current.t0Y }) : nullopt,
	//		previous.t0Down ? optional<FloatXY>({ previous.t0X, previous.t0Y }) : nullopt);
	// }

	// if (current.t1Down || previous.t1Down)
	//{
	//	DisplayTouchInfo(current.t1Down ? current.t1Id : previous.t1Id,
	//		current.t1Down ? optional<FloatXY>({ current.t1X, current.t1Y }) : nullopt,
	//		previous.t1Down ? optional<FloatXY>({ previous.t1X, previous.t1Y }) : nullopt);
	// }

	shared_ptr<JoyShock> js = handle_to_joyshock[jcHandle];
	int tpSizeX, tpSizeY;
	if (!js || jsl->GetTouchpadDimension(jcHandle, tpSizeX, tpSizeY) == false)
		return;

	lock_guard guard(js->_context->callback_lock);

	TOUCH_POINT point0, point1;

	point0.posX = newState.t0Down ? newState.t0X : -1.f; // Absolute position in percentage
	point0.posY = newState.t0Down ? newState.t0Y : -1.f;
	point0.movX = prevState.t0Down ? int16_t((newState.t0X - prevState.t0X) * tpSizeX) : 0; // Relative movement in unit
	point0.movY = prevState.t0Down ? int16_t((newState.t0Y - prevState.t0Y) * tpSizeY) : 0;
	point1.posX = newState.t1Down ? newState.t1X : -1.f;
	point1.posY = newState.t1Down ? newState.t1Y : -1.f;
	point1.movX = prevState.t1Down ? int16_t((newState.t1X - prevState.t1X) * tpSizeX) : 0;
	point1.movY = prevState.t1Down ? int16_t((newState.t1Y - prevState.t1Y) * tpSizeY) : 0;

	auto mode = js->getSetting<TouchpadMode>(SettingID::TOUCHPAD_MODE);
	// js->handleButtonChange(ButtonID::TOUCH, point0.isDown() || point1.isDown()); // This is handled by dual stage "trigger" step
	if (!point0.isDown() && !point1.isDown())
	{

		static const std::function<bool(ButtonID)> IS_TOUCH_BUTTON = [](ButtonID id)
		{
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
		auto &grid_size = *SettingsManager::getV<FloatXY>(SettingID::GRID_SIZE);
		// Handle grid
		int index0 = -1, index1 = -1;
		if (point0.isDown())
		{
			float row = floorf(point0.posY * grid_size.value().y());
			float col = floorf(point0.posX * grid_size.value().x());
			// cout << "I should be in button " << row << " " << col << endl;
			index0 = int(row * grid_size.value().x() + col);
		}

		if (point1.isDown())
		{
			float row = floorf(point1.posY * grid_size.value().y());
			float col = floorf(point1.posX * grid_size.value().x());
			// cout << "I should be in button " << row << " " << col << endl;
			index1 = int(row * grid_size.value().x() + col);
		}

		for (size_t i = 0; i < grid_mappings.size(); ++i)
		{
			auto optId = magic_enum::enum_cast<ButtonID>(int(FIRST_TOUCH_BUTTON + i));

			// JSM can get touch button callbacks before the grid buttons are setup at startup. Just skip then.
			if (optId && js->gridButtons.size() == grid_mappings.size())
				js->handleButtonChange(*optId, i == index0 || i == index1);
		}

		// Handle stick
		js->handleTouchStickChange(js->touchpads[0], point0.isDown(), point0.movX, point0.movY, delta_time);
		js->handleTouchStickChange(js->touchpads[1], point1.isDown(), point1.movX, point1.movY, delta_time);
	}
	else if (mode == TouchpadMode::MOUSE)
	{
		// Disable gestures
		// if (point0.isDown() && point1.isDown())
		//{
		// if (prevState.t0Down && prevState.t1Down)
		//{
		//	float x = fabsf(newState.t0X - newState.t1X);
		//	float y = fabsf(newState.t0Y - newState.t1Y);
		//	float angle = atan2f(y, x) / PI * 360;
		//	float dist = sqrt(x * x + y * y);
		//	x = fabsf(prevState.t0X - prevState.t1X);
		//	y = fabsf(prevState.t0Y - prevState.t1Y);
		//	float oldAngle = atan2f(y, x) / PI * 360;
		//	float oldDist = sqrt(x * x + y * y);
		//	if (angle != oldAngle)
		//		DEBUG_LOG << "Angle went from " << oldAngle << " degrees to " << angle << " degress. Diff is " << angle - oldAngle << " degrees. ";
		//	js->touch_scroll_x.processScroll(angle - oldAngle, js->getSetting<FloatXY>(SettingID::SCROLL_SENS).x(), js->time_now);
		//	if (dist != oldDist)
		//		DEBUG_LOG << "Dist went from " << oldDist << " points to " << dist << " points. Diff is " << dist - oldDist << " points. ";
		//	js->touch_scroll_y.processScroll(dist - oldDist, js->getSetting<FloatXY>(SettingID::SCROLL_SENS).y(), js->time_now);
		//}
		// else
		//{
		//	js->touch_scroll_x.Reset(js->time_now);
		//	js->touch_scroll_y.Reset(js->time_now);
		//}
		//}
		// else
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
	else if (mode == TouchpadMode::PS_TOUCHPAD)
	{
		if (js->CheckVigemState())
		{
			std::optional<FloatXY> p0 = newState.t0Down ? std::make_optional<FloatXY>(newState.t0X, newState.t0Y) : std::nullopt;
			std::optional<FloatXY> p1 = newState.t1Down ? std::make_optional<FloatXY>(newState.t1X, newState.t1Y) : std::nullopt;
			js->_context->_vigemController->setTouchState(p0, p1);
		}
	}
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
	auto tick_time = *SettingsManager::get<float>(SettingID::TICK_TIME);
	static auto &right_trigger_offset = *SettingsManager::get<int>(SettingID::RIGHT_TRIGGER_OFFSET);
	static auto &right_trigger_range = *SettingsManager::get<int>(SettingID::RIGHT_TRIGGER_RANGE);
	static auto &left_trigger_offset = *SettingsManager::get<int>(SettingID::LEFT_TRIGGER_OFFSET);
	static auto &left_trigger_range = *SettingsManager::get<int>(SettingID::LEFT_TRIGGER_RANGE);
	switch (triggerCalibrationStep)
	{
	case 1:
		COUT << "Softly press on the right trigger until you just feel the resistance." << endl;
		COUT << "Then press the dpad down button to proceed, or press HOME to abandon." << endl;
		tick_time.set(100.f);
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
			right_trigger_offset.set(jc->right_effect.start);
			tick_time.set(40);
			triggerCalibrationStep++;
		}
		++jc->right_effect.start;
		break;
	case 4:
		DEBUG_LOG << "trigger pos is at " << int(rpos * 255.f) << " (" << int(rpos * 100.f) << "%) and effect pos is at " << int(jc->right_effect.start) << endl;
		if (int(rpos * 255.f) > 240)
		{
			tick_time.set(100);
			triggerCalibrationStep++;
		}
		++jc->right_effect.start;
		break;
	case 5:
		DEBUG_LOG << "trigger pos is at " << int(rpos * 255.f) << " (" << int(rpos * 100.f) << "%) and effect pos is at " << int(jc->right_effect.start) << endl;
		if (int(rpos * 255.f) == 255)
		{
			triggerCalibrationStep++;
			right_trigger_range.set(int(jc->right_effect.start - right_trigger_offset));
		}
		++jc->right_effect.start;
		break;
	case 6:
		COUT << "Softly press on the left trigger until you just feel the resistance." << endl;
		COUT << "Then press the cross button to proceed, or press HOME to abandon." << endl;
		tick_time.set(100);
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
			left_trigger_offset.set(jc->left_effect.start);
			tick_time.set(40);
			triggerCalibrationStep++;
		}
		++jc->left_effect.start;
		break;
	case 9:
		DEBUG_LOG << "trigger pos is at " << int(lpos * 255.f) << " (" << int(lpos * 100.f) << "%) and effect pos is at " << int(jc->left_effect.start) << endl;
		if (int(lpos * 255.f) > 240)
		{
			tick_time.set(100);
			triggerCalibrationStep++;
		}
		++jc->left_effect.start;
		break;
	case 10:
		DEBUG_LOG << "trigger pos is at " << int(lpos * 255.f) << " (" << int(lpos * 100.f) << "%) and effect pos is at " << int(jc->left_effect.start) << endl;
		if (int(lpos * 255.f) == 255)
		{
			triggerCalibrationStep++;
			left_trigger_range.set(int(jc->left_effect.start - left_trigger_offset));
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
		tick_time.reset();
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

	if (SettingsManager::getV<Switch>(SettingID::AUTO_CALIBRATE_GYRO)->value() == Switch::ON)
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
	// float angle = sqrtf(inGyroX * inGyroX + inGyroY * inGyroY + inGyroZ * inGyroZ) * PI / 180.f * deltaTime;
	// Vec normAxis = Vec(-inGyroX, -inGyroY, -inGyroZ).Normalized();
	// Quat reverseRotation = Quat(cosf(angle * 0.5f), normAxis.x, normAxis.y, normAxis.z);
	// reverseRotation.Normalize();
	// jc->lastGrav *= reverseRotation;
	// Vec newGrav = Vec(-imu.accelX, -imu.accelY, -imu.accelZ);
	// jc->lastGrav += (newGrav - jc->lastGrav) * 0.01f;
	//
	// Vec normFancyGrav = Vec(inGravX, inGravY, inGravZ).Normalized();
	// Vec normSimpleGrav = jc->lastGrav.Normalized();
	//
	// float gravAngleDiff = acosf(normFancyGrav.Dot(normSimpleGrav)) * 180.f / PI;

	// COUT << "Angle diff: " << gravAngleDiff << "\n\tFancy gravity: " << normFancyGrav.x << ", " << normFancyGrav.y << ", " << normFancyGrav.z << "\n\tSimple gravity: " << normSimpleGrav.x << ", " << normSimpleGrav.y << ", " << normSimpleGrav.z << "\n";
	// COUT << "Quat: " << inQuatW << ", " << inQuatX << ", " << inQuatY << ", " << inQuatZ << "\n";

	// inGravX = normSimpleGrav.x;
	// inGravY = normSimpleGrav.y;
	// inGravZ = normSimpleGrav.z;

	// COUT << "DS4 accel: %.4f, %.4f, %.4f\n", imuState.accelX, imuState.accelY, imuState.accelZ);
	// COUT << "\tDS4 gyro: %.4f, %.4f, %.4f\n", imuState.gyroX, imuState.gyroY, imuState.gyroZ);
	// COUT << "\tDS4 quat: %.4f, %.4f, %.4f, %.4f | accel: %.4f, %.4f, %.4f | grav: %.4f, %.4f, %.4f\n",
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
				// const float yawRelaxFactor = 1.41f; // 45 degree buffer
				// const float yawRelaxFactor = 1.15f; // 30 degree buffer
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
						// const float rollRelaxFactor = 2.f; // 60 degree buffer
						const float rollRelaxFactor = 1.41f; // 45 degree buffer
						// const float rollRelaxFactor = 1.15f; // 30 degree buffer
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
	auto tick_time = SettingsManager::get<float>(SettingID::TICK_TIME)->value();
	auto numGyroSamples = jc->getSetting(SettingID::GYRO_SMOOTH_TIME) * 1000.f / tick_time;
	if (numGyroSamples < 1)
		numGyroSamples = 1; // need at least 1 sample
	auto threshold = jc->getSetting(SettingID::GYRO_SMOOTH_THRESHOLD);
	jc->GetSmoothedGyro(gyroX, gyroY, gyroLength, threshold / 2.0f, threshold, int(numGyroSamples), gyroX, gyroY);
	// COUT << "%d Samples for threshold: %0.4f\n", numGyroSamples, gyro_smooth_threshold * maxSmoothingSamples);

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
			leftAny = leftLength > deadzoneInner;
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
			rightAny = rightLength > deadzoneInner;
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
	float gyro_x_sign_to_use = float(jc->getSetting<AxisMode>(SettingID::GYRO_AXIS_X));
	float gyro_y_sign_to_use = float(jc->getSetting<AxisMode>(SettingID::GYRO_AXIS_Y));

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

		jc->processStick(calX, calY, jc->left_stick, mouseCalibrationFactor, deltaTime, leftAny, lockMouse, camSpeedX, camSpeedY);
		jc->left_stick.lastX = calX;
		jc->left_stick.lastY = calY;
	}

	if (jc->controller_split_type != JS_SPLIT_TYPE_LEFT)
	{
		auto axisSign = jc->getSetting<AxisSignPair>(SettingID::RIGHT_STICK_AXIS);
		float calX = jsl->GetRightX(jc->handle) * float(axisSign.first);
		float calY = jsl->GetRightY(jc->handle) * float(axisSign.second);

		jc->processStick(calX, calY, jc->right_stick, mouseCalibrationFactor, deltaTime, rightAny, lockMouse, camSpeedX, camSpeedY);
		jc->right_stick.lastX = calX;
		jc->right_stick.lastY = calY;
	}

	if (jc->controller_split_type == JS_SPLIT_TYPE_FULL ||
		(jc->controller_split_type & (int)jc->getSetting<JoyconMask>(SettingID::JOYCON_MOTION_MASK)) == 0)
	{
		Quat neutralQuat = Quat(jc->neutralQuatW, jc->neutralQuatX, jc->neutralQuatY, jc->neutralQuatZ);
		Vec grav = Vec(inGravX, inGravY, inGravZ) * neutralQuat.Inverse();

		float lastCalX = jc->motion_stick.lastX;
		float lastCalY = jc->motion_stick.lastY;
		//float lastCalX = jc->motion_stick.lastX;
		//float lastCalY = jc->motion_stick.lastY;
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

		jc->processStick(calX, calY, jc->motion_stick, mouseCalibrationFactor, deltaTime, motionAny, lockMouse, camSpeedX, camSpeedY);
		jc->motion_stick.lastX = calX;
		jc->motion_stick.lastY = calY;

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
					// COUT << "LEAN ANGLE: " << (leanSign * absLeanAngle) << "    REMAPPED: " << (leanSign * remappedLeanAngle) << "     STICK OUT: " << signedStickValue << endl;
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
			jc->handleButtonChange(ButtonID::RSL, buttons & (1 << JSOFFSET_SL2)); // Xbox Elite back paddles
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
			jc->handleButtonChange(ButtonID::RSL, buttons & (1 << JSOFFSET_SL2));
			jc->handleButtonChange(ButtonID::RSR, buttons & (1 << JSOFFSET_SR2));
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
	                               [](const auto &pair)
	                               {
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
			processGyroStick(jc.get(), 0.f, 0.f, 0.f, StickMode::LEFT_STICK, false);
		}
		else if (gyroOutput == GyroOutput::RIGHT_STICK)
		{
			processGyroStick(jc.get(), 0.f, 0.f, 0.f, StickMode::RIGHT_STICK, false);
		}
		else if (gyroOutput == GyroOutput::PS_MOTION)
		{
			if (jc->CheckVigemState())
			{
				jc->_context->_vigemController->setGyro(imu.accelX, imu.accelY, imu.accelZ, imu.gyroX, imu.gyroY, imu.gyroZ);
			}
		}
	}

	// optionally ignore the gyro of one of the joycons
	if (!lockMouse && gyroOutput == GyroOutput::MOUSE &&
	  (jc->controller_split_type == JS_SPLIT_TYPE_FULL ||
	    (jc->controller_split_type & (int)jc->getSetting<JoyconMask>(SettingID::JOYCON_GYRO_MASK)) == 0))
	{
		// COUT << "GX: %0.4f GY: %0.4f GZ: %0.4f\n", imuState.gyroX, imuState.gyroY, imuState.gyroZ);
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
		tray->AddMenuItem(U("Reconnect controllers"), []()
		  { WriteToConsole("RECONNECT_CONTROLLERS"); });
		tray->AddMenuItem(
		  U("AutoLoad"), [](bool isChecked)
		  { SettingsManager::get<Switch>(SettingID::AUTOLOAD)->set(isChecked ? Switch::ON : Switch::OFF); },
		  bind(&PollingThread::isRunning, autoLoadThread.get()));

		if (whitelister && whitelister->IsAvailable())
		{
			tray->AddMenuItem(
			  U("Whitelist"), [](bool isChecked)
			  { isChecked ?
				  do_WHITELIST_ADD() :
				  do_WHITELIST_REMOVE(); },
			  bind(&Whitelister::operator bool, whitelister.get()));
		}
		tray->AddMenuItem(
		  U("Calibrate all devices"), [](bool isChecked)
		  { isChecked ?
			  WriteToConsole("RESTART_GYRO_CALIBRATION") :
			  WriteToConsole("FINISH_GYRO_CALIBRATION"); },
		  []()
		  { return devicesCalibrating; });

		string autoloadFolder{ AUTOLOAD_FOLDER() };
		for (auto file : ListDirectory(autoloadFolder.c_str()))
		{
			string fullPathName = ".\\AutoLoad\\" + file;
			auto noext = file.substr(0, file.find_last_of('.'));
			tray->AddMenuItem(U("AutoLoad folder"), UnicodeString(noext.begin(), noext.end()), [fullPathName]
			  {
				WriteToConsole(string(fullPathName.begin(), fullPathName.end()));
				autoLoadThread->Stop(); });
		}
		std::string gyroConfigsFolder{ GYRO_CONFIGS_FOLDER() };
		for (auto file : ListDirectory(gyroConfigsFolder.c_str()))
		{
			string fullPathName = ".\\GyroConfigs\\" + file;
			auto noext = file.substr(0, file.find_last_of('.'));
			tray->AddMenuItem(U("GyroConfigs folder"), UnicodeString(noext.begin(), noext.end()), [fullPathName]
			  {
				WriteToConsole(string(fullPathName.begin(), fullPathName.end()));
				autoLoadThread->Stop(); });
		}
		tray->AddMenuItem(U("Calculate RWC"), []()
		  {
			WriteToConsole("CALCULATE_REAL_WORLD_CALIBRATION");
			ShowConsole(); });
		tray->AddMenuItem(
		  U("Hide when minimized"), [](bool isChecked)
		  {
			  SettingsManager::get<Switch>(SettingID::HIDE_MINIMIZED)->set(isChecked ? Switch::ON : Switch::OFF);
			  if (!isChecked)
				  UnhideConsole(); },
		  bind(&PollingThread::isRunning, minimizeThread.get()));
		tray->AddMenuItem(U("Quit"), []()
		  { WriteToConsole("QUIT"); });
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
	auto sim_press_window = SettingsManager::getV<float>(SettingID::SIM_PRESS_WINDOW);
	if (sim_press_window && next <= sim_press_window->value())
	{
		CERR << SettingID::HOLD_PRESS_TIME << " can only be set to a value higher than " << SettingID::SIM_PRESS_WINDOW << " which is " << sim_press_window->value() << "ms." << endl;
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
	auto virtual_controller = SettingsManager::getV<ControllerScheme>(SettingID::VIRTUAL_CONTROLLER);
	if (next.hasViGEmBtn())
	{
		if (virtual_controller && virtual_controller->value() == ControllerScheme::NONE)
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
	auto virtual_controller = SettingsManager::getV<ControllerScheme>(SettingID::VIRTUAL_CONTROLLER);

	if (next == TriggerMode::X_LT || next == TriggerMode::X_RT)
	{
		if (virtual_controller && virtual_controller->value() == ControllerScheme::NONE)
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
	auto virtual_controller = SettingsManager::getV<ControllerScheme>(SettingID::VIRTUAL_CONTROLLER);
	if (next >= StickMode::LEFT_STICK && next <= StickMode::RIGHT_WIND_X)
	{
		if (virtual_controller && virtual_controller->value() == ControllerScheme::NONE)
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
	auto virtual_controller = SettingsManager::getV<ControllerScheme>(SettingID::VIRTUAL_CONTROLLER);
	if (next == GyroOutput::PS_MOTION && virtual_controller && virtual_controller->value() != ControllerScheme::DS4)
	{
		COUT_WARN << "Before using gyro mode PS_MOTION, you need to set ";
		COUT_INFO << "VIRTUAL_CONTROLLER = DS4" << endl;
		return current;
	}
	if (next == GyroOutput::LEFT_STICK || next == GyroOutput::RIGHT_STICK)
	{
		if (virtual_controller && virtual_controller->value() == ControllerScheme::NONE)
		{
			COUT_WARN << "Before using this gyro mode, you need to set VIRTUAL_CONTROLLER." << endl;
			return current;
		}
		for (auto &js : handle_to_joyshock)
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
		stickRingMode->set(RingMode::INNER);
	}
	else if (newValue == StickMode::OUTER_RING)
	{
		stickRingMode->set(RingMode::OUTER);
	}
}

ControllerScheme UpdateVirtualController(ControllerScheme prevScheme, ControllerScheme nextScheme)
{
	string error;
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
				success &= js.second->_context->_vigemController && js.second->_context->_vigemController->isInitialized(&error);
				if (!error.empty())
				{
					CERR << error << endl;
					error.clear();
				}
				if (!success)
				{
					js.second->_context->_vigemController.release();
					break;
				}
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
		for (auto id = FIRST_TOUCH_BUTTON + numberOfButtons; successfulRemove; ++id)
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
			touchButton.setFilter(&filterMapping);
			grid_mappings.push_back(touchButton);
			registry->add(new JSMAssignment<Mapping>(grid_mappings.back()));
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
	static auto left_stick_axis = SettingsManager::get<AxisSignPair>(SettingID::LEFT_STICK_AXIS);
	static auto right_stick_axis = SettingsManager::get<AxisSignPair>(SettingID::RIGHT_STICK_AXIS);
	static auto motion_stick_axis = SettingsManager::get<AxisSignPair>(SettingID::MOTION_STICK_AXIS);
	static auto touch_stick_axis = SettingsManager::get<AxisSignPair>(SettingID::TOUCH_STICK_AXIS);
	if (isVertical)
	{
		left_stick_axis->set(AxisSignPair{ left_stick_axis->value().first, newAxisMode });
		right_stick_axis->set(AxisSignPair{ right_stick_axis->value().first, newAxisMode });
		motion_stick_axis->set(AxisSignPair{ motion_stick_axis->value().first, newAxisMode });
		touch_stick_axis->set(AxisSignPair{ touch_stick_axis->value().first, newAxisMode });
	}
	else // is horizontal
	{
		left_stick_axis->set(AxisSignPair{ newAxisMode, left_stick_axis->value().second });
		right_stick_axis->set(AxisSignPair{ newAxisMode, right_stick_axis->value().second });
		motion_stick_axis->set(AxisSignPair{ newAxisMode, motion_stick_axis->value().second });
		touch_stick_axis->set(AxisSignPair{ newAxisMode, touch_stick_axis->value().second });
	}
}

class GyroSensAssignment : public JSMAssignment<FloatXY>
{
public:
	GyroSensAssignment(SettingID id, JSMSetting<FloatXY> &gyroSens)
	  : JSMAssignment(magic_enum::enum_name(id).data(), string(magic_enum::enum_name(gyroSens._id)), gyroSens)
	{
		// min and max gyro sens already have a listener
		gyroSens.removeOnChangeListener(_listenerId);
	}
};

class StickDeadzoneAssignment : public JSMAssignment<float>
{
public:
	StickDeadzoneAssignment(SettingID id, JSMSetting<float> &stickDeadzone)
	  : JSMAssignment(magic_enum::enum_name(id).data(), string(magic_enum::enum_name(stickDeadzone._id)), stickDeadzone)
	{
		// min and max gyro sens already have a listener
		stickDeadzone.removeOnChangeListener(_listenerId);
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
	  : GyroButtonAssignment(magic_enum::enum_name(id).data(), magic_enum::enum_name(id).data(), *SettingsManager::get<GyroSettings>(SettingID::GYRO_ON), always_off)
	{
	}

	GyroButtonAssignment *SetListener()
	{
		_listenerId = _var.addOnChangeListener(bind(&GyroButtonAssignment::DisplayNewValue, this, placeholders::_1));
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
			// Create Modeshift
			string name{ chord };
			(name += op) += _displayName;
			unique_ptr<JSMCommand> chordAssignment((new GyroButtonAssignment(_name, name, *settingVar->atChord(*optBtn), _always_off, *optBtn))->SetListener());
			chordAssignment->SetHelp(_help)->SetParser(bind(&GyroButtonAssignment::ModeshiftParser, *optBtn, settingVar, &_parse, placeholders::_1, placeholders::_2, placeholders::_3))->SetTaskOnDestruction(bind(&JSMSetting<GyroSettings>::processModeshiftRemoval, settingVar, *optBtn));
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
		stringstream ss(arguments.data());
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
			vector<string_view> list;
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
			vector<string_view> list;
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

// Contains all settings that can be modeshifted. They should be accessed only via Joyshock::getSetting
void initJsmSettings(CmdRegistry *commandRegistry)
{
	auto left_ring_mode = new JSMSetting<RingMode>(SettingID::LEFT_RING_MODE, RingMode::OUTER);
	left_ring_mode->setFilter(&filterInvalidValue<RingMode, RingMode::INVALID>);
	SettingsManager::add(left_ring_mode);
	commandRegistry->add((new JSMAssignment<RingMode>(*left_ring_mode))
	                       ->SetHelp("Pick a ring where to apply the LEFT_RING binding. Valid values are the following: INNER and OUTER."));

	auto left_stick_mode = new JSMSetting<StickMode>(SettingID::LEFT_STICK_MODE, StickMode::NO_MOUSE);
	left_stick_mode->setFilter(&filterStickMode);
	left_stick_mode->addOnChangeListener(bind(&UpdateRingModeFromStickMode, left_ring_mode, placeholders::_1));
	SettingsManager::add(left_stick_mode);
	commandRegistry->add((new JSMAssignment<StickMode>(*left_stick_mode))
	                       ->SetHelp("Set a mouse mode for the left stick. Valid values are the following:\nNO_MOUSE, AIM, FLICK, FLICK_ONLY, ROTATE_ONLY, MOUSE_RING, MOUSE_AREA, OUTER_RING, INNER_RING, SCROLL_WHEEL, LEFT_STICK, RIGHT_STICK"));

	auto right_ring_mode = new JSMSetting<RingMode>(SettingID::RIGHT_RING_MODE, RingMode::OUTER);
	right_ring_mode->setFilter(&filterInvalidValue<RingMode, RingMode::INVALID>);
	SettingsManager::add(right_ring_mode);
	commandRegistry->add((new JSMAssignment<RingMode>(*right_ring_mode))
	                       ->SetHelp("Pick a ring where to apply the RIGHT_RING binding. Valid values are the following: INNER and OUTER."));

	auto right_stick_mode = new JSMSetting<StickMode>(SettingID::RIGHT_STICK_MODE, StickMode::NO_MOUSE);
	right_stick_mode->setFilter(&filterStickMode);
	right_stick_mode->addOnChangeListener(bind(&UpdateRingModeFromStickMode, right_ring_mode, ::placeholders::_1));
	SettingsManager::add(right_stick_mode);
	commandRegistry->add((new JSMAssignment<StickMode>(*right_stick_mode))
	                       ->SetHelp("Set a mouse mode for the right stick. Valid values are the following:\nNO_MOUSE, AIM, FLICK, FLICK_ONLY, ROTATE_ONLY, MOUSE_RING, MOUSE_AREA, OUTER_RING, INNER_RING LEFT_STICK, RIGHT_STICK"));

	auto motion_ring_mode = new JSMSetting<RingMode>(SettingID::MOTION_RING_MODE, RingMode::OUTER);
	motion_ring_mode->setFilter(&filterInvalidValue<RingMode, RingMode::INVALID>);
	SettingsManager::add(motion_ring_mode);
	commandRegistry->add((new JSMAssignment<RingMode>(*motion_ring_mode))
	                       ->SetHelp("Pick a ring where to apply the MOTION_RING binding. Valid values are the following: INNER and OUTER."));

	auto motion_stick_mode = new JSMSetting<StickMode>(SettingID::MOTION_STICK_MODE, StickMode::NO_MOUSE);
	motion_stick_mode->setFilter(&filterMotionStickMode);
	motion_stick_mode->addOnChangeListener(bind(&UpdateRingModeFromStickMode, motion_ring_mode, ::placeholders::_1));
	SettingsManager::add(motion_stick_mode);
	commandRegistry->add((new JSMAssignment<StickMode>(*motion_stick_mode))
	                       ->SetHelp("Set a mouse mode for the motion-stick -- the whole controller is treated as a stick. Valid values are the following:\nNO_MOUSE, AIM, FLICK, FLICK_ONLY, ROTATE_ONLY, MOUSE_RING, MOUSE_AREA, OUTER_RING, INNER_RING LEFT_STICK, RIGHT_STICK"));

	auto mouse_x_from_gyro = new JSMSetting<GyroAxisMask>(SettingID::MOUSE_X_FROM_GYRO_AXIS, GyroAxisMask::Y);
	mouse_x_from_gyro->setFilter(&filterInvalidValue<GyroAxisMask, GyroAxisMask::INVALID>);
	SettingsManager::add(mouse_x_from_gyro);
	commandRegistry->add((new JSMAssignment<GyroAxisMask>(*mouse_x_from_gyro))
	                       ->SetHelp("Pick a gyro axis to operate on the mouse's X axis. Valid values are the following: X, Y and Z."));

	auto mouse_y_from_gyro = new JSMSetting<GyroAxisMask>(SettingID::MOUSE_Y_FROM_GYRO_AXIS, GyroAxisMask::X);
	mouse_y_from_gyro->setFilter(&filterInvalidValue<GyroAxisMask, GyroAxisMask::INVALID>);
	SettingsManager::add(mouse_y_from_gyro);
	commandRegistry->add((new JSMAssignment<GyroAxisMask>(*mouse_y_from_gyro))
	                       ->SetHelp("Pick a gyro axis to operate on the mouse's Y axis. Valid values are the following: X, Y and Z."));

	auto gyro_settings = new JSMSetting<GyroSettings>(SettingID::GYRO_ON, GyroSettings());
	gyro_settings->setFilter([](GyroSettings current, GyroSettings next)
	  { return next.ignore_mode != GyroIgnoreMode::INVALID ? next : current; });
	SettingsManager::add(gyro_settings);
	commandRegistry->add((new GyroButtonAssignment(SettingID::GYRO_OFF, false))
	                       ->SetHelp("Assign a controller button to disable the gyro when pressed."));
	commandRegistry->add((new GyroButtonAssignment(SettingID::GYRO_ON, true))->SetListener() // Set only one listener
	                       ->SetHelp("Assign a controller button to enable the gyro when pressed."));

	auto joycon_gyro_mask = new JSMSetting<JoyconMask>(SettingID::JOYCON_GYRO_MASK, JoyconMask::IGNORE_LEFT);
	joycon_gyro_mask->setFilter(&filterInvalidValue<JoyconMask, JoyconMask::INVALID>);
	SettingsManager::add(joycon_gyro_mask);
	commandRegistry->add((new JSMAssignment<JoyconMask>(*joycon_gyro_mask))
	                       ->SetHelp("When using two Joycons, select which one will be used for gyro. Valid values are the following:\nUSE_BOTH, IGNORE_LEFT, IGNORE_RIGHT, IGNORE_BOTH"));

	auto joycon_motion_mask = new JSMSetting<JoyconMask>(SettingID::JOYCON_MOTION_MASK, JoyconMask::IGNORE_RIGHT);
	joycon_motion_mask->setFilter(&filterInvalidValue<JoyconMask, JoyconMask::INVALID>);
	SettingsManager::add(joycon_motion_mask);
	commandRegistry->add((new JSMAssignment<JoyconMask>(*joycon_motion_mask))
	                       ->SetHelp("When using two Joycons, select which one will be used for non-gyro motion. Valid values are the following:\nUSE_BOTH, IGNORE_LEFT, IGNORE_RIGHT, IGNORE_BOTH"));

	auto zlMode = new JSMSetting<TriggerMode>(SettingID::ZL_MODE, TriggerMode::NO_FULL);
	zlMode->setFilter(&filterTriggerMode);
	SettingsManager::add(zlMode);
	commandRegistry->add((new JSMAssignment<TriggerMode>(*zlMode))
	                       ->SetHelp("Controllers with a right analog trigger can use one of the following dual stage trigger modes:\nNO_FULL, NO_SKIP, MAY_SKIP, MUST_SKIP, MAY_SKIP_R, MUST_SKIP_R, NO_SKIP_EXCLUSIVE, X_LT, X_RT, PS_L2, PS_R2"));

	auto zrMode = new JSMSetting<TriggerMode>(SettingID::ZR_MODE, TriggerMode::NO_FULL);
	zrMode->setFilter(&filterTriggerMode);
	SettingsManager::add(zrMode);
	commandRegistry->add((new JSMAssignment<TriggerMode>(*zrMode))
	                       ->SetHelp("Controllers with a left analog trigger can use one of the following dual stage trigger modes:\nNO_FULL, NO_SKIP, MAY_SKIP, MUST_SKIP, MAY_SKIP_R, MUST_SKIP_R, NO_SKIP_EXCLUSIVE, X_LT, X_RT, PS_L2, PS_R2"));

	auto flick_snap_mode = new JSMSetting<FlickSnapMode>(SettingID::FLICK_SNAP_MODE, FlickSnapMode::NONE);
	flick_snap_mode->setFilter(&filterInvalidValue<FlickSnapMode, FlickSnapMode::INVALID>);
	SettingsManager::add(flick_snap_mode);
	commandRegistry->add((new JSMAssignment<FlickSnapMode>(*flick_snap_mode))
	                       ->SetHelp("Snap flicks to cardinal directions. Valid values are the following: NONE or 0, FOUR or 4 and EIGHT or 8."));

	auto min_gyro_sens = new JSMSetting<FloatXY>(SettingID::MIN_GYRO_SENS, { 0.0f, 0.0f });
	min_gyro_sens->setFilter(&filterFloatPair);
	SettingsManager::add(min_gyro_sens);
	commandRegistry->add((new JSMAssignment<FloatXY>(*min_gyro_sens))
	                       ->SetHelp("Minimum gyro sensitivity when turning controller at or below MIN_GYRO_THRESHOLD.\nYou can assign a second value as a different vertical sensitivity."));

	auto max_gyro_sens = new JSMSetting<FloatXY>(SettingID::MAX_GYRO_SENS, { 0.0f, 0.0f });
	max_gyro_sens->setFilter(&filterFloatPair);
	SettingsManager::add(max_gyro_sens);
	commandRegistry->add((new JSMAssignment<FloatXY>(*max_gyro_sens))
	                       ->SetHelp("Maximum gyro sensitivity when turning controller at or above MAX_GYRO_THRESHOLD.\nYou can assign a second value as a different vertical sensitivity."));

	commandRegistry->add((new GyroSensAssignment(SettingID::GYRO_SENS, *max_gyro_sens))->SetHelp(""));
	commandRegistry->add((new GyroSensAssignment(SettingID::GYRO_SENS, *min_gyro_sens))
	                       ->SetHelp("Sets a gyro sensitivity to use. This sets both MIN_GYRO_SENS and MAX_GYRO_SENS to the same values. You can assign a second value as a different vertical sensitivity."));

	auto min_gyro_threshold = new JSMSetting<float>(SettingID::MIN_GYRO_THRESHOLD, 0.0f);
	min_gyro_threshold->setFilter(&filterFloat);
	SettingsManager::add(min_gyro_threshold);
	commandRegistry->add((new JSMAssignment<float>(*min_gyro_threshold))
	                       ->SetHelp("Degrees per second at and below which to apply minimum gyro sensitivity."));

	auto max_gyro_threshold = new JSMSetting<float>(SettingID::MAX_GYRO_THRESHOLD, 0.0f);
	max_gyro_threshold->setFilter(&filterFloat);
	SettingsManager::add(max_gyro_threshold);
	commandRegistry->add((new JSMAssignment<float>(*max_gyro_threshold))
	                       ->SetHelp("Degrees per second at and above which to apply maximum gyro sensitivity."));

	auto stick_power = new JSMSetting<float>(SettingID::STICK_POWER, 1.0f);
	stick_power->setFilter(&filterFloat);
	SettingsManager::add(stick_power);
	commandRegistry->add((new JSMAssignment<float>(*stick_power))
	                       ->SetHelp("Power curve for stick input when in AIM mode. 1 for linear, 0 for no curve (full strength once out of deadzone). Higher numbers make more of the stick's range appear like a very slight tilt."));

	auto stick_sens = new JSMSetting<FloatXY>(SettingID::STICK_SENS, { 360.0f, 360.0f });
	stick_sens->setFilter(&filterFloatPair);
	SettingsManager::add(stick_sens);
	commandRegistry->add((new JSMAssignment<FloatXY>(*stick_sens))
	                      ->SetHelp("Stick sensitivity when using classic AIM mode."));

	auto real_world_calibration = new JSMSetting<float>(SettingID::REAL_WORLD_CALIBRATION, 40.0f);
	real_world_calibration->setFilter(&filterFloat);
	SettingsManager::add(real_world_calibration);
	commandRegistry->add((new JSMAssignment<float>(*real_world_calibration))
	                       ->SetHelp("Calibration value mapping mouse values to in game degrees. This value is used for FLICK mode, and to make GYRO and stick AIM sensitivities use real world values."));

	auto virtual_stick_calibration = new JSMSetting<float>(SettingID::VIRTUAL_STICK_CALIBRATION, 360.0f);
	virtual_stick_calibration->setFilter(&filterFloat);
	SettingsManager::add(virtual_stick_calibration);
	commandRegistry->add((new JSMAssignment<float>(*virtual_stick_calibration))
	                       ->SetHelp("With a virtual controller, how fast a full tilt of the stick will turn the controller, in degrees per second. This value is used for FLICK mode with virtual controllers and to make GYRO sensitivities use real world values."));

	auto in_game_sens = new JSMSetting<float>(SettingID::IN_GAME_SENS, 1.0f);
	in_game_sens->setFilter(bind(&fmaxf, 0.0001f, ::placeholders::_2));
	SettingsManager::add(in_game_sens);
	commandRegistry->add((new JSMAssignment<float>(*in_game_sens))
	                       ->SetHelp("Set this value to the sensitivity you use in game. It is used by stick FLICK and AIM modes as well as GYRO aiming."));

	auto trigger_threshold = new JSMSetting<float>(SettingID::TRIGGER_THRESHOLD, 0.0f);
	trigger_threshold->setFilter(&filterFloat);
	SettingsManager::add(trigger_threshold);
	commandRegistry->add((new JSMAssignment<float>(*trigger_threshold))
	                       ->SetHelp("Set this to a value between 0 and 1. This is the threshold at which a soft press binding is triggered. Or set the value to -1 to use hair trigger mode"));

	auto left_stick_axis = new JSMSetting<AxisSignPair>(SettingID::LEFT_STICK_AXIS, { AxisMode::STANDARD, AxisMode::STANDARD });
	left_stick_axis->setFilter(&filterSignPair);
	SettingsManager::add(left_stick_axis);
	commandRegistry->add((new JSMAssignment<AxisSignPair>(*left_stick_axis))
	                       ->SetHelp("When in AIM mode, set stick X axis inversion. Valid values are the following:\nSTANDARD or 1, and INVERTED or -1"));

	auto right_stick_axis = new JSMSetting<AxisSignPair>(SettingID::RIGHT_STICK_AXIS, { AxisMode::STANDARD, AxisMode::STANDARD });
	right_stick_axis->setFilter(&filterSignPair);
	SettingsManager::add(right_stick_axis);
	commandRegistry->add((new JSMAssignment<AxisSignPair>(*right_stick_axis))
	                       ->SetHelp("When in AIM mode, set stick X axis inversion. Valid values are the following:\nSTANDARD or 1, and INVERTED or -1"));

	auto motion_stick_axis = new JSMSetting<AxisSignPair>(SettingID::MOTION_STICK_AXIS, { AxisMode::STANDARD, AxisMode::STANDARD });
	motion_stick_axis->setFilter(&filterSignPair);
	SettingsManager::add(motion_stick_axis);
	commandRegistry->add((new JSMAssignment<AxisSignPair>(*motion_stick_axis))
	                       ->SetHelp("When in AIM mode, set stick X axis inversion. Valid values are the following:\nSTANDARD or 1, and INVERTED or -1"));

	auto touch_stick_axis = new JSMSetting<AxisSignPair>(SettingID::TOUCH_STICK_AXIS, { AxisMode::STANDARD, AxisMode::STANDARD });
	touch_stick_axis->setFilter(&filterSignPair);
	SettingsManager::add(touch_stick_axis);
	commandRegistry->add((new JSMAssignment<AxisSignPair>(*touch_stick_axis))
	                       ->SetHelp("When in AIM mode, set stick X axis inversion. Valid values are the following:\nSTANDARD or 1, and INVERTED or -1"));

	// Legacy command
	auto aim_x_sign = new JSMSetting<AxisMode>(SettingID::STICK_AXIS_X, AxisMode::STANDARD);
	aim_x_sign->setFilter(&filterInvalidValue<AxisMode, AxisMode::INVALID>)->addOnChangeListener(bind(OnNewStickAxis, placeholders::_1, false));
	SettingsManager::add(aim_x_sign);
	commandRegistry->add(new JSMAssignment<AxisMode>(*aim_x_sign, true));

	// Legacy command
	auto aim_y_sign = new JSMSetting<AxisMode>(SettingID::STICK_AXIS_Y, AxisMode::STANDARD);
	aim_y_sign->setFilter(&filterInvalidValue<AxisMode, AxisMode::INVALID>)->addOnChangeListener(bind(OnNewStickAxis, placeholders::_1, true));
	SettingsManager::add(aim_y_sign);
	commandRegistry->add(new JSMAssignment<AxisMode>(*aim_y_sign, true));

	auto gyro_x_sign = new JSMSetting<AxisMode>(SettingID::GYRO_AXIS_Y, AxisMode::STANDARD);
	gyro_x_sign->setFilter(&filterInvalidValue<AxisMode, AxisMode::INVALID>);
	SettingsManager::add(gyro_x_sign);
	commandRegistry->add((new JSMAssignment<AxisMode>(*gyro_x_sign))
	                       ->SetHelp("Set gyro X axis inversion. Valid values are the following:\nSTANDARD or 1, and INVERTED or -1"));

	auto gyro_y_sign = new JSMSetting<AxisMode>(SettingID::GYRO_AXIS_X, AxisMode::STANDARD);
	gyro_y_sign->setFilter(&filterInvalidValue<AxisMode, AxisMode::INVALID>);
	SettingsManager::add(gyro_y_sign);
	commandRegistry->add((new JSMAssignment<AxisMode>(*gyro_y_sign))
	                       ->SetHelp("Set gyro Y axis inversion. Valid values are the following:\nSTANDARD or 1, and INVERTED or -1"));

	auto flick_time = new JSMSetting<float>(SettingID::FLICK_TIME, 0.1f);
	flick_time->setFilter(bind(&fmaxf, 0.0001f, ::placeholders::_2));
	SettingsManager::add(flick_time);
	commandRegistry->add((new JSMAssignment<float>(*flick_time))
	                       ->SetHelp("Sets how long a flick takes in seconds. This value is used by stick FLICK mode."));

	auto flick_time_exponent = new JSMSetting<float>(SettingID::FLICK_TIME_EXPONENT, 0.0f);
	flick_time_exponent->setFilter(&filterFloat);
	SettingsManager::add(flick_time_exponent);
	commandRegistry->add((new JSMAssignment<float>(*flick_time_exponent))
	                       ->SetHelp("Applies a delta exponent to flick_time, effectively making flick speed depend on the flick angle: use 0 for no effect and 1 for linear. This value is used by stick FLICK mode."));

	auto gyro_smooth_time = new JSMSetting<float>(SettingID::GYRO_SMOOTH_TIME, 0.125f);
	gyro_smooth_time->setFilter(bind(&fmaxf, 0.0001f, ::placeholders::_2));
	SettingsManager::add(gyro_smooth_time);
	commandRegistry->add((new JSMAssignment<float>(*gyro_smooth_time))
	                       ->SetHelp("This length of the smoothing window in seconds. Smoothing is only applied below the GYRO_SMOOTH_THRESHOLD, with a smooth transition to full smoothing."));

	auto gyro_smooth_threshold = new JSMSetting<float>(SettingID::GYRO_SMOOTH_THRESHOLD, 0.0f);
	gyro_smooth_threshold->setFilter(&filterPositive);
	SettingsManager::add(gyro_smooth_threshold);
	commandRegistry->add((new JSMAssignment<float>(*gyro_smooth_threshold))
	                       ->SetHelp("When the controller's angular velocity is below this threshold (in degrees per second), smoothing will be applied."));

	auto gyro_cutoff_speed = new JSMSetting<float>(SettingID::GYRO_CUTOFF_SPEED, 0.0f);
	gyro_cutoff_speed->setFilter(&filterPositive);
	SettingsManager::add(gyro_cutoff_speed);
	commandRegistry->add((new JSMAssignment<float>(*gyro_cutoff_speed))
	                       ->SetHelp("Gyro deadzone. Gyro input will be ignored when below this angular velocity (in degrees per second). This should be a last-resort stability option."));

	auto gyro_cutoff_recovery = new JSMSetting<float>(SettingID::GYRO_CUTOFF_RECOVERY, 0.0f);
	gyro_cutoff_recovery->setFilter(&filterPositive);
	SettingsManager::add(gyro_cutoff_recovery);
	commandRegistry->add((new JSMAssignment<float>(*gyro_cutoff_recovery))
	                       ->SetHelp("Below this threshold (in degrees per second), gyro sensitivity is pushed down towards zero. This can tighten and steady aim without a deadzone."));

	auto stick_acceleration_rate = new JSMSetting<float>(SettingID::STICK_ACCELERATION_RATE, 0.0f);
	stick_acceleration_rate->setFilter(&filterPositive);
	SettingsManager::add(stick_acceleration_rate);
	commandRegistry->add((new JSMAssignment<float>(*stick_acceleration_rate))
	                       ->SetHelp("When in AIM mode and the stick is fully tilted, stick sensitivity increases over time. This is a multiplier starting at 1x and increasing this by this value per second."));

	auto stick_acceleration_cap = new JSMSetting<float>(SettingID::STICK_ACCELERATION_CAP, 1000000.0f);
	stick_acceleration_cap->setFilter(bind(&fmaxf, 1.0f, ::placeholders::_2));
	SettingsManager::add(stick_acceleration_cap);
	commandRegistry->add((new JSMAssignment<float>(*stick_acceleration_cap))
	                       ->SetHelp("When in AIM mode and the stick is fully tilted, stick sensitivity increases over time. This value is the maximum sensitivity multiplier."));

	auto left_stick_deadzone_inner = new JSMSetting<float>(SettingID::LEFT_STICK_DEADZONE_INNER, 0.15f);
	left_stick_deadzone_inner->setFilter(&filterClamp01);
	SettingsManager::add(left_stick_deadzone_inner);
	commandRegistry->add((new JSMAssignment<float>(*left_stick_deadzone_inner))
	                       ->SetHelp("Defines a radius of the left stick within which all values will be ignored. This value can only be between 0 and 1 but it should be small. Stick input out of this radius will be adjusted."));

	auto left_stick_deadzone_outer = new JSMSetting<float>(SettingID::LEFT_STICK_DEADZONE_OUTER, 0.1f);
	left_stick_deadzone_outer->setFilter(&filterClamp01);
	SettingsManager::add(left_stick_deadzone_outer);
	commandRegistry->add((new JSMAssignment<float>(*left_stick_deadzone_outer))
	                       ->SetHelp("Defines a distance from the left stick's outer edge for which the stick will be considered fully tilted. This value can only be between 0 and 1 but it should be small. Stick input out of this deadzone will be adjusted."));

	auto flick_deadzone_angle = new JSMSetting<float>(SettingID::FLICK_DEADZONE_ANGLE, 0.0f);
	flick_deadzone_angle->setFilter(&filterPositive);
	SettingsManager::add(flick_deadzone_angle);
	commandRegistry->add((new JSMAssignment<float>(*flick_deadzone_angle))
	                       ->SetHelp("Defines a minimum angle (in degrees) for the flick to be considered a flick. Helps ignore unintentional turns when tilting the stick straight forward."));

	auto right_stick_deadzone_inner = new JSMSetting<float>(SettingID::RIGHT_STICK_DEADZONE_INNER, 0.15f);
	right_stick_deadzone_inner->setFilter(&filterClamp01);
	SettingsManager::add(right_stick_deadzone_inner);
	commandRegistry->add((new JSMAssignment<float>(*right_stick_deadzone_inner))
	                       ->SetHelp("Defines a radius of the right stick within which all values will be ignored. This value can only be between 0 and 1 but it should be small. Stick input out of this radius will be adjusted."));

	auto right_stick_deadzone_outer = new JSMSetting<float>(SettingID::RIGHT_STICK_DEADZONE_OUTER, 0.1f);
	right_stick_deadzone_outer->setFilter(&filterClamp01);
	SettingsManager::add(right_stick_deadzone_outer);
	commandRegistry->add((new JSMAssignment<float>(*right_stick_deadzone_outer))
	                       ->SetHelp("Defines a distance from the right stick's outer edge for which the stick will be considered fully tilted. This value can only be between 0 and 1 but it should be small. Stick input out of this deadzone will be adjusted."));

	commandRegistry->add((new StickDeadzoneAssignment(SettingID::STICK_DEADZONE_INNER, *right_stick_deadzone_inner))->SetHelp(""));
	commandRegistry->add((new StickDeadzoneAssignment(SettingID::STICK_DEADZONE_INNER, *left_stick_deadzone_inner))->SetHelp(""));

	commandRegistry->add((new StickDeadzoneAssignment(SettingID::STICK_DEADZONE_OUTER, *right_stick_deadzone_outer))->SetHelp(""));
	commandRegistry->add((new StickDeadzoneAssignment(SettingID::STICK_DEADZONE_OUTER, *left_stick_deadzone_outer))
	                       ->SetHelp("Defines a distance from both sticks' outer edge for which the stick will be considered fully tilted. This value can only be between 0 and 1 but it should be small. Stick input out of this deadzone will be adjusted."));

	auto motion_deadzone_inner = new JSMSetting<float>(SettingID::MOTION_DEADZONE_INNER, 15.f);
	motion_deadzone_inner->setFilter(&filterPositive);
	SettingsManager::add(motion_deadzone_inner);
	commandRegistry->add((new JSMAssignment<float>(*motion_deadzone_inner))
	                       ->SetHelp("Defines a radius of the motion-stick within which all values will be ignored. This value can only be between 0 and 1 but it should be small. Stick input out of this radius will be adjusted."));

	auto motion_deadzone_outer = new JSMSetting<float>(SettingID::MOTION_DEADZONE_OUTER, 135.f);
	motion_deadzone_outer->setFilter(&filterPositive);
	SettingsManager::add(motion_deadzone_outer);
	commandRegistry->add((new JSMAssignment<float>(*motion_deadzone_outer))
	                       ->SetHelp("Defines a distance from the motion-stick's outer edge for which the stick will be considered fully tilted. Stick input out of this deadzone will be adjusted."));

	auto angle_to_axis_deadzone_inner = new JSMSetting<float>(SettingID::ANGLE_TO_AXIS_DEADZONE_INNER, 0.f);
	angle_to_axis_deadzone_inner->setFilter(&filterPositive);
	SettingsManager::add(angle_to_axis_deadzone_inner);
	commandRegistry->add((new JSMAssignment<float>(*angle_to_axis_deadzone_inner))
	                       ->SetHelp("Defines an angle within which _ANGLE_TO_X and _ANGLE_TO_Y stick modes will be ignored (in degrees). Since a circular deadzone is already used for deciding whether the stick is engaged at all, it's recommended not to use an inner angular deadzone, which is why the default value is 0."));

	auto angle_to_axis_deadzone_outer = new JSMSetting<float>(SettingID::ANGLE_TO_AXIS_DEADZONE_OUTER, 10.f);
	angle_to_axis_deadzone_outer->setFilter(&filterPositive);
	SettingsManager::add(angle_to_axis_deadzone_outer);
	commandRegistry->add((new JSMAssignment<float>(*angle_to_axis_deadzone_outer))
	                       ->SetHelp("Defines an angle from max or min rotation that will be treated as max or min rotation, respectively, for _ANGLE_TO_X and _ANGLE_TO_Y stick modes. Since players intending to point the stick perfectly up/down or perfectly left/right will usually be off by a few degrees, this enables players to more easily hit their intended min/max values, so the default value is 10 degrees."));

	auto lean_threshold = new JSMSetting<float>(SettingID::LEAN_THRESHOLD, 15.f);
	lean_threshold->setFilter(&filterPositive);
	SettingsManager::add(lean_threshold);
	commandRegistry->add((new JSMAssignment<float>(*lean_threshold))
	                       ->SetHelp("How far the controller must be leaned left or right to trigger a LEAN_LEFT or LEAN_RIGHT binding."));

	auto controller_orientation = new JSMSetting<ControllerOrientation>(SettingID::CONTROLLER_ORIENTATION, ControllerOrientation::FORWARD);
	controller_orientation->setFilter(&filterInvalidValue<ControllerOrientation, ControllerOrientation::INVALID>);
	SettingsManager::add(controller_orientation);
	commandRegistry->add((new JSMAssignment<ControllerOrientation>(*controller_orientation))
	                       ->SetHelp("Let the stick modes account for how you're holding the controller:\nFORWARD, LEFT, RIGHT, BACKWARD"));

	auto gyro_space = new JSMSetting<GyroSpace>(SettingID::GYRO_SPACE, GyroSpace::LOCAL);
	gyro_space->setFilter(&filterInvalidValue<GyroSpace, GyroSpace::INVALID>);
	SettingsManager::add(gyro_space);
	commandRegistry->add((new JSMAssignment<GyroSpace>(*gyro_space))
	                       ->SetHelp("How gyro input is converted to 2D input. With LOCAL, your MOUSE_X_FROM_GYRO_AXIS and MOUSE_Y_FROM_GYRO_AXIS settings decide which local angular axis maps to which 2D mouse axis.\nYour other options are PLAYER_TURN and PLAYER_LEAN. These both take gravity into account to combine your axes more reliably.\n\tUse PLAYER_TURN if you like to turn your camera or move your cursor by turning your controller side to side.\n\tUse PLAYER_LEAN if you'd rather lean your controller to turn the camera."));

	auto trackball_decay = new JSMSetting<float>(SettingID::TRACKBALL_DECAY, 1.0f);
	trackball_decay->setFilter(&filterPositive);
	SettingsManager::add(trackball_decay);
	commandRegistry->add((new JSMAssignment<float>(*trackball_decay))
	                       ->SetHelp("Choose the rate at which trackball gyro slows down. 0 means no decay, 1 means it'll halve each second, 2 to halve each 1/2 seconds, etc."));

	auto screen_resolution_x = new JSMSetting<float>(SettingID::SCREEN_RESOLUTION_X, 1920.0f);
	screen_resolution_x->setFilter(&filterPositive);
	SettingsManager::add(screen_resolution_x);
	commandRegistry->add((new JSMAssignment<float>(*screen_resolution_x))
	                       ->SetHelp("Indicate your monitor's horizontal resolution when using the stick mode MOUSE_RING."));

	auto screen_resolution_y = new JSMSetting<float>(SettingID::SCREEN_RESOLUTION_Y, 1080.0f);
	screen_resolution_y->setFilter(&filterPositive);
	SettingsManager::add(screen_resolution_y);
	commandRegistry->add((new JSMAssignment<float>(*screen_resolution_y))
	                       ->SetHelp("Indicate your monitor's vertical resolution when using the stick mode MOUSE_RING."));

	auto mouse_ring_radius = new JSMSetting<float>(SettingID::MOUSE_RING_RADIUS, 128.0f);
	mouse_ring_radius->setFilter([](float c, float n) -> float
	  { return n <= SettingsManager::get<float>(SettingID::SCREEN_RESOLUTION_Y)->value() ? floorf(n) : c; });
	SettingsManager::add(mouse_ring_radius);
	commandRegistry->add((new JSMAssignment<float>(*mouse_ring_radius))
	                       ->SetHelp("Pick a radius on which the cursor will be allowed to move. This value is used for stick mode MOUSE_RING and MOUSE_AREA."));

	auto rotate_smooth_override = new JSMSetting<float>(SettingID::ROTATE_SMOOTH_OVERRIDE, -1.0f);
	// No filtering needed for rotate_smooth_override
	SettingsManager::add(rotate_smooth_override);
	commandRegistry->add((new JSMAssignment<float>(*rotate_smooth_override))
	                       ->SetHelp("Some smoothing is applied to flick stick rotations to account for the controller's stick resolution. This value overrides the smoothing threshold."));

	auto flick_snap_strength = new JSMSetting<float>(SettingID::FLICK_SNAP_STRENGTH, 01.0f);
	flick_snap_strength->setFilter(&filterClamp01);
	SettingsManager::add(flick_snap_strength);
	commandRegistry->add((new JSMAssignment<float>(*flick_snap_strength))
	                       ->SetHelp("If FLICK_SNAP_MODE is set to something other than NONE, this sets the degree of snapping -- 0 for none, 1 for full snapping to the nearest direction, and values in between will bias you towards the nearest direction instead of snapping."));

	auto trigger_skip_delay = new JSMSetting<float>(SettingID::TRIGGER_SKIP_DELAY, 150.0f);
	trigger_skip_delay->setFilter(&filterPositive);
	SettingsManager::add(trigger_skip_delay);
	commandRegistry->add((new JSMAssignment<float>(*trigger_skip_delay))
	                       ->SetHelp("Sets the amount of time in milliseconds within which the user needs to reach the full press to skip the soft pull binding of the trigger."));

	auto turbo_period = new JSMSetting<float>(SettingID::TURBO_PERIOD, 80.0f);
	turbo_period->setFilter(&filterPositive);
	SettingsManager::add(turbo_period);
	commandRegistry->add((new JSMAssignment<float>(*turbo_period))
	                       ->SetHelp("Sets the time in milliseconds to wait between each turbo activation."));

	auto hold_press_time = new JSMSetting<float>(SettingID::HOLD_PRESS_TIME, 150.0f);
	hold_press_time->setFilter(&filterHoldPressDelay);
	SettingsManager::add(hold_press_time);
	commandRegistry->add((new JSMAssignment<float>(*hold_press_time))
	                       ->SetHelp("Sets the amount of time in milliseconds to hold a button before the hold press is enabled. Releasing the button before this time will trigger the tap press. Turbo press only starts after this delay."));

	auto sim_press_window = new JSMVariable<float>(50.0f);
	sim_press_window->setFilter(&filterPositive);
	SettingsManager::add(SettingID::SIM_PRESS_WINDOW, sim_press_window);
	commandRegistry->add((new JSMAssignment<float>("SIM_PRESS_WINDOW", *sim_press_window))
	                       ->SetHelp("Sets the amount of time in milliseconds within which both buttons of a simultaneous press needs to be pressed before enabling the sim press mappings. This setting does not support modeshift."));

	auto dbl_press_window = new JSMSetting<float>(SettingID::DBL_PRESS_WINDOW, 150.0f);
	dbl_press_window->setFilter(&filterPositive);
	SettingsManager::add(dbl_press_window);
	commandRegistry->add((new JSMAssignment<float>("DBL_PRESS_WINDOW", *dbl_press_window))
	                       ->SetHelp("Sets the amount of time in milliseconds within which the user needs to press a button twice before enabling the double press mappings. This setting does not support modeshift."));

	auto tick_time = new JSMSetting<float>(SettingID::TICK_TIME, 3);
	tick_time->setFilter(&filterTickTime);
	SettingsManager::add(tick_time);
	commandRegistry->add((new JSMAssignment<float>("TICK_TIME", *tick_time))
	                       ->SetHelp("Sets the time in milliseconds that JoyShockMaper waits before reading from each controller again."));

	auto light_bar = new JSMSetting<Color>(SettingID::LIGHT_BAR, 0xFFFFFF);
	// light_bar needs no filter or listener. The callback polls and updates the color.
	SettingsManager::add(light_bar);
	commandRegistry->add((new JSMAssignment<Color>(*light_bar))
	                       ->SetHelp("Changes the color bar of the DS4. Either enter as a hex code (xRRGGBB), as three decimal values between 0 and 255 (RRR GGG BBB), or as a common color name in all caps and underscores."));

	auto scroll_sens = new JSMSetting<FloatXY>(SettingID::SCROLL_SENS, { 30.f, 30.f });
	scroll_sens->setFilter(&filterFloatPair);
	SettingsManager::add(scroll_sens);
	commandRegistry->add((new JSMAssignment<FloatXY>(*scroll_sens))
	                       ->SetHelp("Scrolling sensitivity for sticks."));

	auto autoloadSwitch = new JSMVariable<Switch>(Switch::ON);
	autoLoadThread.reset(new JSM::AutoLoad(commandRegistry, autoloadSwitch->value() == Switch::ON)); // Start by default
	autoloadSwitch->setFilter(&filterInvalidValue<Switch, Switch::INVALID>)->addOnChangeListener(bind(&UpdateThread, autoLoadThread.get(), placeholders::_1));
	SettingsManager::add(SettingID::AUTOLOAD, autoloadSwitch);
	auto *autoloadCmd = new JSMAssignment<Switch>("AUTOLOAD", *autoloadSwitch);
	commandRegistry->add(autoloadCmd);

	auto grid_size = new JSMVariable(FloatXY{ 2.f, 1.f });
	grid_size->setFilter([](auto current, auto next)
	  {
		float floorX = floorf(next.x());
		float floorY = floorf(next.y());
		return floorX * floorY >= 1 && floorX * floorY <= 25 ? FloatXY{ floorX, floorY } : current; });
	grid_size->addOnChangeListener(bind(&OnNewGridDimensions, commandRegistry, placeholders::_1), true); // Call the listener now
	SettingsManager::add(SettingID::GRID_SIZE, grid_size);
	commandRegistry->add((new JSMAssignment<FloatXY>("GRID_SIZE", *grid_size))
	                       ->SetHelp("When TOUCHPAD_MODE is set to GRID_AND_STICK, this variable sets the number of rows and columns in the grid. The product of the two numbers need to be between 1 and 25."));

	auto touchpad_mode = new JSMSetting<TouchpadMode>(SettingID::TOUCHPAD_MODE, TouchpadMode::GRID_AND_STICK);
	touchpad_mode->setFilter(&filterInvalidValue<TouchpadMode, TouchpadMode::INVALID>);
	SettingsManager::add(touchpad_mode);
	commandRegistry->add((new JSMAssignment<TouchpadMode>("TOUCHPAD_MODE", *touchpad_mode))
	                       ->SetHelp("Assign a mode to the touchpad. Valid values are GRID_AND_STICK or MOUSE."));

	auto touch_ring_mode = new JSMSetting<RingMode>(SettingID::TOUCH_RING_MODE, RingMode::OUTER);
	touch_ring_mode->setFilter(&filterInvalidValue<RingMode, RingMode::INVALID>);
	SettingsManager::add(touch_ring_mode);
	commandRegistry->add((new JSMAssignment<RingMode>(*touch_ring_mode))
	                       ->SetHelp("Sets the ring mode for the touch stick. Valid values are INNER and OUTER"));

	auto touch_stick_mode = new JSMSetting<StickMode>(SettingID::TOUCH_STICK_MODE, StickMode::NO_MOUSE);
	touch_stick_mode->setFilter(&filterInvalidValue<StickMode, StickMode::INVALID>)->addOnChangeListener(bind(&UpdateRingModeFromStickMode, touch_ring_mode, ::placeholders::_1));
	SettingsManager::add(touch_stick_mode);
	commandRegistry->add((new JSMAssignment<StickMode>(*touch_stick_mode))
	                       ->SetHelp("Set a mouse mode for the touchpad stick. Valid values are the following:\nNO_MOUSE, AIM, FLICK, FLICK_ONLY, ROTATE_ONLY, MOUSE_RING, MOUSE_AREA, OUTER_RING, INNER_RING"));

	auto touch_deadzone_inner = new JSMSetting<float>(SettingID::TOUCH_DEADZONE_INNER, 0.3f);
	touch_deadzone_inner->setFilter(&filterPositive);
	SettingsManager::add(touch_deadzone_inner);
	commandRegistry->add((new JSMAssignment<float>(*touch_deadzone_inner))
	                       ->SetHelp("Sets the radius of the circle in which a touch stick input sends no output."));

	auto touch_stick_radius = new JSMSetting<float>(SettingID::TOUCH_STICK_RADIUS, 300.f);
	touch_stick_radius->setFilter([](auto current, auto next)
	  { return filterPositive(current, floorf(next)); });
	SettingsManager::add(touch_stick_radius);
	commandRegistry->add((new JSMAssignment<float>(*touch_stick_radius))
	                       ->SetHelp("Set the radius of the touchpad stick. The center of the stick is always the first point of contact. Use a very large value (ex: 800) to use it as swipe gesture."));

	auto touchpad_sens = new JSMSetting<FloatXY>(SettingID::TOUCHPAD_SENS, { 1.f, 1.f });
	touchpad_sens->setFilter(filterFloatPair);
	SettingsManager::add(touchpad_sens);
	commandRegistry->add((new JSMAssignment<FloatXY>(*touchpad_sens))
	                       ->SetHelp("Changes the sensitivity of the touchpad when set as a mouse. Enter a second value for a different vertical sensitivity."));

	auto hide_minimized = new JSMVariable<Switch>(Switch::OFF);
	hide_minimized->setFilter(&filterInvalidValue<Switch, Switch::INVALID>);
	hide_minimized->addOnChangeListener(bind(&UpdateThread, minimizeThread.get(), placeholders::_1));
	SettingsManager::add(SettingID::HIDE_MINIMIZED, hide_minimized);
	commandRegistry->add((new JSMAssignment<Switch>("HIDE_MINIMIZED", *hide_minimized))
	                       ->SetHelp("JSM will be hidden in the notification area when minimized if this setting is ON. Otherwise it stays in the taskbar."));

	auto virtual_controller = new JSMVariable<ControllerScheme>(ControllerScheme::NONE);
	virtual_controller->setFilter(&UpdateVirtualController);
	virtual_controller->addOnChangeListener(&OnVirtualControllerChange);
	SettingsManager::add(SettingID::VIRTUAL_CONTROLLER, virtual_controller);
	commandRegistry->add((new JSMAssignment<ControllerScheme>(magic_enum::enum_name(SettingID::VIRTUAL_CONTROLLER).data(), *virtual_controller))
	                       ->SetHelp("Sets the vigem virtual controller type. Can be NONE (default), XBOX (360) or DS4 (PS4)."));

	auto touch_ds_mode = new JSMSetting<TriggerMode>(SettingID::TOUCHPAD_DUAL_STAGE_MODE, TriggerMode::NO_SKIP);
	;
	touch_ds_mode->setFilter(&filterTouchpadDualStageMode);
	SettingsManager::add(touch_ds_mode);
	commandRegistry->add((new JSMAssignment<TriggerMode>(*touch_ds_mode))
	                       ->SetHelp("Dual stage mode for the touchpad TOUCH and CAPTURE (i.e. click) bindings."));

	auto rumble_enable = new JSMVariable<Switch>(Switch::ON);
	rumble_enable->setFilter(&filterInvalidValue<Switch, Switch::INVALID>);
	SettingsManager::add(SettingID::RUMBLE, rumble_enable);
	commandRegistry->add((new JSMAssignment<Switch>(magic_enum::enum_name(SettingID::RUMBLE).data(), *rumble_enable))
	                       ->SetHelp("Disable the rumbling feature from vigem. Valid values are ON and OFF."));

	auto adaptive_trigger = new JSMSetting<Switch>(SettingID::ADAPTIVE_TRIGGER, Switch::ON);
	adaptive_trigger->setFilter(&filterInvalidValue<Switch, Switch::INVALID>);
	SettingsManager::add(adaptive_trigger);
	commandRegistry->add((new JSMAssignment<Switch>(*adaptive_trigger))
	                       ->SetHelp("Control the adaptive trigger feature of the DualSense. Valid values are ON and OFF."));

	auto left_trigger_effect = new JSMSetting<AdaptiveTriggerSetting>(SettingID::LEFT_TRIGGER_EFFECT, AdaptiveTriggerSetting{});
	left_trigger_effect->setFilter(static_cast<AdaptiveTriggerSetting (*)(AdaptiveTriggerSetting, AdaptiveTriggerSetting)>(&filterInvalidValue));
	SettingsManager::add(left_trigger_effect);
	commandRegistry->add((new JSMAssignment<AdaptiveTriggerSetting>(*left_trigger_effect))
	                       ->SetHelp("Sets the adaptive trigger effect on the left trigger:\n"
	                                 "OFF: No effect\n"
	                                 "ON: Use effect generated by JSM depending on ZL_MODE\n"
	                                 "RESISTANCE start[0 9] force[0 8]: Some resistance starting at point\n"
	                                 "BOW start[0 8] end[0 8] forceStart[0 8] forceEnd[0 8]: increasingly strong resistance\n"
	                                 "GALLOPING start[0 8] end[0 9] foot1[0 6] foot2[0 7] freq[Hz]: Two pulses repeated periodically\n"
	                                 "SEMI_AUTOMATIC start[2 7] end[0 8] force[0 8]: Trigger effect\n"
	                                 "AUTOMATIC start[0 9] strength[0 8] freq[Hz]: Regular pulse effect\n"
	                                 "MACHINE start[0 9] end[0 9] force1[0 7] force2[0 7] freq[Hz] period: Irregular pulsing"));

	auto right_trigger_effect = new JSMSetting<AdaptiveTriggerSetting>(SettingID::RIGHT_TRIGGER_EFFECT, AdaptiveTriggerSetting{});
	right_trigger_effect->setFilter(static_cast<AdaptiveTriggerSetting (*)(AdaptiveTriggerSetting, AdaptiveTriggerSetting)>(&filterInvalidValue));
	SettingsManager::add(right_trigger_effect);
	commandRegistry->add((new JSMAssignment<AdaptiveTriggerSetting>(*right_trigger_effect))
	                       ->SetHelp("Sets the adaptive trigger effect on the right trigger:\n"
	                                 "OFF: No effect\n"
	                                 "ON: Use effect generated by JSM depending on ZR_MODE\n"
	                                 "RESISTANCE start[0 9] force[0 8]: Some resistance starting at point\n"
	                                 "BOW start[0 8] end[0 8] forceStart[0 8] forceEnd[0 8]: increasingly strong resistance\n"
	                                 "GALLOPING start[0 8] end[0 9] foot1[0 6] foot2[0 7] freq[Hz]: Two pulses repeated periodically\n"
	                                 "SEMI_AUTOMATIC start[2 7] end[0 8] force[0 8]: Trigger effect\n"
	                                 "AUTOMATIC start[0 9] strength[0 8] freq[Hz]: Regular pulse effect\n"
	                                 "MACHINE start[0 9] end[0 9] force1[0 7] force2[0 7] freq[Hz] period: Irregular pulsing"));

	auto right_trigger_offset = new JSMVariable<int>(25);
	right_trigger_offset->setFilter(&filterClampByte);
	SettingsManager::add(SettingID::RIGHT_TRIGGER_OFFSET, right_trigger_offset);
	commandRegistry->add((new JSMAssignment<int>(magic_enum::enum_name(SettingID::RIGHT_TRIGGER_OFFSET).data(), *right_trigger_offset)));

	auto left_trigger_offset = new JSMVariable<int>(25);
	left_trigger_offset->setFilter(&filterClampByte);
	SettingsManager::add(SettingID::LEFT_TRIGGER_OFFSET, left_trigger_offset);
	commandRegistry->add((new JSMAssignment<int>(magic_enum::enum_name(SettingID::LEFT_TRIGGER_OFFSET).data(), *left_trigger_offset)));

	auto right_trigger_range = new JSMVariable<int>(150);
	right_trigger_range->setFilter(&filterClampByte);
	SettingsManager::add(SettingID::RIGHT_TRIGGER_RANGE, right_trigger_range);
	commandRegistry->add((new JSMAssignment<int>(magic_enum::enum_name(SettingID::RIGHT_TRIGGER_RANGE).data(), *right_trigger_range)));

	auto left_trigger_range = new JSMVariable<int>(150);
	left_trigger_range->setFilter(&filterClampByte);
	SettingsManager::add(SettingID::LEFT_TRIGGER_RANGE, left_trigger_range);
	commandRegistry->add((new JSMAssignment<int>(magic_enum::enum_name(SettingID::LEFT_TRIGGER_RANGE).data(), *left_trigger_range)));

	auto auto_calibrate_gyro = new JSMVariable<Switch>(Switch::OFF);
	auto_calibrate_gyro->setFilter(&filterInvalidValue<Switch, Switch::INVALID>);
	SettingsManager::add(SettingID::AUTO_CALIBRATE_GYRO, auto_calibrate_gyro);
	commandRegistry->add((new JSMAssignment<Switch>("AUTO_CALIBRATE_GYRO", *auto_calibrate_gyro))
	                       ->SetHelp("Gyro calibration happens automatically when this setting is ON. Otherwise you'll need to calibrate the gyro manually when using gyro aiming."));

	auto left_stick_undeadzone_inner = new JSMSetting<float>(SettingID::LEFT_STICK_UNDEADZONE_INNER, 0.f);
	left_stick_undeadzone_inner->setFilter(&filterClamp01);
	SettingsManager::add(left_stick_undeadzone_inner);
	commandRegistry->add((new JSMAssignment<float>(*left_stick_undeadzone_inner))
	                       ->SetHelp("When outputting as a virtual controller, account for this much inner deadzone being applied in the target game. This value can only be between 0 and 1 but it should be small."));

	auto left_stick_undeadzone_outer = new JSMSetting<float>(SettingID::LEFT_STICK_UNDEADZONE_OUTER, 0.f);
	left_stick_undeadzone_outer->setFilter(&filterClamp01);
	SettingsManager::add(left_stick_undeadzone_outer);
	commandRegistry->add((new JSMAssignment<float>(*left_stick_undeadzone_outer))
	                       ->SetHelp("When outputting as a virtual controller, account for this much outer deadzone being applied in the target game. This value can only be between 0 and 1 but it should be small."));

	auto left_stick_unpower = new JSMSetting<float>(SettingID::LEFT_STICK_UNPOWER, 0.f);
	left_stick_unpower->setFilter(&filterFloat);
	SettingsManager::add(left_stick_unpower);
	commandRegistry->add((new JSMAssignment<float>(*left_stick_unpower))
	                       ->SetHelp("When outputting as a virtual controller, account for this power curve being applied in the target game."));

	auto right_stick_undeadzone_inner = new JSMSetting<float>(SettingID::RIGHT_STICK_UNDEADZONE_INNER, 0.f);
	right_stick_undeadzone_inner->setFilter(&filterClamp01);
	SettingsManager::add(right_stick_undeadzone_inner);
	commandRegistry->add((new JSMAssignment<float>(*right_stick_undeadzone_inner))
	                       ->SetHelp("When outputting as a virtual controller, account for this much inner deadzone being applied in the target game. This value can only be between 0 and 1 but it should be small."));

	auto right_stick_undeadzone_outer = new JSMSetting<float>(SettingID::RIGHT_STICK_UNDEADZONE_OUTER, 0.f);
	right_stick_undeadzone_outer->setFilter(&filterClamp01);
	SettingsManager::add(right_stick_undeadzone_outer);
	commandRegistry->add((new JSMAssignment<float>(*right_stick_undeadzone_outer))
	                       ->SetHelp("When outputting as a virtual controller, account for this much outer deadzone being applied in the target game. This value can only be between 0 and 1 but it should be small."));

	auto right_stick_unpower = new JSMSetting<float>(SettingID::RIGHT_STICK_UNPOWER, 0.f);
	right_stick_unpower->setFilter(&filterFloat);
	SettingsManager::add(right_stick_unpower);
	commandRegistry->add((new JSMAssignment<float>(*right_stick_unpower))
	                       ->SetHelp("When outputting as a virtual controller, account for this power curve being applied in the target game."));

	auto left_stick_virtual_scale = new JSMSetting<float>(SettingID::LEFT_STICK_VIRTUAL_SCALE, 1.f);
	left_stick_virtual_scale->setFilter(&filterFloat);
	SettingsManager::add(left_stick_virtual_scale);
	commandRegistry->add((new JSMAssignment<float>(*left_stick_virtual_scale))
	                       ->SetHelp("When outputting as a virtual controller, use this to adjust the scale of the left stick output. This does not affect the gyro->stick conversion."));

	auto right_stick_virtual_scale = new JSMSetting<float>(SettingID::RIGHT_STICK_VIRTUAL_SCALE, 1.f);
	right_stick_virtual_scale->setFilter(&filterFloat);
	SettingsManager::add(right_stick_virtual_scale);
	commandRegistry->add((new JSMAssignment<float>(*right_stick_virtual_scale))
	                       ->SetHelp("When outputting as a virtual controller, use this to adjust the scale of the right stick output. This does not affect the gyro->stick conversion."));

	auto wind_stick_range = new JSMSetting<float>(SettingID::WIND_STICK_RANGE, 900.f);
	wind_stick_range->setFilter(&filterPositive);
	SettingsManager::add(wind_stick_range);
	commandRegistry->add((new JSMAssignment<float>(*wind_stick_range))
	                       ->SetHelp("When using the WIND stick modes, this is how many degrees the stick has to be wound to cover the full range of the ouptut, from minimum value to maximum value."));

	auto wind_stick_power = new JSMSetting<float>(SettingID::WIND_STICK_POWER, 1.f);
	wind_stick_power->setFilter(&filterPositive);
	SettingsManager::add(wind_stick_power);
	commandRegistry->add((new JSMAssignment<float>(*wind_stick_power))
	                       ->SetHelp("Power curve for WIND stick modes, letting you have more or less sensitivity towards the neutral position."));

	auto unwind_rate = new JSMSetting<float>(SettingID::UNWIND_RATE, 1800.f);
	unwind_rate->setFilter(&filterPositive);
	SettingsManager::add(unwind_rate);
	commandRegistry->add((new JSMAssignment<float>(*unwind_rate))
	                       ->SetHelp("How quickly the WIND sticks unwind on their own when the relevant stick isn't engaged (in degrees per second)."));

	auto gyro_output = new JSMSetting<GyroOutput>(SettingID::GYRO_OUTPUT, GyroOutput::MOUSE);
	gyro_output->setFilter(&filterGyroOutput);
	SettingsManager::add(gyro_output);
	commandRegistry->add((new JSMAssignment<GyroOutput>(*gyro_output))
	                       ->SetHelp("Whether gyro should be converted to mouse, left stick, or right stick movement. If you don't want to use gyro aiming, simply leave GYRO_SENS set to 0."));

	auto flick_stick_output = new JSMSetting<GyroOutput>(SettingID::FLICK_STICK_OUTPUT, GyroOutput::MOUSE);
	flick_stick_output->setFilter(&filterInvalidValue<GyroOutput, GyroOutput::INVALID>);
	SettingsManager::add(flick_stick_output);
	commandRegistry->add((new JSMAssignment<GyroOutput>(*flick_stick_output))
	                       ->SetHelp("Whether flick stick should be converted to a mouse, left stick, or right stick movement."));

	auto currentWorkingDir = new JSMVariable<PathString>(GetCWD());
	currentWorkingDir->setFilter([](PathString current, PathString next) -> PathString
	  { return SetCWD(string(next)) ? next : current; });
	currentWorkingDir->addOnChangeListener(bind(&RefreshAutoLoadHelp, autoloadCmd), true);
	SettingsManager::add(SettingID::JSM_DIRECTORY, currentWorkingDir);
	commandRegistry->add((new JSMAssignment<PathString>("JSM_DIRECTORY", *currentWorkingDir))
	                       ->SetHelp("If AUTOLOAD doesn't work properly, set this value to the path to the directory holding the JoyShockMapper.exe file. Make sure a folder named \"AutoLoad\" exists there."));

	auto mouselike_factor = new JSMSetting<FloatXY>(SettingID::MOUSELIKE_FACTOR, {90.f, 90.f});
	mouselike_factor->setFilter(&filterFloatPair);
	SettingsManager::add(SettingID::MOUSELIKE_FACTOR, mouselike_factor);
	commandRegistry->add((new JSMAssignment<FloatXY>(*mouselike_factor))
		->SetHelp("Stick sensitivity of the relative movement when in HYBRID_AIM mode. Like the sensitivity of a mouse."));

	auto return_deadzone_is_active = new JSMSetting<Switch>(SettingID::RETURN_DEADZONE_IS_ACTIVE, Switch::ON);
	return_deadzone_is_active->setFilter(&filterInvalidValue<Switch, Switch::INVALID>);
	SettingsManager::add(SettingID::RETURN_DEADZONE_IS_ACTIVE, return_deadzone_is_active);
	commandRegistry->add((new JSMAssignment<Switch>(*return_deadzone_is_active))
		->SetHelp("In HYBRID_AIM stick mode, select the mode's behaviour in the deadzone.\n"\
			"This deadzone is determined by the angle of the output from the stick position to the center.\n"
			"It is fully active up to RETURN_DEADZONE_ANGLE and tapers off until RETURN_DEADZONE_CUTOFF_ANGLE.\n"
			"When in DEADZONE_INNER it transitions to an output deadzone based on the distance to the center so the relative part of the input smoothly fades back in."));
	
	auto edge_push_is_active = new JSMSetting<Switch>(SettingID::EDGE_PUSH_IS_ACTIVE, Switch::ON);
	edge_push_is_active->setFilter(&filterInvalidValue<Switch, Switch::INVALID>);
	SettingsManager::add(SettingID::EDGE_PUSH_IS_ACTIVE, edge_push_is_active);
	commandRegistry->add((new JSMAssignment<Switch>(*edge_push_is_active))
	        ->SetHelp("In HYBRID_AIM stick mode, enables continuous travelling when the stick is at the edge."));
		

	auto return_deadzone_angle = new JSMSetting<float>(SettingID::RETURN_DEADZONE_ANGLE, 45.f);
	return_deadzone_angle->setFilter([](float c, float n)
	  { return std::clamp(n, 0.f, 90.f); });
	SettingsManager::add(SettingID::RETURN_DEADZONE_ANGLE, return_deadzone_angle);
	commandRegistry->add((new JSMAssignment<float>(*return_deadzone_angle))
		->SetHelp("In HYBRID_AIM stick mode, angle to the center in which the return deadzone is still partially active.\n"\
				  "Valid values range from 0 to 90"));

	auto return_deadzone_cutoff_angle = new JSMSetting<float>(SettingID::RETURN_DEADZONE_ANGLE_CUTOFF, 90.f);
	return_deadzone_cutoff_angle->setFilter(&filterFloat);
	SettingsManager::add(SettingID::RETURN_DEADZONE_ANGLE_CUTOFF, return_deadzone_cutoff_angle);
	commandRegistry->add((new JSMAssignment<float>(*return_deadzone_cutoff_angle))
	    ->SetHelp("In HYBRID_AIM stick mode, angle to the center in which the return deadzone is fully active.\n"\
			      "Valid values range from 0 to 90"));
		

}

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

	grid_mappings.reserve(int(ButtonID::T25) - FIRST_TOUCH_BUTTON); // This makes sure the items will never get copied and cause crashes
	mappings.reserve(MAPPING_SIZE);
	for (int id = 0; id < MAPPING_SIZE; ++id)
	{
		JSMButton newButton(ButtonID(id), Mapping::NO_MAPPING);
		newButton.setFilter(&filterMapping);
		mappings.push_back(newButton);
	}
	// console
	initConsole();
	COUT_BOLD << "Welcome to JoyShockMapper version " << version << '!' << endl;
	// if (whitelister) COUT << "JoyShockMapper was successfully whitelisted!" << endl;
	//  Threads need to be created before listeners
	CmdRegistry commandRegistry;
	initJsmSettings(&commandRegistry);
	minimizeThread.reset(new PollingThread("Minimize thread", &MinimizePoll, nullptr, 1000, SettingsManager::getV<Switch>(SettingID::HIDE_MINIMIZED)->value() == Switch::ON)); // Start by default

	for (int i = argc - 1; i >= 0; --i)
	{
#if _WIN32
		string arg(&argv[i][0], &argv[i][wcslen(argv[i])]);
#else
		string arg = string(argv[0]);
#endif
		if (filesystem::is_directory(filesystem::status(arg)) &&
		  SettingsManager::getV<PathString>(SettingID::JSM_DIRECTORY)->set(arg).compare(arg) == 0)
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

	// Add all button mappings as commands
	assert(MAPPING_SIZE == buttonHelpMap.size() && "Please update the button help map in ButtonHelp.cpp");
	for (auto &mapping : mappings)
	{
		commandRegistry.add((new JSMAssignment<Mapping>(mapping.getName(), mapping))->SetHelp(buttonHelpMap.at(mapping._id)));
	}
	// SL and SR are shorthand for two different mappings
	commandRegistry.add(new JSMAssignment<Mapping>("SL", "LSL", mappings[(int)ButtonID::LSL], true));
	commandRegistry.add(new JSMAssignment<Mapping>("SL", "RSL", mappings[(int)ButtonID::RSL], true));
	commandRegistry.add(new JSMAssignment<Mapping>("SR", "LSR", mappings[(int)ButtonID::LSR], true));
	commandRegistry.add(new JSMAssignment<Mapping>("SR", "RSR", mappings[(int)ButtonID::RSR], true));

	// Add Macro commands
	commandRegistry.add((new JSMMacro("RESET_MAPPINGS"))->SetMacro(bind(&do_RESET_MAPPINGS, &commandRegistry))->SetHelp("Delete all custom bindings and reset to default,\nand run script OnReset.txt in JSM_DIRECTORY."));
	commandRegistry.add((new JSMMacro("NO_GYRO_BUTTON"))->SetMacro(bind(&do_NO_GYRO_BUTTON))->SetHelp("Enable gyro at all times, without any GYRO_OFF binding."));
	commandRegistry.add((new JSMMacro("RECONNECT_CONTROLLERS"))->SetMacro(bind(&do_RECONNECT_CONTROLLERS, placeholders::_2))->SetHelp("Look for newly connected controllers. Specify MERGE (default) or SPLIT whether you want to consider joycons as a single or separate controllers."));
	commandRegistry.add((new JSMMacro("COUNTER_OS_MOUSE_SPEED"))->SetMacro(bind(do_COUNTER_OS_MOUSE_SPEED))->SetHelp("JoyShockMapper will load the user's OS mouse sensitivity value to consider it in its calculations."));
	commandRegistry.add((new JSMMacro("IGNORE_OS_MOUSE_SPEED"))->SetMacro(bind(do_IGNORE_OS_MOUSE_SPEED))->SetHelp("Disable JoyShockMapper's consideration of the the user's OS mouse sensitivity value."));
	commandRegistry.add((new JSMMacro("CALCULATE_REAL_WORLD_CALIBRATION"))->SetMacro(bind(&do_CALCULATE_REAL_WORLD_CALIBRATION, placeholders::_2))->SetHelp("Get JoyShockMapper to recommend you a REAL_WORLD_CALIBRATION value after performing the calibration sequence. Visit GyroWiki for details:\nhttp://gyrowiki.jibbsmart.com/blog:joyshockmapper-guide#calibrating"));
	commandRegistry.add((new JSMMacro("SLEEP"))->SetMacro(bind(&do_SLEEP, placeholders::_2))->SetHelp("Sleep for the given number of seconds, or one second if no number is given. Can't sleep more than 10 seconds per command."));
	commandRegistry.add((new JSMMacro("FINISH_GYRO_CALIBRATION"))->SetMacro(bind(&do_FINISH_GYRO_CALIBRATION))->SetHelp("Finish calibrating the gyro in all controllers."));
	commandRegistry.add((new JSMMacro("RESTART_GYRO_CALIBRATION"))->SetMacro(bind(&do_RESTART_GYRO_CALIBRATION))->SetHelp("Start calibrating the gyro in all controllers."));
	commandRegistry.add((new JSMMacro("SET_MOTION_STICK_NEUTRAL"))->SetMacro(bind(&do_SET_MOTION_STICK_NEUTRAL))->SetHelp("Set the neutral orientation for motion stick to whatever the orientation of the controller is."));
	commandRegistry.add((new JSMMacro("README"))->SetMacro(bind(&do_README))->SetHelp("Open the latest JoyShockMapper README in your browser."));
	commandRegistry.add((new JSMMacro("WHITELIST_SHOW"))->SetMacro(bind(&do_WHITELIST_SHOW))->SetHelp("Open the whitelister application"));
	commandRegistry.add((new JSMMacro("WHITELIST_ADD"))->SetMacro(bind(&do_WHITELIST_ADD))->SetHelp("Add JoyShockMapper to the whitelisted applications."));
	commandRegistry.add((new JSMMacro("WHITELIST_REMOVE"))->SetMacro(bind(&do_WHITELIST_REMOVE))->SetHelp("Remove JoyShockMapper from whitelisted applications."));
	commandRegistry.add(new HelpCmd(commandRegistry));
	commandRegistry.add((new JSMMacro("CLEAR"))->SetMacro(bind(&ClearConsole))->SetHelp("Removes all text in the console screen"));
	commandRegistry.add((new JSMMacro("CALIBRATE_TRIGGERS"))->SetMacro([](JSMMacro *, in_string)
	                                                          {
		                                                        triggerCalibrationStep = 1;
		                                                        return true; })
	                      ->SetHelp("Starts the trigger calibration procedure for the dualsense triggers."));
	bool quit = false;
	commandRegistry.add((new JSMMacro("QUIT"))
	                      ->SetMacro([&quit](JSMMacro *, in_string)
	                        {
		                      quit = true;
		                      WriteToConsole(""); // If ran from autoload thread, you need to send RETURN to resume the main loop and check the quit flag.
		                      return true; })
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
			SettingsManager::getV<Switch>(SettingID::AUTOLOAD)->set(Switch::OFF);
		}
	}
	// The main loop is simple and reads like pseudocode
	string enteredCommand;
	while (!quit)
	{
		getline(cin, enteredCommand);
		commandRegistry.processLine(enteredCommand);
	}
#ifdef _WIN32
	LocalFree(argv);
#endif
	CleanUp();
	return 0;
}
