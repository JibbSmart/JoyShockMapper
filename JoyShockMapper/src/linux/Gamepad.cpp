#include "Gamepad.h"

struct _XUSB_REPORT
{
};

struct _DS4_REPORT
{
};
/*
void Gamepad::x360Notification(
  PVIGEM_CLIENT client,
  PVIGEM_TARGET target,
  uint8_t largeMotor,
  uint8_t smallMotor,
  uint8_t ledNumber,
  void *userData)
{
}

void Gamepad::ds4Notification(
  PVIGEM_CLIENT client,
  PVIGEM_TARGET target,
  uint8_t largeMotor,
  uint8_t smallMotor,
  Indicator lightbarColor,
  void *userData)
{
}


Gamepad::Gamepad(ControllerScheme scheme)
  : _stateX360()
  , _stateDS4()
  , _notification()
{
}

Gamepad::Gamepad(ControllerScheme scheme, Callback notification)
  : Gamepad(scheme)
{
}

Gamepad::~Gamepad()
{
}

void Gamepad::init_x360()
{
}

void Gamepad::init_ds4()
{
}
*/

bool Gamepad::isInitialized(std::string *errorMsg)
{
	return false;
}

void Gamepad::setButton(KeyCode btn, bool pressed)
{
}

ControllerScheme Gamepad::getType() const
{
	return ControllerScheme::INVALID;
}


void Gamepad::setLeftStick(float x, float y)
{
}

void Gamepad::setRightStick(float x, float y)
{
}

void Gamepad::setLeftTrigger(float val)
{
}

void Gamepad::setRightTrigger(float val)
{
}

void Gamepad::update()
{
}
