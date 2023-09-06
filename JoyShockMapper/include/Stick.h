#pragma once

#include "JoyShockMapper.h"
#include "DigitalButton.h"
#include <chrono>

class JoyShock;

class ScrollAxis
{
protected:
	float _leftovers;
	DigitalButton* _negativeButton;
	DigitalButton* _positiveButton;
	int _touchpadId;
	ButtonID _pressedBtn;

public:
	static function<void(JoyShock*, ButtonID, int, bool)> _handleButtonChange;

	ScrollAxis()
		: _leftovers(0.f)
		, _negativeButton(nullptr)
		, _positiveButton(nullptr)
		, _touchpadId(-1)
		, _pressedBtn(ButtonID::NONE)
	{
	}

	void init(DigitalButton& negativeBtn, DigitalButton& positiveBtn, int touchpadId = -1);

	inline bool isInitialized() const
	{
		return _negativeButton && _positiveButton;
	}

	void processScroll(float distance, float sens, chrono::steady_clock::time_point now);

	void reset(chrono::steady_clock::time_point now);
};


struct Stick
{
	Stick(SettingID innerDeadzone,
		SettingID outerDeadzone,
		SettingID ringMode,
		SettingID stickMode,
		ButtonID ringId,
		ButtonID leftId,
		ButtonID rightId,
		ButtonID upId,
		ButtonID downId)
		: _innerDeadzone(innerDeadzone)
		, _outerDeadzone(outerDeadzone)
		, _ringMode(ringMode)
		, _stickMode(stickMode)
		, _ringId(ringId)
		, _leftId(leftId)
		, _rightId(rightId)
		, _upId(upId)
		, _downId(downId)
	{
	}
	SettingID _innerDeadzone;
	SettingID _outerDeadzone;
	SettingID _ringMode;
	SettingID _stickMode;
	ButtonID _ringId;
	ButtonID _leftId;
	ButtonID _rightId;
	ButtonID _upId;
	ButtonID _downId;
	int _touchpadIndex = -1;

	// Flick stick
	chrono::steady_clock::time_point started_flick;
	bool is_flicking = false;
	float delta_flick = 0.0;
	float flick_percent_done = 0.0;
	float flick_rotation_counter = 0.0;
	ScrollAxis scroll;
	float acceleration = 1.0;

	// Modeshifting the stick mode can create quirky behaviours on transition. These flags
	// will be set upon returning to default mode and ignore stick inputs until the stick
	// returns to neutral
	bool ignore_stick_mode = false;

	float lastX = 0.f;
	float lastY = 0.f;

	// Hybrid aim
	float edgePushAmount = 0.0f;
	float smallestMagnitude = 0.f;
	static constexpr int SMOOTHING_STEPS = 4;
	float previousOutputX[SMOOTHING_STEPS] = {};
	float previousOutputY[SMOOTHING_STEPS] = {};
	float previousOutputRadial[SMOOTHING_STEPS] = {};
	float previousVelocitiesX[SMOOTHING_STEPS] = {};
	float previousVelocitiesY[SMOOTHING_STEPS] = {};
	int smoothingCounter = 0;
};

struct TouchStick : public Stick
{
	FloatXY _currentLocation = { 0.f, 0.f };
	bool _prevDown = false;
	// Handle a single touch related action. On per touch point
	ScrollAxis verticalScroll;
	map<ButtonID, DigitalButton> buttons; // Each touchstick gets it's own digital buttons. Is that smart?

	TouchStick(int index, shared_ptr<DigitalButton::Context> common, int handle);

	inline bool wasDown() const
	{
		return _prevDown;
	}
};

