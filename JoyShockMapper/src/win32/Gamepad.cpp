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

static PVIGEM_CLIENT client = nullptr;
static size_t clientCounter = 0;


Gamepad::Gamepad()
{
	clientCounter++;
	if (client == nullptr)
	{
		client = vigem_alloc();
		if (client == nullptr)
		{
			std::stringstream ss;
			ss << "Uh, not enough memory to do that?!" << std::endl;
			_errorMsg = ss.str();
			return;
		}

		const auto retval = vigem_connect(client);

		if (!VIGEM_SUCCESS(retval))
		{
			std::stringstream ss;
			ss << "ViGEm Bus connection failed with error code: 0x" << std::hex << uint32_t(retval) << std::endl;
			_errorMsg = ss.str();
			return;
		}
	}

	//
	// Allocate handle to identify new pad
	//
	_gamepad = vigem_target_x360_alloc();

	//
	// Add client to the bus, this equals a plug-in event
	//
	const auto err = vigem_target_add(client, _gamepad);

	//
	// Error handling
	//
	if (!VIGEM_SUCCESS(err))
	{
		std::stringstream ss;
		ss << "Target plugin failed with error code: 0x" << std::hex << uint32_t(err) << std::endl;
		_errorMsg = ss.str();
	}

	// vigem_target_x360_register_notification
}

Gamepad::~Gamepad()
{
	// vigem_target_x360_unregister_notification
	//
	// We're done with this pad, free resources (this disconnects the virtual device)
	//
	vigem_target_remove(client, _gamepad);
	vigem_target_free(_gamepad);
	if (--clientCounter == 0)
	{
		vigem_disconnect(client);
		vigem_free(client);
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

Gamepad::TargetUpdate Gamepad::getUpdater()
{
	return [this] (XINPUT_GAMEPAD *state)
	{
		vigem_target_x360_update(client, _gamepad, *reinterpret_cast<_XUSB_REPORT*>(state));
	};
}

Gamepad::Update::Update(Gamepad & gamepad)
	: _state(new XINPUT_GAMEPAD)
{
	_targetUpdate = gamepad.getUpdater();
}

Gamepad::Update::~Update() {
	if(_targetUpdate)
		_targetUpdate(_state.get());
}

WORD SetPressed(WORD buttons, WORD mask) { return buttons | mask; }
WORD ClearPressed(WORD buttons, WORD mask) { return buttons & ~mask; }

void Gamepad::Update::setButton(ButtonID btn, bool pressed) {
	decltype(&SetPressed) op = pressed ? &SetPressed : &ClearPressed;

	switch (btn)
	{
	case ButtonID::UP:
		op(_state->wButtons, XINPUT_GAMEPAD_DPAD_UP);
		break;
	case ButtonID::DOWN:
		op(_state->wButtons, XINPUT_GAMEPAD_DPAD_DOWN);
		break;
	case ButtonID::LEFT:
		op(_state->wButtons, XINPUT_GAMEPAD_DPAD_LEFT);
		break;
	case ButtonID::RIGHT:
		op(_state->wButtons, XINPUT_GAMEPAD_DPAD_RIGHT);
		break;
	case ButtonID::L:
		op(_state->wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER);
		break;
	case ButtonID::MINUS:
		op(_state->wButtons, XINPUT_GAMEPAD_BACK);
		break;
	case ButtonID::E:
		op(_state->wButtons, XINPUT_GAMEPAD_X);
		break;
	case ButtonID::S:
		op(_state->wButtons, XINPUT_GAMEPAD_A);
		break;
	case ButtonID::N:
		op(_state->wButtons, XINPUT_GAMEPAD_Y);
		break;
	case ButtonID::W:
		op(_state->wButtons, XINPUT_GAMEPAD_B);
		break;
	case ButtonID::R:
		op(_state->wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER);
		break;
	case ButtonID::PLUS:
		op(_state->wButtons, XINPUT_GAMEPAD_START);
		break;
	case ButtonID::L3:
		op(_state->wButtons, XINPUT_GAMEPAD_LEFT_THUMB);
		break;
	case ButtonID::R3:
		op(_state->wButtons, XINPUT_GAMEPAD_RIGHT_THUMB);
		break;
	default:
		break;
	}
}

void Gamepad::Update::setLeftStick(float x, float y) 
{
	_state->sThumbLX = uint16_t(clamp(x, 0.f, 1.f) * SHRT_MAX);
	_state->sThumbLY = uint16_t(clamp(y, 0.f, 1.f) * SHRT_MAX);
}

void Gamepad::Update::setRightStick(float x, float y) 
{
	_state->sThumbRX = uint16_t(clamp(x, 0.f, 1.f) * SHRT_MAX);
	_state->sThumbRY = uint16_t(clamp(y, 0.f, 1.f) * SHRT_MAX);
}

void Gamepad::Update::setLeftTrigger(float val)
{
	_state->bLeftTrigger = uint8_t(clamp(val, 0.f, 1.f) * UCHAR_MAX);
}

void Gamepad::Update::setRightTrigger(float val) 
{
	_state->bRightTrigger = uint8_t(clamp(val, 0.f, 1.f) * UCHAR_MAX);
}

void Gamepad::Update::send()
{
	_targetUpdate(_state.get());
	_targetUpdate = nullptr;
}