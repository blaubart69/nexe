#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <winsock2.h>

#include <wchar.h>
#include <stdio.h>


#include "ProcessRedirect.h"
#include "NameResolver.h"

int TestProcessRedirect(void) {
	ProcessRedirect pr;

	WCHAR commandLine[] = L"%comspec% /c dir c:\\";
	//WCHAR params[] = L"/c";
	if (!pr.Start(commandLine,
		"localhost",
		[](const char* buf, size_t length, LPVOID context) {
		printf("CALLBACK! size=%zd\n", length);
	},
		NULL)) {
		printf("could not start process. rc=%d\n", GetLastError());
		return 1;
	}

	while (SleepEx(500, TRUE) == WAIT_IO_COMPLETION) {
		printf("wait_io_compl\n");
	}
	return 0;
}
int TestNameResolution(LPWSTR Hostname)
{

	BOOL ok = StartNameQuery(
			Hostname
		,	[](in_addr ipv4, in6_addr ipv6, LPVOID context) { }
		,	NULL);
	if (!ok) {
		printf("StartNameQuery failed");
		return 1;
	}
	else {
		printf("submitted name query");
	}

	while (SleepEx(5000, TRUE) == WAIT_IO_COMPLETION) {
		printf("wait_io_compl\n");
	}
	return 0;

}
int main(int argc, char* argv[]) {

	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	/* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
	wVersionRequested = MAKEWORD(2, 2);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		/* Tell the user that we could not find a usable */
		/* Winsock DLL.                                  */
		printf("WSAStartup failed with error: %d\n", err);
		return 1;
	}

	WCHAR Hostname[] = L"beebunt";
	TestNameResolution(Hostname);

	WSACleanup();

	return 0;
}

/*
#define FILE_ATTRIBUTE_DIRECTORY            0x00000010
typedef unsigned long       DWORD;

//-------------------------------------------------------------------------------------------------
bool IsDotDir(const wchar_t cFileName[], const DWORD dwFileAttributes) {
//-------------------------------------------------------------------------------------------------

	int i;

	i = ((dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 0x10 : 0)
		+ ((cFileName[0] == L'.') ? 0x08 : 0)
		+ ((cFileName[1] == L'\0') ? 0x04 : 0)
		+ ((cFileName[1] == L'.') ? 0x02 : 0)
		+ ((cFileName[2] == L'\0') ? 0x01 : 0);

	return (i >= 0x1B) ? true : false;
}
*/