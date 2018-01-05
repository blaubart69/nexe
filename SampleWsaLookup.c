https://msdn.microsoft.com/en-us/library/windows/desktop/aa371682(v=vs.85).aspx



#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <nspapi.h>
#include <svcguid.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <wchar.h>

INT WSAHostNameToAddress( LPTSTR HostName
                        , INT AddressFamily
                        , LPWSAPROTOCOL_INFO lpProtocolInfo
                        , LPSOCKADDR lpAddress
                        , LPINT lpAddressLength ) {

    INT              rc = 1;
    GUID             Guid = SVCID_INET_HOSTADDRBYNAME;
    WSAQUERYSET      *pWsaQuerySet = NULL;
    DWORD            dwWsaQuerySetSize = 4096;
    HANDLE           hLookup = NULL;

    if ( *lpAddressLength < sizeof( SOCKADDR )) {
        WSASetLastError( ERROR_INVALID_PARAMETER );
    } else if (( pWsaQuerySet = HeapAlloc( GetProcessHeap()
                                         , HEAP_ZERO_MEMORY
                                         , dwWsaQuerySetSize )) == NULL ) {
        WSASetLastError( ERROR_NOT_ENOUGH_MEMORY );
    } else {
        pWsaQuerySet->dwSize = sizeof( WSAQUERYSET );
        pWsaQuerySet->lpszServiceInstanceName = HostName;
        pWsaQuerySet->lpServiceClassId = &Guid;
        pWsaQuerySet->dwNameSpace = NS_ALL;
        if ( WSALookupServiceBegin( pWsaQuerySet, LUP_RETURN_ADDR, &hLookup ) != 0 ) {
            hLookup = NULL;
        } else if ( WSALookupServiceNext( hLookup, 0, &dwWsaQuerySetSize, pWsaQuerySet ) != 0 ) {
            ;
        } else if ( pWsaQuerySet->dwNumberOfCsAddrs < 1 ) {
            WSASetLastError( ERROR_NO_DATA );
        } else {
            CopyMemory( lpAddress, pWsaQuerySet->lpcsaBuffer[ 0 ].RemoteAddr.lpSockaddr, sizeof( SOCKADDR ));
            lpAddress->sa_family = (u_short)AddressFamily;
            rc = 0;
        } /* endif */
        if ( hLookup != NULL ) {
            DWORD dwError = WSAGetLastError();
            WSALookupServiceEnd( hLookup );
            if ( dwError ) WSASetLastError( dwError );
        } /* endif */
    } /* endif */

    if ( pWsaQuerySet != NULL ) HeapFree( GetProcessHeap(), 0, pWsaQuerySet );

   return rc;
}



INT _tmain( INT argc, LPTSTR argv[] ) {

  WSADATA WsaData;
  SOCKADDR_IN SockAddrIn;
  DWORD dw = sizeof( SockAddrIn );
  TCHAR IPString[ 16 ];

  WsaData.wVersion = 0;

  if( argc != 2 ) {
    _tprintf( TEXT( "USAGE: %s hostname\n" ), argv[ 0 ] );
  } else if ( WSAStartup( MAKEWORD( 2, 2 ), &WsaData ) != 0 ) {
    WsaData.wVersion = 0;
    _tprintf( TEXT( "ERROR: WSAStartup failed: %d\n" ), GetLastError());
  } else if ( WSAHostNameToAddress( argv[ 1 ], AF_INET, NULL, (PSOCKADDR)&( SockAddrIn ), &dw ) != 0 ) {
    _tprintf( TEXT( "ERROR: WSAHostNameToAddress(%s) failed: %d\n" ), argv[ 1 ], WSAGetLastError());
  } else {
    dw = sizeof( IPString );
    WSAAddressToString( (PSOCKADDR)&( SockAddrIn ), sizeof( SockAddrIn ), NULL, IPString, &dw );
    _tprintf( TEXT( "INFO: %s is %s\n" ), argv[ 1 ], IPString );
  } /* endif */

  if ( WsaData.wVersion != 0 ) WSACleanup();
  
  return 0;

}

