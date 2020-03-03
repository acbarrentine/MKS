#include <PCH.h>
#include <filesystem>

std::string GetLastErrorString() {
	DWORD err = GetLastError();

	char* msg_buf;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(char*)&msg_buf,
		0,
		NULL);
	std::string msg = msg_buf;
	LocalFree(msg_buf);
	return msg;
}

void Win32Fatal(const char* function)
{
	std::cerr << function << ": " << GetLastErrorString() << "\n";
}

int SpawnProcess(std::string command, std::string& output)
{
	SECURITY_ATTRIBUTES security_attributes = {};
	security_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	security_attributes.bInheritHandle = TRUE;

	// Must be inheritable so subprocesses can dup to children.
	HANDLE nul =
		CreateFileA("NUL", GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			&security_attributes, OPEN_EXISTING, 0, NULL);
	if (nul == INVALID_HANDLE_VALUE)
	{
		std::cout << "couldn't open nul\n";
		return -1;
	}

	HANDLE stdout_read, stdout_write;
	if (!CreatePipe(&stdout_read, &stdout_write, &security_attributes, 0))
	{
		//Win32Fatal("CreatePipe");
		return -1;
	}

	if (!SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0))
	{
		//Win32Fatal("SetHandleInformation");
		return -1;
	}

	PROCESS_INFORMATION process_info = {};
	STARTUPINFOA startup_info = {};
	startup_info.cb = sizeof(STARTUPINFOA);
	startup_info.hStdInput = nul;
	startup_info.hStdError = ::GetStdHandle(STD_ERROR_HANDLE);
	startup_info.hStdOutput = stdout_write;
	startup_info.dwFlags |= STARTF_USESTDHANDLES;

	//char commandLineBuf[16 * 1024];
	//sprintf_s(commandLineBuf, "\"%s\" %s", command.c_str(), args.c_str());

	if (!CreateProcessA(nullptr, command.data(), NULL, NULL,
		/* inherit handles */ TRUE, 0,
		nullptr, NULL,
		&startup_info, &process_info))
	{
		Win32Fatal("CreateProcess");
		return -1;
	}

	if (!CloseHandle(nul) ||
		!CloseHandle(stdout_write))
	{
		Win32Fatal("CloseHandle");
		return -1;
	}

	// Read all output of the subprocess.
	DWORD read_len = 1;
	char buf[64 << 10];
	while (read_len) {
		read_len = 0;
		if (!::ReadFile(stdout_read, buf, sizeof(buf), &read_len, NULL) &&
			GetLastError() != ERROR_BROKEN_PIPE)
		{
			Win32Fatal("ReadFile");
			return -1;
		}
		output.append(buf, read_len);
	}

	// Wait for it to exit and grab its exit code.
	if (WaitForSingleObject(process_info.hProcess, INFINITE) == WAIT_FAILED)
	{
		Win32Fatal("WaitForSingleObject");
		return -1;
	}
	DWORD exit_code = 0;
	if (!GetExitCodeProcess(process_info.hProcess, &exit_code))
	{
		Win32Fatal("GetExitCodeProcess");
		return -1;
	}

	if (!CloseHandle(stdout_read) ||
		!CloseHandle(process_info.hProcess) ||
		!CloseHandle(process_info.hThread)
		)
	{
		Win32Fatal("CloseHandle");
		return -1;
	}

	return exit_code;
}
