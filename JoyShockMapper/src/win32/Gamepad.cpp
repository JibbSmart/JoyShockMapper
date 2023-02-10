#include <Windows.h>
#include "Gamepad.h"
#include "ViGEm/Client.h"
#define INCLUDE_MATH_DEFINES
#include <math.h> // M_PI
#include <algorithm>
#include <sstream>

//
// Link against SetupAPI
//
#pragma comment(lib, "setupapi.lib")

inline void DS4_REPORT_EX_INIT(DS4_REPORT_EX* ds4reportEx)
{
	memset(ds4reportEx, 0, (sizeof(DS4_REPORT_EX)));
	ds4reportEx->Report.wButtons = DS4_BUTTON_DPAD_NONE;
	ds4reportEx->Report.bThumbLX = 0x7F;
	ds4reportEx->Report.bThumbLY = 0x7F;
	ds4reportEx->Report.bThumbRX = 0x7F;
	ds4reportEx->Report.bThumbRY = 0x7F;
}

template<typename T>
void SetPressed(T& buttons, WORD mask)
{
	buttons |= mask;
}
template<typename T>
void ClearPressed(T& buttons, WORD mask)
{
	buttons &= ~mask;
}

class VigemClient
{
	static unique_ptr<VigemClient> _inst;

	PVIGEM_CLIENT _client = nullptr;
	VIGEM_ERROR _error = VIGEM_ERROR::VIGEM_ERROR_NONE;

	VigemClient()
	{
		_client = vigem_alloc();
		_error = vigem_connect(_client);
	}

public:
	~VigemClient()
	{
		vigem_disconnect(_client);
		vigem_free(_client);
	}

	static PVIGEM_CLIENT get(VIGEM_ERROR* outError = nullptr)
	{
		if (!_inst || !_inst->_client)
		{
			_inst.reset(new VigemClient());
		}
		if (outError)
		{
			*outError = _inst->_error;
		}
		return _inst->_client;
	}
};

unique_ptr<VigemClient> VigemClient::_inst;

template<>
ostream& operator<<<VIGEM_ERROR>(ostream& out, VIGEM_ERROR errCode)
{
	switch (errCode)
	{
	case VIGEM_ERROR_NONE:
		out << "VIGEM_ERROR_NONE";
		break;
	case VIGEM_ERROR_BUS_NOT_FOUND:
		out << "VIGEM_ERROR_BUS_NOT_FOUND";
		break;
	case VIGEM_ERROR_NO_FREE_SLOT:
		out << "VIGEM_ERROR_NO_FREE_SLOT";
		break;
	case VIGEM_ERROR_INVALID_TARGET:
		out << "VIGEM_ERROR_INVALID_TARGET";
		break;
	case VIGEM_ERROR_REMOVAL_FAILED:
		out << "VIGEM_ERROR_REMOVAL_FAILED";
		break;
	case VIGEM_ERROR_ALREADY_CONNECTED:
		out << "VIGEM_ERROR_ALREADY_CONNECTED";
		break;
	case VIGEM_ERROR_TARGET_UNINITIALIZED:
		out << "VIGEM_ERROR_TARGET_UNINITIALIZED";
		break;
	case VIGEM_ERROR_TARGET_NOT_PLUGGED_IN:
		out << "VIGEM_ERROR_TARGET_NOT_PLUGGED_IN";
		break;
	case VIGEM_ERROR_BUS_VERSION_MISMATCH:
		out << "VIGEM_ERROR_BUS_VERSION_MISMATCH";
		break;
	case VIGEM_ERROR_BUS_ACCESS_FAILED:
		out << "VIGEM_ERROR_BUS_ACCESS_FAILED";
		break;
	case VIGEM_ERROR_CALLBACK_ALREADY_REGISTERED:
		out << "VIGEM_ERROR_CALLBACK_ALREADY_REGISTERED";
		break;
	case VIGEM_ERROR_CALLBACK_NOT_FOUND:
		out << "VIGEM_ERROR_CALLBACK_NOT_FOUND";
		break;
	case VIGEM_ERROR_BUS_ALREADY_CONNECTED:
		out << "VIGEM_ERROR_BUS_ALREADY_CONNECTED";
		break;
	case VIGEM_ERROR_BUS_INVALID_HANDLE:
		out << "VIGEM_ERROR_BUS_INVALID_HANDLE";
		break;
	case VIGEM_ERROR_XUSB_USERINDEX_OUT_OF_RANGE:
		out << "VIGEM_ERROR_XUSB_USERINDEX_OUT_OF_RANGE";
		break;
	case VIGEM_ERROR_INVALID_PARAMETER:
		out << "VIGEM_ERROR_INVALID_PARAMETER";
		break;
	default:
		break;
	}
	return out;
}

class VigemGamepad : public Gamepad
{
public:
	VigemGamepad(Callback notification)
	  : _notification(notification)
	{
		std::stringstream ss;
		VIGEM_ERROR error = VIGEM_ERROR::VIGEM_ERROR_NONE;
		PVIGEM_CLIENT client = VigemClient::get(&error);
		if (client == nullptr)
		{
			_errorMsg = "Uh, not enough memory to do that?!";
		}
		else if (error == VIGEM_ERROR_BUS_NOT_FOUND)
		{
			ss << "ViGEm bus is not installed. You can download the latest version of it here:" << endl
			   << "https://github.com/ViGEm/ViGEmBus/releases/latest";
			_errorMsg = ss.str();
		}
		else if (!VIGEM_SUCCESS(error))
		{
			ss << "ViGEm Bus connection failed: " << error;
			_errorMsg = ss.str();
		}
	}

	virtual ~VigemGamepad()
	{
		if (isInitialized())
		{
			// We're done with this pad, free resources (this disconnects the virtual device)
			if (PVIGEM_CLIENT client = VigemClient::get(); client != nullptr)
			{
				vigem_target_remove(client, _gamepad);
			}
			vigem_target_free(_gamepad);
		}
	}

	bool isInitialized(std::string* errorMsg = nullptr) override
	{
		if (!_errorMsg.empty() && errorMsg != nullptr)
		{
			*errorMsg = _errorMsg;
		}
		return _errorMsg.empty() && vigem_target_is_attached(_gamepad) == TRUE;
	}
GamepadImpl::~GamepadImpl()
{
	// vigem_target_x360_unregister_notification
	//
	// We're done with this pad, free resources (this disconnects the virtual device)
	//
	PVIGEM_CLIENT client = VigemClient::get();
	if (isInitialized())
	{
		vigem_target_x360_unregister_notification(_gamepad);
		if (client)
			vigem_target_remove(client, _gamepad);
		vigem_target_free(_gamepad);
	}
}

	void setStick(float x, float y, bool isLeft) override
	{
		isLeft ? setLeftStick(x, y) : setRightStick(x, y);
	}

	void setGyro(float accelX, float accelY, float accelZ, float gyroX, float gyroY, float gyroZ) override {}
	void setTouchState(std::optional<FloatXY> press1, std::optional<FloatXY> press2) override {}

protected:

	Callback _notification = nullptr;
	PVIGEM_TARGET _gamepad = nullptr;
};

class XboxGamepad : public VigemGamepad
{
public:
	XboxGamepad(Callback notification)
	  : VigemGamepad(notification)
	  , _stateX360()
	{
		XUSB_REPORT_INIT(&_stateX360);

		if (_errorMsg.empty())
		{
			// Allocate handle to identify new pad
			_gamepad = vigem_target_x360_alloc();

			// Add _client to the bus, this equals a plug-in event
			VIGEM_ERROR error = vigem_target_add(VigemClient::get(), _gamepad);
			if (!VIGEM_SUCCESS(error))
			{
				std::stringstream ss;
				ss << "Target plugin failed: " << error;
				_errorMsg = ss.str();
				return;
			}

			error = vigem_target_x360_register_notification(VigemClient::get(), _gamepad, &XboxGamepad::x360Notification, this);
			if (!VIGEM_SUCCESS(error))
			{
				std::stringstream ss;
				ss << "Target plugin failed: " << error;
				_errorMsg = ss.str();
			}
		}
	}

	virtual ~XboxGamepad()
	{
		if (isInitialized())
		{
			vigem_target_x360_unregister_notification(_gamepad);
		}
	}
	void setButton(KeyCode btn, bool pressed) override
	{
		auto op = pressed ? &SetPressed<WORD> : &ClearPressed<WORD>;
		static std::map<WORD, uint16_t> buttonMap{
			{ X_UP, XUSB_GAMEPAD_DPAD_UP },
			{ X_DOWN, XUSB_GAMEPAD_DPAD_DOWN },
			{ X_LEFT, XUSB_GAMEPAD_DPAD_LEFT },
			{ X_RIGHT, XUSB_GAMEPAD_DPAD_RIGHT },
			{ X_LB, XUSB_GAMEPAD_LEFT_SHOULDER },
			{ X_BACK, XUSB_GAMEPAD_BACK },
			{ X_X, XUSB_GAMEPAD_X },
			{ X_A, XUSB_GAMEPAD_A },
			{ X_Y, XUSB_GAMEPAD_Y },
			{ X_B, XUSB_GAMEPAD_B },
			{ X_RB, XUSB_GAMEPAD_RIGHT_SHOULDER },
			{ X_START, XUSB_GAMEPAD_START },
			{ X_LS, XUSB_GAMEPAD_LEFT_THUMB },
			{ X_RS, XUSB_GAMEPAD_RIGHT_THUMB },
			{ X_GUIDE, XUSB_GAMEPAD_GUIDE },
		};

		if (auto found = buttonMap.find(btn.code); found != buttonMap.end())
		{
			op(_stateX360.wButtons, found->second);
		}
	}
	void setLeftStick(float x, float y) override
	{
		_stateX360.sThumbLX = clamp(int(_stateX360.sThumbLX + SHRT_MAX * clamp(x, -1.f, 1.f)), SHRT_MIN, SHRT_MAX);
		_stateX360.sThumbLY = clamp(int(_stateX360.sThumbLY + SHRT_MAX * clamp(y, -1.f, 1.f)), SHRT_MIN, SHRT_MAX);
		
	}
	void setRightStick(float x, float y) override
	{
		_stateX360.sThumbRX = clamp(int(_stateX360.sThumbRX + SHRT_MAX * clamp(x, -1.f, 1.f)), SHRT_MIN, SHRT_MAX);
		_stateX360.sThumbRY = clamp(int(_stateX360.sThumbRY + SHRT_MAX * clamp(y, -1.f, 1.f)), SHRT_MIN, SHRT_MAX);
	}
	void setLeftTrigger(float val) override
	{
		_stateX360.bLeftTrigger = clamp(int(_stateX360.bLeftTrigger + uint8_t(clamp(val, 0.f, 1.f) * UCHAR_MAX)), 0, UCHAR_MAX);
	}
	void setRightTrigger(float val) override
	{
		_stateX360.bRightTrigger = clamp(int(_stateX360.bRightTrigger + uint8_t(clamp(val, 0.f, 1.f) * UCHAR_MAX)), 0, UCHAR_MAX);
	}
	void update() override
	{
		if (isInitialized())
		{
			vigem_target_x360_update(VigemClient::get(), _gamepad, _stateX360);
			XUSB_REPORT_INIT(&_stateX360);
		}
	}

	ControllerScheme getType() const override
	{
		return ControllerScheme::XBOX;
	}

private:
	static void CALLBACK x360Notification(
		PVIGEM_CLIENT client,
		PVIGEM_TARGET target,
		uint8_t largeMotor,
		uint8_t smallMotor,
		uint8_t ledNumber,
		void* userData)
	{
		auto originator = static_cast<XboxGamepad*>(userData);
		if (client == VigemClient::get() && originator && originator->_gamepad == target && originator->_notification)
		{
			Indicator indicator;
			indicator.led = ledNumber;
			originator->_notification(largeMotor, smallMotor, indicator);
		}
	}

	XUSB_REPORT _stateX360;
};

class Ds4Gamepad : public VigemGamepad
{
public:
	Ds4Gamepad(Callback notification)
	  : VigemGamepad(notification)
	  , _stateDS4()
	{
		DS4_REPORT_EX_INIT(&_stateDS4);

		if (_errorMsg.empty())
		{
			// Allocate handle to identify new pad
			_gamepad = vigem_target_ds4_alloc();

			// Add _client to the bus, this equals a plug-in event
			VIGEM_ERROR error = vigem_target_add(VigemClient::get(), _gamepad);
			if (!VIGEM_SUCCESS(error))
			{
				std::stringstream ss;
				ss << "Target plugin failed: " << error;
				_errorMsg = ss.str();
				return;
			}

			error = vigem_target_ds4_register_notification(VigemClient::get(), _gamepad, reinterpret_cast<PFN_VIGEM_DS4_NOTIFICATION>(&Ds4Gamepad::ds4Notification), this);
			if (!VIGEM_SUCCESS(error))
			{
				std::stringstream ss;
				ss << "Target plugin failed: " << error;
				_errorMsg = ss.str();
			}
		}
	}
	virtual void setButton(KeyCode btn, bool pressed) override;
	virtual void setLeftStick(float x, float y) override
	{
		_stateDS4.Report.bThumbLX = clamp(int(_stateDS4.Report.bThumbLX + UCHAR_MAX * (clamp(x / 2.f, -.5f, .5f) + .5f)), 0, UCHAR_MAX);
		_stateDS4.Report.bThumbLY = clamp(int(_stateDS4.Report.bThumbLY + UCHAR_MAX * (clamp(-y / 2.f, -.5f, .5f) + .5f)), 0, UCHAR_MAX);
	}
	virtual void setRightStick(float x, float y) override
	{
		_stateDS4.Report.bThumbRX = clamp(int(_stateDS4.Report.bThumbRX + UCHAR_MAX * (clamp(x / 2.f, -.5f, .5f))), 0, UCHAR_MAX);
		_stateDS4.Report.bThumbRY = clamp(int(_stateDS4.Report.bThumbRY + UCHAR_MAX * (clamp(-y / 2.f, -.5f, .5f))), 0, UCHAR_MAX);
	}
	virtual void setLeftTrigger(float val) override
	{
		_stateDS4.Report.bTriggerL = clamp(int(_stateDS4.Report.bTriggerL + uint8_t(clamp(val, 0.f, 1.f) * UCHAR_MAX)), 0, UCHAR_MAX);
		if (val > 0)
			SetPressed(_stateDS4.Report.wButtons, DS4_BUTTON_TRIGGER_LEFT);
		else
			ClearPressed(_stateDS4.Report.wButtons, DS4_BUTTON_TRIGGER_LEFT);
	}
	virtual void setRightTrigger(float val) override
	{
		_stateDS4.Report.bTriggerR = clamp(int(_stateDS4.Report.bTriggerR + uint8_t(clamp(val, 0.f, 1.f) * UCHAR_MAX)), 0, UCHAR_MAX);
		if (val > 0)
			SetPressed(_stateDS4.Report.wButtons, DS4_BUTTON_TRIGGER_RIGHT);
		else
			ClearPressed(_stateDS4.Report.wButtons, DS4_BUTTON_TRIGGER_RIGHT);
	}
	virtual void setGyro(float accelX, float accelY, float accelZ, float gyroX, float gyroY, float gyroZ) override
	{
		// reset Analog Data ?
		static constexpr float accelToRaw = 9.8f;
		_stateDS4.Report.wAccelX = SHORT(accelX * accelToRaw);
		_stateDS4.Report.wAccelY = SHORT(accelY * accelToRaw);
		_stateDS4.Report.wAccelZ = SHORT(accelZ * accelToRaw);

		static constexpr float gyroToRaw = 3.14159265358979323846264338327950288 / 180.f;
		_stateDS4.Report.wGyroX = SHORT(accelX * gyroToRaw);
		_stateDS4.Report.wGyroY = SHORT(accelY * gyroToRaw);
		_stateDS4.Report.wGyroZ = SHORT(accelZ * gyroToRaw);
	}
	virtual void setTouchState(std::optional<FloatXY> press1, std::optional<FloatXY> press2) override
	{
		if (press1)
		{
			_stateDS4.Report.sCurrentTouch.bIsUpTrackingNum1 = 0;
			int32_t xy24b = int32_t(press1->y() * 943.0f) << 12;
			xy24b |= int32_t(press1->x() * 1920.0f);
			_stateDS4.Report.sCurrentTouch.bTouchData1[2] = uint8_t(xy24b >> 16);
			_stateDS4.Report.sCurrentTouch.bTouchData1[1] = uint8_t((xy24b >> 8) & 0xFF);
			_stateDS4.Report.sCurrentTouch.bTouchData1[0] = uint8_t(xy24b & 0xFF);
		}
		if (press2)
		{
			_stateDS4.Report.sCurrentTouch.bIsUpTrackingNum1 = 0;
			int32_t xy24b = int32_t(press2->y() * 943.0f) << 12;
			xy24b |= int32_t(press2->x() * 1920.0f);
			_stateDS4.Report.sCurrentTouch.bTouchData1[2] = uint8_t(xy24b >> 16);
			_stateDS4.Report.sCurrentTouch.bTouchData1[1] = uint8_t((xy24b >> 8) & 0xFF);
			_stateDS4.Report.sCurrentTouch.bTouchData1[0] = uint8_t(xy24b & 0xFF);
		}
	}
	virtual void update() override
	{
		if (isInitialized())
		{
			vigem_target_ds4_update_ex(VigemClient::get(), _gamepad, _stateDS4);
			DS4_REPORT_EX_INIT(&_stateDS4);
		}
	}

	virtual ControllerScheme getType() const override
	{
		return ControllerScheme::DS4;
	}

private:
	static void CALLBACK ds4Notification(
	  PVIGEM_CLIENT client,
	  PVIGEM_TARGET target,
	  uint8_t largeMotor,
	  uint8_t smallMotor,
	  Indicator lightbarColor,
	  void* userData)
	{
		auto originator = static_cast<Ds4Gamepad*>(userData);
		if (client == VigemClient::get() && originator && originator->_gamepad == target && originator->_notification)
		{
			originator->_notification(largeMotor, smallMotor, lightbarColor);
		}
	}

	DS4_REPORT_EX _stateDS4;
};

class PSHat
{
	WORD _value;

public:
	PSHat(WORD init = DS4_BUTTON_DPAD_NONE)
		: _value(init)
	{
	}

	WORD set(WORD direction)
	{
		// puke
		switch (_value)
		{
		case DS4_BUTTON_DPAD_NONE:
			switch (direction)
			{
			case X_UP:
				_value = DS4_BUTTON_DPAD_NORTH;
				break;
			case X_DOWN:
				_value = DS4_BUTTON_DPAD_SOUTH;
				break;
			case X_LEFT:
				_value = DS4_BUTTON_DPAD_WEST;
				break;
			case X_RIGHT:
				_value = DS4_BUTTON_DPAD_EAST;
				break;
			}
			break;
		case DS4_BUTTON_DPAD_NORTHWEST:
			switch (direction)
			{
			case X_DOWN:
				_value = DS4_BUTTON_DPAD_WEST;
				break;
			case X_RIGHT:
				_value = DS4_BUTTON_DPAD_NORTH;
				break;
			}
			break;
		case DS4_BUTTON_DPAD_WEST:
			switch (direction)
			{
			case X_UP:
				_value = DS4_BUTTON_DPAD_NORTHWEST;
				break;
			case X_DOWN:
				_value = DS4_BUTTON_DPAD_SOUTHWEST;
				break;
			case X_RIGHT:
				_value = DS4_BUTTON_DPAD_NONE;
				break;
			}
			break;
		case DS4_BUTTON_DPAD_SOUTHWEST:
			switch (direction)
			{
			case X_UP:
				_value = DS4_BUTTON_DPAD_WEST;
				break;
			case X_RIGHT:
				_value = DS4_BUTTON_DPAD_SOUTH;
				break;
			}
			break;
		case DS4_BUTTON_DPAD_SOUTH:
			switch (direction)
			{
			case X_UP:
				_value = DS4_BUTTON_DPAD_NONE;
				break;
			case X_LEFT:
				_value = DS4_BUTTON_DPAD_SOUTHWEST;
				break;
			case X_RIGHT:
				_value = DS4_BUTTON_DPAD_SOUTHEAST;
				break;
			}
			break;
		case DS4_BUTTON_DPAD_SOUTHEAST:
			switch (direction)
			{
			case X_UP:
				_value = DS4_BUTTON_DPAD_EAST;
				break;
			case X_LEFT:
				_value = DS4_BUTTON_DPAD_SOUTH;
				break;
			}
			break;
		case DS4_BUTTON_DPAD_EAST:
			switch (direction)
			{
			case X_UP:
				_value = DS4_BUTTON_DPAD_NORTHEAST;
				break;
			case X_DOWN:
				_value = DS4_BUTTON_DPAD_SOUTHEAST;
				break;
			case X_LEFT:
				_value = DS4_BUTTON_DPAD_NONE;
				break;
			}
			break;
		case DS4_BUTTON_DPAD_NORTHEAST:
			switch (direction)
			{
			case X_DOWN:
				_value = DS4_BUTTON_DPAD_EAST;
				break;
			case X_LEFT:
				_value = DS4_BUTTON_DPAD_NORTH;
				break;
			}
			break;
		case DS4_BUTTON_DPAD_NORTH:
			switch (direction)
			{
			case X_DOWN:
				_value = DS4_BUTTON_DPAD_NONE;
				break;
			case X_LEFT:
				_value = DS4_BUTTON_DPAD_NORTHWEST;
				break;
			case X_RIGHT:
				_value = DS4_BUTTON_DPAD_NORTHEAST;
				break;
			}
			break;
		}
		return _value;
	}

	WORD clear(WORD direction)
	{
		// puke
		switch (_value)
		{
		case DS4_BUTTON_DPAD_NORTHWEST:
			switch (direction)
			{
			case X_UP:
				_value = DS4_BUTTON_DPAD_WEST;
				break;
			case X_LEFT:
				_value = DS4_BUTTON_DPAD_NORTH;
				break;
			}
			break;
		case DS4_BUTTON_DPAD_WEST:
			switch (direction)
			{
			case X_LEFT:
				_value = DS4_BUTTON_DPAD_NONE;
				break;
			}
			break;
		case DS4_BUTTON_DPAD_SOUTHWEST:
			switch (direction)
			{
			case X_DOWN:
				_value = DS4_BUTTON_DPAD_WEST;
				break;
			case X_LEFT:
				_value = DS4_BUTTON_DPAD_SOUTH;
				break;
			}
			break;
		case DS4_BUTTON_DPAD_SOUTH:
			switch (direction)
			{
			case X_DOWN:
				_value = DS4_BUTTON_DPAD_NONE;
				break;
			}
			break;
		case DS4_BUTTON_DPAD_SOUTHEAST:
			switch (direction)
			{
			case X_DOWN:
				_value = DS4_BUTTON_DPAD_EAST;
				break;
			case X_RIGHT:
				_value = DS4_BUTTON_DPAD_SOUTH;
				break;
			}
			break;
		case DS4_BUTTON_DPAD_EAST:
			switch (direction)
			{
			case X_RIGHT:
				_value = DS4_BUTTON_DPAD_NONE;
				break;
			}
			break;
		case DS4_BUTTON_DPAD_NORTHEAST:
			switch (direction)
			{
			case X_UP:
				_value = DS4_BUTTON_DPAD_EAST;
				break;
			case X_RIGHT:
				_value = DS4_BUTTON_DPAD_NORTH;
				break;
			}
			break;
		case DS4_BUTTON_DPAD_NORTH:
			switch (direction)
			{
			case X_UP:
				_value = DS4_BUTTON_DPAD_NONE;
				break;
			}
			break;
		}
		return _value;
	}
};

void Ds4Gamepad::setButton(KeyCode btn, bool pressed)
{
	decltype(&SetPressed<WORD>) op_w = pressed ? &SetPressed<WORD> : &ClearPressed<WORD>;
	decltype(&SetPressed<UCHAR>) op_b = pressed ? &SetPressed<UCHAR> : &ClearPressed<UCHAR>;

	switch (btn.code)
	{
	case PS_UP:
	case PS_DOWN:
	case PS_LEFT:
	case PS_RIGHT:
	{
		PSHat hat(_stateDS4.Report.wButtons & 0x000F);
		_stateDS4.Report.wButtons = (_stateDS4.Report.wButtons & 0xFFF0) | (pressed ? hat.set(btn.code) : hat.clear(btn.code));
	}
	break;

	case PS_HOME:
		op_b(_stateDS4.Report.bSpecial, DS4_SPECIAL_BUTTON_PS);
		break;
	case PS_PAD_CLICK:
		op_b(_stateDS4.Report.bSpecial, DS4_SPECIAL_BUTTON_TOUCHPAD);
		break;
	case PS_L1:
		op_w(_stateDS4.Report.wButtons, DS4_BUTTON_SHOULDER_LEFT);
		break;
	case PS_SHARE:
		op_w(_stateDS4.Report.wButtons, DS4_BUTTON_SHARE);
		break;
	case PS_SQUARE:
		op_w(_stateDS4.Report.wButtons, DS4_BUTTON_SQUARE);
		break;
	case PS_CROSS:
		op_w(_stateDS4.Report.wButtons, DS4_BUTTON_CROSS);
		break;
	case PS_TRIANGLE:
		op_w(_stateDS4.Report.wButtons, DS4_BUTTON_TRIANGLE);
		break;
	case PS_CIRCLE:
		op_w(_stateDS4.Report.wButtons, DS4_BUTTON_CIRCLE);
		break;
	case PS_R1:
		op_w(_stateDS4.Report.wButtons, DS4_BUTTON_SHOULDER_RIGHT);
		break;
	case PS_OPTIONS:
		op_w(_stateDS4.Report.wButtons, DS4_BUTTON_OPTIONS);
		break;
	case PS_L3:
		op_w(_stateDS4.Report.wButtons, DS4_BUTTON_THUMB_LEFT);
		break;
	case PS_R3:
		op_w(_stateDS4.Report.wButtons, DS4_BUTTON_THUMB_RIGHT);
		break;
	default:
		break;
	}
}

Gamepad *Gamepad::getNew(ControllerScheme scheme, Callback notification)
{
	switch (scheme)
	{
	case ControllerScheme::XBOX:
		return new XboxGamepad(notification);
	case ControllerScheme::DS4:
		return new Ds4Gamepad(notification);
	}
	return nullptr;
}
