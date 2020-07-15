#include <cstdlib>

#include "InputHelpers.h"

const char *AUTOLOAD_FOLDER = []{
	return _strdup((GetCWD() + "\\Autoload\\").c_str());
}();

const char *GYRO_CONFIGS_FOLDER = []{
	return _strdup((GetCWD() + "\\GyroConfigs\\").c_str());
}();

const char *BASE_JSM_CONFIG_FOLDER = []{
    return _strdup((GetCWD() + "\\").c_str());
}();
