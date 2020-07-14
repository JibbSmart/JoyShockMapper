#include "JoyShockMapper.h"
#include "inputHelpers.h"
#include <sstream>

// Should we decide to run C++17, this entire file could be replaced by magic_enum header

istream &operator >>(istream &in, ButtonID &button) {
	string s;
	in >> s;
	if (s.rfind("NONE", 0) == 0) {
		button = ButtonID::NONE;
	}
	else if (s.compare("UP") == 0) {
		button = ButtonID::UP;
	}
	else if (s.compare("DOWN") == 0) {
		button = ButtonID::DOWN;
	}
	else if (s.compare("LEFT") == 0) {
		button = ButtonID::LEFT;
	}
	else if (s.compare("RIGHT") == 0) {
		button = ButtonID::RIGHT;
	}
	else if (s.compare("L") == 0) {
		button = ButtonID::L;
	}
	else if (s.compare("ZL") == 0) {
		button = ButtonID::ZL;
	}
	else if (s.compare("-") == 0) {
		button = ButtonID::MINUS;
	}
	else if (s.compare("CAPTURE") == 0) {
		button = ButtonID::CAPTURE;
	}
	else if (s.compare("E") == 0) {
		button = ButtonID::E;
	}
	else if (s.compare("S") == 0) {
		button = ButtonID::S;
	}
	else if (s.compare("N") == 0) {
		button = ButtonID::N;
	}
	else if (s.compare("W") == 0) {
		button = ButtonID::W;
	}
	else if (s.compare("R") == 0) {
		button = ButtonID::R;
	}
	else if (s.compare("ZR") == 0) {
		button = ButtonID::ZR;
	}
	else if (s.compare("+") == 0) {
		button = ButtonID::PLUS;
	}
	else if (s.compare("HOME") == 0) {
		button = ButtonID::HOME;
	}
	else if (s.compare("SL") == 0) {
		button = ButtonID::SL;
	}
	else if (s.compare("SR") == 0) {
		button = ButtonID::SR;
	}
	else if (s.compare("L3") == 0) {
		button = ButtonID::L3;
	}
	else if (s.compare("R3") == 0) {
		button = ButtonID::R3;
	}
	else if (s.compare("LUP") == 0) {
		button = ButtonID::LUP;
	}
	else if (s.compare("LDOWN") == 0) {
		button = ButtonID::LDOWN;
	}
	else if (s.compare("LLEFT") == 0) {
		button = ButtonID::LLEFT;
	}
	else if (s.compare("LRIGHT") == 0) {
		button = ButtonID::LRIGHT;
	}
	else if (s.compare("RUP") == 0) {
		button = ButtonID::RUP;
	}
	else if (s.compare("RDOWN") == 0) {
		button = ButtonID::RDOWN;
	}
	else if (s.compare("RLEFT") == 0) {
		button = ButtonID::RLEFT;
	}
	else if (s.compare("RRIGHT") == 0) {
		button = ButtonID::RRIGHT;
	}
	else if (s.compare("ZRF") == 0) {
		button = ButtonID::ZRF;
	}
	else if (s.compare("ZLF") == 0) {
		button = ButtonID::ZLF;
	}
	else if (s.compare("LRING") == 0) {
		button = ButtonID::LRING;
	}
	else if (s.compare("RRING") == 0) {
		button = ButtonID::RRING;
	}
	else
	{
		button = ButtonID::ERR;
	}
	return in;
}
ostream &operator <<(ostream &out, ButtonID button) {
	switch (button)
	{
	case ButtonID::NONE:
		out << "NONE";
		break;
	case ButtonID::UP:
		out << "UP";
		break;
	case ButtonID::DOWN:
		out << "DOWN";
		break;
	case ButtonID::LEFT:
		out << "LEFT";
		break;
	case ButtonID::RIGHT:
		out << "RIGHT";
		break;
	case ButtonID::L:
		out << "L";
		break;
	case ButtonID::ZL:
		out << "ZL";
		break;
	case ButtonID::MINUS:
		out << "-";
		break;
	case ButtonID::CAPTURE:
		out << "CAPTURE";
		break;
	case ButtonID::E:
		out << "E";
		break;
	case ButtonID::S:
		out << "S";
		break;
	case ButtonID::N:
		out << "N";
		break;
	case ButtonID::W:
		out << "W";
		break;
	case ButtonID::R:
		out << "R";
		break;
	case ButtonID::ZR:
		out << "ZR";
		break;
	case ButtonID::PLUS:
		out << "+";
		break;
	case ButtonID::HOME:
		out << "HOME";
		break;
	case ButtonID::SL:
		out << "SL";
		break;
	case ButtonID::SR:
		out << "SR";
		break;
	case ButtonID::L3:
		out << "L3";
		break;
	case ButtonID::R3:
		out << "R3";
		break;
	case ButtonID::LUP:
		out << "LUP";
		break;
	case ButtonID::LDOWN:
		out << "LDOWN";
		break;
	case ButtonID::LLEFT:
		out << "LLEFT";
		break;
	case ButtonID::LRIGHT:
		out << "LRIGHT";
		break;
	case ButtonID::LRING:
		out << "LRING";
		break;
	case ButtonID::RUP:
		out << "RUP";
		break;
	case ButtonID::RDOWN:
		out << "RDOWN";
		break;
	case ButtonID::RLEFT:
		out << "RLEFT";
		break;
	case ButtonID::RRIGHT:
		out << "RRIGHT";
		break;
	case ButtonID::RRING:
		out << "LRING";
		break; 
	case ButtonID::ZLF:
		out << "ZLF";
		break;
	case ButtonID::ZRF:
		out << "ZRF";
		break;
	default:
		out << "ERROR";
		break;
	}
	return out;
}

istream &operator >>(istream &in, SettingID &setting)
{
	string s;
	in >> s;
	if (s.compare("MIN_GYRO_SENS") == 0) {
		setting = SettingID::MIN_GYRO_SENS;
	}
	else if (s.compare("MAX_GYRO_SENS") == 0) {
		setting = SettingID::MAX_GYRO_SENS;
	}
	else if (s.compare("MIN_GYRO_THRESHOLD") == 0) {
		setting = SettingID::MIN_GYRO_THRESHOLD;
	}
	else if (s.compare("MAX_GYRO_THRESHOLD") == 0) {
		setting = SettingID::MAX_GYRO_THRESHOLD;
	}
	else if (s.compare("SCREEN_RESOLUTION_X") == 0) {
		setting = SettingID::SCREEN_RESOLUTION_X;
	}
	else if (s.compare("SCREEN_RESOLUTION_Y") == 0) {
		setting = SettingID::SCREEN_RESOLUTION_Y;
	}
	else if (s.compare("ROTATE_SMOOTH_OVERRIDE") == 0) {
		setting = SettingID::ROTATE_SMOOTH_OVERRIDE;
	}
	else if (s.compare("MOUSE_RING_RADIUS") == 0) {
		setting = SettingID::MOUSE_RING_RADIUS;
	}
	else if (s.compare("FLICK_SNAP_MODE") == 0) {
		setting = SettingID::FLICK_SNAP_MODE;
	}
	else if (s.compare("FLICK_SNAP_STRENGTH") == 0) {
		setting = SettingID::FLICK_SNAP_STRENGTH;
	}
	else if (s.compare("STICK_POWER") == 0) {
		setting = SettingID::STICK_POWER;
	}
	else if (s.compare("STICK_SENS") == 0) {
		setting = SettingID::STICK_SENS;
	}
	else if (s.compare("REAL_WORLD_CALIBRATION") == 0) {
		setting = SettingID::REAL_WORLD_CALIBRATION;
	}
	else if (s.compare("IN_GAME_SENS") == 0) {
		setting = SettingID::IN_GAME_SENS;
	}
	else if (s.compare("TRIGGER_THRESHOLD") == 0) {
		setting = SettingID::TRIGGER_THRESHOLD;
	}
	else if (s.compare("RESET_MAPPINGS") == 0) {
		setting = SettingID::RESET_MAPPINGS;
	}
	else if (s.compare("NO_GYRO_BUTTON") == 0) {
		setting = SettingID::NO_GYRO_BUTTON;
	}
	else if (s.compare("LEFT_STICK_MODE") == 0) {
		setting = SettingID::LEFT_STICK_MODE;
	}
	else if (s.compare("RIGHT_STICK_MODE") == 0) {
		setting = SettingID::RIGHT_STICK_MODE;
	}
	else if (s.compare("LEFT_RING_MODE") == 0) {
		setting = SettingID::LEFT_RING_MODE;
	}
	else if (s.compare("RIGHT_RING_MODE") == 0) {
		setting = SettingID::RIGHT_RING_MODE;
	}
	else if (s.compare("GYRO_OFF") == 0) {
		setting = SettingID::GYRO_OFF;
	}
	else if (s.compare("GYRO_ON") == 0) {
		setting = SettingID::GYRO_ON;
	}
	else if (s.compare("STICK_AXIS_X") == 0) {
		setting = SettingID::STICK_AXIS_X;
	}
	else if (s.compare("STICK_AXIS_Y") == 0) {
		setting = SettingID::STICK_AXIS_Y;
	}
	else if (s.compare("GYRO_AXIS_X") == 0) {
		setting = SettingID::GYRO_AXIS_X;
	}
	else if (s.compare("GYRO_AXIS_Y") == 0) {
		setting = SettingID::GYRO_AXIS_Y;
	}
	else if (s.compare("RECONNECT_CONTROLLERS") == 0) {
		setting = SettingID::RECONNECT_CONTROLLERS;
	}
	else if (s.compare("COUNTER_OS_MOUSE_SPEED") == 0) {
		setting = SettingID::COUNTER_OS_MOUSE_SPEED;
	}
	else if (s.compare("IGNORE_OS_MOUSE_SPEED") == 0) {
		setting = SettingID::IGNORE_OS_MOUSE_SPEED;
	}
	else if (s.compare("JOYCON_GYRO_MASK") == 0) {
		setting = SettingID::JOYCON_GYRO_MASK;
	}
	else if (s.compare("GYRO_SENS") == 0) {
		setting = SettingID::GYRO_SENS;
	}
	else if (s.compare("FLICK_TIME") == 0) {
		setting = SettingID::FLICK_TIME;
	}
	else if (s.compare("GYRO_SMOOTH_THRESHOLD") == 0) {
		setting = SettingID::GYRO_SMOOTH_THRESHOLD;
	}
	else if (s.compare("GYRO_SMOOTH_TIME") == 0) {
		setting = SettingID::GYRO_SMOOTH_TIME;
	}
	else if (s.compare("GYRO_CUTOFF_SPEED") == 0) {
		setting = SettingID::GYRO_CUTOFF_SPEED;
	}
	else if (s.compare("GYRO_CUTOFF_RECOVERY") == 0) {
		setting = SettingID::GYRO_CUTOFF_RECOVERY;
	}
	else if (s.compare("STICK_ACCELERATION_RATE") == 0) {
		setting = SettingID::STICK_ACCELERATION_RATE;
	}
	else if (s.compare("STICK_ACCELERATION_CAP") == 0) {
		setting = SettingID::STICK_ACCELERATION_CAP;
	}
	else if (s.compare("STICK_DEADZONE_INNER") == 0) {
		setting = SettingID::STICK_DEADZONE_INNER;
	}
	else if (s.compare("STICK_DEADZONE_OUTER") == 0) {
		setting = SettingID::STICK_DEADZONE_OUTER;
	}
	else if (s.rfind("CALCULATE_REAL_WORLD_CALIBRATION", 0) == 0) {
		setting = SettingID::CALCULATE_REAL_WORLD_CALIBRATION;
	}
	else if (s.rfind("FINISH_GYRO_CALIBRATION", 0) == 0) {
		setting = SettingID::FINISH_GYRO_CALIBRATION;
	}
	else if (s.rfind("RESTART_GYRO_CALIBRATION", 0) == 0) {
		setting = SettingID::RESTART_GYRO_CALIBRATION;
	}
	else if (s.rfind("MOUSE_X_FROM_GYRO_AXIS", 0) == 0) {
		setting = SettingID::MOUSE_X_FROM_GYRO_AXIS;
	}
	else if (s.rfind("MOUSE_Y_FROM_GYRO_AXIS", 0) == 0) {
		setting = SettingID::MOUSE_Y_FROM_GYRO_AXIS;
	}
	else if (s.rfind("ZR_MODE", 0) == 0) {
		setting = SettingID::ZR_DUAL_STAGE_MODE;
	}
	else if (s.rfind("ZL_MODE", 0) == 0) {
		setting = SettingID::ZL_DUAL_STAGE_MODE;
	}
	else if (s.rfind("AUTOLOAD", 0) == 0) {
		setting = SettingID::AUTOLOAD;
	}
	else if (s.rfind("HELP", 0) == 0) {
		setting = SettingID::HELP;
	}
	else if (s.rfind("WHITELIST_SHOW", 0) == 0) {
		setting = SettingID::WHITELIST_SHOW;
	}
	else if (s.rfind("WHITELIST_ADD", 0) == 0) {
		setting = SettingID::WHITELIST_ADD;
	}
	else if (s.rfind("WHITELIST_REMOVE", 0) == 0) {
		setting = SettingID::WHITELIST_REMOVE;
	}
	else {
		setting = SettingID::ERR;
	}
	return in;
}
ostream &operator <<(ostream &out, SettingID setting)
{
	switch (setting)
	{

	case SettingID::MIN_GYRO_SENS:
		out << "MIN_GYRO_SENS";
		break;
	case SettingID::MAX_GYRO_SENS:
		out << "MAX_GYRO_SENS";
		break;
	case SettingID::MIN_GYRO_THRESHOLD:
		out << "MIN_GYRO_THRESHOLD";
		break;
	case SettingID::MAX_GYRO_THRESHOLD:
		out << "MAX_GYRO_THRESHOLD";
		break;
	case SettingID::SCREEN_RESOLUTION_X:
		out << "SCREEN_RESOLUTION_X";
		break;
	case SettingID::SCREEN_RESOLUTION_Y:
		out << "SCREEN_RESOLUTION_Y";
		break;
	case SettingID::ROTATE_SMOOTH_OVERRIDE:
		out << "ROTATE_SMOOTH_OVERRIDE";
		break;
	case SettingID::MOUSE_RING_RADIUS:
		out << "MOUSE_RING_RADIUS";
		break;
	case SettingID::FLICK_SNAP_MODE:
		out << "FLICK_SNAP_MODE";
		break;
	case SettingID::FLICK_SNAP_STRENGTH:
		out << "FLICK_SNAP_STRENGTH";
		break;
	case SettingID::STICK_POWER:
		out << "STICK_POWER";
		break;
	case SettingID::STICK_SENS:
		out << "STICK_SENS";
		break;
	case SettingID::REAL_WORLD_CALIBRATION:
		out << "REAL_WORLD_CALIBRATION";
		break;
	case SettingID::IN_GAME_SENS:
		out << "IN_GAME_SENS";
		break;
	case SettingID::TRIGGER_THRESHOLD:
		out << "TRIGGER_THRESHOLD";
		break;
	case SettingID::RESET_MAPPINGS:
		out << "RESET_MAPPINGS";
		break;
	case SettingID::NO_GYRO_BUTTON:
		out << "NO_GYRO_BUTTON";
		break;
	case SettingID::LEFT_STICK_MODE:
		out << "LEFT_STICK_MODE";
		break;
	case SettingID::RIGHT_STICK_MODE:
		out << "RIGHT_STICK_MODE";
		break;
	case SettingID::LEFT_RING_MODE:
		out << "LEFT_RING_MODE";
		break;
	case SettingID::RIGHT_RING_MODE:
		out << "RIGHT_RING_MODE";
		break;
	case SettingID::GYRO_OFF:
		out << "GYRO_OFF";
		break;
	case SettingID::GYRO_ON:
		out << "GYRO_ON";
		break;
	case SettingID::STICK_AXIS_X:
		out << "STICK_AXIS_X";
		break;
	case SettingID::STICK_AXIS_Y:
		out << "STICK_AXIS_Y";
		break;
	case SettingID::GYRO_AXIS_X:
		out << "GYRO_AXIS_X";
		break;
	case SettingID::GYRO_AXIS_Y:
		out << "GYRO_AXIS_Y";
		break;
	case SettingID::RECONNECT_CONTROLLERS:
		out << "RECONNECT_CONTROLLERS";
		break;
	case SettingID::COUNTER_OS_MOUSE_SPEED:
		out << "COUNTER_OS_MOUSE_SPEED";
		break;
	case SettingID::IGNORE_OS_MOUSE_SPEED:
		out << "IGNORE_OS_MOUSE_SPEED";
		break;
	case SettingID::JOYCON_GYRO_MASK:
		out << "JOYCON_GYRO_MASK";
		break;
	case SettingID::GYRO_SENS:
		out << "GYRO_SENS";
		break;
	case SettingID::FLICK_TIME:
		out << "FLICK_TIME";
		break;
	case SettingID::GYRO_SMOOTH_THRESHOLD:
		out << "GYRO_SMOOTH_THRESHOLD";
		break;
	case SettingID::GYRO_SMOOTH_TIME:
		out << "GYRO_SMOOTH_TIME";
		break;
	case SettingID::GYRO_CUTOFF_SPEED:
		out << "GYRO_CUTOFF_SPEED";
		break;
	case SettingID::GYRO_CUTOFF_RECOVERY:
		out << "GYRO_CUTOFF_RECOVERY";
		break;
	case SettingID::STICK_ACCELERATION_RATE:
		out << "STICK_ACCELERATION_RATE";
		break;
	case SettingID::STICK_ACCELERATION_CAP:
		out << "STICK_ACCELERATION_CAP";
		break;
	case SettingID::STICK_DEADZONE_INNER:
		out << "STICK_DEADZONE_INNER";
		break;
	case SettingID::STICK_DEADZONE_OUTER:
		out << "STICK_DEADZONE_OUTER";
		break;
	case SettingID::CALCULATE_REAL_WORLD_CALIBRATION:
		out << "CALCULATE_REAL_WORLD_CALIBRATION";
		break;
	case SettingID::FINISH_GYRO_CALIBRATION:
		out << "FINISH_GYRO_CALIBRATION";
		break;
	case SettingID::RESTART_GYRO_CALIBRATION:
		out << "RESTART_GYRO_CALIBRATION";
		break;
	case SettingID::MOUSE_X_FROM_GYRO_AXIS:
		out << "MOUSE_X_FROM_GYRO_AXIS";
		break;
	case SettingID::MOUSE_Y_FROM_GYRO_AXIS:
		out << "MOUSE_Y_FROM_GYRO_AXIS";
		break;
	case SettingID::ZR_DUAL_STAGE_MODE:
		out << "ZR_MODE";
		break;
	case SettingID::ZL_DUAL_STAGE_MODE:
		out << "ZL_MODE";
		break;
	case SettingID::AUTOLOAD:
		out << "AUTOLOAD";
		break;
	case SettingID::HELP:
		out << "HELP";
		break;
	case SettingID::WHITELIST_SHOW:
		out << "WHITELIST_SHOW";
		break;
	case SettingID::WHITELIST_ADD:
		out << "WHITELIST_ADD";
		break;
	case SettingID::WHITELIST_REMOVE:
		out << "WHITELIST_REMOVE";
		break;;
	default:
		out << "ERROR";
	}
	return out;
}

istream &operator >>(istream &in, StickMode &stickMode) {
	string name;
	in >> name;
	if (name.compare("AIM") == 0) {
		stickMode = StickMode::aim;
	}
	else if (name.compare("FLICK") == 0) {
		stickMode = StickMode::flick;
	}
	else if (name.compare("NO_MOUSE") == 0) {
		stickMode = StickMode::none;
	}
	else if (name.compare("FLICK_ONLY") == 0) {
		stickMode = StickMode::flickOnly;
	}
	else if (name.compare("ROTATE_ONLY") == 0) {
		stickMode = StickMode::rotateOnly;
	}
	else if (name.compare("MOUSE_RING") == 0) {
		stickMode = StickMode::mouseRing;
	}
	else if (name.compare("MOUSE_AREA") == 0) {
		stickMode = StickMode::mouseArea;
	}
	// just here for backwards compatibility
	else if (name.compare("INNER_RING") == 0) {
		stickMode = StickMode::inner;
	}
	else if (name.compare("OUTER_RING") == 0) {
		stickMode = StickMode::outer;
	}
	else
	{
		stickMode = StickMode::invalid;
	}
	return in;
}
ostream &operator <<(ostream &out, StickMode stickMode) {
	switch (stickMode)
	{
	case StickMode::none:
		out << "NONE";
		break;
	case StickMode::aim:
		out << "AIM";
		break;
	case StickMode::flick:
		out << "FLICK";
		break;
	case StickMode::flickOnly:
		out << "FLICK_ONLY";
		break;
	case StickMode::rotateOnly:
		out << "ROTATE_ONLY";
		break;
	case StickMode::mouseRing:
		out << "MOUSE_RING";
		break;
	case StickMode::mouseArea:
		out << "MOUSE_AREA";
		break;
	case StickMode::outer:
		out << "OUTER_RING";
		break;
	case StickMode::inner:
		out << "INNER_RING";
		break;
	default:
		out << "INVALID";
		break;
	}
	return out;
}

istream &operator >>(istream &in, RingMode &ringMode) {
	string name;
	in >> name;
	if (name.compare("INNER") == 0) {
		ringMode = RingMode::inner;
	}
	else if (name.compare("OUTER") == 0) {
		ringMode = RingMode::outer;
	}
	else
	{
		ringMode = RingMode::invalid;
	}
	return in;
}
ostream &operator <<(ostream &out, RingMode ringMode)
{
	switch (ringMode)
	{
	case RingMode::outer:
		out << "OUTER";
		break;
	case RingMode::inner:
		out << "INNER";
		break;
	default:
		out << "INVALID";
		break;
	}
	return out;
}

istream &operator >>(istream &in, FlickSnapMode &fsm)
{
	string name;
	in >> name;
	if (name.compare("NONE") == 0) {
		fsm = FlickSnapMode::none;
	}
	else if (name.compare("4") == 0) {
		fsm = FlickSnapMode::four;
	}
	else if (name.compare("8") == 0) {
		fsm = FlickSnapMode::eight;
	}
	else
	{
		fsm = FlickSnapMode::invalid;
	}
	return in;
}
ostream &operator <<(ostream &out, FlickSnapMode fsm) {
	switch (fsm)
	{
	case FlickSnapMode::none:
		out << "NONE";
		break;
	case FlickSnapMode::four:
		out << "4";
		break;
	case FlickSnapMode::eight:
		out << "8";
		break;
	default:
		out << "invalid";
		break;
	}
	return out;
}

istream &operator >>(istream &in, AxisMode &axisMode) {
	string name;
	in >> name;
	if (name.compare("STANDARD") == 0) {
		axisMode = AxisMode::standard;
	}
	else if (name.compare("INVERTED") == 0) {
		axisMode = AxisMode::inverted;
	}
	else
	{
		axisMode = AxisMode::invalid;
	}
	return in;
}
ostream &operator <<(ostream &out, AxisMode axisMode)
{
	switch (axisMode)
	{
	case AxisMode::standard:
		out << "STANDARD";
		break;
	case AxisMode::inverted:
		out << "INVERTED";
		break;
	default:
		out << "INVALID";
		break;
	}
	return out;
}

istream &operator >>(istream &in, TriggerMode &triggerMode) {
	string name;
	in >> name;
	if (name.compare("NO_FULL") == 0) {
		triggerMode = TriggerMode::noFull;
	}
	else if (name.compare("NO_SKIP") == 0) {
		triggerMode = TriggerMode::noSkip;
	}
	else if (name.compare("MAY_SKIP") == 0) {
		triggerMode = TriggerMode::maySkip;
	}
	else if (name.compare("MUST_SKIP") == 0) {
		triggerMode = TriggerMode::mustSkip;
	}
	else if (name.compare("MAY_SKIP_R") == 0) {
		triggerMode = TriggerMode::maySkipResp;
	}
	else if (name.compare("MUST_SKIP_R") == 0) {
		triggerMode = TriggerMode::mustSkipResp;
	}
	else
	{
		triggerMode = TriggerMode::invalid;
	}
	return in;
}
ostream &operator <<(ostream &out, TriggerMode triggerMode)
{
	switch (triggerMode)
	{
	case TriggerMode::noFull:
		out << "NO_FULL";
		break;
	case TriggerMode::noSkip:
		out << "NO_SKIP";
		break;
	case TriggerMode::maySkip:
		out << "MAY_SKIP";
		break;
	case TriggerMode::mustSkip:
		out << "MUST_SKIP";
		break;
	case TriggerMode::maySkipResp:
		out << "MAY_SKIP_R";
		break;
	case TriggerMode::mustSkipResp:
		out << "MUST_SKIP_R";
		break;
	default:
		out << "INVALID";
		break;
	}
	return out;
}

istream &operator >>(istream &in, GyroAxisMask &gyroMask) {
	string name;
	in >> name;
	if (name.compare("X") == 0) {
		gyroMask = GyroAxisMask::x;
	}
	else if (name.compare("Y") == 0) {
		gyroMask = GyroAxisMask::y;
	}
	else if (name.compare("Z") == 0) {
		gyroMask = GyroAxisMask::z;
	}
	else if (name.compare("NONE") == 0) {
		gyroMask = GyroAxisMask::none;
	}
	else
	{
		gyroMask = GyroAxisMask::invalid;
	}
	return in;
}
ostream &operator <<(ostream &out, GyroAxisMask gyroMask)
{
	switch (gyroMask)
	{
	case GyroAxisMask::none:
		out << "NONE";
		break;
	case GyroAxisMask::x:
		out << "X";
		break;
	case GyroAxisMask::y:
		out << "Y";
		break;
	case GyroAxisMask::z:
		out << "Z";
		break;
	default:
		out << "INVALID";
		break;
	}
	return out;
}

istream &operator >>(istream &in, JoyconMask &joyconMask) {
	string name;
	in >> name;
	if (name.compare("USE_BOTH") == 0) {
		joyconMask = JoyconMask::useBoth;
	}
	else if (name.compare("IGNORE_LEFT") == 0) {
		joyconMask = JoyconMask::ignoreLeft;
	}
	else if (name.compare("IGNORE_RIGHT") == 0) {
		joyconMask = JoyconMask::ignoreRight;
	}
	else if (name.compare("IGNORE_BOTH") == 0) {
		joyconMask = JoyconMask::ignoreBoth;
	}
	else
	{
		joyconMask = JoyconMask::invalid;
	}
	return in;
}
ostream &operator <<(ostream &out, JoyconMask joyconMask)
{
	switch (joyconMask)
	{
	case JoyconMask::useBoth:
		out << "USE_BOTH";
		break;
	case JoyconMask::ignoreLeft:
		out << "IGNORE_LEFT";
		break;
	case JoyconMask::ignoreRight:
		out << "IGNORE_RIGHT";
		break;
	case JoyconMask::ignoreBoth:
		out << "IGNORE_BOTH";
		break;
	default:
		out << "INVALID";
		break;
	}
	return out;
}

istream &operator >>(istream &in, GyroSettings &gyro_settings)
{
	string valueName;
	in >> valueName;
	ButtonID rhsMappingIndex = ButtonID::ERR;
	stringstream(valueName) >> rhsMappingIndex;
	if (rhsMappingIndex >= ButtonID::NONE)
	{
		gyro_settings.button = rhsMappingIndex;
		gyro_settings.ignore_mode = GyroIgnoreMode::button;
	}
	else
	{
		if (valueName.compare("LEFT_STICK") == 0)
		{
			gyro_settings.button = ButtonID::NONE;
			gyro_settings.ignore_mode = GyroIgnoreMode::left;
		}
		else if (valueName.compare("RIGHT_STICK") == 0)
		{
			gyro_settings.button = ButtonID::NONE;
			gyro_settings.ignore_mode = GyroIgnoreMode::right;
		}
		else
		{
			gyro_settings.ignore_mode = GyroIgnoreMode::invalid;
		}
	}
	return in;
}
ostream &operator <<(ostream &out, GyroSettings gyro_settings)
{
	switch (gyro_settings.ignore_mode)
	{
	case GyroIgnoreMode::left:
		out << "LEFT_STICK";
		break;
	case GyroIgnoreMode::right:
		out << "RIGHT_STICK";
		break;
	case GyroIgnoreMode::button:
		if (gyro_settings.button == ButtonID::NONE)
			out << "No button disables or enables gyro";
		else
			out << "some button (TODO: button to string conversion)";  //TODO: button to string conversion
		break;
	default:
		out << "INVALID";
		break;
	}
	return out;
}

istream &operator >>(istream &in, Mapping &mapping)
{
	string valueName(128, '\0');
	in.getline(&valueName[0], valueName.size());
	valueName.resize(strlen(valueName.c_str()));
	stringstream ss(valueName);
	valueName.clear();
	ss >> valueName;
	mapping.pressBind = nameToKey(valueName);
	valueName.clear();
	ss >> valueName;
	mapping.holdBind = nameToKey(valueName);
	return in;
}

ostream &operator <<(ostream &out, Mapping mapping)
{
	out << "Yikes, this might take a bit of work :(" << endl;
	return out;
}
