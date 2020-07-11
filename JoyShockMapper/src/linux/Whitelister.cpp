#include "Whitelister.h"

bool Whitelister::ShowHIDCerberus()
{
    return false;
}

bool Whitelister::IsHIDCerberusRunning()
{
    return false;
}

bool Whitelister::Add(std::string *)
{
    return false;
}

bool Whitelister::Remove(std::string *)
{
    return false;
}

std::string Whitelister::SendToHIDGuardian(std::string)
{
    return "";
}

std::string Whitelister::readUrl2(std::string &, long &, std::string *)
{
    return "";
}

void Whitelister::mParseUrl(std::string, std::string &, std::string &, std::string &)
{
}

int Whitelister::getHeaderLength(char *)
{
    return 0;
}
