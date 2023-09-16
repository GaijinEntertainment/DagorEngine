#include "socket.h"

namespace ipc
{

namespace
{
const int would_block = WSAEWOULDBLOCK;

Socket::sock_t create_socket()
{
  Socket::sock_t s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  u_long mode = 1;
  if (ioctlsocket(s, FIONBIO, &mode))
    return Socket::invalid_socket;
  return s;
}


struct Address
{
  Address(const char *addr, unsigned short port)
  {
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr(addr);
    sa.sin_port = htons(port);
  }

  struct sockaddr_in sa;
}; //struct Address

} // namespace anonymous

Socket::Socket()
  : sock(invalid_socket), err(0)
{
  WSADATA wd;
  WSAStartup(2, &wd);
}


Socket::~Socket()
{
  this->close();
  WSACleanup();
}


bool Socket::listen(const char* addr, unsigned short port)
{
  this->sock = create_socket();
  if (this->isValid())
  {
    Address a(addr, port);
    if (SOCKET_ERROR == bind(this->sock, (const sockaddr*)&a.sa, sizeof(a.sa)))
      this->close();
  }

  if (this->isValid())
    if (SOCKET_ERROR == ::listen(this->sock, SOMAXCONN))
      this->close();

  return this->isValid();
}

bool Socket::accept(Socket& s)
{
  s.sock = ::accept(this->sock, NULL, NULL);
  int error = ::WSAGetLastError();
  if (!s.isValid() && error != would_block)
    this->close();

  return s.isValid() || error == would_block;
}

bool Socket::connect(const char* addr, unsigned short port, bool do_block)
{
  this->sock = create_socket();
  if (do_block)
  {
    u_long mode = 0;
    ioctlsocket(this->sock, FIONBIO, &mode);
  }

  Address a(addr, port);
  int error = would_block;
  while (this->isValid() && error == would_block)
  {
    int r = ::connect(this->sock, (const sockaddr*)&a.sa, sizeof(a.sa));
    error = ::WSAGetLastError();
    if (r == SOCKET_ERROR && error != would_block)
      this->close();
    else
      break;
  }

  if (this->isValid() && do_block)
  {
    u_long mode = 1;
    ioctlsocket(this->sock, FIONBIO, &mode);
  }

  return this->isValid();
}

size_t Socket::send(const uint8_t *data, size_t size)
{
  size_t sent = 0;
  while (this->isValid() && sent < size)
  {
    int l = ::send(this->sock, (char*)(data + sent), size - sent, 0);
    if (l != SOCKET_ERROR)
      sent += l;
    else if (::WSAGetLastError() != would_block)
      this->close();
  }
  return sent;
}

size_t Socket::recv(uint8_t *data, size_t max_size)
{
  if (!this->isValid())
    return 0;

  int received = ::recv(this->sock, (char*)data, max_size, 0);
  int error = ::WSAGetLastError();
  if (received == SOCKET_ERROR)
  {
    if (error == would_block)
      received = 0;
    else
      this->close();
  }

  return (size_t)received;
}

void Socket::close()
{
  this->err = ::WSAGetLastError();
  closesocket(this->sock);
  this->sock = Socket::invalid_socket;
}

} // namespace ipc
