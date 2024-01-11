#include "Mapping.h"
#include "InputHelpers.h"
#include <regex>
#include <cstring>

ostream &operator<<(ostream &out, const Mapping &mapping)
{
	return out << (mapping._command.empty() ? mapping._description : mapping._command);
}

istream &operator>>(istream &in, Mapping &mapping)
{
	// Has friend access
	string valueName(128, '\0');
	in.getline(&valueName[0], valueName.size());
	valueName.resize(strlen(valueName.c_str()));
	smatch results;
	int count = 0;

	mapping._command = valueName;
	static constexpr string_view rgx = R"(\s*([!\^]?)((\".*?\")|\w*[0-9A-Z]|\W)([\\\/+'_]?)\s*(.*))";
	while (regex_match(valueName, results, regex(rgx.data())) && !results[0].str().empty())
	{
		Mapping::ActionModifier actMod =
		  results[1].str().empty()   ? Mapping::ActionModifier::None :
		  results[1].str()[0] == '!' ? Mapping::ActionModifier::Instant :
		  results[1].str()[0] == '^' ? Mapping::ActionModifier::Toggle :
		                               Mapping::ActionModifier::INVALID;

		string keyStr(results[2]);

		Mapping::EventModifier evtMod =
		  results[4].str().empty()    ? Mapping::EventModifier::Auto :
		  results[4].str()[0] == '\\' ? Mapping::EventModifier::StartPress :
		  results[4].str()[0] == '+'  ? Mapping::EventModifier::TurboPress :
		  results[4].str()[0] == '/'  ? Mapping::EventModifier::ReleasePress :
		  results[4].str()[0] == '\'' ? Mapping::EventModifier::TapPress :
		  results[4].str()[0] == '_'  ? Mapping::EventModifier::HoldPress :
		                                Mapping::EventModifier::INVALID;

		string leftovers(results[5]);

		KeyCode key(keyStr);
		if (evtMod == Mapping::EventModifier::Auto)
		{
			evtMod = count == 0 ? (leftovers.empty() ? Mapping::EventModifier::StartPress : Mapping::EventModifier::TapPress) :
			                      (count == 1 ? Mapping::EventModifier::HoldPress : Mapping::EventModifier::Auto);
		}

		// Some exceptions :(
		if (key.code == COMMAND_ACTION && actMod == Mapping::ActionModifier::None)
		{
			// Any command actions are instant by default
			actMod = Mapping::ActionModifier::Instant;
		}
		else if (key.code == CALIBRATE && actMod == Mapping::ActionModifier::None &&
		  (evtMod == Mapping::EventModifier::TapPress || evtMod == Mapping::EventModifier::ReleasePress))
		{
			// Calibrate only makes sense on tap or release if it toggles. Also preserves legacy.
			actMod = Mapping::ActionModifier::Toggle;
		}

		if (key.code == 0 ||
		  key.code == COMMAND_ACTION && actMod != Mapping::ActionModifier::Instant ||
		  actMod == Mapping::ActionModifier::INVALID ||
		  evtMod == Mapping::EventModifier::INVALID ||
		  evtMod == Mapping::EventModifier::Auto && count >= 2 ||
		  evtMod == Mapping::EventModifier::ReleasePress && actMod == Mapping::ActionModifier::None ||
		  !mapping.AddMapping(key, evtMod, actMod))
		{
			// error!!!
			in.setstate(in.failbit);
			break;
		}
		valueName = leftovers;
		count++;
	} // Next item

	return in;
}

bool operator==(const Mapping &lhs, const Mapping &rhs)
{
	// Very flawfull :(
	return lhs.command() == rhs.command();
}

Mapping::Mapping(in_string mapping)
{
	stringstream ss(mapping.data());
	ss >> *this;
	if (ss.fail())
	{
		clear();
	}
}

void Mapping::ProcessEvent(BtnEvent evt, EventActionIf &button) const
{
	// COUT << button._id << " processes event " << evt << endl;
	auto entry = _eventMapping.find(evt);
	if (entry != _eventMapping.end() && entry->second) // Skip over empty entries
	{
		switch (evt)
		{
		case BtnEvent::OnPress:
			COUT << button.getDisplayName() << ": true" << endl;
			break;
		case BtnEvent::OnRelease:
		case BtnEvent::OnHoldRelease:
			COUT << button.getDisplayName() << ": false" << endl;
			break;
		case BtnEvent::OnTap:
			COUT << button.getDisplayName() << ": tapped" << endl;
			break;
		case BtnEvent::OnHold:
			COUT << button.getDisplayName() << ": held" << endl;
			break;
		case BtnEvent::OnTurbo:
			COUT << button.getDisplayName() << ": turbo" << endl;
			break;
		}

		if (entry->second)
		{
			DEBUG_LOG << button.getDisplayName() << " processes event " << evt << endl;
			entry->second(&button);
		}
	}
}

void Mapping::InsertEventMapping(BtnEvent evt, EventActionIf::Callback action)
{
	auto existingActions = _eventMapping.find(evt);
	_eventMapping[evt] = existingActions == _eventMapping.end() ? action :
	                                                              bind(&RunBothActions, placeholders::_1, existingActions->second, action); // Chain with already existing mapping, if any
}

bool Mapping::AddMapping(KeyCode key, EventModifier evtMod, ActionModifier actMod)
{
	EventActionIf::Callback apply, apply2, release;
	if (key.code == 0)
	{
		return false;
	}
	if (key.code == CALIBRATE)
	{
		apply = bind(&EventActionIf::StartCalibration, placeholders::_1);
		release = bind(&EventActionIf::FinishCalibration, placeholders::_1);
		_tapDurationMs = MAGIC_EXTENDED_TAP_DURATION; // Unused in regular press
	}
	else if (key.code >= GYRO_INV_X && key.code <= GYRO_TRACKBALL)
	{
		apply = bind(&EventActionIf::ApplyGyroAction, placeholders::_1, key);
		release = bind(&EventActionIf::RemoveGyroAction, placeholders::_1);
		_tapDurationMs = MAGIC_EXTENDED_TAP_DURATION; // Unused in regular press
	}
	else if (key.code == COMMAND_ACTION)
	{
		_ASSERT_EXPR(Mapping::_isCommandValid, "You need to assign a function to this field. It should be a function that validates the command line.");
		if (!Mapping::_isCommandValid(key.name))
		{
			COUT << "Error: \"" << key.name << "\" is not a valid command" << endl;
			return false;
		}
		apply = bind(&WriteToConsole, key.name);
		release = EventActionIf::Callback();
	}
	else if (key.code == RUMBLE)
	{
		union Rumble
		{
			int raw;
			array<uint8_t, 2> bytes;
		} rumble;
		rumble.raw = stoi(key.name.substr(1, 4), nullptr, 16);
		apply = bind(&EventActionIf::SetRumble, placeholders::_1, rumble.bytes[0] << 8, rumble.bytes[1] << 8);
		release = bind(&EventActionIf::SetRumble, placeholders::_1, 0, 0);
		_tapDurationMs = MAGIC_EXTENDED_TAP_DURATION; // Unused in regular press
	}
	else //
	{
		_hasViGEmBtn |= isControllerKey(key.code); // Set flag if vigem button
		apply = bind(&EventActionIf::ApplyBtnPress, placeholders::_1, key);
		release = bind(&EventActionIf::ApplyBtnRelease, placeholders::_1, key);
	}

	BtnEvent applyEvt, releaseEvt;
	switch (evtMod)
	{
	case EventModifier::StartPress:
		applyEvt = BtnEvent::OnPress;
		releaseEvt = BtnEvent::OnRelease;
		break;
	case EventModifier::TapPress:
		applyEvt = BtnEvent::OnTap;
		releaseEvt = BtnEvent::OnTapRelease;
		break;
	case EventModifier::HoldPress:
		applyEvt = BtnEvent::OnHold;
		releaseEvt = BtnEvent::OnHoldRelease;
		break;
	case EventModifier::ReleasePress:
		// Acttion Modifier is required
		applyEvt = BtnEvent::OnRelease;
		releaseEvt = BtnEvent::OnInstantRelease;
		break;
	case EventModifier::TurboPress:
		applyEvt = BtnEvent::OnTurbo;
		releaseEvt = BtnEvent::OnInstantRelease;
		apply2 = bind(&EventActionIf::RegisterInstant, placeholders::_1, applyEvt);
		if (actMod == ActionModifier::None)
			std::swap(apply, release);
		break;
	default: // EventModifier::INVALID or None
		return false;
	}

	switch (actMod)
	{
	case ActionModifier::Toggle:
		apply = bind(&EventActionIf::ApplyButtonToggle, placeholders::_1, key, apply, release);
		release = EventActionIf::Callback();
		break;
	case ActionModifier::Instant:
	{
		apply2 = bind(&EventActionIf::RegisterInstant, placeholders::_1, applyEvt);
		apply = bind(&Mapping::RunBothActions, placeholders::_1, apply, apply2);
	}
	break;
	case ActionModifier::INVALID:
		return false;
		// None applies no modification... Hey!
	}

	if (evtMod == EventModifier::TurboPress)
	{
		apply = bind(&Mapping::RunBothActions, placeholders::_1, apply, apply2);
		// On turbo you also always need to clear the turbo on release
		InsertEventMapping(BtnEvent::OnRelease, actMod == ActionModifier::Instant ? release : apply);
	}

	InsertEventMapping(applyEvt, apply);
	InsertEventMapping(releaseEvt, release);

	stringstream ss;
	// Update Description
	if (_description.compare("no input") != 0)
	{
		ss << _description;
		if (_eventMapping.size() > 2 && _eventMapping.find(BtnEvent::OnPress) != _eventMapping.end())
		{
			ss << " on Start Press";
		}
		ss << " and ";
	}
	if (actMod != Mapping::ActionModifier::None)
	{
		ss << actMod << " ";
	}
	ss << key.name;
	if (_eventMapping.size() > 3 || evtMod != Mapping::EventModifier::StartPress)
	{
		ss << " on " << evtMod;
	}
	// else don't display event modifier when using default binding on single key
	_description = ss.str();
	return true;
}

bool Mapping::AppendToCommand(KeyCode key, EventModifier evtMod, ActionModifier actMod)
{
	if (key.name.empty() || evtMod == EventModifier::INVALID || actMod == ActionModifier::INVALID)
	{
		return false;
	}
	stringstream ss;
	if (!_command.empty())
	{
		ss << _command << " ";
	}

	if (actMod != ActionModifier::None)
	{
		ss << (actMod == ActionModifier::Instant ? '!' : '^');
	}
	ss << key.name;

	if (evtMod != EventModifier::Auto)
	{
		ss << (evtMod == EventModifier::StartPress ? '\\' :
		    evtMod == EventModifier::TurboPress    ? '+' :
		    evtMod == EventModifier::ReleasePress  ? '/' :
		    evtMod == EventModifier::TapPress      ? '\'' :
		 /* evtMod == EventModifier::HoldPress    */ '_'); 
	}
	_command = ss.str();
	return true;
}

void Mapping::RunBothActions(EventActionIf *btn, EventActionIf::Callback action1, EventActionIf::Callback action2)
{
	if (action1)
		action1(btn);
	if (action2)
		action2(btn);
}
