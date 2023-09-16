#ifndef _DAGOR_POSIX_WIN32_ARPA_INET_H
#define _DAGOR_POSIX_WIN32_ARPA_INET_H
#pragma once


#if _WIN32_WINNT <= 0x0501

#ifdef __cplusplus
extern  "C" {
#endif

int inet_pton_compat(int af, const char *src, void *dst);
const char *inet_ntop_compat(int af, const void* src, char* dst, size_t size);

#define inet_pton inet_pton_compat
#define inet_ntop inet_ntop_compat

#ifdef  __cplusplus
}
#endif

#else
#include  <Ws2tcpip.h>
#endif

#endif
