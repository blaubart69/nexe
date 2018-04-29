#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <inaddr.h>
#include <in6addr.h>

typedef void (*ResolveCompleted)(in_addr ipv4, in6_addr ipv6, LPVOID context);

BOOL StartNameQuery(LPWSTR Hostname, ResolveCompleted onResolved, LPVOID context);


