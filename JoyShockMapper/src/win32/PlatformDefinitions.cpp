#include <cstdlib>

#include "JoyShockMapper.h"
#include "InputHelpers.h"

const char *AUTOLOAD_FOLDER() {
	return _strdup((GetCWD() + "\\Autoload\\").c_str());
};

const char *GYRO_CONFIGS_FOLDER() {
	return _strdup((GetCWD() + "\\GyroConfigs\\").c_str());
};

const char *BASE_JSM_CONFIG_FOLDER() {
	return _strdup((GetCWD() + "\\").c_str());
};

/// Valid inputs:
/// 0-9, N0-N9, F1-F29, A-Z, (L, R, )CONTROL, (L, R, )ALT, (L, R, )SHIFT, TAB, ENTER
/// (L, M, R)MOUSE, SCROLL(UP, DOWN)
/// NONE
/// And characters: ; ' , . / \ [ ] + - `
/// Yes, this looks slow. But it's only there to help set up faster mappings
WORD nameToKey(const std::string &name)
{
	// https://msdn.microsoft.com/en-us/library/dd375731%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
	auto length = name.length();
	if (length == 1)
	{
		// direct mapping to a number or character key
		char character = name.at(0);
		if (character >= '0' && character <= '9')
		{
			return character - '0' + 0x30;
		}
		if (character >= 'A' && character <= 'Z')
		{
			return character - 'A' + 0x41;
		}
		if (character == '+')
		{
			return VK_OEM_PLUS;
		}
		if (character == '-')
		{
			return VK_OEM_MINUS;
		}
		if (character == ',')
		{
			return VK_OEM_COMMA;
		}
		if (character == '.')
		{
			return VK_OEM_PERIOD;
		}
		if (character == ';')
		{
			return VK_OEM_1;
		}
		if (character == '/')
		{
			return VK_OEM_2;
		}
		if (character == '`')
		{
			return VK_OEM_3;
		}
		if (character == '[')
		{
			return VK_OEM_4;
		}
		if (character == '\\')
		{
			return VK_OEM_5;
		}
		if (character == ']')
		{
			return VK_OEM_6;
		}
		if (character == '\'')
		{
			return VK_OEM_7;
		}
	}
	if (length == 2)
	{
		// function key?
		char character = name.at(0);
		char character2 = name.at(1);
		if (character == 'F')
		{
			if (character2 >= '1' && character2 <= '9')
			{
				return character2 - '1' + VK_F1;
			}
		}
		else if (character == 'N')
		{
			if (character2 >= '0' && character2 <= '9')
			{
				return character2 - '0' + VK_NUMPAD0;
			}
		}
	}
	if (length == 3)
	{
		// could be function keys still
		char character = name.at(0);
		char character2 = name.at(1);
		char character3 = name.at(2);
		if (character == 'F')
		{
			if (character2 == '1' || character2 <= '2')
			{
				if (character3 >= '0' && character3 <= '9')
				{
					return (character2 - '1') * 10 + VK_F10 + (character3 - '0');
				}
			}
		}
	}
	if (length > 2 && name[0] == '"' && name[length - 1] == '"')
	{
		return COMMAND_ACTION;
	}
	if (name.compare("LEFT") == 0)
	{
		return VK_LEFT;
	}
	if (name.compare("RIGHT") == 0)
	{
		return VK_RIGHT;
	}
	if (name.compare("UP") == 0)
	{
		return VK_UP;
	}
	if (name.compare("DOWN") == 0)
	{
		return VK_DOWN;
	}
	if (name.compare("SPACE") == 0)
	{
		return VK_SPACE;
	}
	if (name.compare("CONTROL") == 0)
	{
		return VK_CONTROL;
	}
	if (name.compare("LCONTROL") == 0)
	{
		return VK_LCONTROL;
	}
	if (name.compare("RCONTROL") == 0)
	{
		return VK_RCONTROL;
	}
	if (name.compare("SHIFT") == 0)
	{
		return VK_SHIFT;
	}
	if (name.compare("LSHIFT") == 0)
	{
		return VK_LSHIFT;
	}
	if (name.compare("RSHIFT") == 0)
	{
		return VK_RSHIFT;
	}
	if (name.compare("ALT") == 0)
	{
		return VK_MENU;
	}
	if (name.compare("LALT") == 0)
	{
		return VK_LMENU;
	}
	if (name.compare("RALT") == 0)
	{
		return VK_RMENU;
	}
	if (name.compare("LWINDOWS") == 0)
	{
		return VK_LWIN;
	}
	if (name.compare("RWINDOWS") == 0)
	{
		return VK_RWIN;
	}
	if (name.compare("CONTEXT") == 0)
	{
		return VK_APPS;
	}
	if (name.compare("TAB") == 0)
	{
		return VK_TAB;
	}
	if (name.compare("ENTER") == 0)
	{
		return VK_RETURN;
	}
	if (name.compare("ESC") == 0)
	{
		return VK_ESCAPE;
	}
	if (name.compare("PAGEUP") == 0)
	{
		return VK_PRIOR;
	}
	if (name.compare("PAGEDOWN") == 0)
	{
		return VK_NEXT;
	}
	if (name.compare("HOME") == 0)
	{
		return VK_HOME;
	}
	if (name.compare("END") == 0)
	{
		return VK_END;
	}
	if (name.compare("INSERT") == 0)
	{
		return VK_INSERT;
	}
	if (name.compare("DELETE") == 0)
	{
		return VK_DELETE;
	}
	if (name.compare("LMOUSE") == 0)
	{
		return VK_LBUTTON;
	}
	if (name.compare("RMOUSE") == 0)
	{
		return VK_RBUTTON;
	}
	if (name.compare("MMOUSE") == 0)
	{
		return VK_MBUTTON;
	}
	if (name.compare("BMOUSE") == 0)
	{
		return VK_XBUTTON1;
	}
	if (name.compare("FMOUSE") == 0)
	{
		return VK_XBUTTON2;
	}
	if (name.compare("SCROLLDOWN") == 0)
	{
		return V_WHEEL_DOWN;
	}
	if (name.compare("SCROLLUP") == 0)
	{
		return V_WHEEL_UP;
	}
	if (name.compare("BACKSPACE") == 0)
	{
		return VK_BACK;
	}
	if (name.compare("MULTIPLY") == 0)
	{
		return VK_MULTIPLY;
	}
	if (name.compare("ADD") == 0)
	{
		return VK_ADD;
	}
	if (name.compare("SUBSTRACT") == 0)
	{
		return VK_SUBTRACT;
	}
	if (name.compare("SUBTRACT") == 0)
	{
		return VK_SUBTRACT;
	}
	if (name.compare("DIVIDE") == 0)
	{
		return VK_DIVIDE;
	}
	if (name.compare("DECIMAL") == 0)
	{
		return VK_DECIMAL;
	}
	if (name.compare("CAPS_LOCK") == 0)
	{
		return VK_CAPITAL;
	}
	if (name.compare("SCREENSHOT") == 0)
	{
		return VK_SNAPSHOT;
	}
	if (name.compare("SCROLL_LOCK") == 0)
	{
		return VK_SCROLL;
	}
	if (name.compare("NUM_LOCK") == 0)
	{
		return VK_NUMLOCK;
	}
	if (name.compare("MUTE") == 0)
	{
		return VK_VOLUME_MUTE;
	}
	if (name.compare("VOLUME_DOWN") == 0)
	{
		return VK_VOLUME_DOWN;
	}
	if (name.compare("VOLUME_UP") == 0)
	{
		return VK_VOLUME_UP;
	}
	if (name.compare("NEXT_TRACK") == 0)
	{
		return VK_MEDIA_NEXT_TRACK;
	}
	if (name.compare("PREV_TRACK") == 0)
	{
		return VK_MEDIA_PREV_TRACK;
	}
	if (name.compare("STOP_TRACK") == 0)
	{
		return VK_MEDIA_STOP;
	}
	if (name.compare("PLAY_PAUSE") == 0)
	{
		return VK_MEDIA_PLAY_PAUSE;
	}
	if (name.compare("NONE") == 0)
	{
		return NO_HOLD_MAPPED;
	}
	if (name.compare("CALIBRATE") == 0)
	{
		return CALIBRATE;
	}
	if (name.compare("GYRO_INV_X") == 0)
	{
		return GYRO_INV_X;
	}
	if (name.compare("GYRO_INV_Y") == 0)
	{
		return GYRO_INV_Y;
	}
	if (name.compare("GYRO_INVERT") == 0)
	{
		return GYRO_INVERT;
	}
	if (name.compare("GYRO_TRACK_X") == 0)
	{
		return GYRO_TRACK_X;
	}
	if (name.compare("GYRO_TRACK_Y") == 0)
	{
		return GYRO_TRACK_Y;
	}
	if (name.compare("GYRO_TRACKBALL") == 0)
	{
		return GYRO_TRACKBALL;
	}
	if (name.compare("GYRO_ON") == 0)
	{
		return GYRO_ON_BIND;
	}
	if (name.compare("GYRO_OFF") == 0)
	{
		return GYRO_OFF_BIND;
	}
	return 0x00;
}
