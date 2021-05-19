#pragma once

#include "pocket_fsm.h"
#include "JoyShockMapper.h"
#include "Gamepad.h"
#include <chrono>
#include <deque>
#include <mutex>

// Forward declarations
class JSMButton;
class GamepadMotion;
class DigitalButton;      // Finite State Machine
struct DigitalButtonImpl; // Button implementation

// The enum values match the concrete class names
enum class BtnState
{
	NoPress,
	BtnPress,
	TapPress,
	WaitSim,
	SimPressMaster,
	SimPressSlave,
	SimRelease,
	DblPressStart,
	DblPressNoPress,
	DblPressNoPressTap,
	DblPressNoPressHold,
	DblPressPress,
	InstRelease,
	INVALID
};

// Send this event anytime the button is pressed down or active
struct Pressed
{
	chrono::steady_clock::time_point time_now; // Timestamp of the poll
	float turboTime;                           // active turbo period setting in ms
	float holdTime;                            // active hold press setting in ms
	float dblPressWindow;					   // active dbl press window setting in ms
};

// Send this event anytime the button is at rest or inactive
struct Released
{
	chrono::steady_clock::time_point time_now; // Timestamp of the poll
	float turboTime;                           // active turbo period setting in ms
	float holdTime;                            // active hold press setting in ms
	float dblPressWindow;					   // active dbl press window setting in ms
};

// The sync event is created internally
struct Sync;

// Getter for the button press duration. Pass timestamp of the poll.
struct GetDuration
{
	chrono::steady_clock::time_point in_now;
	float out_duration;
};

// Setter for the press time
typedef chrono::steady_clock::time_point SetPressTime;

// A basic digital button state reacts to the following events
class DigitalButtonState : public pocket_fsm::StatePimplIF<DigitalButtonImpl>
{
	BASE_STATE(DigitalButtonState)

	// Display logs on entry for debigging
	REACT(OnEntry)
	override;

	// ignored
	REACT(OnExit)
	override { }

	// Adds chord to stack if absent
	REACT(Pressed);

	// Remove chord from stack if present
	REACT(Released);

	// ignored by default
	REACT(Sync) { }

	// Always assign press time
	REACT(SetPressTime)
	final;

	// Return press duration
	REACT(GetDuration)
	final;

	// Get matching enum value
	virtual BtnState getState() const = 0;
};

// Feed this state machine with Pressed and Released events and it will sort out
// what mappings to activate internally
class DigitalButton : public pocket_fsm::FiniteStateMachine<DigitalButtonState>
{
public:
	// All digital buttons need a reference to the same instance of the common structure within the same controller.
	// It enables the buttons to synchronize and be aware of the state of the whole controller, access gyro etc...
	struct Common
	{
		Common(Gamepad::Callback virtualControllerCallback, GamepadMotion *mainMotion);
		bool HasActiveToggle(const KeyCode &key) const;
		deque<pair<ButtonID, KeyCode>> gyroActionQueue; // Queue of gyro control actions currently in effect
		deque<pair<ButtonID, KeyCode>> activeTogglesQueue;
		deque<ButtonID> chordStack; // Represents the current active buttons in order from most recent to latest
		unique_ptr<Gamepad> _vigemController;
		function<DigitalButton *(ButtonID)> _getMatchingSimBtn; // A functor to JoyShock::GetMatchingSimBtn
		function<void(int small, int big)> _rumble;             // A functor to JoyShock::Rumble
		mutex callback_lock;                                    // Needs to be in the common struct for both joycons to use the same
		GamepadMotion *rightMainMotion = nullptr;
		GamepadMotion *leftMotion = nullptr;
	};

	DigitalButton(shared_ptr<DigitalButton::Common> btnCommon, JSMButton &mapping);

	const ButtonID _id;

	// Get the enum identifier of the current state
	BtnState getState() const
	{
		return getCurrentState()->getState();
	}
};
