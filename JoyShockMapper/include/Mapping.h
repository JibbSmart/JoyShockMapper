#pragma once

#include "JoyShockMapper.h"

// The list of different function that can be bound in the mapping
class EventActionIf
{
public:
	typedef function<void(EventActionIf *)> Callback;

	virtual void RegisterInstant(BtnEvent evt) = 0;
	virtual void ApplyGyroAction(KeyCode gyroAction) = 0;
	virtual void RemoveGyroAction() = 0;
	virtual void SetRumble(int smallRumble, int bigRumble) = 0;
	virtual void ApplyBtnPress(KeyCode key) = 0;
	virtual void ApplyBtnRelease(KeyCode key) = 0;
	virtual void ApplyButtonToggle(KeyCode key, Callback apply, Callback release) = 0;
	virtual void StartCalibration() = 0;
	virtual void FinishCalibration() = 0;
	virtual const char *getDisplayName() = 0;
};

// This structure handles the mapping of a button, buy processing and action
// to be done on tap, hold, turbo and others. It holds a map of actions to perform
// when a specific event happens. This replaces the old Mapping structure.
class Mapping
{
public:
	enum class ActionModifier
	{
		None,
		Toggle,
		Instant,
		INVALID
	};
	enum class EventModifier
	{
		None,
		StartPress,
		ReleasePress,
		TurboPress,
		TapPress,
		HoldPress,
		INVALID
	};

	// Identifies having no binding mapped
	static const Mapping NO_MAPPING;

	// This functor nees to be set to way to validate a command line string;
	static function<bool(in_string)> _isCommandValid;

	string _description = "no input";
	string _command;

private:
	map<BtnEvent, EventActionIf::Callback> _eventMapping;
	float _tapDurationMs = MAGIC_TAP_DURATION;
	bool _hasViGEmBtn = false;

	void InsertEventMapping(BtnEvent evt, EventActionIf::Callback action);
	static void RunBothActions(EventActionIf *btn, EventActionIf::Callback action1, EventActionIf::Callback action2);

public:
	Mapping() = default;

	Mapping(in_string mapping);

	Mapping(int dummy)
	  : Mapping()
	{
	}

	void ProcessEvent(BtnEvent evt, EventActionIf &button) const;

	bool AddMapping(KeyCode key, EventModifier evtMod, ActionModifier actMod = ActionModifier::None);

	inline bool isValid() const
	{
		return !_eventMapping.empty();
	}

	inline float getTapDuration() const
	{
		return _tapDurationMs;
	}

	inline void clear()
	{
		_eventMapping.clear();
		_description.clear();
		_tapDurationMs = MAGIC_TAP_DURATION;
		_hasViGEmBtn = false;
	}

	inline bool hasViGEmBtn() const
	{
		return _hasViGEmBtn;
	}
};

istream &operator>>(istream &in, Mapping &mapping);
ostream &operator<<(ostream &out, Mapping mapping);
bool operator==(const Mapping &lhs, const Mapping &rhs);
inline bool operator!=(const Mapping &lhs, const Mapping &rhs)
{
	return !(lhs == rhs);
}
