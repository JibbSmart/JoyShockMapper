#include "JoyShockMapper.h"
#include "inputHelpers.h"
#include <sstream>

// Should we decide to run C++17, this entire file could be replaced by magic_enum header

template <ButtonID>
istream & operator >>(istream &in, ButtonID &rhv)
{
	string s;
	in >> s;
	if (s.compare("-") == 0) 
		rhv = ButtonID::MINUS;
	else if (s.compare("+") == 0)
		rhv = ButtonID::PLUS;
	else
	{
		auto opt = magic_enum::enum_cast<E>(s);
		rhv = opt ? *opt : *magic_enum::enum_cast<E>("INVALID");
	}
	return in;
}

template<ButtonID>
ostream &operator <<(ostream &out, ButtonID rhv)
{
	if (rhv == ButtonID::PLUS)
		out << "+";
	else if (rhv == ButtonID::MINUS)
		out << "-";
	else
		out << magic_enum::enum_name(rhv);
	return out;
}

template<ButtonID>
istream &operator >>(istream &in, FlickSnapMode &fsm)
{
	string name;
	in >> name;
	if (name.compare("4") == 0) {
		fsm = FlickSnapMode::FOUR;
	}
	else if (name.compare("8") == 0) {
		fsm = FlickSnapMode::EIGHT;
	}
	else
	{
		auto opt = magic_enum::enum_cast<E>(s);
		fsm = opt ? *opt : *magic_enum::enum_cast<E>("INVALID");
	}
	return in;
}

template<ButtonID>
ostream &operator <<(ostream &out, FlickSnapMode fsm) {
	if(fsm ==  FlickSnapMode::FOUR)
		out << "4";
	else if(fsm == FlickSnapMode::EIGHT)
		out << "8";
	else
		out << magic_enum::enum_name(fsm);
	return out;
}


istream &operator >>(istream &in, StickMode &stickMode) {
	string name;
	in >> name;
	if (name.compare("AIM") == 0) {
		stickMode = StickMode::AIM;
	}
	else if (name.compare("FLICK") == 0) {
		stickMode = StickMode::FLICK;
	}
	else if (name.compare("NO_MOUSE") == 0) {
		stickMode = StickMode::NONE;
	}
	else if (name.compare("FLICK_ONLY") == 0) {
		stickMode = StickMode::FLICK_ONLY;
	}
	else if (name.compare("ROTATE_ONLY") == 0) {
		stickMode = StickMode::ROTATE_ONLY;
	}
	else if (name.compare("MOUSE_RING") == 0) {
		stickMode = StickMode::MOUSE_RING;
	}
	else if (name.compare("MOUSE_AREA") == 0) {
		stickMode = StickMode::MOUSE_AREA;
	}
	// just here for backwards compatibility
	else if (name.compare("INNER_RING") == 0) {
		stickMode = StickMode::INNER;
	}
	else if (name.compare("OUTER_RING") == 0) {
		stickMode = StickMode::OUTER;
	}
	else
	{
		stickMode = StickMode::INVALID;
	}
	return in;
}
ostream &operator <<(ostream &out, StickMode stickMode) {
	switch (stickMode)
	{
	case StickMode::NONE:
		out << "NONE";
		break;
	case StickMode::AIM:
		out << "AIM";
		break;
	case StickMode::FLICK:
		out << "FLICK";
		break;
	case StickMode::FLICK_ONLY:
		out << "FLICK_ONLY";
		break;
	case StickMode::ROTATE_ONLY:
		out << "ROTATE_ONLY";
		break;
	case StickMode::MOUSE_RING:
		out << "MOUSE_RING";
		break;
	case StickMode::MOUSE_AREA:
		out << "MOUSE_AREA";
		break;
	case StickMode::OUTER:
		out << "OUTER_RING";
		break;
	case StickMode::INNER:
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
		ringMode = RingMode::INNER;
	}
	else if (name.compare("OUTER") == 0) {
		ringMode = RingMode::OUTER;
	}
	else
	{
		ringMode = RingMode::INVALID;
	}
	return in;
}
ostream &operator <<(ostream &out, RingMode ringMode)
{
	switch (ringMode)
	{
	case RingMode::OUTER:
		out << "OUTER";
		break;
	case RingMode::INNER:
		out << "INNER";
		break;
	default:
		out << "INVALID";
		break;
	}
	return out;
}

istream &operator >>(istream &in, AxisMode &axisMode) {
	string name;
	in >> name;
	if (name.compare("STANDARD") == 0) {
		axisMode = AxisMode::STANDARD;
	}
	else if (name.compare("INVERTED") == 0) {
		axisMode = AxisMode::INVERTED;
	}
	else
	{
		axisMode = AxisMode::INVALID;
	}
	return in;
}
ostream &operator <<(ostream &out, AxisMode axisMode)
{
	switch (axisMode)
	{
	case AxisMode::STANDARD:
		out << "STANDARD";
		break;
	case AxisMode::INVERTED:
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
		triggerMode = TriggerMode::NO_FULL;
	}
	else if (name.compare("NO_SKIP") == 0) {
		triggerMode = TriggerMode::NO_SKIP;
	}
	else if (name.compare("MAY_SKIP") == 0) {
		triggerMode = TriggerMode::MAY_SKIP;
	}
	else if (name.compare("MUST_SKIP") == 0) {
		triggerMode = TriggerMode::MUST_SKIP;
	}
	else if (name.compare("MAY_SKIP_R") == 0) {
		triggerMode = TriggerMode::MAY_SKIP_R;
	}
	else if (name.compare("MUST_SKIP_R") == 0) {
		triggerMode = TriggerMode::MUST_SKIP_R;
	}
	else
	{
		triggerMode = TriggerMode::INVALID;
	}
	return in;
}
ostream &operator <<(ostream &out, TriggerMode triggerMode)
{
	switch (triggerMode)
	{
	case TriggerMode::NO_FULL:
		out << "NO_FULL";
		break;
	case TriggerMode::NO_SKIP:
		out << "NO_SKIP";
		break;
	case TriggerMode::MAY_SKIP:
		out << "MAY_SKIP";
		break;
	case TriggerMode::MUST_SKIP:
		out << "MUST_SKIP";
		break;
	case TriggerMode::MAY_SKIP_R:
		out << "MAY_SKIP_R";
		break;
	case TriggerMode::MUST_SKIP_R:
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
		gyroMask = GyroAxisMask::X;
	}
	else if (name.compare("Y") == 0) {
		gyroMask = GyroAxisMask::Y;
	}
	else if (name.compare("Z") == 0) {
		gyroMask = GyroAxisMask::Z;
	}
	else if (name.compare("NONE") == 0) {
		gyroMask = GyroAxisMask::NONE;
	}
	else
	{
		gyroMask = GyroAxisMask::INVALID;
	}
	return in;
}
ostream &operator <<(ostream &out, GyroAxisMask gyroMask)
{
	switch (gyroMask)
	{
	case GyroAxisMask::NONE:
		out << "NONE";
		break;
	case GyroAxisMask::X:
		out << "X";
		break;
	case GyroAxisMask::Y:
		out << "Y";
		break;
	case GyroAxisMask::Z:
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
		joyconMask = JoyconMask::USE_BOTH;
	}
	else if (name.compare("IGNORE_LEFT") == 0) {
		joyconMask = JoyconMask::IGNORE_LEFT;
	}
	else if (name.compare("IGNORE_RIGHT") == 0) {
		joyconMask = JoyconMask::IGNORE_RIGHT;
	}
	else if (name.compare("IGNORE_BOTH") == 0) {
		joyconMask = JoyconMask::IGNORE_BOTH;
	}
	else
	{
		joyconMask = JoyconMask::INVALID;
	}
	return in;
}
ostream &operator <<(ostream &out, JoyconMask joyconMask)
{
	switch (joyconMask)
	{
	case JoyconMask::USE_BOTH:
		out << "USE_BOTH";
		break;
	case JoyconMask::IGNORE_LEFT:
		out << "IGNORE_LEFT";
		break;
	case JoyconMask::IGNORE_RIGHT:
		out << "IGNORE_RIGHT";
		break;
	case JoyconMask::IGNORE_BOTH:
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
		gyro_settings.ignore_mode = GyroIgnoreMode::BUTTON;
	}
	else
	{
		if (valueName.compare("LEFT_STICK") == 0)
		{
			gyro_settings.button = ButtonID::NONE;
			gyro_settings.ignore_mode = GyroIgnoreMode::LEFT_STICK;
		}
		else if (valueName.compare("RIGHT_STICK") == 0)
		{
			gyro_settings.button = ButtonID::NONE;
			gyro_settings.ignore_mode = GyroIgnoreMode::RIGHT_STICK;
		}
		else
		{
			gyro_settings.ignore_mode = GyroIgnoreMode::INVALID;
		}
	}
	return in;
}
ostream &operator <<(ostream &out, GyroSettings gyro_settings)
{
	switch (gyro_settings.ignore_mode)
	{
	case GyroIgnoreMode::LEFT_STICK:
		out << "LEFT_STICK";
		break;
	case GyroIgnoreMode::RIGHT_STICK:
		out << "RIGHT_STICK";
		break;
	case GyroIgnoreMode::BUTTON:
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

bool operator ==(const GyroSettings &lhs, const GyroSettings &rhs)
{
	return lhs.always_off == rhs.always_off && 
		   lhs.button == rhs.button && 
		   lhs.ignore_mode == rhs.ignore_mode;
}

bool operator ==(const Mapping &lhs, const Mapping &rhs)
{
	return lhs.pressBind == rhs.pressBind &&
		   lhs.holdBind == rhs.holdBind;
}

bool operator ==(const FloatXY &lhs, const FloatXY &rhs)
{
	return lhs.first == rhs.first &&
		   lhs.second == rhs.second;
}