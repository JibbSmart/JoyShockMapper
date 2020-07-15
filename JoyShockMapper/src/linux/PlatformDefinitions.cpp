#include <cstdlib>

#include <string>
#include <cstring>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

const char *AUTOLOAD_FOLDER = []{
	std::string directory;

	const auto XDG_CONFIG_HOME = getenv("XDG_CONFIG_HOME");
	if (XDG_CONFIG_HOME == nullptr)
	{
		directory = std::string{ getenv("HOME") } + "/.config/JoyShockMapper/";
	}
	else
	{
		directory = std::string{ XDG_CONFIG_HOME } + "/JoyShockMapper/";
	}

	::mkdir(directory.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	directory += "AutoLoad/";
	::mkdir(directory.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	return strdup(directory.c_str());
}();

const char *GYRO_CONFIGS_FOLDER = []{
	std::string directory;

	const auto XDG_CONFIG_HOME = getenv("XDG_CONFIG_HOME");
	if (XDG_CONFIG_HOME == nullptr)
	{
		directory = std::string{ getenv("HOME") } + "/.config/JoyShockMapper/";
	}
	else
	{
		directory = std::string{ XDG_CONFIG_HOME } + "/JoyShockMapper/";
	}

	::mkdir(directory.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	directory += "GyroConfigs/";
	::mkdir(directory.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	return strdup(directory.c_str());
}();

const char *BASE_JSM_CONFIG_FOLDER = []{
	std::string directory;

	const auto XDG_CONFIG_HOME = getenv("XDG_CONFIG_HOME");
	if (XDG_CONFIG_HOME == nullptr)
	{
		directory = std::string{ getenv("HOME") } + "/.config/JoyShockMapper/";
	}
	else
	{
		directory = std::string{ XDG_CONFIG_HOME } + "/JoyShockMapper/";
	}

	::mkdir(directory.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	return strdup(directory.c_str());
}();

unsigned long GetCurrentProcessId()
{
    return ::getpid();
}
