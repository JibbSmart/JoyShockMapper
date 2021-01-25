#pragma once

#include "JoyShockMapper.h"
#include <functional>

// Forward Declare and typedefs
typedef struct _VIGEM_TARGET_T *PVIGEM_TARGET;
typedef struct _VIGEM_CLIENT_T *PVIGEM_CLIENT;
typedef struct _XINPUT_GAMEPAD XINPUT_GAMEPAD;
typedef enum _VIGEM_ERRORS VIGEM_ERRORS;

class Gamepad
{
public:
	typedef function<void(UCHAR largeMotor, UCHAR smallMotor, UCHAR ledNumber)> Notification;
	Gamepad();
	virtual ~Gamepad();

	bool isInitialized(std::string *errorMsg = nullptr);
	inline string getError() const
	{
		return _errorMsg;
	}

	void setButton(KeyCode btn, bool pressed);
	void setLeftStick(float x, float y);
	void setRightStick(float x, float y);
	void setLeftTrigger(float);
	void setRightTrigger(float);
	void update();

	Notification _notification = nullptr;

private:
	static void x360Notification(
		PVIGEM_CLIENT client,
		PVIGEM_TARGET target,
		UCHAR largeMotor,
		UCHAR smallMotor,
		UCHAR ledNumber,
		LPVOID userData
	);

	std::string _errorMsg;
	PVIGEM_TARGET _gamepad = nullptr;
	unique_ptr<XINPUT_GAMEPAD> _state;
};
