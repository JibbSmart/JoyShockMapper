#include "Gamepad.h"

class GamepadImpl : public Gamepad
{
public:
	// TODO: Implement this class by interacting with some external virtual controller software

	GamepadImpl()
	{

	}

	virtual ~GamepadImpl()
	{

	}

	virtual bool isInitialized(std::string* errorMsg = nullptr) override
	{

	}

	virtual void setButton(KeyCode btn, bool pressed) override
	{

	}

	virtual void setLeftStick(float x, float y) override
	{

	}

	virtual void setRightStick(float x, float y) override
	{

	}

	virtual void setLeftTrigger(float) override
	{

	}

	virtual void setRightTrigger(float) override
	{
	
	}

	virtual void update() override
	{

	}
}

Gamepad *Gamepad::getNew(ControllerScheme scheme, Callback notification)
{
	// return new GamepadImpl();
	return nullptr;
}
