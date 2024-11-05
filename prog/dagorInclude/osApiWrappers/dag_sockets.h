//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <util/dag_globDef.h>
#include <string.h>

#include <supp/dag_define_KRNLIMP.h>


#if _TARGET_PC_WIN | _TARGET_XBOX
typedef uintptr_t os_socket_t;
#else
typedef int os_socket_t;
#endif
const os_socket_t OS_SOCKET_INVALID = (os_socket_t)-1;

enum OsSocketAddressFamily
{
  OSAF_UNSPEC,
  OSAF_IPV4,
  OSAF_IPV6,
  OSAF_UNIX
};

enum OsSocketType
{
  OST_TCP,
  OST_UDP
};

enum OsSocketOption
{
  OSO_NONBLOCK
};

// bitmask, use power of 2
enum OsSocketSendFlags
{
  OSSF_DONTROUTE = 1,
  OSSF_OOB = 2
};

static constexpr int OS_SOCKET_ERR_WOULDBLOCK = 11;

struct os_socket_addr
{
  alignas(4) char opaque[16];
};
struct os_socket_addr_in6
{
  os_socket_addr base;
  char opaque2[12];
};
struct os_socket_addr_unix
{
  os_socket_addr base;
  char opaque2[94];
};

KRNLIMP int os_sockets_init();
KRNLIMP int os_sockets_shutdown();

KRNLIMP os_socket_t os_socket_create(OsSocketAddressFamily af, OsSocketType type);
KRNLIMP int os_socket_close(os_socket_t s);
KRNLIMP int os_socket_connect(os_socket_t s, const os_socket_addr *addr, int addr_len);
KRNLIMP int os_socket_send(os_socket_t s, const char *buf, int len, int flags = 0);
KRNLIMP int os_socket_sendto(os_socket_t s, const char *buf, int len, int flags = 0, const os_socket_addr *addr = NULL,
  int addr_len = 0);
KRNLIMP int os_socket_recvfrom(os_socket_t s, char *buf, int len, int flags = 0, os_socket_addr *from = NULL, int *from_len = NULL);

KRNLIMP int os_socket_read_select(os_socket_t s, int time_sec = 0, int time_usec = 0);

KRNLIMP int os_socket_set_option(os_socket_t s, OsSocketOption option, intptr_t value);

KRNLIMP int os_socket_bind(os_socket_t s, const os_socket_addr *addr, int addr_len);
KRNLIMP int os_socket_listen(os_socket_t s, int maxconn); // if 'maxconn' is negative than it would be set to maximum
KRNLIMP os_socket_t os_socket_accept(os_socket_t s, os_socket_addr *addr, int *addr_len);


KRNLIMP int os_socket_set_reuse_addr(os_socket_t socket, bool reuse_addr);
KRNLIMP int os_socket_set_no_delay(os_socket_t socket, bool no_delay);
KRNLIMP int os_socket_set_send_timeout(os_socket_t socket, int timeout_sec);
KRNLIMP int os_socket_set_recv_timeout(os_socket_t socket, int timeout_sec);
KRNLIMP int os_socket_set_no_sigpipe(os_socket_t socket);
KRNLIMP int os_socket_enable_keepalive(intptr_t sock, bool keep_alive);

KRNLIMP int os_socket_error(os_socket_t s);
KRNLIMP int os_socket_last_error();
KRNLIMP const char *os_socket_error_str(int error, char *buf, int len);

KRNLIMP int os_socket_addr_from_string(OsSocketAddressFamily af, const char *str, os_socket_addr *addr, int addr_len);
KRNLIMP int os_socket_addr_to_string(const os_socket_addr *addr, int addr_len, char *buf, int buf_len);
KRNLIMP int os_socket_addr_set_family(os_socket_addr *addr, int addr_len, OsSocketAddressFamily af);
KRNLIMP OsSocketAddressFamily os_socket_addr_get_family(const os_socket_addr *addr, int addr_len);
KRNLIMP int os_socket_addr_set_port(os_socket_addr *addr, int addr_len, uint16_t port);
KRNLIMP int os_socket_addr_get_port(const os_socket_addr *addr, int addr_len);
KRNLIMP int os_socket_addr_set(os_socket_addr *addr, int addr_len, const void *ip); // ip in network byte order
KRNLIMP int os_socket_addr_get(const os_socket_addr *addr, int addr_len, void *ip);

KRNLIMP uint32_t os_htonl(uint32_t hostlong);
KRNLIMP uint16_t os_htons(uint16_t hostshort);
KRNLIMP uint32_t os_ntohl(uint32_t netlong);
KRNLIMP uint16_t os_ntohs(uint16_t netshort);

KRNLIMP char *os_gethostname(char *buf, size_t blen, const char *def = NULL);

KRNLIMP int os_socket_sys_handle(os_socket_t s);

namespace sockets
{

namespace details
{
template <unsigned T>
struct SocketAddrTypeSelector;
template <>
struct SocketAddrTypeSelector<OSAF_IPV4>
{
  typedef os_socket_addr socket_addr_t;
};
template <>
struct SocketAddrTypeSelector<OSAF_IPV6>
{
  typedef os_socket_addr_in6 socket_addr_t;
};
template <>
struct SocketAddrTypeSelector<OSAF_UNIX>
{
  typedef os_socket_addr_unix socket_addr_t;
};
}; // namespace details

template <OsSocketAddressFamily so_family>
class SocketAddr
{
public:
  SocketAddr() { invalidate(); }

  SocketAddr(const char *host, uint16_t port)
  {
    if (
      os_socket_addr_from_string(so_family, host, &addr, sizeof(addr)) != 0 || os_socket_addr_set_port(&addr, sizeof(addr), port) != 0)
      invalidate();
  }

  SocketAddr(const char *str)
  {
    if (os_socket_string_to_addr(so_family, str, &addr, sizeof(addr)) != 0)
      invalidate();
  }

  SocketAddr(const os_socket_addr *src_addr, int addr_len)
  {
    int af = os_socket_addr_get_family(src_addr, addr_len);
    G_UNUSED(af);
    G_ASSERT(af == so_family);
    G_ASSERT(addr_len == sizeof(addr));
    memcpy(&addr, src_addr, addr_len);
  }

  SocketAddr(const SocketAddr &src) { *this = src; }

  SocketAddr &operator=(const SocketAddr &src)
  {
    memcpy(&addr, &src.addr, sizeof(addr));
    return *this;
  }

  bool setHost(const char *host)
  {
    bool valid = os_socket_addr_from_string(so_family, host, &addr, sizeof(addr)) == 0;
    if (!valid)
      invalidate();
    return valid;
  }

  bool setPort(uint16_t port)
  {
    bool valid = os_socket_addr_set_port(&addr, sizeof(addr), port) == 0;
    if (!valid)
      invalidate();
    return valid;
  }

  const os_socket_addr *getRawAddr(int &raw_size) const
  {
    raw_size = sizeof(addr);
    return (os_socket_addr *)&addr;
  }

  const char *str(char *buf, int buf_len) const
  {
    if (!buf || buf_len <= 0 || !isValid() || os_socket_addr_to_string(&addr, sizeof(addr), buf, buf_len) != 0)
      return NULL;
    return buf;
  }

  OsSocketAddressFamily getAddressFamily() const { return so_family; }
  bool isValid() const { return os_socket_addr_get_family(&addr, sizeof(addr)) != OSAF_UNSPEC; }

private:
  void invalidate() { os_socket_addr_set_family(&addr, sizeof(addr), OSAF_UNSPEC); }

  typename details::SocketAddrTypeSelector<so_family>::socket_addr_t addr;
};


class Socket
{
public:
  KRNLIMP Socket(OsSocketAddressFamily af_, OsSocketType st_, bool init = true);

  KRNLIMP ~Socket() { close(); }

  operator bool() const { return sock != OS_SOCKET_INVALID; }

  KRNLIMP void close();
  KRNLIMP void reset();

  template <OsSocketAddressFamily so_family>
  KRNLIMP int connect(const SocketAddr<so_family> &addr);

  KRNLIMP int send(const char *buf, int len, int flags = 0);

  template <OsSocketAddressFamily so_family>
  KRNLIMP int sendto(const SocketAddr<so_family> &addr, const char *buf, int len, int flags = 0);

  // For UDP sockets this function must be prefered rather than sendto()
  // on some platforms after wakeup from suspend state sockets maybe destroyed.
  // sendtoWithResetOnError will recreate them in this case.
  template <OsSocketAddressFamily so_family>
  int sendtoWithResetOnError(const SocketAddr<so_family> &addr, const char *buf, int len, int flags = 0)
  {
    int osAddrLen = 0;
    const os_socket_addr *osAddr = addr.getRawAddr(osAddrLen);
    return sendtoWithResetOnErrorInternal(osAddr, osAddrLen, buf, len, flags);
  }


  KRNLIMP int setopt(OsSocketOption opt, intptr_t value);

  int getLastError() const { return lastError; }
  const char *getLastErrorStr(char *buf, int buf_len) const { return os_socket_error_str(lastError, buf, buf_len); }
  os_socket_t getRawSocket() const { return sock; }

protected:
  os_socket_t sock;
  int lastError;
  OsSocketAddressFamily af;
  OsSocketType st;

private:
  KRNLIMP Socket(const Socket &);
  KRNLIMP Socket &operator=(const Socket &);

  KRNLIMP int sendtoWithResetOnErrorInternal(const os_socket_addr *osAddr, int osAddrLen, const char *buf, int len, int flags);
};

}; // namespace sockets

#include <supp/dag_undef_KRNLIMP.h>
