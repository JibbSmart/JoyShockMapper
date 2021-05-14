#pragma once

#include <string>

#ifdef _WIN32

#include <Windows.h>
#include <iostream>
#include <sstream>
#include <mutex>

constexpr uint16_t DEFAULT_COLOR = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; // White
#define FOREGROUND_YELLOW FOREGROUND_RED | FOREGROUND_GREEN

static std::mutex print_mutex;

#define U(string) L##string

using TrayIconData = HINSTANCE;
using UnicodeString = std::wstring;

// Current Working Directory can now be changed: these need to be dynamic
extern const char *AUTOLOAD_FOLDER();
extern const char *GYRO_CONFIGS_FOLDER();
extern const char *BASE_JSM_CONFIG_FOLDER();

#elif defined(__linux__)

#include <cassert>

#define WINAPI
#define VK_OEM_PLUS 0xBB
#define VK_OEM_MINUS 0xBD
#define VK_OEM_COMMA 0xBC
#define VK_OEM_PERIOD 0xBE
#define VK_OEM_1 0xBA
#define VK_OEM_2 0xBF
#define VK_OEM_3 0xC0
#define VK_OEM_4 0xDB
#define VK_OEM_5 0xDC
#define VK_OEM_6 0xDD
#define VK_OEM_7 0xDE
#define VK_F1 0x70
#define VK_F2 0x71
#define VK_F3 0x72
#define VK_F4 0x73
#define VK_F5 0x74
#define VK_F6 0x75
#define VK_F7 0x76
#define VK_F8 0x77
#define VK_F9 0x78
#define VK_F10 0x79
#define VK_F11 0x7A
#define VK_F12 0x7B
#define VK_F13 0x7C
#define VK_F14 0x7D
#define VK_F15 0x7E
#define VK_F16 0x7F
#define VK_F17 0x80
#define VK_F18 0x81
#define VK_F19 0x82
#define VK_NUMPAD0 0x60
#define VK_NUMPAD1 0x61
#define VK_NUMPAD2 0x62
#define VK_NUMPAD3 0x63
#define VK_NUMPAD4 0x64
#define VK_NUMPAD5 0x65
#define VK_NUMPAD6 0x66
#define VK_NUMPAD7 0x67
#define VK_NUMPAD8 0x68
#define VK_NUMPAD9 0x69
#define VK_BACK 0x08
#define VK_LEFT 0x25
#define VK_RIGHT 0x27
#define VK_UP 0x26
#define VK_DOWN 0x28
#define VK_SPACE 0x20
#define VK_CONTROL 0x11
#define VK_LCONTROL 0xA2
#define VK_RCONTROL 0xA3
#define VK_SHIFT 0x10
#define VK_LSHIFT 0xA0
#define VK_RSHIFT 0xA1
#define VK_MENU 0x12
#define VK_LMENU 0xA4
#define VK_RMENU 0xA5
#define VK_TAB 0x09
#define VK_RETURN 0x0D
#define VK_ESCAPE 0x1B
#define VK_PRIOR 0x21
#define VK_NEXT 0x22
#define VK_HOME 0x24
#define VK_END 0x23
#define VK_INSERT 0x2D
#define VK_DELETE 0x2E
#define VK_LBUTTON 0x01
#define VK_RBUTTON 0x02
#define VK_MBUTTON 0x04
#define VK_XBUTTON1 0x05
#define VK_XBUTTON2 0x06

#define U(string) string
#define _ASSERT_EXPR(condition, message) assert(condition)

using BOOL = bool;
using WORD = unsigned short;
using DWORD = unsigned long;
using HANDLE = unsigned long;
using LPVOID = void *;
using TrayIconData = void *;
using UnicodeString = std::string;

// Current Working Directory can now be changed: these need to be dynamic
extern const char *AUTOLOAD_FOLDER();
extern const char *GYRO_CONFIGS_FOLDER();
extern const char *BASE_JSM_CONFIG_FOLDER();

extern unsigned long GetCurrentProcessId();

// https://www.theurbanpenguin.com/4184-2/
#define FOREGROUND_BLUE 34 // text color is blue.
#define FOREGROUND_GREEN 32 // text color is green.
#define FOREGROUND_RED 31 // text color is red.
#define FOREGROUND_YELLOW 33 // Text color is yellow
#define FOREGROUND_INTENSITY 0x0100 // text color is bold.
#define DEFAULT_COLOR 37 // text color is white

template<std::ostream *stdio, uint16_t color>
struct ColorStream : public std::stringstream
{
	~ColorStream()
	{

		(*stdio) << "\033[" << (color >> 8) << ';' << (color & 0x00FF) << 'm' << str() << "\033[0;" << DEFAULT_COLOR << 'm';
	}
};

#else
#error "Unknown platform"
#endif
