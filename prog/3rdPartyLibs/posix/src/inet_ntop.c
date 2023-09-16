#include  <string.h>
#include  <stdio.h>
#include  <errno.h>

#ifdef  _TARGET_PC_WIN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 // WinXP by default
#endif
#endif
#if _TARGET_PC_WIN|_TARGET_XBOX
# include <Ws2tcpip.h>
# if (NTDDI_VERSION >= NTDDI_VISTA)
#   define HAVE_NTOP 1
# endif
#else
# include <sys/socket.h>
# include <sys/types.h>
#endif

#if !defined(HAVE_NTOP)

static const char* inet_ntop4(const unsigned char* src, char* dst, size_t len)
{
  char  tmp[sizeof "255.255.255.255"];
  if (sprintf(tmp, "%u.%u.%u.%u", src[0], src[1], src[2], src[3]) >= len)
  {
    errno = ENOSPC;
    return  NULL;
  }
  return  strcpy(dst, tmp);
}

const char* inet_ntop_compat(int af, const void* src, char* dst, size_t len)
{
  if (af == AF_INET)
    return  inet_ntop4((const unsigned char*)src, dst, len);
  errno = EAFNOSUPPORT;
  return  NULL;
}

#endif
