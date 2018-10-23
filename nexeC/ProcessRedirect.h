#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

typedef void(*OnProcStdoutWritten)(const char* buf, size_t size, LPVOID context);

BOOL ProcRedirectStart(
	_Inout_ LPWSTR				commandLine,
	_In_	OnProcStdoutWritten	onWritten,
	_In_	LPVOID				context);