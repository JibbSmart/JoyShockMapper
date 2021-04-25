// Source: https://stackoverflow.com/questions/1011339/how-do-you-make-a-http-request-with-c
#include "Whitelister.h"

#include <Windows.h>
#include <WinSock2.h>
#include <iostream>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")

constexpr uint16_t HID_GUARDIAN_PORT = 26762;

// Keep windows types outside of h file
static SOCKET connectToServer(const string& szServerName, WORD portNum)
{
	struct hostent* hp;
	unsigned int addr;
	struct sockaddr_in server;
	SOCKET conn;

	conn = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (conn == INVALID_SOCKET)
		return NULL;

	if (inet_addr(szServerName.c_str()) == INADDR_NONE)
	{
		hp = gethostbyname(szServerName.c_str());
	}
	else
	{
		addr = inet_addr(szServerName.c_str());
		hp = gethostbyaddr((char*)&addr, sizeof(addr), AF_INET);
	}

	if (hp == NULL)
	{
		closesocket(conn);
		return NULL;
	}

	server.sin_addr.s_addr = *((unsigned long*)hp->h_addr);
	server.sin_family = AF_INET;
	server.sin_port = htons(portNum);
	if (connect(conn, (struct sockaddr*)&server, sizeof(server)))
	{
		closesocket(conn);
		return NULL;
	}
	return conn;
}

class HidGuardianWhitelister : public Whitelister
{
public:
	HidGuardianWhitelister(bool add = false)
		: Whitelister(add)
	{
		if (add)
		{
			Add();
		}
	}

	~HidGuardianWhitelister()
	{
		Remove();
	}

	bool IsAvailable() override
	{
		// Source: https://stackoverflow.com/questions/7808085/how-to-get-the-status-of-a-service-programmatically-running-stopped
		SC_HANDLE theService, scm;
		SERVICE_STATUS_PROCESS ssStatus;
		DWORD dwBytesNeeded;

		scm = OpenSCManagerA(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);
		if (!scm)
		{
			return 0;
		}

		theService = OpenService(scm, L"HidCerberus.Srv", SERVICE_QUERY_STATUS);
		if (!theService)
		{
			CloseServiceHandle(scm);
			return 0;
		}

		auto result = QueryServiceStatusEx(theService, SC_STATUS_PROCESS_INFO,
			reinterpret_cast<LPBYTE>(&ssStatus), sizeof(SERVICE_STATUS_PROCESS),
			&dwBytesNeeded);

		CloseServiceHandle(theService);
		CloseServiceHandle(scm);

		return result == TRUE && ssStatus.dwCurrentState == SERVICE_RUNNING;
	}

	bool ShowConsole() override
	{
		std::cout << "Your PID is " << GetCurrentProcessId() << endl
		          << "Open HIDCerberus at the following adress in your browser:" << endl
			      << "http://localhost:26762/" << endl;
		return true;
		// SECURE CODING! https://www.oreilly.com/library/view/secure-programming-cookbook/0596003943/ch01s08.html
		//STARTUPINFOA startupInfo;
		//PROCESS_INFORMATION procInfo;
		//memset(&startupInfo, 0, sizeof(STARTUPINFOA));
		//memset(&procInfo, 0, sizeof(PROCESS_INFORMATION));
		//auto pid = GetCurrentProcessId();
		//auto success = CreateProcessA(NULL, R"(cmd /C "start http://localhost:26762/")", NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &startupInfo, &procInfo);
		//if (success == TRUE)
		//{
		//	CloseHandle(procInfo.hProcess);
		//	CloseHandle(procInfo.hThread);
		//	return true;
		//}
		//auto err = GetLastError();
		//return false;
	}

	virtual bool Add(string* optErrMsg = nullptr) override
	{
		if (!_whitelisted && IsAvailable())
		{
			UINT64 pid = GetCurrentProcessId();
			stringstream ss;
			ss << R"(http://localhost/api/v1/hidguardian/whitelist/add/)" << pid;
			auto result = SendToHIDGuardian(ss.str());

			if (result.compare(R"(["OK"])") == 0)
			{
				_whitelisted = true;
				return true;
			}

			if (optErrMsg)
				*optErrMsg = result;
		}
		return false;
	}

	virtual bool Remove(string* optErrMsg = nullptr) override
	{
		if (_whitelisted && IsAvailable())
		{
			UINT64 pid = GetCurrentProcessId();
			stringstream ss;
			ss << R"(http://localhost/api/v1/hidguardian/whitelist/remove/)" << pid;
			auto result = SendToHIDGuardian(ss.str());
			if (result.compare(R"(["OK"])") == 0)
			{
				_whitelisted = false;
				return true;
			}

			if (optErrMsg)
				*optErrMsg = result;
		}
		return false;
	}

private:
	string SendToHIDGuardian(string command);
	string readUrl2(string& szUrl, long& bytesReturnedOut, string* headerOut);
	void mParseUrl(string mUrl, string& serverName, string& filepath, string& filename);
	int getHeaderLength(char* content);
};

string HidGuardianWhitelister::SendToHIDGuardian(string command)
{
	long fileSize = -1;
	WSADATA wsaData;
	string memBuffer, headerBuffer;

	if (WSAStartup(0x101, &wsaData) == 0)
	{
		memBuffer = readUrl2(command, fileSize, &headerBuffer);

		WSACleanup();
	}
	return memBuffer;
}

string HidGuardianWhitelister::readUrl2(string& szUrl, long& bytesReturnedOut, string* headerOut)
{
	constexpr size_t bufSize = 512;
	string readBuffer(bufSize, '\0');
	string sendBuffer(bufSize, '\0');
	stringstream tmpBuffer;
	string result;
	SOCKET conn;
	string server, filepath, filename;
	long totalBytesRead, thisReadSize, headerLen;

	mParseUrl(szUrl, server, filepath, filename);

	///////////// step 1, connect //////////////////////
	conn = connectToServer(server, HID_GUARDIAN_PORT);

	///////////// step 2, send GET request /////////////
	tmpBuffer << "GET " << filepath << " HTTP/1.0"
		<< "\r\n"
		<< "Host: " << server << "\r\n\r\n";
	sendBuffer = tmpBuffer.str();
	send(conn, sendBuffer.c_str(), sendBuffer.length(), 0);

	///////////// step 3 - get received bytes ////////////////
	// Receive until the peer closes the connection
	totalBytesRead = 0;
	do
	{
		thisReadSize = recv(conn, &readBuffer[0], readBuffer.size(), 0);

		if (thisReadSize > 0)
		{
			result.append(readBuffer);
			totalBytesRead += thisReadSize;
		}
	} while (thisReadSize > 0);

	headerLen = getHeaderLength(&result[0]);
	if (headerOut)
	{
		*headerOut = result.substr(0, headerLen);
	}
	result.erase(0, headerLen);
	result.resize(strlen(result.c_str()));

	bytesReturnedOut = totalBytesRead - headerLen;
	closesocket(conn);
	return result;
}

void HidGuardianWhitelister::mParseUrl(string url, string& serverName, string& filepath, string& filename)
{
	string::size_type n;

	if (url.substr(0, 7) == "http://")
		url.erase(0, 7);

	if (url.substr(0, 8) == "https://")
		url.erase(0, 8);

	n = url.find('/');

	if (n != string::npos)
	{
		serverName = url.substr(0, n);
		filepath = url.substr(n);
		n = filepath.rfind('/');
		filename = filepath.substr(n + 1);
	}
	else
	{
		serverName = url;
		filepath = "/";
		filename = "";
	}
}

int HidGuardianWhitelister::getHeaderLength(char* content)
{
	const char* srchStr1 = "\r\n\r\n", * srchStr2 = "\n\r\n\r";
	char* findPos;
	int ofset = -1;

	findPos = strstr(content, srchStr1);
	if (findPos != NULL)
	{
		ofset = findPos - content;
		ofset += strlen(srchStr1);
	}

	else
	{
		findPos = strstr(content, srchStr2);
		if (findPos != NULL)
		{
			ofset = findPos - content;
			ofset += strlen(srchStr2);
		}
	}
	return ofset;
}
