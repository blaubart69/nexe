#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "buffer.h"

typedef void(*ProcCompleted)(const char* buf, size_t size, LPVOID context);


class ProcessRedirect
{
	struct ioctx {
		OVERLAPPED			overlapped;
		ProcessRedirect*	instance;
	} m_ioctx;

	HANDLE m_pipe_out_read;
	char m_readBuffer[4096];
	buffer* m_buf;

	ProcCompleted m_onProcCompleted;
	LPVOID m_onCompletedContext;

	friend VOID CALLBACK ReadExFinished(
		_In_    DWORD        dwErrorCode,
		_In_    DWORD        dwNumberOfBytesTransfered,
		_Inout_ LPOVERLAPPED lpOverlapped);

public:
	ProcessRedirect();
	~ProcessRedirect();
	BOOL Start(
		_In_	LPCWSTR			exe,
		_Inout_ LPWSTR			params,
		_In_	LPCSTR			Hostname,
		_In_	ProcCompleted	onProcCompleted,
		_In_	LPVOID			context);
};
