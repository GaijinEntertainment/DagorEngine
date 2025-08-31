// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_sockets.h>
#include <osApiWrappers/dag_miscApi.h>
#include <perfMon/dag_cpuFreq.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define IPV6_SUPPORTED 1
#define UNIX_SUPPORTED 0

#if _TARGET_PC_WIN
#if !defined(_WIN32_WINNT) || _WIN32_WINNT <= 0x0501
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0601 // Win7 (for inet_pton/inet_ntop)
#endif
#include <WS2tcpip.h>
#include <io.h>

static const int LOCAL_EINVAL = WSAEINVAL;
static const int LOCAL_EAGAIN = WSAEWOULDBLOCK;
static inline int get_errno() { return WSAGetLastError(); }

#pragma comment(lib, "Ws2_32.lib")
#elif _TARGET_XBOX

#include <WS2tcpip.h>
#include <io.h>
#include <malloc.h>
static const int LOCAL_EINVAL = WSAEINVAL;
static const int LOCAL_EAGAIN = WSAEWOULDBLOCK;
static inline int get_errno() { return WSAGetLastError(); }

#elif _TARGET_C3




#else

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>

#if _TARGET_C1 | _TARGET_C2

#else
#include <fcntl.h>
#include <sys/un.h>
#undef UNIX_SUPPORTED
#define UNIX_SUPPORTED 1
#endif

static const int LOCAL_EINVAL = EINVAL;
static const int LOCAL_EAGAIN = EAGAIN;
static inline int get_errno() { return errno; }

#endif

#include <util/dag_globDef.h>

#if !IPV6_SUPPORTED
struct in6_addr
{
  uint8_t u6_addr8[16];
};
struct sockaddr_in6
{
  uint16_t sin6_family;
  uint16_t sin6_port;
  uint32_t sin6_flowinfo;
  in6_addr sin6_addr;
  uint32_t sin6_scope_id;
};
#endif

#if !UNIX_SUPPORTED
struct sockaddr_un
{
  uint16_t sun_family;
  char sun_path[108]; /* Path name.  */
};
#endif

#define ASSURE(cond, err, ret) \
  if (!(cond))                 \
  {                            \
    set_last_error((err));     \
    return (ret);              \
  }

#ifndef SOMAXCONN
#define SOMAXCONN 128
#endif

#if defined(__APPLE__) && !defined(SOL_TCP)
#define SOL_TCP IPPROTO_TCP
#endif

#if _TARGET_PC_WIN || _TARGET_XBOX
using SockOptArg = char *;
using socklen_t = int;
#else
using SockOptArg = void *;
#endif

thread_local int socket_last_error = 0;
static int g_socket_library_initialized = 0;

static inline void set_last_error(int err) { socket_last_error = (err == LOCAL_EAGAIN) ? OS_SOCKET_ERR_WOULDBLOCK : err; }


int os_sockets_init()
{
#if _TARGET_PC_WIN | _TARGET_XBOX
  WSADATA wd;
  int initResult = WSAStartup(2, &wd);
  ASSURE(initResult == 0, initResult, -1);
#if _TARGET_PC_WIN // do actually make sure that socket can be created (as some errors are delayed until real socket created)
  os_socket_t probeSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (probeSocket == OS_SOCKET_INVALID)
  {
    set_last_error(get_errno());
    WSACleanup();
    return -1;
  }
  closesocket(probeSocket);
#endif
#elif _TARGET_C3





#endif
  ++g_socket_library_initialized;
  return 0;
}

void assert_if_sockets_not_initialized()
{
  G_ASSERTF(g_socket_library_initialized > 0, "Socket library has not been initialized, call os_sockets_init() before");
}

/* Application no longer requires the use of the network interface. */
int os_sockets_shutdown()
{
  assert_if_sockets_not_initialized();

  if (g_socket_library_initialized > 0)
  {
    --g_socket_library_initialized;
    if (!g_socket_library_initialized)
    {
#if _TARGET_C3

#elif _TARGET_PC_WIN | _TARGET_XBOX
      ASSURE(WSACleanup() == 0, get_errno(), -1);
#endif
    }
  }
  return 0;
}


os_socket_t os_socket_create(OsSocketAddressFamily af, OsSocketType type)
{
  assert_if_sockets_not_initialized();

  int realAf, realType, proto;
  switch (af)
  {
    case OSAF_IPV4:
      realAf = AF_INET;
      switch (type)
      {
        case OST_TCP:
          realType = SOCK_STREAM;
          proto = IPPROTO_TCP;
          break;
        case OST_UDP:
          realType = SOCK_DGRAM;
          proto = IPPROTO_UDP;
          break;
        default: ASSURE(false, LOCAL_EINVAL, -1);
      }
      break;

#if UNIX_SUPPORTED
    case OSAF_UNIX:
      realAf = AF_UNIX;
      proto = 0; //  the only subprotocol for unix sockets
      switch (type)
      {
        case OST_TCP: realType = SOCK_STREAM; break;
        case OST_UDP: realType = SOCK_DGRAM; break;
        default: ASSURE(false, LOCAL_EINVAL, -1);
      }
      break;
#endif

    default: ASSURE(false, LOCAL_EINVAL, -1);
  }
  os_socket_t s = (os_socket_t)socket(realAf, realType, proto);
  ASSURE(s != OS_SOCKET_INVALID, get_errno(), s);
  return s;
}


int os_socket_close(os_socket_t s)
{
#if _TARGET_PC_WIN | _TARGET_XBOX
  int r = closesocket(s);
#elif _TARGET_C3

#else
  int r = close(s);
#endif
  ASSURE(r == 0, get_errno(), r);
  return r;
}


int os_socket_connect(os_socket_t s, const os_socket_addr *addr, int addr_len)
{
  int r = connect(s, (const sockaddr *)addr, addr_len);
  ASSURE(r == 0, get_errno(), r);
  return r;
}


int os_socket_sendto(os_socket_t s, const char *buf, int len, int flags, const os_socket_addr *addr, int addr_len)
{
#if !(_TARGET_C1 | _TARGET_C2)
  int realFlags = 0;
#if _TARGET_PC_LINUX | _TARGET_PC_ANDROID
  realFlags |= MSG_NOSIGNAL;
#endif
  if (flags & OSSF_DONTROUTE)
    realFlags |= MSG_DONTROUTE;
  if (flags & OSSF_OOB)
    realFlags |= MSG_OOB;
  int r = sendto(s, buf, len, realFlags, (const sockaddr *)addr, addr_len);
#else

#endif
  ASSURE(r >= 0, get_errno(), r);
  return r;
}


int os_socket_send(os_socket_t s, const char *buf, int len, int flags) { return os_socket_sendto(s, buf, len, flags, NULL, 0); }


int os_socket_set_option(os_socket_t s, OsSocketOption option, intptr_t value)
{
  int r = 0;
  switch (option)
  {
    case OSO_NONBLOCK:
    {
#if _TARGET_PC_WIN | _TARGET_XBOX
      u_long mode = value ? 1 : 0;
      r = ioctlsocket(s, FIONBIO, &mode);
#elif _TARGET_C1 | _TARGET_C2

#else
      int flags = fcntl(s, F_GETFL, 0);
      ASSURE(flags != -1, errno, -1);
      r = fcntl(s, F_SETFL, value ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK));
#endif
    }
    break;
    default: ASSURE(false, LOCAL_EINVAL, -1);
  }

  ASSURE(r == 0, get_errno(), -1);

  return r;
}

int os_socket_read_select(os_socket_t s, int time_sec, int time_usec)
{
  fd_set receiveSet;
  FD_ZERO(&receiveSet);
  FD_SET(s, &receiveSet);

  timeval lim = {time_sec, time_usec};
#if _TARGET_PC_WIN | _TARGET_XBOX
  return ::select(0, &receiveSet, nullptr, nullptr, &lim);
#else
  return ::select(s + 1, &receiveSet, nullptr, nullptr, &lim);
#endif
}

int os_socket_recvfrom(os_socket_t s, char *buf, int len, int flags, os_socket_addr *addr, int *addr_len)
{
  G_STATIC_ASSERT(sizeof(socklen_t) == sizeof(int));
  int r = recvfrom(s, buf, len, flags, (sockaddr *)addr, (socklen_t *)addr_len);
  ASSURE(r >= 0, get_errno(), -1);
  return r;
}

int os_socket_bind(os_socket_t s, const os_socket_addr *addr, int addr_len)
{
  int r = bind(s, (const sockaddr *)addr, addr_len);
  ASSURE(r >= 0, get_errno(), -1);
  return r;
}

int os_socket_listen(os_socket_t s, int maxconn)
{
  int r = listen(s, maxconn < 0 ? SOMAXCONN : maxconn);
  ASSURE(r >= 0, get_errno(), -1);
  return r;
}

os_socket_t os_socket_accept(os_socket_t s, os_socket_addr *addr, int *addr_len)
{
  os_socket_t r = accept(s, (sockaddr *)addr, (socklen_t *)addr_len);
  ASSURE(r != OS_SOCKET_INVALID, get_errno(), r);
  return r;
}

int os_socket_set_reuse_addr(os_socket_t socket, bool reuse_addr)
{
  int v = reuse_addr ? 1 : 0;
  // todo: may be add SO_EXCLUSIVEADDRUSE on Windows? otherwise there is a possibility of malicious
  return setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (char *)&v, sizeof(v));
}

int os_socket_set_no_delay(os_socket_t socket, bool no_delay)
{
  int v = no_delay ? 1 : 0;
  return setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char *)&v, sizeof(v));
}

int os_socket_set_send_timeout(os_socket_t socket, int timeout_sec)
{
#if !_WIN32
  struct timeval timeout;
  memset(&timeout, 0, sizeof(timeout));
  timeout.tv_sec = timeout_sec;
#else
  DWORD timeout = timeout_sec * 1000;
#endif
  return setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
}

int os_socket_set_recv_timeout(os_socket_t socket, int timeout_sec)
{
#if !_WIN32
  struct timeval timeout;
  memset(&timeout, 0, sizeof(timeout));
  timeout.tv_sec = timeout_sec;
#else
  DWORD timeout = timeout_sec * 1000;
#endif
  return setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
}

int os_socket_set_no_sigpipe(os_socket_t socket)
{
#ifdef __APPLE__
  int set = 1;
  return setsockopt(socket, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
#endif
  G_UNUSED(socket);
  return 0;
}

int os_socket_enable_keepalive(os_socket_t socket, bool keep_alive)
{
  int v = keep_alive ? 1 : 0;
  return setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, (char *)&v, sizeof(int));
}

int os_socket_error(os_socket_t s)
{
  int val;
  socklen_t vallen = sizeof(val);
  getsockopt(s, SOL_SOCKET, SO_ERROR, (SockOptArg)&val, &vallen);
  return val;
}

int os_socket_last_error() { return socket_last_error; }

const char *os_socket_error_str(int error, char *buf, int len)
{
  if (error == OS_SOCKET_ERR_WOULDBLOCK)
    error = LOCAL_EAGAIN;

#if _TARGET_PC_WIN
  if (!FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, (DWORD)error, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), buf, len, NULL))
    SNPRINTF(buf, len, "Unknown error %d", error);
#elif _TARGET_XBOX
  SNPRINTF(buf, len, "Unknown error %d", error);
#else
#if _TARGET_PC_LINUX
  const char *errStr = strerror_r(error, buf, len); // GNU-specific
  if (errStr && errStr != buf)
  {
    strncpy(buf, errStr, len);
    buf[len - 1] = '\0';
  }
  else if (!errStr)
#elif defined(__USE_GNU) && __ANDROID_API__ >= 23
  return strerror_r(error, buf, len); // POSIX-compliant
#else
  int ret = strerror_r(error, buf, len); // POSIX-compliant
  if (ret)
#endif
    SNPRINTF(buf, len, "Unknown error %d", error);
#endif

  return buf;
}


static bool resolve_hostname(const char *str, sockaddr *addr, int addr_len)
{
  G_ASSERT(str);
  G_ASSERT(*str);
  G_ASSERT(addr);
  G_ASSERT(addr->sa_family == AF_INET || addr->sa_family == AF_INET6);
  G_ASSERT(addr_len >= ((addr->sa_family == AF_INET) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6)));
  ((void)addr_len);

  sockaddr_in *ipv4Addr = (sockaddr_in *)addr;
  sockaddr_in6 *ipv6Addr = (sockaddr_in6 *)addr;
  if (inet_pton(addr->sa_family, str,
        (addr->sa_family == AF_INET) ? (void *)&ipv4Addr->sin_addr : (void *)&ipv6Addr->sin6_addr)) // in case of dotted addr don't
                                                                                                    // reference DNS
    return true;

#if !(_TARGET_C1 | _TARGET_C2)
  addrinfo hints;
  memset(&hints, 0, sizeof(addrinfo));
  hints.ai_family = addr->sa_family;
#if !(_TARGET_PC_WIN | _TARGET_XBOX) // http://src.chromium.org/viewvc/chrome/trunk/src/net/dns/host_resolver_proc.cc#l149
  hints.ai_flags = AI_ADDRCONFIG;
#endif

  addrinfo *result = NULL;
  int rescode = getaddrinfo(str, nullptr, &hints, &result);
  G_ASSERT(!rescode != !result); // either should be an error or result, not both

#if _TARGET_XBOX // Xbox sockets are not available during the first seconds after startup.
  const int xboxSocketsStartupDelayMs = 20000;
  while (rescode != 0 && get_time_msec() < xboxSocketsStartupDelayMs)
  {
    sleep_msec(500);
    debug("time=%dms, getaddrinfo error=%d, trying to resolve %s", get_time_msec(), get_errno(), str);
    rescode = getaddrinfo(str, nullptr, &hints, &result);
    if (rescode == 0)
      debug("getaddrinfo succeeded");
  }
#endif

  ASSURE(rescode == 0, get_errno(), false);

  G_ASSERT(result->ai_family == addr->sa_family);
  memcpy(addr, result->ai_addr, (addr->sa_family == AF_INET) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6));

  freeaddrinfo(result);
#else








#endif

  return true;
}


int os_socket_addr_from_string(OsSocketAddressFamily af, const char *str, os_socket_addr *addr, int addr_len)
{
  G_STATIC_ASSERT(sizeof(sockaddr_in) <= sizeof(os_socket_addr));
  G_STATIC_ASSERT(sizeof(sockaddr_in6) <= sizeof(os_socket_addr_in6));
  G_STATIC_ASSERT(sizeof(sockaddr_un) <= sizeof(os_socket_addr_unix));

  ASSURE(str && *str && addr, LOCAL_EINVAL, -1);

  memset(addr, 0, addr_len);
  const char *pColon = NULL;

  switch (af)
  {
    case OSAF_IPV4:
    {
      ASSURE(addr_len >= sizeof(sockaddr_in), LOCAL_EINVAL, -1);
      ((sockaddr *)addr)->sa_family = AF_INET;
      pColon = strchr(str, ':');
    }
    break;
#if IPV6_SUPPORTED
    case OSAF_IPV6:
    {
      ASSURE(addr_len >= sizeof(sockaddr_in6), LOCAL_EINVAL, -1);
      ((sockaddr *)addr)->sa_family = AF_INET6;
      const char *pRBrack = strchr(str, ']');
      pColon = (pRBrack && pRBrack[1] == ':') ? pRBrack + 1 : NULL;
    }
    break;
#endif

#if UNIX_SUPPORTED
    case OSAF_UNIX:
    {
      ASSURE(addr_len >= sizeof(sockaddr_un), LOCAL_EINVAL, -1);
      ((sockaddr *)addr)->sa_family = AF_UNIX;
      char *dst = ((sockaddr_un *)addr)->sun_path;
      const size_t dst_len = sizeof(((sockaddr_un *)(NULL))->sun_path);
      strncpy(dst, str, dst_len);
      dst[dst_len - 1] = '\0';
    }
      return 0;
#endif

    default: ASSURE(false, EAFNOSUPPORT, -1);
  }

  if (pColon) // port exists?
  {
    char hostBuf[256];
    int hostLen = pColon - str;
    ASSURE(hostLen < sizeof(hostBuf), LOCAL_EINVAL, -1);
    strncpy(hostBuf, str, hostLen);
    hostBuf[hostLen] = '\0';
    char *endPtr = NULL;
    uint32_t port32 = strtoul(pColon + 1, &endPtr, 10);
    ASSURE(endPtr && *endPtr == '\0' && port32 <= 0xFFFF, LOCAL_EINVAL, -1);

    if (!resolve_hostname(hostBuf, (sockaddr *)addr, addr_len))
      return -1;
    return os_socket_addr_set_port(addr, addr_len, (uint16_t)port32);
  }
  else if (!resolve_hostname(str, (sockaddr *)addr, addr_len)) // just set host
    return -1;
  return 0;
}


int os_socket_addr_to_string(const os_socket_addr *addr, int addr_len, char *buf, int buf_len)
{
  ASSURE(addr && buf, LOCAL_EINVAL, -1);
  const sockaddr *sa = (const sockaddr *)addr;
  const void *src = NULL;
  switch (sa->sa_family)
  {
    case AF_INET:
      ASSURE(addr_len >= sizeof(sockaddr_in), LOCAL_EINVAL, -1);
      src = &((const sockaddr_in *)sa)->sin_addr;
      break;
    case AF_INET6:
      ASSURE(addr_len >= sizeof(sockaddr_in6), LOCAL_EINVAL, -1);
      src = &((const sockaddr_in6 *)sa)->sin6_addr;
      break;

    case AF_UNIX:
      ASSURE(addr_len >= sizeof(sockaddr_un), LOCAL_EINVAL, -1);
      src = &((const sockaddr_un *)sa)->sun_path;
      strncpy(buf, (const char *)src, buf_len);
      buf[buf_len - 1] = '\0';
      return 0;

    default: ASSURE(false, EAFNOSUPPORT, -1);
  }
  ASSURE(inet_ntop(sa->sa_family, (void *)src, buf, buf_len), get_errno(), -1);
  return 0;
}


int os_socket_addr_set_family(os_socket_addr *addr, int addr_len, OsSocketAddressFamily af)
{
  ASSURE(addr && (af == OSAF_IPV4 || af == OSAF_IPV6), LOCAL_EINVAL, -1);
  sockaddr *sa = (sockaddr *)addr;
  switch (af)
  {
    case OSAF_UNSPEC:
      ASSURE(addr_len >= sizeof(sockaddr), LOCAL_EINVAL, -1);
      sa->sa_family = AF_UNSPEC;
      break;
    case OSAF_IPV4:
      ASSURE(addr_len >= sizeof(sockaddr_in), LOCAL_EINVAL, -1);
      sa->sa_family = AF_INET;
      break;
#if IPV6_SUPPORTED
    case OSAF_IPV6:
      ASSURE(addr_len >= sizeof(sockaddr_in6), LOCAL_EINVAL, -1);
      sa->sa_family = AF_INET6;
      break;
#endif
#if UNIX_SUPPORTED
    case OSAF_UNIX:
      ASSURE(addr_len >= sizeof(sockaddr_un), LOCAL_EINVAL, -1);
      sa->sa_family = AF_UNIX;
      break;
#endif
    default: ASSURE(false, EAFNOSUPPORT, -1);
  }
  return 0;
}


OsSocketAddressFamily os_socket_addr_get_family(const os_socket_addr *addr, int addr_len)
{
  ASSURE(addr, LOCAL_EINVAL, OSAF_UNSPEC);
  const sockaddr *sa = (const sockaddr *)addr;
  switch (sa->sa_family)
  {
    case AF_UNSPEC: ASSURE(addr_len >= sizeof(sockaddr), LOCAL_EINVAL, OSAF_UNSPEC); return OSAF_UNSPEC;
    case AF_INET: ASSURE(addr_len >= sizeof(sockaddr_in), LOCAL_EINVAL, OSAF_UNSPEC); return OSAF_IPV4;
    case AF_INET6: ASSURE(addr_len >= sizeof(sockaddr_in6), LOCAL_EINVAL, OSAF_UNSPEC); return OSAF_IPV6;
    case AF_UNIX: ASSURE(addr_len >= sizeof(sockaddr_un), LOCAL_EINVAL, OSAF_UNSPEC); return OSAF_UNIX;
  }
  set_last_error(EAFNOSUPPORT);
  return OSAF_UNSPEC;
}


int os_socket_addr_set_port(os_socket_addr *addr, int addr_len, uint16_t port)
{
  sockaddr *sa = (sockaddr *)addr;
  ASSURE(sa, LOCAL_EINVAL, -1);
  switch (sa->sa_family)
  {
    case AF_INET:
      ASSURE(addr_len >= sizeof(sockaddr_in), LOCAL_EINVAL, -1);
      ((sockaddr_in *)sa)->sin_port = htons(port);
      return 0;
    case AF_INET6:
      ASSURE(addr_len >= sizeof(sockaddr_in6), LOCAL_EINVAL, -1);
      ((sockaddr_in6 *)sa)->sin6_port = htons(port);
      return 0;
    case AF_UNIX: ASSURE(addr_len >= sizeof(sockaddr_un), LOCAL_EINVAL, -1); return 0;
  }
  set_last_error(EAFNOSUPPORT);
  return -1;
}


int os_socket_addr_get_port(const os_socket_addr *addr, int addr_len)
{
  const sockaddr *sa = (const sockaddr *)addr;
  ASSURE(sa, LOCAL_EINVAL, -1);
  switch (sa->sa_family)
  {
    case AF_INET: ASSURE(addr_len >= sizeof(sockaddr_in), LOCAL_EINVAL, -1); return ntohs(((sockaddr_in *)sa)->sin_port);
    case AF_INET6: ASSURE(addr_len >= sizeof(sockaddr_in6), LOCAL_EINVAL, -1); return ntohs(((sockaddr_in6 *)sa)->sin6_port);
  }
  set_last_error(EAFNOSUPPORT);
  return -1;
}


int os_socket_addr_set(os_socket_addr *addr, int addr_len, const void *ip)
{
  ASSURE(addr && ip, LOCAL_EINVAL, -1);
  sockaddr *sa = (sockaddr *)addr;
  size_t sz = 0;
  void *dst = NULL;
  switch (sa->sa_family)
  {
    case AF_INET:
      ASSURE(addr_len >= sizeof(sockaddr_in), LOCAL_EINVAL, -1);
      sz = sizeof(in_addr);
      dst = &((sockaddr_in *)sa)->sin_addr;
      break;
    case AF_INET6:
      ASSURE(addr_len >= sizeof(sockaddr_in6), LOCAL_EINVAL, -1);
      sz = sizeof(in6_addr);
      dst = &((sockaddr_in6 *)sa)->sin6_addr;
      break;
    default: ASSURE(false, EAFNOSUPPORT, -1);
  }
  memcpy(dst, ip, sz);
  return 0;
}


int os_socket_addr_get(const os_socket_addr *addr, int addr_len, void *ip)
{
  ASSURE(addr && ip, LOCAL_EINVAL, -1);
  const sockaddr *sa = (const sockaddr *)addr;
  size_t sz = 0;
  const void *src = NULL;
  switch (sa->sa_family)
  {
    case AF_INET:
      ASSURE(addr_len >= sizeof(sockaddr_in), LOCAL_EINVAL, -1);
      sz = sizeof(in_addr);
      src = &((const sockaddr_in *)sa)->sin_addr;
      break;
    case AF_INET6:
      ASSURE(addr_len >= sizeof(sockaddr_in6), LOCAL_EINVAL, -1);
      sz = sizeof(in6_addr);
      src = &((const sockaddr_in6 *)sa)->sin6_addr;
      break;
    default: ASSURE(false, EAFNOSUPPORT, -1);
  }
  memcpy(ip, src, sz);
  return 0;
}

uint32_t os_htonl(uint32_t hostlong) { return htonl(hostlong); }
uint16_t os_htons(uint16_t hostshort) { return htons(hostshort); }
uint32_t os_ntohl(uint32_t netlong) { return ntohl(netlong); }
uint16_t os_ntohs(uint16_t netshort) { return ntohs(netshort); }

char *os_gethostname(char *buf, size_t blen, const char *def)
{
#if _TARGET_PC | _TARGET_XBOX | _TARGET_ANDROID
  if (gethostname(buf, (int)blen) == 0)
    return buf;
#endif
  if (!def)
    return NULL;
  strncpy(buf, def, blen);
  buf[blen - 1] = '\0';
  return buf;
}

int os_socket_sys_handle(os_socket_t s)
{
#if _TARGET_PC_WIN | _TARGET_XBOX
  return _open_osfhandle(s, 0);
#else
  return s;
#endif
}


sockets::Socket::Socket(OsSocketAddressFamily af_, OsSocketType st_, bool init) : af(af_), st(st_)
{
  lastError = 0;
  if (init)
  {
    sock = os_socket_create(af, st);
    if (sock == OS_SOCKET_INVALID)
      lastError = os_socket_last_error();
  }
  else
    sock = OS_SOCKET_INVALID;
}

void sockets::Socket::close()
{
  if (sock != OS_SOCKET_INVALID)
  {
    os_socket_close(sock);
    sock = OS_SOCKET_INVALID;
  }
}

void sockets::Socket::reset()
{
  close();
  sock = os_socket_create(af, st);
  if (sock == OS_SOCKET_INVALID)
    lastError = os_socket_last_error();
}

template <OsSocketAddressFamily so_family>
int sockets::Socket::connect(const SocketAddr<so_family> &addr)
{
  int osAddrLen = 0;
  const os_socket_addr *rawAddr = addr.getRawAddr(osAddrLen);
  int r = os_socket_connect(sock, rawAddr, osAddrLen);
  if (r == -1)
    lastError = os_socket_last_error();
  return r;
}

int sockets::Socket::send(const char *buf, int len, int flags)
{
  int r = os_socket_send(sock, buf, len, flags);
  if (r == -1)
    lastError = os_socket_last_error();
  return r;
}

template <OsSocketAddressFamily so_family>
int sockets::Socket::sendto(const SocketAddr<so_family> &addr, const char *buf, int len, int flags)
{
  int osAddrLen = 0;
  const os_socket_addr *osAddr = addr.getRawAddr(osAddrLen);
  int r = os_socket_sendto(sock, buf, len, flags, osAddr, osAddrLen);
  if (r == -1)
    lastError = os_socket_last_error();
  return r;
}


int sockets::Socket::sendtoWithResetOnErrorInternal(const os_socket_addr *osAddr, int osAddrLen, const char *buf, int len, int flags)
{
  int r = os_socket_sendto(sock, buf, len, flags, osAddr, osAddrLen);
  if (r == -1)
  {
    int err = os_socket_last_error();
    if (err == LOCAL_EINVAL && st == OST_UDP)
    {
      char addrbuf[32] = {0};
      os_socket_addr_to_string(osAddr, osAddrLen, addrbuf, sizeof(addrbuf));
      debug("[network] failed to send UDP frame to %s. socket had been destroyed.", addrbuf);
      reset();
      if (sock != OS_SOCKET_INVALID)
        r = os_socket_sendto(sock, buf, len, flags, osAddr, osAddrLen);
    }
    else
    {
      lastError = err;
    }
  }
  return r;
}

int sockets::Socket::setopt(OsSocketOption opt, intptr_t value)
{
  int r = os_socket_set_option(sock, opt, value);
  if (r == -1)
    lastError = os_socket_last_error();
  return r;
}

#include <supp/dag_define_KRNLIMP.h>
template KRNLIMP int sockets::Socket::sendto<OSAF_IPV4>(const SocketAddr<OSAF_IPV4> &, const char *, int, int);
template KRNLIMP int sockets::Socket::connect<OSAF_IPV4>(const SocketAddr<OSAF_IPV4> &);

#define EXPORT_PULL dll_pull_osapiwrappers_sockets
#include <supp/exportPull.h>
