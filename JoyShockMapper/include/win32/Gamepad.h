#pragma once

#include "JoyShockMapper.h"
#include <functional>

// Forward Declare and typedefs
typedef struct _VIGEM_TARGET_T *PVIGEM_TARGET;
typedef struct _VIGEM_CLIENT_T *PVIGEM_CLIENT;
typedef struct _XINPUT_GAMEPAD XINPUT_GAMEPAD;
typedef struct _DS4_REPORT DS4_REPORT;
typedef enum _VIGEM_ERRORS VIGEM_ERRORS;

union Indicator
{
	UCHAR led;
	UCHAR rgb[3];
	UINT32 colorCode;
};

class Gamepad
{
public:
	typedef function<void(UCHAR largeMotor, UCHAR smallMotor, Indicator indicator)> Notification;
	Gamepad(ControllerScheme scheme);
	Gamepad(ControllerScheme scheme, Notification notification);
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

	ControllerScheme getType() const;

private:
	static void x360Notification(
		PVIGEM_CLIENT client,
		PVIGEM_TARGET target,
		UCHAR largeMotor,
		UCHAR smallMotor,
		UCHAR ledNumber,
		LPVOID userData
	);

	static void ds4Notification(
		PVIGEM_CLIENT client,
		PVIGEM_TARGET target,
		UCHAR largeMotor,
		UCHAR smallMotor,
		Indicator lightbarColor,
		LPVOID userData
	);


	void init_x360();
	void init_ds4();

	void setButtonX360(KeyCode btn, bool pressed);
	void setButtonDS4(KeyCode btn, bool pressed);

	Notification _notification = nullptr;
	std::string _errorMsg;
	PVIGEM_TARGET _gamepad = nullptr;
	unique_ptr<XINPUT_GAMEPAD> _stateX360;
	unique_ptr<DS4_REPORT> _stateDS4;
};
