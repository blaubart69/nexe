#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <wchar.h>
#include <stdio.h>

#include "ProcessRedirect.h"

int main(int argc, char* argv[]) {

	ProcessRedirect pr;

	WCHAR params[] = L"/c dir c:\\";
	//WCHAR params[] = L"/c";
	if (!pr.Start(L"%comspec%", params,
	"localhost", 
	[](const char* buf, size_t length) {
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