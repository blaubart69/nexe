#include "NameResolver.h"

#include <winsock2.h>
#include <nspapi.h>
#include <svcguid.h>

#include <stdio.h>

typedef struct _myIoCtlContext {
	WSAOVERLAPPED	overlapped;
	WSAQUERYSET		wsaqueryset;
	LPVOID			context;
} myIoCtlContext;

void CALLBACK IoCtlCallback(
	IN DWORD dwError,
	IN DWORD cbTransferred,
	IN LPWSAOVERLAPPED lpOverlapped,
	IN DWORD dwFlags
) 
{
	WSAQUERYSET*	 pWsaQuerySet = &(((myIoCtlContext*)lpOverlapped)->wsaqueryset);
	
	printf("WSANSPIoctl hit!\n");
}

BOOL StartNameQuery(LPWSTR Hostname, ResolveCompleted onResolved, LPVOID userContext)
{
	INT              rc = 1;
	GUID             Guid = SVCID_INET_HOSTADDRBYNAME;
	myIoCtlContext*	 pmyCtlCtx;
	WSAQUERYSET*	 pWsaQuerySet = NULL;
	//DWORD            dwWsaQuerySetSize = 4096;
	HANDLE           hLookup = NULL;

	if ((pmyCtlCtx = (myIoCtlContext*)HeapAlloc(GetProcessHeap()
		, HEAP_ZERO_MEMORY
		, sizeof(myIoCtlContext))) == NULL) {
		WSASetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}
	//
	//
	//

	AFPROTOCOLS    afproto[2] = { { AF_INET, IPPROTO_UDP },
								  { AF_INET, IPPROTO_TCP } };

	pWsaQuerySet = &(pmyCtlCtx->wsaqueryset);
	pWsaQuerySet->dwSize = sizeof(WSAQUERYSET);
	pWsaQuerySet->lpszServiceInstanceName = Hostname;
	pWsaQuerySet->lpServiceClassId = &Guid;
	pWsaQuerySet->dwNumberOfProtocols = 2;
	pWsaQuerySet->lpafpProtocols = afproto;
	pWsaQuerySet->dwNameSpace = NS_PNRPNAME;
	//pWsaQuerySet->dwNameSpace = NS_DNS;


	if (WSALookupServiceBegin(pWsaQuerySet, LUP_RETURN_ADDR, &hLookup) != 0) {
	//if (WSALookupServiceBegin(pWsaQuerySet, LUP_RETURN_BLOB | LUP_RETURN_NAME, &hLookup) != 0) {
		printf("E-WSALookupServiceBegin: LastErr: %d\n", WSAGetLastError());
		return FALSE;
	}
	//
	//
	//
	pmyCtlCtx->context = userContext;
	//
	//
	//
	WSACOMPLETION wsaCompletionStruct;
	//wsaCompletionStruct.Type							  = NSP_NOTIFY_APC;
	wsaCompletionStruct.Type = NSP_NOTIFY_IMMEDIATELY;
	wsaCompletionStruct.Parameters.Apc.lpOverlapped		  = &(pmyCtlCtx->overlapped);
	wsaCompletionStruct.Parameters.Apc.lpfnCompletionProc = IoCtlCallback;

	DWORD cbBytesReturned;
	int ioctlRc = WSANSPIoctl(
			hLookup
		,	SIO_NSP_NOTIFY_CHANGE
		,	NULL
		,	0
		,	NULL
		,	0
		,	&cbBytesReturned
		,	&wsaCompletionStruct);
	if (! (ioctlRc == NO_ERROR || ioctlRc == WSA_IO_PENDING) ) {
		printf("E-WSANSPIoctl: rc=%d LastErr=%d\n", ioctlRc, WSAGetLastError());
		return FALSE;
	}

	return TRUE;
}
