#include <cstdlib>

#include <string>
#include <cstring>
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "JoyShockMapper.h"
#include "PlatformDefinitions.h"


// https://www.theurbanpenguin.com/4184-2/
#define FOREGROUND_BLUE 34 // text color is blue.
#define FOREGROUND_GREEN 32 // text color is green.
#define FOREGROUND_RED 31 // text color is red.
#define FOREGROUND_YELLOW 33 // Text color is yellow
#define FOREGROUND_INTENSITY 0x0100 // text color is bold.
#define DEFAULT_COLOR 37 // text color is white

template<std::ostream *stdio, uint16_t color>
struct ColorStream : public std::stringbuf
{
	~ColorStream()
	{

		(*stdio) << "\033[" << (color >> 8) << ';' << (color & 0x00FF) << 'm' << str() << "\033[0;" << DEFAULT_COLOR << 'm';
	}
};

streambuf *Log::makeBuffer(Level level)
{
	switch (level)
	{
	case Level::ERR:
		return new ColorStream<&std::cerr, FOREGROUND_RED | FOREGROUND_INTENSITY>();
	case Level::WARN:
		return new ColorStream<&cout, FOREGROUND_YELLOW | FOREGROUND_INTENSITY>();
	case Level::INFO:
		return new ColorStream<&cout, FOREGROUND_BLUE | FOREGROUND_INTENSITY>();
#if defined(NDEBUG) // release
	case Level::UT:
		return new NullBuffer(); // unused
#else
	case Level::UT:
		return new ColorStream<&cout, FOREGROUND_BLUE | FOREGROUND_RED>(); // purplish
#endif
	case Level::BOLD:
		return new ColorStream<&cout, FOREGROUND_GREEN | FOREGROUND_INTENSITY>();
	default:
		return new ColorStream<&std::cout, FOREGROUND_GREEN>();
	}
}

const char *AUTOLOAD_FOLDER() {
	std::string directory;

	const auto XDG_CONFIG_HOME = getenv("XDG_CONFIG_HOME");
	if (XDG_CONFIG_HOME == nullptr)
	{
		directory = std::string{ getenv("HOME") } + "/.config";
	}
	else
	{
		directory = XDG_CONFIG_HOME;
	}

	directory = directory + "/JoyShockMapper/AutoLoad/";
	return strdup(directory.c_str());
};

const char *GYRO_CONFIGS_FOLDER() {
	std::string directory;

	const auto XDG_CONFIG_HOME = getenv("XDG_CONFIG_HOME");
	if (XDG_CONFIG_HOME == nullptr)
	{
		directory = std::string{ getenv("HOME") } + "/.config";
	}
	else
	{
		directory = XDG_CONFIG_HOME;
	}

	directory = directory + "/JoyShockMapper/GyroConfigs/";
	return strdup(directory.c_str());
};

const char *BASE_JSM_CONFIG_FOLDER() {
	std::string directory;

	const auto XDG_CONFIG_HOME = getenv("XDG_CONFIG_HOME");
	if (XDG_CONFIG_HOME == nullptr)
	{
		directory = std::string{ getenv("HOME") } + "/.config";
	}
	else
	{
		directory = XDG_CONFIG_HOME;
	}

	directory = directory + "/JoyShockMapper/";
	return strdup(directory.c_str());
};

unsigned long GetCurrentProcessId()
{
	return ::getpid();
}

/// Valid inputs:
/// 0-9, N0-N9, F1-F29, A-Z, (L, R, )CONTROL, (L, R, )ALT, (L, R, )SHIFT, TAB, ENTER
/// (L, M, R)MOUSE, SCROLL(UP, DOWN)
/// NONE
/// And characters: ; ' , . / \ [ ] + - `
/// Yes, this looks slow. But it's only there to help set up faster mappings
WORD nameToKey(in_string name)
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
