#pragma once

#include "daProfilePlatform.h"
#include <osApiWrappers/dag_sockets.h>
#include <ioSys/dag_genIo.h>

struct SocketHandler
{
  os_socket_t socket = OS_SOCKET_INVALID;
  SocketHandler(os_socket_t s = OS_SOCKET_INVALID) : socket(s) {}
  ~SocketHandler() { close(); }
  void close()
  {
    if (socket != OS_SOCKET_INVALID)
      os_socket_close(socket);
    socket = OS_SOCKET_INVALID;
  }
  SocketHandler(SocketHandler &&a) : socket(a.socket) { a.socket = OS_SOCKET_INVALID; }
  SocketHandler &operator=(SocketHandler &&a)
  {
    os_socket_t c = socket;
    socket = a.socket;
    a.socket = c;
    return *this;
  }
  SocketHandler(const SocketHandler &) = delete;
  SocketHandler &operator=(const SocketHandler &) = delete;
  operator bool() const { return socket != OS_SOCKET_INVALID; }
  explicit operator os_socket_t() const { return socket; }
  os_socket_t get() const { return socket; }
  os_socket_t &get() { return socket; }
};

inline int blocked_socket_recvfrom(os_socket_t s, char *buf, int len)
{
  // for works for non-blocking connection as well. On XDK non blocking sockets do not work.
  for (int lenLeft = len, iter = 0; lenLeft > 0; iter++)
  {
    int read = os_socket_recvfrom(s, buf, lenLeft, 0, NULL, NULL);
    if (read < 0)
      return read;
    lenLeft -= read;
    buf += read;
    if (lenLeft)
      da_profiler::sleep_msec(iter);
  }
  return len;
}

class NetTCPSave : public IGenSave
{
  os_socket_t &sock;

public:
  NetTCPSave(os_socket_t &s) : sock(s) {}
  void close()
  {
    os_socket_close(sock);
    sock = OS_SOCKET_INVALID;
  }
  void write(const void *ptr, int size) override
  {
    if (sock == OS_SOCKET_INVALID)
      return;
    char buf[256];
    if (os_socket_send(sock, (const char *)ptr, size) != size)
    {
      close();
      logwarn("can't write to socket %s", os_socket_error_str(os_socket_last_error(), buf, sizeof(buf)));
    }
  };

  /// Try write data to output stream. Might write less than passed in arguments. Default implementation same as write()
  int tryWrite(const void *ptr, int size) override
  {
    if (sock == OS_SOCKET_INVALID)
      return 0;
    int ret = os_socket_send(sock, (const char *)ptr, size);
    if (ret < 0)
      close();
    return ret;
  }

  int tell() override { return -1; }
  void seekto(int) override {}
  void seektoend(int) override {}
  const char *getTargetName() override { return "socket"; }
  void flush() override {}
  void beginBlock() override { logerr("soket do not support blocks"); }
  void endBlock(unsigned)
  {
    {
      logerr("soket do not support blocks");
    }
  }
  int getBlockLevel() { return 0; }
};

class NetTCPLoad : public IGenLoad
{
  os_socket_t &sock;

public:
  NetTCPLoad(os_socket_t &s) : sock(s) {}
  void close()
  {
    os_socket_close(sock);
    sock = OS_SOCKET_INVALID;
  }
  void read(void *ptr_, int size) override
  {
    if (blocked_socket_recvfrom(sock, (char *)ptr_, size) != size)
    {
      close();
      char buf[256];
      logwarn("Can't read %d from socket %s", size, os_socket_error_str(os_socket_last_error(), buf, sizeof(buf)));
      return;
    }
  }

  int tryRead(void *ptr, int size) override
  {
    if (sock == OS_SOCKET_INVALID)
      return -1;
    const int status = os_socket_read_select(sock);
    if (status == 0)
      return 0;
    if (status == -1)
      return -1;
    return blocked_socket_recvfrom(sock, (char *)ptr, size);
  }

  /// Get current position in input stream.
  int tell() override { return -1; }
  void seekto(int) override {}
  void seekrel(int) override {}
  const char *getTargetName() override { return "socket reader"; }
  int beginBlock(unsigned *) override { return -1; }
  void endBlock() override {}
  int getBlockLength() override { return -1; }
  int getBlockRest() override { return -1; }
  int getBlockLevel() override { return -1; }
};
