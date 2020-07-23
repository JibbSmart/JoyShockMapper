#include <algorithm>
#include <string>
#include <cstdio>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>

#include "InputHelpers.h"

static void terminateHandler(int signo)
{
	if (signo != SIGTERM && signo != SIGINT)
	{
		std::fprintf(stderr, "Caught unexpected signal %d\n", signo);
		return;
	}

	WriteToConsole("QUIT");
}

static const int initialize = [] {
	std::string appRootDir{};

	// Check if we're running as an AppImage, and if so set up the root directory
	const auto APPDIR = ::getenv("APPDIR");
	if (APPDIR != nullptr)
	{
		const auto userId = ::getuid();
		std::printf("\n\033[1;33mRunning as AppImage, make sure user %d has RW permissions to /dev/uinput and /dev/hidraw*\n\033[0m", userId);
		appRootDir = APPDIR;
	}

	// Create the configuration directories for JSM
	std::string configDirectory;

	// Get the default config path, or if not defined set the default to $HOME/.config
	const auto XDG_CONFIG_HOME = getenv("XDG_CONFIG_HOME");
	if (XDG_CONFIG_HOME == nullptr)
	{
		configDirectory = std::string{ getenv("HOME") } + "/.config";
		::mkdir(configDirectory.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	}
	else
	{
		configDirectory = XDG_CONFIG_HOME;
	}

	// Create the base configuration directory if it doesn't already exist
	::mkdir(configDirectory.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	configDirectory = configDirectory + "/JoyShockMapper/";
	::mkdir(configDirectory.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	// Create the AutoLoad directory
	::mkdir((configDirectory + "AutoLoad/").c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	// Create the GyroConfigs directory
	configDirectory += "GyroConfigs/";
	::mkdir(configDirectory.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	const auto globalConfigDirectory = appRootDir + "/etc/JoyShockMapper/GyroConfigs/";

	// Copy the configuration files from etc to the local configuration, without overwritting
	const auto globalConfigFiles = ListDirectory(globalConfigDirectory);
	const auto localConfigFiles = ListDirectory(configDirectory);

	for (const auto &gFile : globalConfigFiles)
	{
		const auto it = std::find(localConfigFiles.begin(), localConfigFiles.end(), gFile);
		if (it == localConfigFiles.end())
		{
			auto source = ::open((globalConfigDirectory + gFile).c_str(), O_RDONLY, 0);
			auto dest = ::open((configDirectory + gFile).c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);

			struct stat stat_source;
			::fstat(source, &stat_source);
			::sendfile(dest, source, 0, stat_source.st_size);

			::close(source);
			::close(dest);
		}
	}

	signal(SIGTERM, &terminateHandler);
	signal(SIGINT, &terminateHandler);

	return 0;
}();
