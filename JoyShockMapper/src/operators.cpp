#include "JoyShockMapper.h"
#include "JslWrapper.h"
#include "ColorCodes.h"

#include <cstring>
#include <sstream>
#include <memory>
#include <algorithm>
#include <iomanip>
#include <cmath>

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

ostream &operator<<(ostream &out, const KeyCode &code)
{
	return out << code.name;
}

istream &operator>>(istream &in, ButtonID &rhv)
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

ostream &operator<<(ostream &out, const ButtonID &rhv)
{
	if (rhv == ButtonID::PLUS)
		out << "+";
	else if (rhv == ButtonID::MINUS)
		out << "-";
	else
		out << magic_enum::enum_name(rhv);
	return out;
}

istream &operator>>(istream &in, FlickSnapMode &fsm)
{
	string name;
	in >> name;
	if (name.compare("0") == 0)
	{
		fsm = FlickSnapMode::NONE;
	}
	else if (name.compare("4") == 0)
	{
		fsm = FlickSnapMode::FOUR;
	}
	else if (name.compare("8") == 0)
	{
		fsm = FlickSnapMode::EIGHT;
	}
	else
	{
		auto opt = magic_enum::enum_cast<FlickSnapMode>(name);
		fsm = opt ? *opt : FlickSnapMode::INVALID;
	}
	return in;
}

ostream &operator<<(ostream &out, const FlickSnapMode &fsm)
{
	if (fsm == FlickSnapMode::FOUR)
		out << "4";
	else if (fsm == FlickSnapMode::EIGHT)
		out << "8";
	else
		out << magic_enum::enum_name(fsm);
	return out;
}

istream &operator>>(istream &in, TriggerMode &tm)
{
	string name;
	in >> name;
	if (name.compare("PS_L2") == 0)
	{
		tm = TriggerMode::X_LT;
	}
	else if (name.compare("PS_R2") == 0)
	{
		tm = TriggerMode::X_RT;
	}
	else
	{
		auto opt = magic_enum::enum_cast<TriggerMode>(name);
		tm = opt ? *opt : TriggerMode::INVALID;
	}
	return in;
}

istream &operator>>(istream &in, GyroSettings &gyro_settings)
{
	string valueName;
	in >> valueName;
	stringstream ss(valueName);
	auto rhsMappingIndex = magic_enum::enum_cast<ButtonID>(valueName);
	if (valueName.compare("NONE\\") == 0) // Special handling to assign none to a modeshift
	{
		gyro_settings.button = ButtonID::NONE;
		gyro_settings.ignore_mode = GyroIgnoreMode::BUTTON;
	}
	else if (rhsMappingIndex && *rhsMappingIndex >= ButtonID::NONE)
	{
		gyro_settings.button = *rhsMappingIndex;
		gyro_settings.ignore_mode = GyroIgnoreMode::BUTTON;
	}
	else
	{
		auto ignoreMode = magic_enum::enum_cast<GyroIgnoreMode>(valueName);
		if (ignoreMode == GyroIgnoreMode::LEFT_STICK)
		{
			gyro_settings.button = ButtonID::NONE;
			gyro_settings.ignore_mode = GyroIgnoreMode::LEFT_STICK;
		}
		else if (ignoreMode == GyroIgnoreMode::RIGHT_STICK)
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

ostream &operator<<(ostream &out, const GyroSettings &gyro_settings)
{
	if (gyro_settings.ignore_mode == GyroIgnoreMode::BUTTON)
	{
		out << gyro_settings.button;
	}
	else
	{
		out << gyro_settings.ignore_mode;
	}
	return out;
}

bool operator==(const GyroSettings &lhs, const GyroSettings &rhs)
{
	return lhs.always_off == rhs.always_off &&
	  lhs.button == rhs.button &&
	  lhs.ignore_mode == rhs.ignore_mode;
}

ostream &operator<<(ostream &out, const FloatXY &fxy)
{
	out << fxy.first;
	if (fxy.first != fxy.second)
		out << " " << fxy.second;
	return out;
}

istream &operator>>(istream &in, FloatXY &fxy)
{
	size_t pos;
	string value;
	getline(in, value);
	auto sens = getFloat(value, &pos);
	if (sens)
	{
		FloatXY newSens{ *sens, *sens };
		sens = getFloat(&value[pos]);
		if (sens)
		{
			newSens.second = *sens;
			value[pos] = 0;
		}
		fxy = newSens;
	}
	else
	{
		in.setstate(in.failbit);
	}
	return in;
}

bool operator==(const FloatXY &lhs, const FloatXY &rhs)
{
	// Do we need more precision than 1e-5?
	return fabs(lhs.first - rhs.first) < 1e-5 &&
	  fabs(lhs.second - rhs.second) < 1e-5;
}

istream &operator>>(istream &in, AxisMode &am)
{
	string name;
	in >> name;
	if (name.compare("1") == 0)
	{
		am = AxisMode::STANDARD;
	}
	else if (name.compare("-1") == 0)
	{
		am = AxisMode::INVERTED;
	}
	else
	{
		auto opt = magic_enum::enum_cast<AxisMode>(name);
		am = opt ? *opt : AxisMode::INVALID;
	}
	return in;
}

ostream &operator<<(ostream &out, const AxisSignPair &asp)
{
	out << asp.first;
	if (asp.first != asp.second)
	{
		out << " " << asp.second;
	}
	return out;
}

istream &operator>>(istream &in, AxisSignPair &asp)
{
	size_t pos;
	string value;
	getline(in, value);
	stringstream ss(value);
	ss >> asp.first;
	if (asp.first != AxisMode::INVALID)
	{
		if (ss.eof())
		{
			asp.second = asp.first;
		}
		else
		{
			AxisMode second = AxisMode::INVALID;
			ss >> second;
			if (second != AxisMode::INVALID)
			{
				asp.second = second;
			}
			else
			{
				in.setstate(in.failbit);
			}
		}
	}
	else
	{
		in.setstate(in.failbit);
	}
	return in;
}

bool operator==(const AxisSignPair &lhs, const AxisSignPair &rhs)
{
	return lhs.first == rhs.first && lhs.second == rhs.second;
}

istream &operator>>(istream &in, PathString &fxy)
{
	// 260 is windows MAX_PATH length
	fxy.resize(260, '\0');
	in.getline(&fxy[0], fxy.size());
	fxy.resize(strlen(fxy.c_str()));
	return in;
}

istream &operator>>(istream &in, Color &color)
{
	if (in.peek() == 'x')
	{
		char pound;
		in >> pound >> std::hex >> color.raw;
	}
	else if (in.peek() >= 'A' && in.peek() <= 'Z')
	{
		string s;
		in >> s;
		auto code = colorCodeMap.find(s);
		if (code == colorCodeMap.end())
		{
			in.setstate(in.failbit);
		}
		else
		{
			color.raw = code->second;
		}
	}
	else
	{
		int r, g, b;
		in >> r >> g >> b;
		color.rgb.r = uint8_t(clamp(r, 0, 255));
		color.rgb.g = uint8_t(clamp(g, 0, 255));
		color.rgb.b = uint8_t(clamp(b, 0, 255));
	}
	return in;
}

ostream &operator<<(ostream &out, const Color &color)
{
	out << 'x' << hex << setw(2) << setfill('0') << int(color.rgb.r)
	    << hex << setw(2) << setfill('0') << int(color.rgb.g)
	    << hex << setw(2) << setfill('0') << int(color.rgb.b);
	return out;
}

bool operator==(const Color &lhs, const Color &rhs)
{
	return lhs.raw == rhs.raw;
}

istream &operator>>(istream &in, AdaptiveTriggerSetting &atm)
{
	string s;
	in >> s;
	auto opt = magic_enum::enum_cast<AdaptiveTriggerMode>(s);

	memset(&atm, 0, sizeof(AdaptiveTriggerSetting));
	atm.mode = opt ? *opt : AdaptiveTriggerMode::INVALID;

	switch (atm.mode)
	{
	case AdaptiveTriggerMode::SEGMENT:
		in >> atm.start >> atm.end >> atm.force;
		break;
	case AdaptiveTriggerMode::RESISTANCE:
		in >> atm.start >> atm.force;
		break;
	case AdaptiveTriggerMode::BOW:
		in >> atm.start >> atm.end >> atm.force >> atm.forceExtra;
		break;
	case AdaptiveTriggerMode::GALLOPING:
		in >> atm.start >> atm.end >> atm.force >> atm.forceExtra >> atm.frequency;
		break;
	case AdaptiveTriggerMode::SEMI_AUTOMATIC:
		in >> atm.start >> atm.end >> atm.force;
		break;
	case AdaptiveTriggerMode::AUTOMATIC:
		in >> atm.start >> atm.force >> atm.frequency;
		break;
	case AdaptiveTriggerMode::MACHINE:
		in >> atm.start >> atm.end >> atm.force >> atm.forceExtra >> atm.frequency >> atm.frequencyExtra;
		break;
	default:
		break;
	}
	return in;
}

ostream &operator<<(ostream &out, const AdaptiveTriggerSetting &atm)
{
	out << atm.mode << ' ';
	switch (atm.mode)
	{
	case AdaptiveTriggerMode::SEGMENT:
		out << atm.start << ' ' << atm.end << ' ' << atm.force;
		break;
	case AdaptiveTriggerMode::RESISTANCE:
		out << atm.start << ' ' << atm.force;
		break;
	case AdaptiveTriggerMode::BOW:
		out << atm.start << ' ' << atm.end << ' ' << atm.force << ' ' << atm.forceExtra;
		break;
	case AdaptiveTriggerMode::GALLOPING:
		out << atm.start << ' ' << atm.end << ' ' << atm.force << ' ' << atm.forceExtra << ' ' << atm.frequency;
		break;
	case AdaptiveTriggerMode::SEMI_AUTOMATIC:
		out << atm.start << ' ' << atm.end << ' ' << atm.force;
		break;
	case AdaptiveTriggerMode::AUTOMATIC:
		out << atm.start << ' ' << atm.force << ' ' << atm.frequency;
		break;
	case AdaptiveTriggerMode::MACHINE:
		out << atm.start << ' ' << atm.end << ' ' << atm.force << ' ' << atm.forceExtra << ' ' << atm.frequency << ' ' << atm.frequencyExtra;
		break;
	default:
		break;
	}
	return out;
}

bool operator==(const AdaptiveTriggerSetting &lhs, const AdaptiveTriggerSetting &rhs)
{
	return lhs.mode == rhs.mode &&
		lhs.start == rhs.start &&
		lhs.end == rhs.end &&
		lhs.force == rhs.force &&
		lhs.frequency == rhs.frequency &&
		lhs.forceExtra == rhs.forceExtra &&
		lhs.frequencyExtra == rhs.frequencyExtra;
}