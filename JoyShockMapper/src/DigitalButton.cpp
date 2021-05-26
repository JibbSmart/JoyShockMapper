#include "DigitalButton.h"
#include "JSMVariable.hpp"
#include "GamepadMotion.h"
#include "InputHelpers.h"

// JSM global variables
extern JSMVariable<ControllerScheme> virtual_controller;
extern JSMVariable<float> sim_press_window;
extern JSMSetting<JoyconMask> joycon_gyro_mask;
extern JSMSetting<JoyconMask> joycon_motion_mask;

struct Sync
{
	pocket_fsm::StateIF *nextState;
	chrono::steady_clock::time_point pressTime;
	Mapping *activeMapping;
	string nameToRelease;
	float turboTime;
	float holdTime;
	float dblPressWindow;
};

// Hidden implementation of the digital button
// This class holds all the logic related to a single digital button. It does not hold the mapping but only a reference
// to it. It also contains it's various states, flags and data. The concrete state of the state machine hands off the
// instance to the next state, and so is persistent across states
struct DigitalButtonImpl : public pocket_fsm::PimplBase, public EventActionIf
{
private:
	static bool isSameKey (KeyCode key, const pair<ButtonID, KeyCode> &pair) {
		return pair.second == key;
	};

public:
	vector<BtnEvent> _instantReleaseQueue;
	unsigned int _turboCount = 0;
	DigitalButtonImpl(JSMButton &mapping, shared_ptr<DigitalButton::Common> common)
	  : _id(mapping._id)
	  , _common(common)
	  , _press_times()
	  , _keyToRelease()
	  , _mapping(mapping)
	  , _instantReleaseQueue()
	{
		_instantReleaseQueue.reserve(2);
	}

	const ButtonID _id; // Always ID first for easy debugging
	string _nameToRelease;
	shared_ptr<DigitalButton::Common> _common;
	chrono::steady_clock::time_point _press_times;
	unique_ptr<Mapping> _keyToRelease; // At key press, remember what to release
	const JSMButton &_mapping;
	DigitalButton *_simPressMaster = nullptr;

	// Pretty wrapper
	inline float GetPressDurationMS(chrono::steady_clock::time_point time_now)
	{
		return static_cast<float>(chrono::duration_cast<chrono::milliseconds>(time_now - _press_times).count());
	}

	void ClearKey()
	{
		_keyToRelease.reset();
		_instantReleaseQueue.clear();
		_nameToRelease.clear();
		_turboCount = 0;
	}

	bool ReleaseInstant(BtnEvent instantEvent)
	{
		auto instant = find(_instantReleaseQueue.begin(), _instantReleaseQueue.end(), instantEvent);
		if (instant != _instantReleaseQueue.end())
		{
			// DEBUG_LOG << "Button " << _id << " releases instant " << instantEvent << endl;
			_keyToRelease->ProcessEvent(BtnEvent::OnInstantRelease, *this);
			_instantReleaseQueue.erase(instant);
			return true;
		}
		return false;
	}

	Mapping *GetPressMapping()
	{
		if (!_keyToRelease)
		{
			// Look at active chord mappings starting with the latest activates chord
			for (auto activeChord = _common->chordStack.cbegin(); activeChord != _common->chordStack.cend(); activeChord++)
			{
				auto binding = _mapping.get(*activeChord);
				if (binding && *activeChord != _id)
				{
					_keyToRelease.reset(new Mapping(*binding));
					_nameToRelease = _mapping.getName(*activeChord);
					return _keyToRelease.get();
				}
			}
			// Chord stack should always include NONE which will provide a value in the loop above
			throw runtime_error("ChordStack should always include ButtonID::NONE, for the chorded variable to return the base value.");
		}
		return _keyToRelease.get();
	}

	void RegisterInstant(BtnEvent evt) override
	{
		//COUT << "Button " << _id << " registers instant " << evt << endl;
		_instantReleaseQueue.push_back(evt);
	}

	void ApplyGyroAction(KeyCode gyroAction) override
	{
		_common->gyroActionQueue.push_back({ _id, gyroAction });
	}

	void RemoveGyroAction() override
	{
		auto gyroAction = find_if(_common->gyroActionQueue.begin(), _common->gyroActionQueue.end(),
		  [this](auto pair) {
			  // On a sim press, release the master button (the one who triggered the press)
			  return pair.first == (_simPressMaster ? _simPressMaster->_id : _id);
		  });
		if (gyroAction != _common->gyroActionQueue.end())
		{
			KeyCode key(gyroAction->second);
			ClearAllActiveToggle(key);
			for (auto currentlyActive = find_if(_common->gyroActionQueue.begin(), _common->gyroActionQueue.end(), bind(isSameKey, key, placeholders::_1));
				 currentlyActive != _common->gyroActionQueue.end();
				 currentlyActive = find_if(currentlyActive, _common->gyroActionQueue.end(), bind(isSameKey, key, placeholders::_1)))
			{
				DEBUG_LOG << "Removing active gyro action for " << key.name << endl;
				currentlyActive = _common->gyroActionQueue.erase(currentlyActive);
			}
		}
	}

	void SetRumble(int smallRumble, int bigRumble) override
	{
		DEBUG_LOG << "Rumbling at " << smallRumble << " and " << bigRumble << endl;
		_common->_rumble(smallRumble, bigRumble);
	}

	void ApplyBtnPress(KeyCode key) override
	{
		if (key.code >= X_UP && key.code <= X_START || key.code == PS_HOME || key.code == PS_PAD_CLICK)
		{
			if (_common->_vigemController)
				_common->_vigemController->setButton(key, true);
		}
		else if (key.code != NO_HOLD_MAPPED && _common->HasActiveToggle(key) == false)
		{
			DEBUG_LOG << "Pressing down on key " << key.name << endl;
			pressKey(key, true);
		}
	}

	void ApplyBtnRelease(KeyCode key) override
	{
		if (key.code >= X_UP && key.code <= X_START || key.code == PS_HOME || key.code == PS_PAD_CLICK)
		{
			if (_common->_vigemController)
				_common->_vigemController->setButton(key, false);
		}
		else if (key.code != NO_HOLD_MAPPED)
		{
			DEBUG_LOG << "Releasing key " << key.name << endl;
			pressKey(key, false);
			ClearAllActiveToggle(key);
		}
	}

	void ApplyButtonToggle(KeyCode key, EventActionIf::Callback apply, EventActionIf::Callback release) override
	{
		auto currentlyActive = find_if(_common->activeTogglesQueue.begin(), _common->activeTogglesQueue.end(),
		  [this, key](pair<ButtonID, KeyCode> pair) {
			  return pair.first == _id && pair.second == key;
		  });
		if (currentlyActive == _common->activeTogglesQueue.end())
		{
			DEBUG_LOG << "Adding active toggle for " << key.name << endl;
			apply(this);
			_common->activeTogglesQueue.push_front({ _id, key });
		}
		else
		{
			release(this); // The bound action here should always erase the active toggle from the queue
		}
	}

	void ClearAllActiveToggle(KeyCode key)
	{
		for (auto currentlyActive = find_if(_common->activeTogglesQueue.begin(), _common->activeTogglesQueue.end(), bind(isSameKey, key, placeholders::_1));
		     currentlyActive != _common->activeTogglesQueue.end();
		     currentlyActive = find_if(currentlyActive, _common->activeTogglesQueue.end(), bind(isSameKey, key, placeholders::_1)))
		{
			DEBUG_LOG << "Removing active toggle for " << key.name << endl;
			currentlyActive = _common->activeTogglesQueue.erase(currentlyActive);
		}
	}

	void StartCalibration() override
	{
		COUT << "Starting continuous calibration" << endl;
		if (_common->leftMotion)
		{
			int gyroMask = int(joycon_gyro_mask.get().value());
			if (gyroMask & int(JoyconMask::IGNORE_LEFT) || gyroMask & int(JoyconMask::IGNORE_LEFT))
			{
				_common->rightMainMotion->ResetContinuousCalibration();
				_common->rightMainMotion->StartContinuousCalibration();
			}
			int motionMask = int(joycon_motion_mask.get().value());
			if (motionMask & int(JoyconMask::IGNORE_RIGHT) || motionMask & int(JoyconMask::IGNORE_RIGHT))
			{
				_common->leftMotion->ResetContinuousCalibration();
				_common->leftMotion->StartContinuousCalibration();
			}
		}
		else
		{
			_common->rightMainMotion->ResetContinuousCalibration();
			_common->rightMainMotion->StartContinuousCalibration();
		}
	}

	void FinishCalibration() override
	{
		if (_common->leftMotion)
		{
			int gyroMask = int(joycon_gyro_mask.get().value());
			if (gyroMask & int(JoyconMask::IGNORE_LEFT) || gyroMask & int(JoyconMask::IGNORE_LEFT))
			{
				_common->rightMainMotion->PauseContinuousCalibration();
			}
			int motionMask = int(joycon_motion_mask.get().value());
			if (motionMask & int(JoyconMask::IGNORE_RIGHT) || motionMask & int(JoyconMask::IGNORE_RIGHT))
			{
				_common->rightMainMotion->PauseContinuousCalibration();
			}
		}
		else
		{
			_common->rightMainMotion->PauseContinuousCalibration();
		}
		COUT << "Gyro calibration set" << endl;
		ClearAllActiveToggle(KeyCode("CALIBRATE"));
	}

	const char *getDisplayName() override
	{
		return _nameToRelease.c_str();
	}
};

// Forward declare concrete states
class NoPress;
class BtnPress;
class TapPress;
class WaitSim;
class SimPressMaster;
class SimPressSlave;
class SimRelease;
class DblPressStart;
class DblPressNoPress;
class DblPressNoPressTap;
class DblPressNoPressHold;
class DblPressPress;
class InstRelease;

// Forward declare concrete nested states
class ActiveStartPress;
class ActiveHoldPress;

// Basic state react
void DigitalButtonState::react(OnEntry &e)
{
	// Uncomment below to diplay a log each time a button changes state
	// DEBUG_LOG << "Button " << pimpl()->_id << " is now in state " << _name << endl;
}

// Basic Press reaction should be called in every concrete Press reaction
void DigitalButtonState::react(Pressed &e)
{
	if (pimpl()->_id < ButtonID::SIZE || pimpl()->_id >= ButtonID::T1) // Can't chord touch stick buttons
	{
		auto foundChord = find(pimpl()->_common->chordStack.begin(), pimpl()->_common->chordStack.end(), pimpl()->_id);
		if (foundChord == pimpl()->_common->chordStack.end())
		{
			//COUT << "Button " << index << " is pressed!" << endl;
			pimpl()->_common->chordStack.push_front(pimpl()->_id); // Always push at the fromt to make it a stack
		}
	}
}

// Basic Release reaction should be called in every concrete Release reaction
void DigitalButtonState::react(Released &e)
{
	if (pimpl()->_id < ButtonID::SIZE || pimpl()->_id >= ButtonID::T1) // Can't chord touch stick buttons?!?
	{
		auto foundChord = find(pimpl()->_common->chordStack.begin(), pimpl()->_common->chordStack.end(), pimpl()->_id);
		if (foundChord != pimpl()->_common->chordStack.end())
		{
			//COUT << "Button " << index << " is released!" << endl;
			pimpl()->_common->chordStack.erase(foundChord); // The chord is released
		}
	}
}

void DigitalButtonState::react(chrono::steady_clock::time_point &e)
{
	// final implementation. All states can be assigned a new press time
	pimpl()->_press_times = e;
}

void DigitalButtonState::react(GetDuration &e)
{
	// final implementation. All states can be querried it's duration time.
	e.out_duration = pimpl()->GetPressDurationMS(e.in_now);
}


// Append to pocket_fsm macro
#define DB_CONCRETE_STATE(statename)           \
	CONCRETE_STATE(statename)                  \
	virtual BtnState getState() const override \
	{                                          \
		return BtnState::statename;            \
	}

// Base state for all nested states in which a mapping is active
class ActiveMappingState : public DigitalButtonState {
public:
	virtual BtnState getState() const override
	{
		return BtnState::INVALID;
	}

	REACT(Released) override
	{
		DigitalButtonState::react(e);
		pimpl()->_keyToRelease->ProcessEvent(BtnEvent::OnRelease, *pimpl());
	}

	REACT(Sync)
	final
	{
	    auto rel = Released{ e.pressTime, e.turboTime, e.holdTime };
		react(rel);
		// Redirect change of state to the caller of the Sync
		e.nextState = _nextState;
		_nextState = nullptr;
		changeState<SimRelease>();
	}
};

// Nested concrete states

class ActiveStartPress : public ActiveMappingState
{
	CONCRETE_STATE(ActiveStartPress)
	INITIAL_STATE(ActiveStartPress)

	REACT(OnEntry) override
	{
		DigitalButtonState::react(e);
		pimpl()->GetPressMapping()->ProcessEvent(BtnEvent::OnPress, *pimpl());
	}

	REACT(Pressed) override
	{
		DigitalButtonState::react(e);
		
		auto elapsed_time = pimpl()->GetPressDurationMS(e.time_now);
		if (elapsed_time > MAGIC_INSTANT_DURATION)
		{
			pimpl()->ReleaseInstant(BtnEvent::OnPress);
		}
		if (elapsed_time > e.holdTime)
		{
			pimpl()->_press_times = e.time_now; // Start counting tap duration
			changeState<ActiveHoldPress>();
		}
	}

	REACT(Released) override
	{
		ActiveMappingState::react(e);
		pimpl()->_press_times = e.time_now; // Start counting tap duration
		changeState<TapPress>();
	}

};

class ActiveHoldPress : public ActiveMappingState
{
	CONCRETE_STATE(ActiveHoldPress)

	REACT(OnEntry) override
	{
		DigitalButtonState::react(e);
		pimpl()->_keyToRelease->ProcessEvent(BtnEvent::OnHold, *pimpl());
		pimpl()->_keyToRelease->ProcessEvent(BtnEvent::OnTurbo, *pimpl());
	}

	REACT(Pressed) override
	{
		auto elapsed_time = pimpl()->GetPressDurationMS(e.time_now);
		if (elapsed_time > e.holdTime + MAGIC_INSTANT_DURATION)
		{
			pimpl()->ReleaseInstant(BtnEvent::OnHold);
		}
		if (floorf((elapsed_time - e.holdTime) / e.turboTime) >= pimpl()->_turboCount)
		{
			pimpl()->_keyToRelease->ProcessEvent(BtnEvent::OnTurbo, *pimpl());
			pimpl()->_turboCount++;
		}
		if (elapsed_time > e.holdTime + pimpl()->_turboCount * e.turboTime + MAGIC_INSTANT_DURATION)
		{
			pimpl()->ReleaseInstant(BtnEvent::OnTurbo);
		}
	}

	REACT(Released) override
	{
		ActiveMappingState::react(e);
		pimpl()->_keyToRelease->ProcessEvent(BtnEvent::OnHoldRelease, *pimpl());
		if (pimpl()->_instantReleaseQueue.empty())
		{
			changeState<NoPress>();
			pimpl()->ClearKey();
		}
		else
		{
			changeState<InstRelease>();
			pimpl()->_press_times = e.time_now; // Start counting tap duration
		}
	}
};

// Core concrete states

class NoPress : public DigitalButtonState
{
	DB_CONCRETE_STATE(NoPress)
	INITIAL_STATE(NoPress)

	REACT(Pressed)
	override
	{
		DigitalButtonState::react(e);
		pimpl()->_press_times = e.time_now;
		if (pimpl()->_mapping.HasSimMappings())
		{
			changeState<WaitSim>();
		}
		else if (pimpl()->_mapping.getDblPressMap())
		{
			// Start counting time between two start presses
			changeState<DblPressStart>();
		}
		else
		{
			changeState<BtnPress>();
		}
	}
};

class BtnPress : public pocket_fsm::NestedStateMachine<ActiveMappingState, DigitalButtonState>
{
	DB_CONCRETE_STATE(BtnPress)

	REACT(OnEntry) override
	{
		DigitalButtonState::react(e);
		initialize(new ActiveStartPress(_pimpl));
	}

	NESTED_REACT(Pressed);
	NESTED_REACT(Released);
};

class TapPress : public DigitalButtonState
{
	DB_CONCRETE_STATE(TapPress)

	REACT(OnEntry) override
	{
		pimpl()->_keyToRelease->ProcessEvent(BtnEvent::OnTap, *pimpl());
	}

	REACT(Pressed)
	override
	{
		DigitalButtonState::react(e);
		pimpl()->ReleaseInstant(BtnEvent::OnRelease);
		pimpl()->ReleaseInstant(BtnEvent::OnTap);
		changeState<NoPress>();
	}

	REACT(Released)
	override
	{
		DigitalButtonState::react(e);
		if (pimpl()->GetPressDurationMS(e.time_now) > MAGIC_INSTANT_DURATION)
		{
			pimpl()->ReleaseInstant(BtnEvent::OnRelease);
			pimpl()->ReleaseInstant(BtnEvent::OnTap);
		}
		if (!pimpl()->_keyToRelease || pimpl()->GetPressDurationMS(e.time_now) > pimpl()->_keyToRelease->getTapDuration())
		{
			changeState<NoPress>();
		}
	}

	REACT(OnExit) override
	{
		pimpl()->_keyToRelease->ProcessEvent(BtnEvent::OnTapRelease, *pimpl());
		pimpl()->ClearKey();
	}
};

class SimPressMaster : public pocket_fsm::NestedStateMachine<ActiveMappingState, DigitalButtonState>
{
	DB_CONCRETE_STATE(SimPressMaster)

	REACT(OnEntry) override
	{
		DigitalButtonState::react(e);
		initialize(new ActiveStartPress(_pimpl));
	}

	NESTED_REACT(Pressed)

	NESTED_REACT(Released)

	NESTED_REACT(Sync)
};

class SimPressSlave : public DigitalButtonState
{
	DB_CONCRETE_STATE(SimPressSlave)

	REACT(Pressed)
	override
	{
		DigitalButtonState::react(e);
		if (pimpl()->_simPressMaster->getState() != BtnState::SimPressMaster)
		{
			// The master button has released! change state now!
			changeState<SimRelease>();
			pimpl()->_simPressMaster = nullptr;
		}
		// else do nothing
	}

	REACT(Released) override
	{
		DigitalButtonState::react(e);
		if (pimpl()->_simPressMaster->getState() != BtnState::SimPressMaster)
		{
			// The master button has released! change state now!
			changeState<SimRelease>();
			pimpl()->_simPressMaster = nullptr;
		}
		else
		{
			// Process at the master's end
			Sync sync;
			sync.pressTime = e.time_now;
			sync.holdTime = e.holdTime;
			sync.turboTime = e.turboTime;
			sync.dblPressWindow = e.dblPressWindow;
			_nextState = pimpl()->_simPressMaster->sendEvent(sync).nextState;
		}
	}
};

class WaitSim : public DigitalButtonState
{
	DB_CONCRETE_STATE(WaitSim)

	REACT(Pressed)
	override
	{
		DigitalButtonState::react(e);
		// Is there a sim mapping on this button where the other button is in WaitSim state too?
		auto simBtn = pimpl()->_common->_getMatchingSimBtn(pimpl()->_id);
		if (simBtn)
		{
			changeState<SimPressSlave>();
			pimpl()->_press_times = e.time_now;                                                          // Reset Timer
			pimpl()->_keyToRelease.reset(new Mapping(pimpl()->_mapping.AtSimPress(simBtn->_id)->get())); // Make a copy
			pimpl()->_nameToRelease = pimpl()->_mapping.getSimPressName(simBtn->_id);
			pimpl()->_simPressMaster = simBtn; // Second to press is the slave

			Sync sync;
			sync.nextState = new SimPressMaster();
			sync.pressTime = e.time_now;
			sync.activeMapping = new Mapping(*pimpl()->_keyToRelease);
			sync.nameToRelease = pimpl()->_nameToRelease;
			sync.dblPressWindow = e.dblPressWindow;
			simBtn->sendEvent(sync);
		}
		else if (pimpl()->GetPressDurationMS(e.time_now) > sim_press_window)
		{
			// Button is still pressed but Sim delay did expire
			if (pimpl()->_mapping.getDblPressMap())
			{
				// Start counting time between two start presses
				changeState<DblPressStart>();
			}
			else // Handle regular press mapping
			{
				changeState<BtnPress>();
				// _press_times = time_now;
			}
		}
		// Else let time flow, stay in this state, no output.
	}

	REACT(Released)
	override
	{
		DigitalButtonState::react(e);
		// Button was released before sim delay expired
		if (pimpl()->_mapping.getDblPressMap())
		{
			// Start counting time between two start presses
			changeState<DblPressStart>();
		}
		else
		{
			changeState<BtnPress>();
		}
	}

	REACT(Sync)
	override
	{
		pimpl()->_simPressMaster = nullptr;
		pimpl()->_press_times = e.pressTime;
		pimpl()->_keyToRelease.reset(e.activeMapping);
		pimpl()->_nameToRelease = e.nameToRelease;
		_nextState = e.nextState; // changeState<SimPressMaster>()
	}
};

class SimRelease : public DigitalButtonState
{
	DB_CONCRETE_STATE(SimRelease)

	REACT(Released)
	override
	{
		DigitalButtonState::react(e);
		changeState<NoPress>();
		pimpl()->ClearKey();
	}
};

class DblPressStart : public pocket_fsm::NestedStateMachine<ActiveMappingState, DigitalButtonState>
{
	DB_CONCRETE_STATE(DblPressStart)
	REACT(OnEntry) override
	{
		DigitalButtonState::react(e);
		initialize(new ActiveStartPress(_pimpl));
	}

	NESTED_REACT(Pressed);

	REACT(Released)
	override
	{
		sendEvent(e);
		if (_nextState)
		{
			if (_nextState->_name == "NoPress")
			{
				delete _nextState;
				_nextState = nullptr;
				changeState<DblPressNoPress>(_onTransition);
			}
			else if (_nextState->_name == "TapPress")
			{
				delete _nextState;
				_nextState = nullptr;
				changeState<DblPressNoPressTap>(_onTransition);
			}
		}
	}
};

class DblPressNoPress : public DigitalButtonState
{
	DB_CONCRETE_STATE(DblPressNoPress)

	REACT(Pressed) override
	{
		DigitalButtonState::react(e);
		if (pimpl()->GetPressDurationMS(e.time_now) > e.dblPressWindow)
		{
			pimpl()->_press_times = e.time_now; // Reset Timer to raise a tap
			changeState<BtnPress>();
		}
		else
		{
			pimpl()->_press_times = e.time_now;
			changeState<DblPressPress>();
		}
	}

	REACT(Released) override
	{
		DigitalButtonState::react(e);
		if (pimpl()->GetPressDurationMS(e.time_now) > MAGIC_INSTANT_DURATION)
		{
			pimpl()->ReleaseInstant(BtnEvent::OnRelease);
		}

		if (pimpl()->GetPressDurationMS(e.time_now) > e.dblPressWindow)
		{
			changeState<NoPress>();
		}
	}
};

class DblPressNoPressTap : public DigitalButtonState
{
	DB_CONCRETE_STATE(DblPressNoPressTap)
	REACT(Pressed)
	override
	{
		if (pimpl()->GetPressDurationMS(e.time_now) > e.dblPressWindow)
		{
			pimpl()->_press_times = e.time_now; // Reset Timer to raise a tap
			changeState<TapPress>();
		}
		else
		{
			pimpl()->_keyToRelease.reset(new Mapping(pimpl()->_mapping.getDblPressMap()->second));
			pimpl()->_nameToRelease = pimpl()->_mapping.getName(pimpl()->_id);
			pimpl()->_press_times = e.time_now;
			changeState<DblPressPress>();
		}
	}

	REACT(Released)
	override
	{
		if (pimpl()->GetPressDurationMS(e.time_now) > e.dblPressWindow)
		{
			pimpl()->_press_times = e.time_now; // Reset Timer to raise a tap
			changeState<TapPress>();
		}
	}
};

class DblPressNoPressHold : public DigitalButtonState
{
	DB_CONCRETE_STATE(DblPressNoPressHold)
	REACT(Pressed)
	{
		DigitalButtonState::react(e);
		if (pimpl()->GetPressDurationMS(e.time_now) > e.dblPressWindow)
		{
			changeState<BtnPress>();
			// Don't reset timer to preserve hold press behaviour
			pimpl()->GetPressMapping()->ProcessEvent(BtnEvent::OnPress, *pimpl());
		}
		else
		{
			changeState<DblPressPress>();
			pimpl()->_press_times = e.time_now;
			pimpl()->_keyToRelease.reset(new Mapping(pimpl()->_mapping.getDblPressMap()->second));
			pimpl()->_nameToRelease = pimpl()->_mapping.getName(pimpl()->_id);
		}
	}

	REACT(Released)
	override
	{
		DigitalButtonState::react(e);
		if (pimpl()->GetPressDurationMS(e.time_now) > e.dblPressWindow)
		{
			changeState<BtnPress>();
			// Don't reset timer to preserve hold press behaviour
		}
	}
};

class DblPressPress : public pocket_fsm::NestedStateMachine<ActiveMappingState, DigitalButtonState>
{
	DB_CONCRETE_STATE(DblPressPress)

	REACT(OnEntry) override
	{
		DigitalButtonState::react(e);
		pimpl()->_keyToRelease.reset(new Mapping(pimpl()->_mapping.getDblPressMap()->second));
		pimpl()->_nameToRelease = pimpl()->_mapping.getName(pimpl()->_id);
		initialize(new ActiveStartPress(_pimpl));
	}

	NESTED_REACT(Pressed);
	NESTED_REACT(Released);
};

class InstRelease : public DigitalButtonState
{
	DB_CONCRETE_STATE(InstRelease)
	REACT(Pressed)
	override
	{
		DigitalButtonState::react(e);
		if (pimpl()->GetPressDurationMS(e.time_now) > MAGIC_INSTANT_DURATION)
		{
			pimpl()->ReleaseInstant(BtnEvent::OnRelease);
			pimpl()->ClearKey();
			changeState<NoPress>();
		}
	}
};

// Top level interface

DigitalButton::DigitalButton(shared_ptr<DigitalButton::Common> btnCommon, JSMButton &mapping)
  : _id(mapping._id)
{
	initialize(new NoPress(new DigitalButtonImpl(mapping, btnCommon)));
}

DigitalButton::Common::Common(Gamepad::Callback virtualControllerCallback, GamepadMotion *mainMotion)
{
	rightMainMotion = mainMotion;
	chordStack.push_front(ButtonID::NONE); //Always hold mapping none at the end to handle modeshifts and chords
#ifdef _WIN32
	if (virtual_controller.get() != ControllerScheme::NONE)
	{
		_vigemController.reset(Gamepad::getNew(virtual_controller.get(), virtualControllerCallback));
	}
#endif
}

bool DigitalButton::Common::HasActiveToggle(const KeyCode& key) const
{
	auto foundToggle = find_if(activeTogglesQueue.cbegin(), activeTogglesQueue.cend(),
		[key] (auto& pair)
		{
			return pair.second == key; 
		});
	return foundToggle != activeTogglesQueue.cend();
}