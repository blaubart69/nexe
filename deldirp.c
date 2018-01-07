///////////////////////////////////////////////////////////////////////////////////////////////////
//
//               D E L D I R P
//
//
//  FILE:        deldirp.c
//
//  PURPOSE:     delete a directory tree in parallel
//
//
//               Copyright (c) 2018 UBIS-Austria, Vienna (AT)
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#define  UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used 
#define STRICT
#include <windows.h>
#include "common.h"
#include "delpmsg.h"

// defines

#define ___OUTPUT_DEBUG_STRING
#define NUMBER_OF_CONCURRENT 0

#define CACHE_LINE  64
#define CACHE_ALIGN __declspec(align(CACHE_LINE))

#define ERROR_FAILED_REPORT_LIMIT 10
#define MAX_U32 0xFFFFFFFFUL

// .INC files

#include <cmdline.c>
#include <message.c>

// global data

CACHE_ALIGN HANDLE        _hCompletionPort;
CACHE_ALIGN HANDLE        _hExit;
CACHE_ALIGN LONG volatile _QueuedCount;
CACHE_ALIGN LONG volatile _FailedCount;
CACHE_ALIGN LONG volatile _DeleteCount;

// new types

typedef struct _ELEMENT {
    LONG volatile lNumberOfChildren;
    struct _ELEMENT *pParent;
    DWORD dwFileAttributes;
    DWORD dwFileNameLength;
    WCHAR wcFileName[ 1 ]; // must be the last
} ELEMENT;

// function prototypes

DWORD WINAPI PoolThread( LPVOID lpParam );
PRIVATE BOOL IsDotDir( WIN32_FIND_DATA *FindBuffer );
PRIVATE VOID ProcessDirectory( ELEMENT *pElement );
PRIVATE DWORD HeapAllocated( HANDLE hHeap );
PRIVATE DWORD GetEnvOpt( LPWSTR Opt );
PRIVATE BOOL GetU32( LPWSTR s, ULONG* pResult );

// logging

#ifdef OUTPUT_DEBUG_STRING
#define LOG(n) Log n
PRIVATE VOID Log( char *msg, ... );
#else
#define LOG(n)
#endif

//=================================================================================================
VOID rawmain( VOID ) {
//=================================================================================================

    LPWSTR  *szArglist = NULL;
    INT     nArgs;
    DWORD   rc = 999, dwAttr, i, Allocated, nThreads;
    ELEMENT *pElement;
    HANDLE  *hThreads;

    _hCompletionPort = NULL;
    _hExit = NULL;

    SetErrorMode( SEM_NOOPENFILEERRORBOX );

    Allocated = HeapAllocated( GetProcessHeap());

    if (( szArglist = CommandLineToArgv2( GetCommandLine(), &nArgs )) == NULL ) {
        Message( MSGID_ERROR_WINAPI_S, TEXT( __FILE__ ), __LINE__
               , L"CommandLineToArgv2", L"", GetLastError());
        rc = 998;
    } else if ( nArgs != 2 ) {
        Message( MSGID_USAGE, TEXT( FILE_VERSION_DELDIRP ), TEXT( __DATE__ ), TEXT( COPYRIGHTTEXT ));
    } else if (( dwAttr = GetFileAttributes( szArglist[ 1 ] )) == INVALID_FILE_ATTRIBUTES ) {
        Message( MSGID_ERROR_WINAPI_S, TEXT( __FILE__ ), __LINE__
               , L"GetFileAttributes", szArglist[ 1 ], GetLastError());
        rc = 1;
    } else if (( _hExit = CreateEvent( NULL, TRUE, FALSE, NULL )) == NULL ) {
        Message( MSGID_ERROR_WINAPI_S, TEXT( __FILE__ ), __LINE__
               , L"CreateEvent", L"Manual", GetLastError());
        rc = 2;
    } else if (( _hCompletionPort = CreateIoCompletionPort( INVALID_HANDLE_VALUE
                                                          , NULL
                                                          , 0
                                                          , NUMBER_OF_CONCURRENT )) == NULL ) {
        Message( MSGID_ERROR_WINAPI_S, TEXT( __FILE__ ), __LINE__
               , L"CreateIoCompletionPort", L"NULL", GetLastError());
        rc = 3;
    } else if (( pElement = HeapAlloc( GetProcessHeap()
                                     , 0
                                     , ( i = sizeof( *pElement ) + (( lstrlen( szArglist[ 1 ] ) + 2 ) * sizeof( WCHAR ))))) == NULL ) {
        Message( MSGID_ERROR_WINAPI_U, TEXT( __FILE__ ), __LINE__
               , L"HeapAlloc", i, ERROR_NOT_ENOUGH_MEMORY );
        rc = 4;
    } else {

        nThreads = GetEnvOpt( L"DELDIRP_OPT_THREADS" );

        hThreads = _alloca( nThreads * sizeof( hThreads[ 0 ] ));

        // --- let's create a number of working threads ---

        for ( i = 0; i < nThreads; i++ ) {
            DWORD dwThreadId;
            if (( hThreads[ i ] = CreateThread( NULL
                                              , 0
                                              , PoolThread
                                              , NULL
                                              , 0
                                              , &dwThreadId )) == NULL ) {
                Message( MSGID_ERROR_WINAPI_U, TEXT( __FILE__ ), __LINE__
                       , L"CreateThread", i, GetLastError());
            } /* endif */
        } /* endfor */

        // --- prepate the root entry ---

        pElement->lNumberOfChildren = 1; // init with 1!
        pElement->pParent           = NULL;
        pElement->dwFileAttributes  = dwAttr;
        pElement->dwFileNameLength  = lstrlen( szArglist[ 1 ] );

        lstrcpy( pElement->wcFileName, szArglist[ 1 ] );

        // --- queue the root entry ---

        _QueuedCount = 1;
        _FailedCount = 0;
        _DeleteCount = 0;

        if ( ! PostQueuedCompletionStatus( _hCompletionPort, 0, (ULONG_PTR)pElement, NULL )) {
            Message( MSGID_ERROR_WINAPI_S, TEXT( __FILE__ ), __LINE__
                   , L"PostQueuedCompletionStatus", L"", GetLastError());
            _FailedCount = 1;
        } else {

            // --- wait and report performance ---

            while ( WaitForSingleObject( _hExit, 1000L ) == WAIT_TIMEOUT ) {
                Message( MSGID_STATUS, _DeleteCount, _QueuedCount, _FailedCount );
            } /* endwhile */

        } /* endif */

        // --- send exit command to all worker threads ---

        for ( i = 0; i < nThreads; i++ ) {
            _InterlockedIncrement( &_QueuedCount );
            PostQueuedCompletionStatus( _hCompletionPort, 0, 0, NULL );
        } /* endfor */

        // --- wait for all worker threads (but not more than a half second) ---

        for ( i = 0; i < nThreads; i++ ) {
            if ( hThreads[ i ] != NULL ) {
                WaitForSingleObject( hThreads[ i ], 500L );
                CloseHandle( hThreads[ i ] );
            } /* endif */
        } /* endfor */

        Message( MSGID_STATUS, _DeleteCount, _QueuedCount, _FailedCount );

        rc = ( _FailedCount > 0 ) ? 5 : 0;

    } /* endif */

    // --- cleanup ---

    if ( _hCompletionPort != NULL ) CloseHandle( _hCompletionPort );
    if ( _hExit           != NULL ) CloseHandle( _hExit );
    if ( szArglist != NULL ) HeapFree( GetProcessHeap(), 0, szArglist );

    // --- check for memory leaks ---

    Allocated = HeapAllocated( GetProcessHeap()) - Allocated; // delta

    if ( Allocated ) {
        Message( MSGID_MEMORY_LEAK, Allocated );
    } /* endif */

    // --- bye bye ---

    ExitProcess( rc );
}

//=================================================================================================
DWORD WINAPI PoolThread( LPVOID lpParam ) {
//=================================================================================================

    BOOL bSuccess;
    DWORD dwSize;
    OVERLAPPED *pOverlapped;
    ELEMENT *pElement, *pParent;

    UNREFERENCED_PARAMETER( lpParam ); 

    for (;;) {

        // --- wait for a job ---

        bSuccess = GetQueuedCompletionStatus( _hCompletionPort
                                            , &dwSize
                                            , (PULONG_PTR)&pElement
                                            , &pOverlapped
                                            , INFINITE );

        _InterlockedDecrement( &_QueuedCount );

        // --- check for termination ---

        if ( pElement == NULL ) {
            break;
        } /* endif */

        if ( pElement->dwFileAttributes & FILE_ATTRIBUTE_READONLY ) {
            SetFileAttributes( pElement->wcFileName
                             , pElement->dwFileAttributes & ~FILE_ATTRIBUTE_READONLY );
        } /* endif */

        // --- check if file or directory ---

        if ( pElement->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {

            // --- DIRECTORY ---

            ProcessDirectory( pElement );

            if ( _InterlockedDecrement( &pElement->lNumberOfChildren ) > 0 ) { // init with 1!
                continue; // remove will be done by one of the children
            } else if ( RemoveDirectory( pElement->wcFileName )) {
                //LOG(( "RD:%.100S\n", pElement->wcFileName ));
                _InterlockedIncrement( &_DeleteCount );
            } else if ( _InterlockedIncrement( &_FailedCount ) < ERROR_FAILED_REPORT_LIMIT ) {
                Message( MSGID_ERROR_WINAPI_S, TEXT( __FILE__ ), __LINE__
                       , L"RemoveDirectory", pElement->wcFileName, GetLastError());
            } /* endif */

        } else {

            // --- FILE ---

            if ( DeleteFile( pElement->wcFileName )) {
                //LOG(( "DF:.100%S\n", pElement->wcFileName ));
                _InterlockedIncrement( &_DeleteCount );
            } else if ( _InterlockedIncrement( &_FailedCount ) < ERROR_FAILED_REPORT_LIMIT ) {
                Message( MSGID_ERROR_WINAPI_S, TEXT( __FILE__ ), __LINE__
                       , L"DeleteFile", pElement->wcFileName, GetLastError());
            } /* endif */

        } /* endif */

        // --- change to parent and free this element context ---

        pParent = pElement->pParent;
        HeapFree( GetProcessHeap(), 0, pElement );

        // --- decrement children counter in parent structure ---

        for (;;) {

            if ( pParent == NULL ) {
                SetEvent( _hExit );
                break;            
            } /* endif */

            if ( _InterlockedDecrement( &(( pParent )->lNumberOfChildren )) > 0 ) {
                break;
            } /* endif */

            // --- the last child removes the parent ---

            if ( RemoveDirectory( pParent->wcFileName )) {
                //LOG(( "RD:%.100S\n", pParent->wcFileName ));
                _InterlockedIncrement( &_DeleteCount );
            } else if ( _InterlockedIncrement( &_FailedCount ) < ERROR_FAILED_REPORT_LIMIT ) {
                Message( MSGID_ERROR_WINAPI_S, TEXT( __FILE__ ), __LINE__
                       , L"RemoveDirectory", pElement->wcFileName, GetLastError());
            } /* endif */

            // --- change to grand parent and free parent context ---

            pElement = pParent;
            pParent = pElement->pParent;
            HeapFree( GetProcessHeap(), 0, pElement );

        } /* endfor */

    } /* endfor */

    return 0;
}
#if 0
//-------------------------------------------------------------------------------------------------
PRIVATE BOOL IsDotDir( WIN32_FIND_DATA *FindBuffer ) {
//-------------------------------------------------------------------------------------------------

    if (( FindBuffer->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) == 0 ) return FALSE;
    if ( FindBuffer->cFileName[ 0 ] != L'.' ) return FALSE;
    if ( FindBuffer->cFileName[ 1 ] == L'\0' ) return TRUE;
    if ( FindBuffer->cFileName[ 1 ] != L'.' ) return FALSE;
    if ( FindBuffer->cFileName[ 2 ] == L'\0' ) return TRUE;

    return FALSE;
}
#else
//-------------------------------------------------------------------------------------------------
PRIVATE BOOL IsDotDir( WIN32_FIND_DATA *FindBuffer ) {
//-------------------------------------------------------------------------------------------------

    //   4   3   2   1   0   BIT
    // +---+---+---+---+---+
    // |DIR|[0]|[1]|[1]|[2]| DATA
    // +---+---+---+---+---+
    // | 1 | 1 | 1 | 1 | X | 0x1E 0x1F ... Not possible (cFileName[ 1 ] can not be '\0' and '.')
    // | 1 | 1 | 1 | 0 | X | 0x1C 0x1D ... Directory "."
    // | 1 | 1 | 0 | 1 | 1 | 0x1B      ... Directory ".."
    // +---+---+---+---+---+
    // | 1 | 1 | 0 | 1 | 0 | 0x1A      ... Directory "..?"
    // | 1 | 1 | 0 | 0 | X | 0x18 0x19 ... Directory ".?"
    // | 1 | 0 | X | X | X | 0x10-0x1F ... Directory with first char not '.'
    // | 0 | X | X | X | X | 0x00-0x0F ... Not a Directory
    // +---+---+---+---+---+

    INT i;

    i  = (( FindBuffer->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ? 0x10 : 0 )
       + (( FindBuffer->cFileName[ 0 ] == L'.' )                      ? 0x08 : 0 )
       + (( FindBuffer->cFileName[ 1 ] == L'\0' )                     ? 0x04 : 0 )
       + (( FindBuffer->cFileName[ 1 ] == L'.' )                      ? 0x02 : 0 )
       + (( FindBuffer->cFileName[ 2 ] == L'\0' )                     ? 0x01 : 0 );

    return ( i >= 0x1B ) ? TRUE : FALSE;
}
#endif
//-------------------------------------------------------------------------------------------------
PRIVATE VOID ProcessDirectory( ELEMENT *pElement ) {
//-------------------------------------------------------------------------------------------------

    HANDLE hSearch;
    DWORD  dwError = NO_ERROR;
    WIN32_FIND_DATA FindBuffer;
    ELEMENT *pChild;

    // --- append \* to name ---

    lstrcpy( pElement->wcFileName + pElement->dwFileNameLength, L"\\*" );

    // --- looking for entries ---

    if (( hSearch = FindFirstFile( pElement->wcFileName
                                 , &FindBuffer )) == INVALID_HANDLE_VALUE ) { 
        dwError = GetLastError();
    } /* endif */

    while ( dwError != ERROR_NO_MORE_FILES ) {

        if ( dwError != NO_ERROR ) {

            Message( MSGID_ERROR_WINAPI_S, TEXT( __FILE__ ), __LINE__
                   , L"FindFirst/NextFile", pElement->wcFileName, dwError );
            break;

        } else if ( IsDotDir( &FindBuffer )) {

            ; // nothing to do

        } else {

            WCHAR *p;
            DWORD n = pElement->dwFileNameLength + lstrlen( FindBuffer.cFileName ) + 1; // + "\\"

            pChild = HeapAlloc( GetProcessHeap()
                              , 0
                              , sizeof( *pChild ) + ( n + 2 ) * sizeof( WCHAR ));       // + "\\*"

            pChild->lNumberOfChildren = 1; // init with 1!
            pChild->pParent           = pElement;
            pChild->dwFileAttributes  = FindBuffer.dwFileAttributes;
            pChild->dwFileNameLength  = n;

            p = pChild->wcFileName;
            lstrcpy( p, pElement->wcFileName );
            p += pElement->dwFileNameLength;
            *p++ = L'\\';
            lstrcpy( p, FindBuffer.cFileName );

            _InterlockedIncrement( &pElement->lNumberOfChildren );
            _InterlockedIncrement( &_QueuedCount );
            PostQueuedCompletionStatus( _hCompletionPort, 0, (ULONG_PTR)pChild, NULL );
                        
        } /* endif */

        // --- search for next entry in this directory --------------------------------------------

        if ( ! FindNextFile( hSearch
                           , &FindBuffer )) {
            dwError = GetLastError();
        } /* endif */

    } /* endwhile */

    // --- local cleanup -------------------------------------------------------------------------------------

    if ( hSearch != INVALID_HANDLE_VALUE ) FindClose( hSearch );

    // --- remove \* from directory name ---

    pElement->wcFileName[ pElement->dwFileNameLength ] = L'\0';
}

//-------------------------------------------------------------------------------------------------
PRIVATE DWORD HeapAllocated( HANDLE hHeap ) {
//-------------------------------------------------------------------------------------------------

    DWORD LastError;
    PROCESS_HEAP_ENTRY Entry;
    PROCESS_HEAP_ENTRY Last;
    DWORD Allocated = 0;

    Entry.lpData = NULL;

    if ( ! HeapLock( hHeap )) {
        Message( MSGID_ERROR_WINAPI_S, TEXT( __FILE__ ), __LINE__
               , L"HeapLock", L"", GetLastError());
    } else {

OutputDebugStringA( "\n" );

        LOG(( "WALK:0x%p\n", hHeap ));

        while ( HeapWalk( hHeap, &Entry )) {
            if ( Entry.wFlags & PROCESS_HEAP_ENTRY_BUSY ) {
                Allocated += Entry.cbData + Entry.cbOverhead;
                Last = Entry;
{
CHAR x[ 10 ]; DWORD a = Entry.cbData;
x[ 5 ] = '\0';
x[ 4 ] = "0123456789"[   a       % 10 ];
x[ 3 ] = "0123456789"[ ( a / 10 ) % 10 ];
x[ 2 ] = "0123456789"[ ( a / 100 ) % 10 ];
x[ 1 ] = "0123456789"[ ( a / 1000 ) % 10 ];
x[ 0 ] = "0123456789"[ ( a / 10000 ) % 10 ];

OutputDebugStringA( x );
}
                // LOG(( "     0x%p,%u,%u\n", Entry.lpData, Entry.cbData, Entry.cbOverhead ));
            } /* endif */
        } /* endwhile */

__debugbreak();

        //LOG(( "%10u allocated\n", Allocated ));

        if (( LastError = GetLastError()) != ERROR_NO_MORE_ITEMS ) {
            Message( MSGID_ERROR_WINAPI_S, TEXT( __FILE__ ), __LINE__
                   , L"HeapWalk", L"", LastError );
        } /* endif */

        if ( ! HeapUnlock( hHeap )) {
            Message( MSGID_ERROR_WINAPI_S, TEXT( __FILE__ ), __LINE__
                   , L"HeapUnlock", L"", GetLastError());
        } /* endif */

    } /* endif */

    return Allocated;
}

//-------------------------------------------------------------------------------------------------
PRIVATE DWORD GetEnvOpt( LPWSTR Opt ) {
//-------------------------------------------------------------------------------------------------

    DWORD n;
    WCHAR pBuffer[ 16 ];

    if (( n = GetEnvironmentVariable( Opt, pBuffer, ELEMENTS( pBuffer ))) == 0 ) {
        n = 64;
    } else if ( n >= ELEMENTS( pBuffer )) {
        n = 64;
    } else if ( ! GetU32( pBuffer, &n )) {
        n = 64;
    } else if ( n == 0 ) {
        n = 64;
    } /* endif */

    return n;
}

//-------------------------------------------------------------------------------------------------
PRIVATE BOOL GetU32( LPWSTR s, ULONG* pResult ) {
//-------------------------------------------------------------------------------------------------

    WCHAR c;
    ULONG rc = 0, Base;

    // this is a "poor man" strtoul.

    c = *s++;

    if ( c == L'\0' ) return FALSE;

    if ( c == L'0' && ( *s == L'x' || *s == L'X' )) {
        c = s[ 1 ];
        s += 2;
        Base = 16;
    } else {
        Base = ( c == L'0' ) ? 8 : 10;
    } /* endif */

    while ( c != L'\0' ) {
        if ( rc > ( MAX_U32 / Base )) return FALSE;
        else if ( c >= L'0' && c <= L'7' ) rc = rc * Base + ( c - L'0' );
        else if ( Base ==  8 ) return FALSE;
        else if ( c >= L'8' && c <= L'9' ) rc = rc * Base + ( c - L'0' );
        else if ( Base == 10 ) return FALSE;
        else if ( c >= L'A' && c <= L'F' ) rc = rc * Base + ( c - L'A' ) + 10;
        else if ( c >= L'a' && c <= L'f' ) rc = rc * Base + ( c - L'a' ) + 10;
        else return FALSE;
        c = *s++;
    } /* endfor */

    *pResult = rc;

    return TRUE;
}

//-------------------------------------------------------------------------------------------------
PRIVATE VOID Log( char *msg, ... ) {
//-------------------------------------------------------------------------------------------------

    CHAR buf[ 128 ];
    va_list vaList ;

    va_start( vaList, msg );
    wvsprintfA( buf, msg, vaList );
    va_end( vaList );

    OutputDebugStringA( buf );
}

// -=EOF=-
