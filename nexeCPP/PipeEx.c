/******************************************************************************\
*       This is a part of the Microsoft Source Code Samples. 
*       Copyright 1995 - 1997 Microsoft Corporation.
*       All rights reserved. 
*       This source code is only intended as a supplement to 
*       Microsoft Development Tools and/or WinHelp documentation.
*       See these sources for detailed information regarding the 
*       Microsoft samples programs.
\******************************************************************************/

/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    pipeex.c

Abstract:

    CreatePipe-like function that lets one or both handles be overlapped

Author:

    Dave Hart  Summer 1997

Revision History:

--*/

#include <windows.h>
//#include <stdio.h>
//#include <strsafe.h>

volatile ULONG g_PipeSerialNumber;

/*++

Routine Description:

The CreatePipeEx API is used to create an anonymous pipe I/O device.
Unlike CreatePipe FILE_FLAG_OVERLAPPED may be specified for one or
both handles.
Two handles to the device are created.  One handle is opened for
reading and the other is opened for writing.  These handles may be
used in subsequent calls to ReadFile and WriteFile to transmit data
through the pipe.

Arguments:

lpReadPipe - Returns a handle to the read side of the pipe.  Data
may be read from the pipe by specifying this handle value in a
subsequent call to ReadFile.

lpWritePipe - Returns a handle to the write side of the pipe.  Data
may be written to the pipe by specifying this handle value in a
subsequent call to WriteFile.

lpPipeAttributes - An optional parameter that may be used to specify
the attributes of the new pipe.  If the parameter is not
specified, then the pipe is created without a security
descriptor, and the resulting handles are not inherited on
process creation.  Otherwise, the optional security attributes
are used on the pipe, and the inherit handles flag effects both
pipe handles.

nSize - Supplies the requested buffer size for the pipe.  This is
only a suggestion and is used by the operating system to
calculate an appropriate buffering mechanism.  A value of zero
indicates that the system is to choose the default buffering
scheme.

Return Value:

TRUE - The operation was successful.

FALSE/NULL - The operation failed. Extended error status is available
using GetLastError.

--*/

_Success_(return)
BOOL
APIENTRY
CreatePipeEx(
    _Out_ LPHANDLE lpReadPipe,
    _Out_ LPHANDLE lpWritePipe,
    _In_  LPSECURITY_ATTRIBUTES lpPipeAttributes,
    _In_  DWORD nSize,
    _In_  DWORD dwReadMode,
    _In_  DWORD dwWriteMode
    )
{
    //
    // Only one valid OpenMode flag - FILE_FLAG_OVERLAPPED
    //

    if ((dwReadMode | dwWriteMode) & (~FILE_FLAG_OVERLAPPED)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (nSize == 0) {
        nSize = 4096;
    }

	WCHAR PipeNameBuffer[50];
    //sprintf_s( PipeNameBuffer, sizeof(PipeNameBuffer),
	StringCbPrintfW(PipeNameBuffer, sizeof(PipeNameBuffer),
             L"\\\\.\\Pipe\\nexe.%08x.%08x",
             GetCurrentProcessId(),
             InterlockedIncrement(&g_PipeSerialNumber)
           );

    *lpReadPipe = CreateNamedPipeW(
                         PipeNameBuffer,
                         PIPE_ACCESS_INBOUND | dwReadMode,
                         PIPE_TYPE_BYTE | PIPE_WAIT,
                         1,             // Number of pipes
                         nSize,         // Out buffer size
                         nSize,         // In buffer size
                         120 * 1000,    // Timeout in ms
                         lpPipeAttributes
                         );

    if (*lpReadPipe == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    *lpWritePipe = CreateFileW(
                        PipeNameBuffer,
                        GENERIC_WRITE,
                        0,                         // No sharing
                        lpPipeAttributes,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL | dwWriteMode,
                        NULL                       // Template file
                      );

    if (INVALID_HANDLE_VALUE == *lpWritePipe) {
		DWORD dwError = GetLastError();
        CloseHandle( *lpReadPipe );
        SetLastError(dwError);
        return FALSE;
    }

    return( TRUE );
}
