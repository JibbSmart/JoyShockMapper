#include "win32/Gamepad.h"
#include <Xinput.h>

//
// Windows basic types 'n' fun
//

#include "ViGEm/Client.h"
#include <string>
#include <sstream>

//
// Link against SetupAPI
//
#pragma comment(lib, "setupapi.lib")

class VigemClient
{
	class Deleter
	{
	public:
		void operator()(VigemClient *vc)
		{
			delete vc;
		}
	};
	// singleton
	static unique_ptr<VigemClient, VigemClient::Deleter> _inst;

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

	static PVIGEM_CLIENT get(VIGEM_ERROR *outError = nullptr)
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

unique_ptr<VigemClient, VigemClient::Deleter> VigemClient::_inst;

template <>
ostream & operator <<<VIGEM_ERROR>(ostream &out, VIGEM_ERROR errCode)
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

Gamepad::Gamepad()
	: _state( new XINPUT_GAMEPAD )
{
	memset(_state.get(), 0, sizeof(XINPUT_GAMEPAD));

	std::stringstream ss;
	VIGEM_ERROR error = VIGEM_ERROR::VIGEM_ERROR_NONE;
	PVIGEM_CLIENT client = VigemClient::get(&error);
	if (client == nullptr)
	{
		ss << "Uh, not enough memory to do that?!";
		_errorMsg = ss.str();
	}
	else if (error == VIGEM_ERROR_BUS_NOT_FOUND)
	{
		ss << "ViGEm bus is not installed. You can download the latest version of it here:" << endl
			<< "https://github.com/ViGEm/ViGEmBus/releases/latest";
		_errorMsg = ss.str();
	}
	else if(!VIGEM_SUCCESS(error))
	{
		ss << "ViGEm Bus connection failed: " << error;
		_errorMsg = ss.str();
	}
	else
	{
		// Allocate handle to identify new pad
		_gamepad = vigem_target_x360_alloc();

		// Add _client to the bus, this equals a plug-in event
		error = vigem_target_add(client, _gamepad);
		if (!VIGEM_SUCCESS(error))
		{
			std::stringstream ss;
			ss << "Target plugin failed: " << error;
			_errorMsg = ss.str();
		}

		if (vigem_target_is_attached(_gamepad) != TRUE)
		{
			_errorMsg = "Target is not attached";
		}

		// vigem_target_x360_register_notification => rumble, player number
	}
}

Gamepad::~Gamepad()
{
	// vigem_target_x360_unregister_notification
	//
	// We're done with this pad, free resources (this disconnects the virtual device)
	//
	PVIGEM_CLIENT client = VigemClient::get();
	if (isInitialized())
	{
		if(client)
			vigem_target_remove(client, _gamepad);
		vigem_target_free(_gamepad);
	}
}

bool Gamepad::isInitialized(std::string *errorMsg)
{
	if (!_errorMsg.empty() && errorMsg != nullptr)
	{
		*errorMsg = _errorMsg;
	}
	return _errorMsg.empty() && vigem_target_is_attached(_gamepad) == TRUE;
}

void SetPressed(WORD &buttons, WORD mask) { buttons |= mask; }
void ClearPressed(WORD &buttons, WORD mask) { buttons &= ~mask; }

void Gamepad::setButton(KeyCode btn, bool pressed) {
	decltype(&SetPressed) op = pressed ? &SetPressed : &ClearPressed;

	switch (btn.code)
	{
	case X_UP:
		op(_state->wButtons, XINPUT_GAMEPAD_DPAD_UP);
		break;
	case X_DOWN:
		op(_state->wButtons, XINPUT_GAMEPAD_DPAD_DOWN);
		break;
	case X_LEFT:
		op(_state->wButtons, XINPUT_GAMEPAD_DPAD_LEFT);
		break;
	case X_RIGHT:
		op(_state->wButtons, XINPUT_GAMEPAD_DPAD_RIGHT);
		break;
	case X_LB:
		op(_state->wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER);
		break;
	case X_BACK:
		op(_state->wButtons, XINPUT_GAMEPAD_BACK);
		break;
	case X_X:
		op(_state->wButtons, XINPUT_GAMEPAD_X);
		break;
	case X_A:
		op(_state->wButtons, XINPUT_GAMEPAD_A);
		break;
	case X_Y:
		op(_state->wButtons, XINPUT_GAMEPAD_Y);
		break;
	case X_B:
		op(_state->wButtons, XINPUT_GAMEPAD_B);
		break;
	case X_RB:
		op(_state->wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER);
		break;
	case X_START:
		op(_state->wButtons, XINPUT_GAMEPAD_START);
		break;
	case X_LS:
		op(_state->wButtons, XINPUT_GAMEPAD_LEFT_THUMB);
		break;
	case X_RS:
		op(_state->wButtons, XINPUT_GAMEPAD_RIGHT_THUMB);
		break;
	default:
		break;
	}
}

void Gamepad::setLeftStick(float x, float y) 
{
	_state->sThumbLX = uint16_t(clamp(x, -1.f, 1.f) * SHRT_MAX);
	_state->sThumbLY = uint16_t(clamp(y, -1.f, 1.f) * SHRT_MAX);
}

void Gamepad::setRightStick(float x, float y) 
{
	_state->sThumbRX = uint16_t(clamp(x, -1.f, 1.f) * SHRT_MAX);
	_state->sThumbRY = uint16_t(clamp(y, -1.f, 1.f) * SHRT_MAX);
}

void Gamepad::setLeftTrigger(float val)
{
	_state->bLeftTrigger = uint8_t(clamp(val, 0.f, 1.f) * UCHAR_MAX);
}

void Gamepad::setRightTrigger(float val) 
{
	_state->bRightTrigger = uint8_t(clamp(val, 0.f, 1.f) * UCHAR_MAX);
}

void Gamepad::update()
{
	if (isInitialized())
	{
		vigem_target_x360_update(VigemClient::get(), _gamepad, *reinterpret_cast<XUSB_REPORT*>(_state.get()));
	}
}
