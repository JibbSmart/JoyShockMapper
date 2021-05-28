#include "Whitelister.h"


class WhitelisterImpl : public Whitelister
{
public:
	// TODO: Implement this class by interacting with some external whitelisting software

	WhitelisterImpl()
	{

	}

	virtual bool ShowConsole() override
	{

	}
	virtual bool IsAvailable() override
	{

	}

	virtual bool Add(string* optErrMsg = nullptr) override
	{

	}
	virtual bool Remove(string* optErrMsg = nullptr) override
	{

	}
};

Whitelister* Whitelister::getNew(bool add)
{
	// return new WhitelisterImpl();
	return nullptr;
}

