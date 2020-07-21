#include "JoyShockMapper.h"
#include "inputHelpers.h"
#include <sstream>

istream & operator >> (istream &in, ButtonID &rhv)
{
	string s;
	in >> s;
	if (s.compare("-") == 0)
		rhv = ButtonID::MINUS;
	else if (s.compare("+") == 0)
		rhv = ButtonID::PLUS;
	else
	{
		auto opt = magic_enum::enum_cast<ButtonID>(s);
		rhv = opt ? *opt : *magic_enum::enum_cast<ButtonID>("INVALID");
	}
	return in;
}

ostream &operator << (ostream &out, ButtonID rhv)
{
	if (rhv == ButtonID::PLUS)
		out << "+";
	else if (rhv == ButtonID::MINUS)
		out << "-";
	else
		out << magic_enum::enum_name(rhv);
	return out;
}

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
		auto opt = magic_enum::enum_cast<FlickSnapMode>(name);
		fsm = opt ? *opt : *magic_enum::enum_cast<FlickSnapMode>("INVALID");
	}
	return in;
}

ostream &operator <<(ostream &out, FlickSnapMode fsm) {
	if(fsm ==  FlickSnapMode::FOUR)
		out << "4";
	else if(fsm == FlickSnapMode::EIGHT)
		out << "8";
	else
		out << magic_enum::enum_name(fsm);
	return out;
}

istream &operator >>(istream &in, GyroSettings &gyro_settings)
{
	string valueName;
	in >> valueName;
	ButtonID rhsMappingIndex = ButtonID::INVALID;
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
		else if(gyro_settings.button != ButtonID::INVALID)
			out << gyro_settings.button;  //TODO: button to string conversion
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
	mapping.pressBind = KeyCode(valueName);
	valueName.clear();
	ss >> valueName;
	mapping.holdBind.code = KeyCode(valueName);
	return in;
}

ostream &operator <<(ostream &out, Mapping mapping)
{
	out << mapping.pressBind.name << " " << mapping.holdBind.name << endl;
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
	return lhs.pressBind.code == rhs.pressBind.code &&
		   lhs.holdBind.code == rhs.holdBind.code;
}

bool operator ==(const FloatXY &lhs, const FloatXY &rhs)
{
	return lhs.first == rhs.first &&
		   lhs.second == rhs.second;
}
