#pragma once

#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

	BOOL
		APIENTRY
		CreatePipeEx(
			_Out_ LPHANDLE lpReadPipe,
			_Out_ LPHANDLE lpWritePipe,
			_In_  LPSECURITY_ATTRIBUTES lpPipeAttributes,
			_In_  DWORD nSize,
			_In_  DWORD dwReadMode,
			_In_  DWORD dwWriteMode
		);

#ifdef __cplusplus
}
#endif
