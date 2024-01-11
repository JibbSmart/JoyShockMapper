#pragma once
#include "InputHelpers.h"

class CmdRegistry;


namespace JSM
{

class AutoLoad : public PollingThread
{
public:
	AutoLoad(CmdRegistry* commandRegistry, bool start);

	virtual ~AutoLoad() = default;

private:
	bool AutoLoadPoll(void* param);
};

} //JSM