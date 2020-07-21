#include "InputHelpers.h"

#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <functional>
#include <string>
#include <thread>
#include <vector>
#include <memory>

#include <libevdev/libevdev-uinput.h>

#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <dlfcn.h>

using WORD = unsigned short;
using DWORD = unsigned long;

using X11Window = unsigned long;
using X11Atom = unsigned long;

#define UINPUT_DEVICE "/dev/uinput"

static void *X11Display{ nullptr };
static X11Atom _NET_WM_PID{ 0 };

static void *(*XOpenDisplay)(const char *);
static int (*XGetInputFocus)(void *, X11Window *, int *);
static int (*XFetchName)(void *, X11Window, char **);
static X11Atom (*XInternAtom)(void *, const char *, int);
static int (*XGetWindowProperty)(void *, X11Window, X11Atom, long, long, int, X11Atom, X11Atom *, int *, unsigned long *, unsigned long *, unsigned char **);
static int (*XFree)(void *);

// Windows' mouse speed settings translate non-linearly to speed.
// Thankfully, the mappings are available here:
// https://liquipedia.net/counterstrike/Mouse_settings#Windows_Sensitivity
static float windowsSensitivityMappings[] = { 0.0, // mouse sensitivity range is 1-20, so just put
												   // nothing in the 0th element
	1.0 / 32.0,
	1.0 / 16.0,
	1.0 / 8.0,
	2.0 / 8.0,
	3.0 / 8.0,
	4.0 / 8.0,
	5.0 / 8.0,
	6.0 / 8.0,
	7.0 / 8.0,
	1.0,
	1.25,
	1.5,
	1.75,
	2.0,
	2.25,
	2.5,
	2.75,
	3.0,
	3.25,
	3.5 };

class VirtualInputDevice
{
private:
	constexpr auto windows_key_to_evdev_key(WORD key) noexcept
	{
		switch (key)
		{
		case '0':
			return KEY_0;
		case '1':
			return KEY_1;
		case '2':
			return KEY_2;
		case '3':
			return KEY_3;
		case '4':
			return KEY_4;
		case '5':
			return KEY_5;
		case '6':
			return KEY_6;
		case '7':
			return KEY_7;
		case '8':
			return KEY_8;
		case '9':
			return KEY_9;
		case 'A':
			return KEY_A;
		case 'B':
			return KEY_B;
		case 'C':
			return KEY_C;
		case 'D':
			return KEY_D;
		case 'E':
			return KEY_E;
		case 'F':
			return KEY_F;
		case 'G':
			return KEY_G;
		case 'H':
			return KEY_H;
		case 'I':
			return KEY_I;
		case 'J':
			return KEY_J;
		case 'K':
			return KEY_K;
		case 'L':
			return KEY_L;
		case 'M':
			return KEY_M;
		case 'N':
			return KEY_N;
		case 'O':
			return KEY_O;
		case 'P':
			return KEY_P;
		case 'Q':
			return KEY_Q;
		case 'R':
			return KEY_R;
		case 'S':
			return KEY_S;
		case 'T':
			return KEY_T;
		case 'U':
			return KEY_U;
		case 'V':
			return KEY_V;
		case 'W':
			return KEY_W;
		case 'X':
			return KEY_X;
		case 'Y':
			return KEY_Y;
		case 'Z':
			return KEY_Z;
		case VK_OEM_PLUS:
			return KEY_EQUAL;
		case VK_OEM_MINUS:
			return KEY_MINUS;
		case VK_OEM_COMMA:
			return KEY_COMMA;
		case VK_OEM_PERIOD:
			return KEY_DOT;
		case VK_OEM_1:
			return KEY_SEMICOLON;
		case VK_OEM_2:
			return KEY_SLASH;
		case VK_OEM_3:
			return KEY_GRAVE;
		case VK_OEM_4:
			return KEY_LEFTBRACE;
		case VK_OEM_5:
			return KEY_BACKSLASH;
		case VK_OEM_6:
			return KEY_RIGHTBRACE;
		case VK_OEM_7:
			return KEY_APOSTROPHE;
		case VK_F1:
			return KEY_F1;
		case VK_F2:
			return KEY_F2;
		case VK_F3:
			return KEY_F3;
		case VK_F4:
			return KEY_F4;
		case VK_F5:
			return KEY_F5;
		case VK_F6:
			return KEY_F6;
		case VK_F7:
			return KEY_F7;
		case VK_F8:
			return KEY_F8;
		case VK_F9:
			return KEY_F9;
		case VK_F10:
			return KEY_F10;
		case VK_F11:
			return KEY_F11;
		case VK_F12:
			return KEY_F12;
		case VK_F13:
			return KEY_F13;
		case VK_F14:
			return KEY_F14;
		case VK_F15:
			return KEY_F15;
		case VK_F16:
			return KEY_F16;
		case VK_F17:
			return KEY_F17;
		case VK_F18:
			return KEY_F18;
		case VK_F19:
			return KEY_F19;
		case VK_NUMPAD0:
			return KEY_NUMERIC_0;
		case VK_NUMPAD1:
			return KEY_NUMERIC_1;
		case VK_NUMPAD2:
			return KEY_NUMERIC_2;
		case VK_NUMPAD3:
			return KEY_NUMERIC_3;
		case VK_NUMPAD4:
			return KEY_NUMERIC_4;
		case VK_NUMPAD5:
			return KEY_NUMERIC_5;
		case VK_NUMPAD6:
			return KEY_NUMERIC_6;
		case VK_NUMPAD7:
			return KEY_NUMERIC_7;
		case VK_NUMPAD8:
			return KEY_NUMERIC_8;
		case VK_NUMPAD9:
			return KEY_NUMERIC_9;
		case VK_LEFT:
			return KEY_LEFT;
		case VK_RIGHT:
			return KEY_RIGHT;
		case VK_UP:
			return KEY_UP;
		case VK_DOWN:
			return KEY_DOWN;
		case VK_SPACE:
			return KEY_SPACE;
		case VK_CONTROL:
			return KEY_LEFTCTRL;
		case VK_LCONTROL:
			return KEY_LEFTCTRL;
		case VK_RCONTROL:
			return KEY_RIGHTCTRL;
		case VK_SHIFT:
			return KEY_LEFTSHIFT;
		case VK_LSHIFT:
			return KEY_LEFTSHIFT;
		case VK_RSHIFT:
			return KEY_RIGHTSHIFT;
		case VK_MENU:
			return KEY_MENU;
		case VK_LMENU:
			return KEY_LEFTALT;
		case VK_RMENU:
			return KEY_RIGHTALT;
		case VK_TAB:
			return KEY_TAB;
		case VK_RETURN:
			return KEY_ENTER;
		case VK_ESCAPE:
			return KEY_ESC;
		case VK_PRIOR:
			return KEY_PREVIOUS;
		case VK_NEXT:
			return KEY_NEXT;
		case VK_HOME:
			return KEY_HOME;
		case VK_END:
			return KEY_END;
		case VK_INSERT:
			return KEY_INSERT;
		case VK_DELETE:
			return KEY_DELETE;
		case VK_LBUTTON:
			return BTN_LEFT;
		case VK_RBUTTON:
			return BTN_RIGHT;
		case VK_MBUTTON:
			return BTN_MIDDLE;
		case VK_XBUTTON1:
			return BTN_SIDE;
		case VK_XBUTTON2:
			return BTN_EXTRA;
		case V_WHEEL_DOWN:
			return V_WHEEL_DOWN;
		case V_WHEEL_UP:
			return V_WHEEL_UP;
		case VK_BACK:
			return KEY_BACK;
			//		case NO_HOLD_MAPPED: return NO_HOLD_MAPPED;
			//		case CALIBRATE: return CALIBRATE;
			//		case GYRO_INV_X: return GYRO_INV_X;
			//		case GYRO_INV_Y: return GYRO_INV_Y;
			//		case GYRO_INVERT: return GYRO_INVERT;
			//		case GYRO_ON_BIND: return GYRO_ON_BIND;
			//		case GYRO_OFF_BIND: return GYRO_OFF_BIND;
		default:
			return 0x00;
		}
	}

public:
	enum class Device
	{
		MOUSE,
		KEYBOARD
	};

public:
	VirtualInputDevice(Device device) noexcept
	  : device_{ libevdev_new() }
	{
		if (device == Device::MOUSE)
		{
			libevdev_set_name(device_, APPLICATION_NAME "_MOUSE");

			libevdev_enable_event_type(device_, EV_REL);
			libevdev_enable_event_code(device_, EV_REL, REL_X, nullptr);
			libevdev_enable_event_code(device_, EV_REL, REL_Y, nullptr);
			libevdev_enable_event_code(device_, EV_REL, REL_WHEEL, nullptr);

			libevdev_enable_event_type(device_, EV_ABS);
			libevdev_enable_event_code(device_, EV_ABS, ABS_X, nullptr);
			libevdev_enable_event_code(device_, EV_ABS, ABS_Y, nullptr);

			libevdev_enable_event_type(device_, EV_KEY);
			libevdev_enable_event_code(device_, EV_KEY, BTN_LEFT, nullptr);
			libevdev_enable_event_code(device_, EV_KEY, BTN_MIDDLE, nullptr);
			libevdev_enable_event_code(device_, EV_KEY, BTN_RIGHT, nullptr);
		}
		else
		{
			libevdev_set_name(device_, APPLICATION_NAME "_KEYBOARD");
			libevdev_enable_event_type(device_, EV_KEY);
			for (std::uint32_t i = 0; i < 250; ++i)
			{
				libevdev_enable_event_code(device_, EV_KEY, i, nullptr);
			}
		}

		const auto error = libevdev_uinput_create_from_device(device_,
		  LIBEVDEV_UINPUT_OPEN_MANAGED,
		  &uinput_device_);
		if (error != 0)
		{
			std::fprintf(stderr, "Failed to create virtual device: %s\n", std::strerror(-error));
		}
	}

	~VirtualInputDevice() noexcept
	{
		libevdev_uinput_destroy(uinput_device_);
		libevdev_free(device_);
	}

public:
	void press_key(WORD key) noexcept
	{
		auto error =
		  libevdev_uinput_write_event(uinput_device_, EV_KEY, windows_key_to_evdev_key(key), 1);
		if (error != 0)
		{
			std::fprintf(stderr, "Failed to to simulate key press: %s\n", std::strerror(-error));
			return;
		}

		error = libevdev_uinput_write_event(uinput_device_, EV_SYN, SYN_REPORT, 0);
		if (error != 0)
		{
			std::fprintf(stderr, "Failed to to simulate key press: %s\n", std::strerror(-error));
			return;
		}
	}

	void release_key(WORD key) noexcept
	{
		auto error =
		  libevdev_uinput_write_event(uinput_device_, EV_KEY, windows_key_to_evdev_key(key), 0);
		if (error != 0)
		{
			std::fprintf(stderr, "Failed to to simulate key release: %s\n", std::strerror(-error));
			return;
		}

		error = libevdev_uinput_write_event(uinput_device_, EV_SYN, SYN_REPORT, 0);
		if (error != 0)
		{
			std::fprintf(stderr, "Failed to to simulate key release: %s\n", std::strerror(-error));
			return;
		}
	}

	void click_key(WORD key) noexcept
	{
		press_key(key);
		release_key(key);
	}

	void mouse_move_relative(std::int32_t x, std::int32_t y) noexcept
	{
		auto error = libevdev_uinput_write_event(uinput_device_, EV_REL, REL_X, x);
		if (error != 0)
		{
			std::fprintf(stderr, "Failed to to simulate mouse move: %s\n", std::strerror(-error));
			return;
		}

		error = libevdev_uinput_write_event(uinput_device_, EV_REL, REL_Y, y);
		if (error != 0)
		{
			std::fprintf(stderr, "Failed to to simulate mouse move: %s\n", std::strerror(-error));
			return;
		}

		error = libevdev_uinput_write_event(uinput_device_, EV_SYN, SYN_REPORT, 0);
		if (error != 0)
		{
			std::fprintf(stderr, "Failed to to simulate mouse move: %s\n", std::strerror(-error));
			return;
		}
	}

	void mouse_move_absolute(std::int32_t x, std::int32_t y) noexcept
	{
		auto error = libevdev_uinput_write_event(uinput_device_, EV_ABS, ABS_X, x);
		if (error != 0)
		{
			std::fprintf(stderr, "Failed to to simulate mouse move: %s\n", std::strerror(-error));
			return;
		}

		error = libevdev_uinput_write_event(uinput_device_, EV_ABS, ABS_Y, y);
		if (error != 0)
		{
			std::fprintf(stderr, "Failed to to simulate mouse move: %s\n", std::strerror(-error));
			return;
		}

		error = libevdev_uinput_write_event(uinput_device_, EV_SYN, SYN_REPORT, 0);
		if (error != 0)
		{
			std::fprintf(stderr, "Failed to to simulate mouse move: %s\n", std::strerror(-error));
			return;
		}
	}

	void mouse_scroll(std::int32_t amount) noexcept
	{
		auto error = libevdev_uinput_write_event(uinput_device_, EV_REL, REL_WHEEL, amount);
		if (error != 0)
		{
			std::fprintf(stderr, "Failed to to simulate mouse scroll: %s\n", std::strerror(-error));
			return;
		}

		error = libevdev_uinput_write_event(uinput_device_, EV_SYN, SYN_REPORT, 0);
		if (error != 0)
		{
			std::fprintf(stderr, "Failed to to simulate mouse move: %s\n", std::strerror(-error));
			return;
		}
	}

private:
	libevdev *device_;
	libevdev_uinput *uinput_device_{ nullptr };
};

// get the user's mouse sensitivity multiplier from the user. In Windows it's an int, but who cares?
// it's well within range for float to represent it exactly also, if this is ported to other
// platforms, we might want non-integer sensitivities
float getMouseSpeed()
{
	return 1.0;
}

namespace
{
VirtualInputDevice mouse{ VirtualInputDevice::Device::MOUSE };
VirtualInputDevice keyboard{ VirtualInputDevice::Device::KEYBOARD };
} // namespace

// send mouse button
int pressMouse(WORD vkKey, bool isPressed)
{
	if (vkKey == V_WHEEL_UP)
	{
		mouse.mouse_scroll(1);
		return 0;
	}

	if (vkKey == V_WHEEL_DOWN)
	{
		mouse.mouse_scroll(-1);
		return 0;
	}

	if (isPressed)
	{
		mouse.press_key(vkKey);
	}
	else
	{
		mouse.release_key(vkKey);
	}

	return 0;
}

// send key press
int pressKey(WORD vkKey, bool pressed)
{
	if (vkKey == 0)
		return 0;
	if (vkKey <= V_WHEEL_DOWN)
	{
		// Highest mouse ID
		return pressMouse(vkKey, pressed);
	}

	if (pressed)
	{
		keyboard.press_key(vkKey);
	}
	else
	{
		keyboard.release_key(vkKey);
	}

	return 0;
}

float accumulatedX = 0;
float accumulatedY = 0;

void moveMouse(float x, float y)
{
	accumulatedX += x;
	accumulatedY += y;

	int applicableX = (int)accumulatedX;
	int applicableY = (int)accumulatedY;

	accumulatedX -= applicableX;
	accumulatedY -= applicableY;

	mouse.mouse_move_relative(applicableX, applicableY);
	// printf("%0.4f %0.4f\n", accumulatedX, accumulatedY);
}

void setMouseNorm(float x, float y)
{
	mouse.mouse_move_absolute(std::roundf(65535.0f * x), std::roundf(65535.0f * y));
}

bool WriteToConsole(const std::string &command)
{
	constexpr auto STDIN_FD{ 0 };

	for (auto c : command)
	{
		if (::ioctl(STDIN_FD, TIOCSTI, &c) < 0)
		{
			perror("ioctl");
			return false;
		}
	}

	char NEW_LINE = '\n';
	::ioctl(STDIN_FD, TIOCSTI, &NEW_LINE);

	return true;
}

BOOL ConsoleCtrlHandler(DWORD)
{
	return false;
};

// just setting up the console with standard stuff
void initConsole(std::function<void()>)
{
}

std::tuple<std::string, std::string> GetActiveWindowName()
{
	if (X11Display == nullptr)
	{
		static auto *libX11 = ::dlopen("libX11.so", RTLD_LAZY);

		if (libX11)
		{
			XOpenDisplay = reinterpret_cast<decltype(XOpenDisplay)>(::dlsym(libX11, "XOpenDisplay"));
			XGetInputFocus = reinterpret_cast<decltype(XGetInputFocus)>(::dlsym(libX11, "XGetInputFocus"));
			XFetchName = reinterpret_cast<decltype(XFetchName)>(::dlsym(libX11, "XFetchName"));
			XInternAtom = reinterpret_cast<decltype(XInternAtom)>(::dlsym(libX11, "XInternAtom"));
			XGetWindowProperty = reinterpret_cast<decltype(XGetWindowProperty)>(::dlsym(libX11, "XGetWindowProperty"));
			XFree = reinterpret_cast<decltype(XFree)>(::dlsym(libX11, "XFree"));

			X11Display = XOpenDisplay(nullptr);
			_NET_WM_PID = XInternAtom(X11Display, "_NET_WM_PID", true);
		}
	}

	std::tuple<std::string, std::string> result;

	if (X11Display != nullptr)
	{
		constexpr std::size_t MAX_PATH{ 256 };
		X11Window focusedWindow;
		char *focusedWindowName = nullptr;
		unsigned char *processPID = nullptr;
		char focusedWindowExecutableName[MAX_PATH];
		int revert;

		focusedWindowExecutableName[0] = '\0';
		XGetInputFocus(X11Display, &focusedWindow, &revert);
		if (focusedWindow > 0)
		{
			XFetchName(X11Display, focusedWindow, &focusedWindowName);

			X11Atom type;
			int format;
			unsigned long itemCount;
			unsigned long bytesAfter;

			XGetWindowProperty(X11Display, focusedWindow, _NET_WM_PID, 0, 1, false, 0, &type, &format, &itemCount, &bytesAfter, &processPID);
			if (processPID != nullptr)
			{
				const auto pid = *reinterpret_cast<unsigned long *>(processPID);
				XFree(processPID);

				std::snprintf(focusedWindowExecutableName, MAX_PATH, "/proc/%lu/comm", pid);
				std::unique_ptr<FILE, decltype(&std::fclose)> cmd{ std::fopen(focusedWindowExecutableName, "r"), &std::fclose };
				std::memset(focusedWindowExecutableName, 0, MAX_PATH);
				if (cmd != nullptr)
				{
					const auto bytesRead = std::fread(focusedWindowExecutableName, sizeof(char), MAX_PATH, cmd.get());
					if (bytesRead > 0)
					{
						if (focusedWindowExecutableName[bytesRead - 1] == '\n')
						{
							focusedWindowExecutableName[bytesRead - 1] = '\0';
						}
					}
				}
			}
		}

		std::get<0>(result) = focusedWindowExecutableName;

		if (focusedWindowName != nullptr)
		{
			std::get<1>(result) = focusedWindowName;
			XFree(focusedWindowName);
		}
	}

	return result;
}

std::vector<std::string> ListDirectory(std::string directory)
{
	std::vector<std::string> fileListing;

	dirent **entries;
	auto entryCount = ::scandir(directory.c_str(), &entries, nullptr, &::alphasort);

	if (entryCount <= 0)
	{
		return fileListing;
	}

	fileListing.reserve(entryCount);

	while (entryCount--)
	{
		const auto entry = entries[entryCount];
		if (entry->d_type == DT_REG)
		{
			fileListing.emplace_back(entry->d_name);
		}
	}

	return fileListing;
}

std::string GetCWD()
{
	std::unique_ptr<char, decltype(&std::free)> pathBuffer{ getcwd(nullptr, 0), &std::free };
	return pathBuffer.get();
}

DWORD ShowOnlineHelp()
{
	return 0;
}

void HideConsole()
{
}

void ShowConsole()
{
}

void ReleaseConsole()
{
}

bool IsVisible()
{
	return true;
}

bool isConsoleMinimized()
{
	return false;
}

PollingThread::~PollingThread()
{
	if (_continue)
	{
		Stop();
		std::this_thread::sleep_for(std::chrono::milliseconds{ _sleepTimeMs });
	}
	// Let poll function cleanup
	pthread_join(_thread, nullptr);
}

bool PollingThread::Start()
{
	if (_thread && !_continue) // thread is running but hasn't stopped yet
	{
		std::this_thread::sleep_for(std::chrono::milliseconds{ _sleepTimeMs });
	}
	if (!_thread) // thread is clear
	{
		_continue = true;
		pthread_create(&_thread, nullptr, (void *(*)(void *)) & PollingThread::pollFunction, this);
		_tid = _thread;
	}
	return isRunning();
}

DWORD PollingThread::pollFunction(LPVOID param)
{
	auto workerThread = static_cast<PollingThread *>(param);
	if (workerThread)
	{
		while (workerThread->_continue && workerThread->_loopContent(workerThread->_funcParam))
		{
			std::this_thread::sleep_for(
			  std::chrono::milliseconds{ workerThread->_sleepTimeMs });
		}
	}

	return 0;
}
