#include "JoyShockMapper.h"
#include "InputHelpers.h"

#include <cstring>
#include <sstream>
#include <memory>

static optional<float> getFloat(const string &str, size_t *newpos = nullptr)
{
	try
	{
		float f = stof(str, newpos);
		return f;
	}
	catch (invalid_argument)
	{
		return nullopt;
	}
}

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
		rhv = opt ? *opt : ButtonID::INVALID;
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
	if (name.compare("0") == 0) {
		fsm = FlickSnapMode::NONE;
	}
	else if (name.compare("4") == 0) {
		fsm = FlickSnapMode::FOUR;
	}
	else if (name.compare("8") == 0) {
		fsm = FlickSnapMode::EIGHT;
	}
	else
	{
		auto opt = magic_enum::enum_cast<FlickSnapMode>(name);
		fsm = opt ? *opt : FlickSnapMode::INVALID;
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
	stringstream ss(valueName);
	ButtonID rhsMappingIndex;
	ss >> rhsMappingIndex;
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
			in.setstate(in.failbit);
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

bool operator ==(const GyroSettings &lhs, const GyroSettings &rhs)
{
	return lhs.always_off == rhs.always_off &&
		   lhs.button == rhs.button &&
		   lhs.ignore_mode == rhs.ignore_mode;
}

ostream &operator << (ostream &out, FloatXY fxy)
{
	out << fxy.first;
	if (fxy.first != fxy.second)
		out << " " << fxy.second;
	return out;
}

istream &operator >> (istream &in, FloatXY &fxy)
{
	size_t pos;
	string value;
	getline(in, value);
	auto sens = getFloat(value, &pos);
	if (sens)
	{
		FloatXY newSens{ *sens, *sens };
		sens = getFloat(&value[pos]);
		if (sens) {
			newSens.second = *sens;
			value[pos] = 0;
		}
		fxy = newSens;
	}
	return in;
}

bool operator ==(const FloatXY &lhs, const FloatXY &rhs)
{
	// Do we need more precision than 1e-5?
	return fabs(lhs.first - rhs.first) < 1e-5 &&
		   fabs(lhs.second - rhs.second) < 1e-5;
}

istream& operator >> (istream& in, AxisMode& am)
{
	string name;
	in >> name;
	if (name.compare("1") == 0) {
		am = AxisMode::STANDARD;
	}
	else if (name.compare("-1") == 0) {
		am = AxisMode::INVERTED;
	}
	else
	{
		auto opt = magic_enum::enum_cast<AxisMode>(name);
		am = opt ? *opt : AxisMode::INVALID;
	}
	return in;
}

istream& operator >> (istream& in, PathString& fxy)
{
	// 260 is windows MAX_PATH length
	fxy.resize(260, '\0');
	in.getline(&fxy[0], fxy.size());
	fxy.resize(strlen(fxy.c_str()));
	return in;
}

istream &operator >> (istream &in, Color &color)
{
	if (in.peek() == 'x')
	{
		char pound;
		in >> pound >> std::hex >> color.raw;
	}
	else
	{
		int16_t r, g, b;
		in >> r >> g >> b;
		color.rgb.r = r;
		color.rgb.g = g;
		color.rgb.b = b;
	}
	return in;
}

ostream &operator << (ostream &out, Color color)
{
	out << 'x' << std::hex << int(color.rgb.r) << int(color.rgb.g) << int(color.rgb.b);
	return out;
}

bool operator ==(const Color &lhs, const Color &rhs)
{
	return lhs.raw == rhs.raw;
}
