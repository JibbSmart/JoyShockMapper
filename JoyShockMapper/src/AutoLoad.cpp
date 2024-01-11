#include "AutoLoad.h"

// https://stackoverflow.com/a/4119881/1130520 gives us case insensitive equality
static bool iequals(const string& a, const string& b)
{
	return equal(a.begin(), a.end(),
	  b.begin(), b.end(),
	  [](char a, char b)
	  {
		  return tolower(a) == tolower(b);
	  });
}
namespace JSM
{

AutoLoad::AutoLoad(CmdRegistry* commandRegistry, bool start)
  : PollingThread("AutoLoad thread", std::bind(&AutoLoad::AutoLoadPoll, this, std::placeholders::_1), (void*)commandRegistry, 1000, start)
{
}

bool AutoLoad::AutoLoadPoll(void* param)
{
	auto registry = reinterpret_cast<CmdRegistry*>(param);
	static string lastModuleName;
	string windowTitle, windowModule;
	tie(windowModule, windowTitle) = GetActiveWindowName();
	if (!windowModule.empty() && windowModule != lastModuleName && windowModule.compare("JoyShockMapper.exe") != 0)
	{
		lastModuleName = windowModule;
		string path(AUTOLOAD_FOLDER());
		auto files = ListDirectory(path);
		auto noextmodule = windowModule.substr(0, windowModule.find_first_of('.'));
		COUT_INFO << "[AUTOLOAD] \"" << windowTitle << "\" in focus: "; // looking for config : " , );
		bool success = false;
		for (auto file : files)
		{
			auto noextconfig = file.substr(0, file.find_first_of('.'));
			if (iequals(noextconfig, noextmodule))
			{
				COUT_INFO << "loading \"AutoLoad\\" << noextconfig << ".txt\"." << endl;
				WriteToConsole(path + file);
				success = true;
				break;
			}
		}
		if (!success)
		{
			COUT_INFO << "create ";
			COUT << "AutoLoad\\" << noextmodule << ".txt";
			COUT_INFO << " to autoload for this application." << endl;
		}
	}
	return true;
}

} // JSM