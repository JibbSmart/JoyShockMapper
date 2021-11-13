#include <Windows.h>
#include "Gamepad.h"
#include "ViGEm/Client.h"

#include <algorithm>
#include <sstream>

//
// Link against SetupAPI
//
#pragma comment(lib, "setupapi.lib")

enum AnalogElement
{
    LSTICK,
    RSTICK,
    LTRIG,
    RTRIG,
    COUNT,
};

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

class GamepadImpl : public Gamepad
{
public:
	GamepadImpl(ControllerScheme scheme, Callback notification);
	virtual ~GamepadImpl();
	virtual bool isInitialized(std::string *errorMsg = nullptr) override;
	virtual void setButton(KeyCode btn, bool pressed) override;
	virtual void setLeftStick(float x, float y) override;
	virtual void setRightStick(float x, float y) override;
	virtual void setStick(float x, float y, bool isLeft) override;
	virtual void setLeftTrigger(float) override;
	virtual void setRightTrigger(float) override;
	virtual void update() override;

	virtual ControllerScheme getType() const override;

private:
	static void CALLBACK x360Notification(
		PVIGEM_CLIENT client,
		PVIGEM_TARGET target,
		uint8_t largeMotor,
		uint8_t smallMotor,
		uint8_t ledNumber,
		void* userData);

	static void CALLBACK ds4Notification(
		PVIGEM_CLIENT client,
		PVIGEM_TARGET target,
		uint8_t largeMotor,
		uint8_t smallMotor,
		Indicator lightbarColor,
		void* userData);

	void init_x360();
	void init_ds4();

	void setButtonX360(KeyCode btn, bool pressed);
	void setButtonDS4(KeyCode btn, bool pressed);

	Callback _notification = nullptr;
	PVIGEM_TARGET _gamepad = nullptr;
	unique_ptr<XUSB_REPORT> _stateX360;
	unique_ptr<DS4_REPORT> _stateDS4;

	std::vector<bool> _resetAnalogData;
};

void GamepadImpl::x360Notification(
	PVIGEM_CLIENT client,
	PVIGEM_TARGET target,
	uint8_t largeMotor,
	uint8_t smallMotor,
	uint8_t ledNumber,
	void* userData)
{
	auto originator = static_cast<GamepadImpl *>(userData);
	if (client == VigemClient::get() && originator && originator->_gamepad == target && originator->_notification)
	{
		Indicator indicator;
		indicator.led = ledNumber;
		originator->_notification(largeMotor, smallMotor, indicator);
	}
}

void GamepadImpl::ds4Notification(
	PVIGEM_CLIENT client,
	PVIGEM_TARGET target,
	uint8_t largeMotor,
	uint8_t smallMotor,
	Indicator lightbarColor,
	void* userData)
{
	auto originator = static_cast<GamepadImpl *>(userData);
	if (client == VigemClient::get() && originator && originator->_gamepad == target && originator->_notification)
	{
		originator->_notification(largeMotor, smallMotor, lightbarColor);
	}
}

GamepadImpl::GamepadImpl(ControllerScheme scheme, Callback notification)
  : _stateX360(new XUSB_REPORT)
  , _stateDS4(new DS4_REPORT)
  , _notification()
  , _resetAnalogData(AnalogElement::COUNT, true)
{
	XUSB_REPORT_INIT(_stateX360.get());
	DS4_REPORT_INIT(_stateDS4.get());

	std::stringstream ss;
	VIGEM_ERROR error = VIGEM_ERROR::VIGEM_ERROR_NONE;
	PVIGEM_CLIENT client = VigemClient::get(&error);
	if (client == nullptr)
	{
		_errorMsg = "Uh, not enough memory to do that?!";
		return;
	}
	else if (error == VIGEM_ERROR_BUS_NOT_FOUND)
	{
		ss << "ViGEm bus is not installed. You can download the latest version of it here:" << endl
			<< "https://github.com/ViGEm/ViGEmBus/releases/latest";
		_errorMsg = ss.str();
		return;
	}
	else if (!VIGEM_SUCCESS(error))
	{
		ss << "ViGEm Bus connection failed: " << error;
		_errorMsg = ss.str();
		return;
	}
	else
	{
		if (scheme == ControllerScheme::XBOX)
			init_x360();
		else if (scheme == ControllerScheme::DS4)
			init_ds4();

		if (vigem_target_is_attached(_gamepad) != TRUE)
		{
			_errorMsg = "Target is not attached";
		}
	}
}

GamepadImpl::~GamepadImpl()
{
	// vigem_target_x360_unregister_notification
	//
	// We're done with this pad, free resources (this disconnects the virtual device)
	//
	vigem_target_x360_unregister_notification(_gamepad);
	PVIGEM_CLIENT client = VigemClient::get();
	if (isInitialized())
	{
		if (client)
			vigem_target_remove(client, _gamepad);
		vigem_target_free(_gamepad);
	}
}

void GamepadImpl::init_x360()
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

	error = vigem_target_x360_register_notification(VigemClient::get(), _gamepad, &GamepadImpl::x360Notification, this);
	if (!VIGEM_SUCCESS(error))
	{
		std::stringstream ss;
		ss << "Target plugin failed: " << error;
		_errorMsg = ss.str();
	}
}

void GamepadImpl::init_ds4()
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

	error = vigem_target_ds4_register_notification(VigemClient::get(), _gamepad, reinterpret_cast<PFN_VIGEM_DS4_NOTIFICATION>(&GamepadImpl::ds4Notification), this);
	if (!VIGEM_SUCCESS(error))
	{
		std::stringstream ss;
		ss << "Target plugin failed: " << error;
		_errorMsg = ss.str();
	}
}

bool GamepadImpl::isInitialized(std::string *errorMsg)
{
	if (!_errorMsg.empty() && errorMsg != nullptr)
	{
		*errorMsg = _errorMsg;
	}
	return _errorMsg.empty() && vigem_target_is_attached(_gamepad) == TRUE;
}

void GamepadImpl::setButton(KeyCode btn, bool pressed)
{
	setButtonDS4(btn, pressed);
	setButtonX360(btn, pressed);
}

ControllerScheme GamepadImpl::getType() const
{
	VIGEM_TARGET_TYPE type = vigem_target_get_type(_gamepad);
	switch (type)
	{
	case VIGEM_TARGET_TYPE::DualShock4Wired:
		return ControllerScheme::DS4;
	case VIGEM_TARGET_TYPE::Xbox360Wired:
	case VIGEM_TARGET_TYPE::XboxOneWired:
		return ControllerScheme::XBOX;
	default:
		return ControllerScheme::INVALID;
	}
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

void GamepadImpl::setButtonX360(KeyCode btn, bool pressed)
{
	decltype(&SetPressed<WORD>) op = pressed ? &SetPressed<WORD> : &ClearPressed<WORD>;

	switch (btn.code)
	{
	case X_UP:
		op(_stateX360->wButtons, XUSB_GAMEPAD_DPAD_UP);
		break;
	case X_DOWN:
		op(_stateX360->wButtons, XUSB_GAMEPAD_DPAD_DOWN);
		break;
	case X_LEFT:
		op(_stateX360->wButtons, XUSB_GAMEPAD_DPAD_LEFT);
		break;
	case X_RIGHT:
		op(_stateX360->wButtons, XUSB_GAMEPAD_DPAD_RIGHT);
		break;
	case X_LB:
		op(_stateX360->wButtons, XUSB_GAMEPAD_LEFT_SHOULDER);
		break;
	case X_BACK:
		op(_stateX360->wButtons, XUSB_GAMEPAD_BACK);
		break;
	case X_X:
		op(_stateX360->wButtons, XUSB_GAMEPAD_X);
		break;
	case X_A:
		op(_stateX360->wButtons, XUSB_GAMEPAD_A);
		break;
	case X_Y:
		op(_stateX360->wButtons, XUSB_GAMEPAD_Y);
		break;
	case X_B:
		op(_stateX360->wButtons, XUSB_GAMEPAD_B);
		break;
	case X_RB:
		op(_stateX360->wButtons, XUSB_GAMEPAD_RIGHT_SHOULDER);
		break;
	case X_START:
		op(_stateX360->wButtons, XUSB_GAMEPAD_START);
		break;
	case X_LS:
		op(_stateX360->wButtons, XUSB_GAMEPAD_LEFT_THUMB);
		break;
	case X_RS:
		op(_stateX360->wButtons, XUSB_GAMEPAD_RIGHT_THUMB);
		break;
	case X_GUIDE:
		op(_stateX360->wButtons, XUSB_GAMEPAD_GUIDE);
		break;
	default:
		break;
	}
}

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

void GamepadImpl::setButtonDS4(KeyCode btn, bool pressed)
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
		PSHat hat(_stateDS4->wButtons & 0x000F);
		_stateDS4->wButtons = (_stateDS4->wButtons & 0xFFF0) | (pressed ? hat.set(btn.code) : hat.clear(btn.code));
	}
	break;

	case PS_HOME:
		op_b(_stateDS4->bSpecial, DS4_SPECIAL_BUTTON_PS);
		break;
	case PS_PAD_CLICK:
		op_b(_stateDS4->bSpecial, DS4_SPECIAL_BUTTON_TOUCHPAD);
		break;
	case PS_L1:
		op_w(_stateDS4->wButtons, DS4_BUTTON_SHOULDER_LEFT);
		break;
	case PS_SHARE:
		op_w(_stateDS4->wButtons, DS4_BUTTON_SHARE);
		break;
	case PS_SQUARE:
		op_w(_stateDS4->wButtons, DS4_BUTTON_SQUARE);
		break;
	case PS_CROSS:
		op_w(_stateDS4->wButtons, DS4_BUTTON_CROSS);
		break;
	case PS_TRIANGLE:
		op_w(_stateDS4->wButtons, DS4_BUTTON_TRIANGLE);
		break;
	case PS_CIRCLE:
		op_w(_stateDS4->wButtons, DS4_BUTTON_CIRCLE);
		break;
	case PS_R1:
		op_w(_stateDS4->wButtons, DS4_BUTTON_SHOULDER_RIGHT);
		break;
	case PS_OPTIONS:
		op_w(_stateDS4->wButtons, DS4_BUTTON_OPTIONS);
		break;
	case PS_L3:
		op_w(_stateDS4->wButtons, DS4_BUTTON_THUMB_LEFT);
		break;
	case PS_R3:
		op_w(_stateDS4->wButtons, DS4_BUTTON_THUMB_RIGHT);
		break;
	default:
		break;
	}
}

void GamepadImpl::setLeftStick(float x, float y)
{
	if (_resetAnalogData[AnalogElement::LSTICK])
	{
		_stateX360->sThumbLX = 0;
		_stateX360->sThumbLY = 0;
		_stateDS4->bThumbLX = 0;
		_stateDS4->bThumbLY = 0;
		_resetAnalogData[AnalogElement::LSTICK] = false;
	}

	_stateX360->sThumbLX = clamp(int(_stateX360->sThumbLX + SHRT_MAX*clamp(x, -1.f, 1.f)), SHRT_MIN, SHRT_MAX);
	_stateX360->sThumbLY = clamp(int(_stateX360->sThumbLY + SHRT_MAX*clamp(y, -1.f, 1.f)), SHRT_MIN, SHRT_MAX);

	_stateDS4->bThumbLX = clamp(int(_stateDS4->bThumbLX + UCHAR_MAX*(clamp(x / 2.f, -.5f, .5f) + .5f)), 0, UCHAR_MAX);
	_stateDS4->bThumbLY = clamp(int(_stateDS4->bThumbLY + UCHAR_MAX*(clamp(-y / 2.f, -.5f, .5f) + .5f)), 0, UCHAR_MAX);
}

void GamepadImpl::setRightStick(float x, float y)
{
	if (_resetAnalogData[AnalogElement::RSTICK])
	{
		_stateX360->sThumbRX = 0;
		_stateX360->sThumbRY = 0;
		_stateDS4->bThumbRX = 0;
		_stateDS4->bThumbRY = 0;
		_resetAnalogData[AnalogElement::RSTICK] = false;
	}

	_stateX360->sThumbRX = clamp(int(_stateX360->sThumbRX + SHRT_MAX*clamp(x, -1.f, 1.f)), SHRT_MIN, SHRT_MAX);
	_stateX360->sThumbRY = clamp(int(_stateX360->sThumbRY + SHRT_MAX*clamp(y, -1.f, 1.f)), SHRT_MIN, SHRT_MAX);

	_stateDS4->bThumbRX = clamp(int(_stateDS4->bThumbRX + UCHAR_MAX*(clamp(x / 2.f, -.5f, .5f) + .5f)), 0, UCHAR_MAX);
	_stateDS4->bThumbRY = clamp(int(_stateDS4->bThumbRY + UCHAR_MAX*(clamp(-y / 2.f, -.5f, .5f) + .5f)), 0, UCHAR_MAX);
}

void GamepadImpl::setStick(float x, float y, bool isLeft)
{
	if (isLeft)
	{
		setLeftStick(x, y);
	}
	else
	{
		setRightStick(x, y);
	}
}

void GamepadImpl::setLeftTrigger(float val)
{
	if (_resetAnalogData[AnalogElement::LTRIG])
	{
		_stateX360->bLeftTrigger = 0;
		_stateDS4->bTriggerL = 0;
		_resetAnalogData[AnalogElement::LTRIG] = false;
	}

	_stateX360->bLeftTrigger = clamp(int(_stateX360->bLeftTrigger + uint8_t(clamp(val, 0.f, 1.f) * UCHAR_MAX)), 0, UCHAR_MAX);
	_stateDS4->bTriggerL = _stateX360->bLeftTrigger;
	if (val > 0)
		SetPressed(_stateDS4->wButtons, DS4_BUTTON_TRIGGER_LEFT);
	else
		ClearPressed(_stateDS4->wButtons, DS4_BUTTON_TRIGGER_LEFT);
}

void GamepadImpl::setRightTrigger(float val)
{
	if (_resetAnalogData[AnalogElement::RTRIG])
	{
		_stateX360->bRightTrigger = 0;
		_stateDS4->bTriggerR = 0;
		_resetAnalogData[AnalogElement::RTRIG] = false;
	}

	_stateX360->bRightTrigger = clamp(int(_stateX360->bRightTrigger + uint8_t(clamp(val, 0.f, 1.f) * UCHAR_MAX)), 0, UCHAR_MAX);
	_stateDS4->bTriggerR = _stateX360->bRightTrigger;
	if (val > 0)
		SetPressed(_stateDS4->wButtons, DS4_BUTTON_TRIGGER_RIGHT);
	else
		ClearPressed(_stateDS4->wButtons, DS4_BUTTON_TRIGGER_RIGHT);
}

void GamepadImpl::update()
{
	if (isInitialized())
	{
		VIGEM_TARGET_TYPE type = vigem_target_get_type(_gamepad);
		switch (type)
		{
		case VIGEM_TARGET_TYPE::DualShock4Wired:
			vigem_target_ds4_update(VigemClient::get(), _gamepad, *_stateDS4.get());
			break;
		case VIGEM_TARGET_TYPE::Xbox360Wired:
		case VIGEM_TARGET_TYPE::XboxOneWired:
			vigem_target_x360_update(VigemClient::get(), _gamepad, *_stateX360.get());
			break;
		}
		_resetAnalogData.clear();
		_resetAnalogData.resize(AnalogElement::COUNT, true);
	}
}

Gamepad *Gamepad::getNew(ControllerScheme scheme, Callback notification)
{
	return new GamepadImpl(scheme, notification);
}
