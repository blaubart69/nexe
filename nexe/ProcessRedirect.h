#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <vector>

typedef void(*ProcCompleted)(char* buf, size_t size);


class ProcessRedirect
{
	struct ioctx {
		OVERLAPPED			overlapped;
		ProcessRedirect*	instance;
	} m_ioctx;

	std::vector<char> m_buf;

	HANDLE m_pipe_out_read;
	//HANDLE m_pipe_out_write;
	char m_readBuffer[4096];

	ProcCompleted m_onProcCompleted;

	friend VOID CALLBACK ReadExFinished(
		_In_    DWORD        dwErrorCode,
		_In_    DWORD        dwNumberOfBytesTransfered,
		_Inout_ LPOVERLAPPED lpOverlapped);

public:
	ProcessRedirect();
	~ProcessRedirect();
	BOOL Start(_In_ LPCWSTR exe, _In_ LPWSTR params, _In_ ProcCompleted onProcCompleted);
};
