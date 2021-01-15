#pragma once

#include "JoyShockMapper.h"

// Forward Declare and typedefs
typedef struct _VIGEM_TARGET_T VIGEM_TARGET;
typedef struct _XINPUT_GAMEPAD XINPUT_GAMEPAD;
typedef enum _VIGEM_ERRORS VIGEM_ERRORS;

class Gamepad
{
public:
	using TargetUpdate = function<void(XINPUT_GAMEPAD*)>;

	class Update
	{
	public:
		Update(Gamepad &gamepad);
		~Update();

		void setButton(ButtonID btn, bool pressed);
		void setLeftStick(float x, float y);
		void setRightStick(float x, float y);
		void setLeftTrigger(float);
		void setRightTrigger(float);
		void send();
	private:
		unique_ptr<XINPUT_GAMEPAD> _state;
		TargetUpdate _targetUpdate;
	};

	Gamepad();
	virtual ~Gamepad();

	bool isInitialized(std::string *errorMsg = nullptr);

	TargetUpdate getUpdater();

private:
	std::string _errorMsg;
	VIGEM_TARGET *_gamepad = nullptr;
};
