#include "ProcessRedirect.h"
#include "CreatePipeEx.h"

BOOL CreatePipeHandles(_Out_ HANDLE* read, _Out_ HANDLE* write);
BOOL StartProcess(_In_ LPCWSTR exe, _Inout_ LPWSTR params, _In_ HANDLE pipe_out_write);

ProcessRedirect::ProcessRedirect()
{
	m_pipe_out_read = NULL;
	//m_pipe_out_write = NULL;
	m_ioctx.instance = this;
	m_ioctx.overlapped = { 0 };
}

ProcessRedirect::~ProcessRedirect()
{
	if (m_pipe_out_read != NULL) {
		CloseHandle(m_pipe_out_read);
	}
	//if (m_pipe_out_write != NULL) {
	//	CloseHandle(m_pipe_out_write);
	//}
}
VOID CALLBACK ReadExFinished(
	_In_    DWORD        dwErrorCode,
	_In_    DWORD        dwNumberOfBytesTransfered,
	_Inout_ LPOVERLAPPED lpOverlapped)
{
#ifdef _DEBUG
	printf("ReadExFinished: Err=%d, Bytes=%d\n", dwErrorCode, dwNumberOfBytesTransfered);
#endif // DEBUG

	ProcessRedirect* self = ((ProcessRedirect::ioctx*)lpOverlapped)->instance;

	BOOL rc = ReadFileEx(
		self->m_pipe_out_read, self->m_readBuffer, sizeof(self->m_readBuffer),
		&(self->m_ioctx.overlapped), ReadExFinished);

	DWORD ReadFileExLastErr = GetLastError();
	printf("ReadFileEx returned. rc=%s, LastErr=%d\n", rc ? "true" : "false", ReadFileExLastErr);

	if (ReadFileExLastErr == ERROR_BROKEN_PIPE) {
		self->m_onProcCompleted(self->m_buf.data(), self->m_buf.size());
	}
}
BOOL ProcessRedirect::Start(_In_ LPCWSTR exe, _Inout_ LPWSTR params, _In_ ProcCompleted onProcCompleted)
{
	m_onProcCompleted = onProcCompleted;

	HANDLE pipe_out_write;

	if (!CreatePipeHandles(&m_pipe_out_read, &pipe_out_write)) {
		return FALSE;
	}
	if (!StartProcess(exe, params, pipe_out_write)) {
		return FALSE;
	}
	//
	// 2017-12-30 Spindler
	// Importante!
	//	Close the WRITE side of the pipe here.
	//	My guess: the write handle has now ben passed to the CreateProcess()
	//  We close it right afterwards to get an
	//	ERROR_BROKEN_PIPE WHEN then process has finished writing to it
	//
	//	If we didn't close it here the pipe will not report the "broken_pipe" error
	//	and therefore we will never notice the end of the writing
	CloseHandle(pipe_out_write);

	BOOL rc = ReadFileEx(
		m_pipe_out_read, m_readBuffer, sizeof(m_readBuffer),
		&(this->m_ioctx.overlapped), ReadExFinished);

	printf("FIRST called ReadFileEx. rc=%s, LastErr=%d\n", rc ? "true" : "false", GetLastError());

	if (rc == FALSE) {
		return FALSE;
	}

	return TRUE;
}

BOOL StartProcess(_In_ LPCWSTR exe, _Inout_ LPWSTR params, _In_ HANDLE pipe_out_write) {
	PROCESS_INFORMATION piProcInfo;
	STARTUPINFO siStartInfo;

	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = pipe_out_write;
	siStartInfo.hStdOutput = pipe_out_write;
	siStartInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	WCHAR expandedExe[1024];
	ExpandEnvironmentStringsW(exe, expandedExe, sizeof(expandedExe) - 1);

	return
		CreateProcess(
			expandedExe,
			params,     // command line 
			NULL,          // process security attributes 
			NULL,          // primary thread security attributes 
			TRUE,          // handles are inherited 
			0,             // creation flags 
			NULL,          // use parent's environment 
			NULL,          // use parent's current directory 
			&siStartInfo,  // STARTUPINFO pointer 
			&piProcInfo);   // receives PROCESS_INFORMATION 
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
