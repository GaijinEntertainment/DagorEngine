// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdint.h>

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

namespace ipc
{

class Socket
{
  public:
    typedef ::SOCKET sock_t;
    static const sock_t invalid_socket = INVALID_SOCKET;
    typedef int error_t;

  public:
    Socket();
    ~Socket();

  public:
    bool listen(const char* addr, unsigned short port);
    bool accept(Socket &dst);
    bool connect(const char* addr, unsigned short port, bool do_block=false);

    size_t send(const uint8_t *data, size_t size);
    size_t recv(uint8_t *data, size_t max_size);

    void close();

    bool isValid() { return this->sock != invalid_socket; }
    error_t lastError() { return this->err; }
    bool operator()() { return this->isValid(); }

  private:
    sock_t sock;
    error_t err;
}; // struct Socket


} // namespace ipc
