#include "ProcessRedirect.h"
#include "CreatePipeEx.h"

BOOL CreatePipeHandles(_Out_ HANDLE* read, _Out_ HANDLE* write);
BOOL StartProcess(_Inout_ LPWSTR commandLine, _In_ HANDLE pipe_out_write);

typedef struct ioctx {
	OVERLAPPED	overlapped;
	char		buffer[4096];
	LPVOID		userContext;
} PROC_CTX;

BOOL ProcRedirectStart(
	_Inout_ LPWSTR				commandLine,
	_In_	OnProcStdoutWritten	onWritten,
	_In_	LPVOID				context)
{
	PROC_CTX* myCtx = (PROC_CTX*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PROC_CTX));
	if (myCtx == NULL)
	{
		return FALSE;
	}

	myCtx->userContext = context;

	HANDLE pipe_out_write;

	if (!CreatePipeHandles(&m_pipe_out_read, &pipe_out_write)) {
		return FALSE;
	}
	if (!StartProcess(commandLine, pipe_out_write)) {
		return FALSE;
	}
	//
	// 2017-12-30 Spindler
	// Importante!
	//	AFTER creating the process and attaching the "pipe_out_write" pipe to it...
	//	close the handle to the pipe here.
	//	My guess: the write handle has now ben passed to the CreateProcess()
	//  We close it right afterwards to get an
	//	ERROR_BROKEN_PIPE WHEN then process has finished writing to it
	//
	//	If we didn't close it here the pipe will not report the "broken_pipe" error
	//	and therefore we will never notice the end of the writing
	CloseHandle(pipe_out_write);

	BOOL rc = ReadFileEx(
		m_pipe_out_read,
		m_readBuffer,
		sizeof(m_readBuffer),
		&(this->m_ioctx.overlapped),
		ReadExFinished);

	printf("FIRST called ReadFileEx. rc=%s, LastErr=%d\n", rc ? "true" : "false", GetLastError());

	return rc;

}

VOID CALLBACK ReadExFinished(
	_In_    DWORD        dwErrorCode,
	_In_    DWORD        dwNumberOfBytesTransfered,
	_Inout_ LPOVERLAPPED lpOverlapped)
{
	PROC_CTX* procCtx = (PROC_CTX*)lpOverlapped;

	if (dwNumberOfBytesTransfered > 0) {
		self->m_buf->append(self->m_readBuffer, dwNumberOfBytesTransfered);
	}

	if (dwErrorCode == ERROR_BROKEN_PIPE) {
		auto[data, len] = self->m_buf->data();
		self->m_onProcCompleted(data, len, self->m_onCompletedContext);
	}
	else {
		BOOL rc = ReadFileEx(
			self->m_pipe_out_read, self->m_readBuffer, sizeof(self->m_readBuffer),
			&(self->m_ioctx.overlapped), ReadExFinished);
	}
}

BOOL StartProcessRedirectOutErrToHandle(_Inout_ LPWSTR commandline, _In_ HANDLE outHandle) {
	PROCESS_INFORMATION piProcInfo;
	STARTUPINFO siStartInfo;

	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = outHandle;
	siStartInfo.hStdOutput = outHandle;
	siStartInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	BOOL rc =
		CreateProcessW(
			NULL,
			commandline,			// command line 
			NULL,					// process security attributes 
			NULL,					// primary thread security attributes 
			TRUE,					// handles are inherited 
			0,						// creation flags 
			NULL,					// use parent's environment 
			NULL,					// use parent's current directory 
			&siStartInfo,			// STARTUPINFO pointer 
			&piProcInfo);			// receives PROCESS_INFORMATION 

	if (rc) {
		CloseHandle(piProcInfo.hProcess);
		CloseHandle(piProcInfo.hThread);
	}

	return rc;
}
BOOL CreatePipeHandles(_Out_ HANDLE* read, _Out_ HANDLE* write)
{
	SECURITY_ATTRIBUTES saAttr;

	// Set the bInheritHandle flag so pipe handles are inherited. 

	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	/*
	// Create a pipe for the child process's STDOUT.
	if (!CreatePipe(read, write, &saAttr, 0)) {
	return FALSE;
	}
	*/
	if (!CreatePipeEx(read, write, &saAttr, 1024, FILE_FLAG_OVERLAPPED, 0)) {
		return FALSE;
	}

	// Ensure the read handle to the pipe for STDOUT is not inherited.
	if (!SetHandleInformation(*read, HANDLE_FLAG_INHERIT, 0)) {
		return FALSE;
	}

	return TRUE;
}
