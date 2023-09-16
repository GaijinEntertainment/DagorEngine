#ifndef _WIN32
#include  <sys/socket.h>
#include  <netinet/in.h>

#if _TARGET_C3




#elif _TARGET_C1 | _TARGET_C2

#endif

#define SOCKET int
#define INVALID_SOCKET (-1)

#if _TARGET_C3


#else
#define closesocket close
#endif

static void setnonblock(int sockfd, unsigned int val)
{
#if _TARGET_C3

#elif _TARGET_C1 | _TARGET_C2

#endif
}

static int is_blocking_socket_error()
{
  int errcode = errno;
  return errcode == EINPROGRESS || errcode == EAGAIN || errcode == EWOULDBLOCK;
}

#else // _WIN32

static int is_blocking_socket_error()
{
  int errcode = WSAGetLastError();
  return errcode == WSAEWOULDBLOCK;
}

static void setnonblock(int sockfd, u_long val)
{
  ioctlsocket(sockfd, FIONBIO, &val);
}

typedef int socklen_t;

#endif // _WIN32

static SOCKET
ev_tcp_socket (void)
{
#if EV_USE_WSASOCKET
  return WSASocket (AF_INET, SOCK_STREAM, 0, 0, 0, 0);
#elif _TARGET_C3

#else
  return socket (AF_INET, SOCK_STREAM, 0);
#endif
}

#if EV_WIN32_NATIVE_PIPES
static int
ev_pipe (int filedes [2])
{
  HANDLE sock[2];
  if (!CreatePipe(&sock[0], &sock[1], NULL, 0))
  {
    ev_syserr ("(libev) error creating signal/async pipe. CreatePipe failed");
    return -1;
  }
  filedes [0] = EV_WIN32_HANDLE_TO_FD ((intptr_t)sock [0]);
  filedes [1] = EV_WIN32_HANDLE_TO_FD ((intptr_t)sock [1]);
  return 0;
}
#else
/* oh, the humanity! */
static int
ev_pipe (int filedes [2])
{
  struct sockaddr_in addr = { 0 };
  socklen_t addr_size = sizeof (addr);
  struct sockaddr_in adr2;
  socklen_t adr2_size = sizeof (adr2);
  SOCKET listener;
  SOCKET sock [2] = { -1, -1 };

  if ((listener = ev_tcp_socket ()) == INVALID_SOCKET)
    return -1;

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
  addr.sin_port = 0;

  if (bind (listener, (struct sockaddr *)&addr, addr_size))
  {
    ev_syserr ("(libev) error creating signal/async pipe. failed to bind listener");
    goto fail;
  }

  if (getsockname (listener, (struct sockaddr *)&addr, &addr_size))
  {
    ev_syserr ("(libev) error creating signal/async pipe. listener getsockname fail");
    goto fail;
  }

  if (listen (listener, 1))
  {
    ev_syserr ("(libev) error creating signal/async pipe. listen fail");
    goto fail;
  }

  if ((sock [0] = ev_tcp_socket ()) == INVALID_SOCKET)
  {
    ev_syserr ("(libev) error creating signal/async pipe. failed to create first pipe end");
    goto fail;
  }

  setnonblock(sock [0], 1);
  if (connect (sock [0], (struct sockaddr *)&addr, addr_size) && !is_blocking_socket_error())
  {
    ev_syserr ("(libev) error creating signal/async pipe. failed to connect to listener");
    goto fail;
  }

  if ((sock [1] = accept (listener, 0, 0)) == INVALID_SOCKET)
  {
    ev_syserr ("(libev) error creating signal/async pipe. failed to accept first pipe end");
    goto fail;
  }
  setnonblock(sock [0], 0);

  /* windows vista returns fantasy port numbers for sockets:
   * example for two interconnected tcp sockets:
   *
   * (Socket::unpack_sockaddr_in getsockname $sock0)[0] == 53364
   * (Socket::unpack_sockaddr_in getpeername $sock0)[0] == 53363
   * (Socket::unpack_sockaddr_in getsockname $sock1)[0] == 53363
   * (Socket::unpack_sockaddr_in getpeername $sock1)[0] == 53365
   *
   * wow! tridirectional sockets!
   *
   * this way of checking ports seems to work:
   */
  if (getpeername (sock [0], (struct sockaddr *)&addr, &addr_size))
  {
    ev_syserr ("(libev) error creating signal/async pipe. getpeername for first pipe end failed");
    goto fail;
  }

  if (getsockname (sock [1], (struct sockaddr *)&adr2, &adr2_size))
  {
    ev_syserr ("(libev) error creating signal/async pipe. getsockname for second pipe end failed");
    goto fail;
  }

  errno = EINVAL;
  if (addr_size != adr2_size
      || addr.sin_addr.s_addr != adr2.sin_addr.s_addr /* just to be sure, I mean, it's windows */
      || addr.sin_port        != adr2.sin_port)
  {
    ev_syserr ("(libev) error creating signal/async pipe. pipe addresses not equal");
    goto fail;
  }

  closesocket (listener);

#if EV_SELECT_IS_WINSOCKET
  filedes [0] = EV_WIN32_HANDLE_TO_FD (sock [0]);
  filedes [1] = EV_WIN32_HANDLE_TO_FD (sock [1]);
#else
  /* when select isn't winsocket, we also expect socket, connect, accept etc.
   * to work on fds */
  filedes [0] = sock [0];
  filedes [1] = sock [1];
#endif

  return 0;

fail:
  closesocket (listener);

  if (sock [0] != INVALID_SOCKET) closesocket (sock [0]);
  if (sock [1] != INVALID_SOCKET) closesocket (sock [1]);

  return -1;
}
#endif

#undef pipe
#define pipe(filedes) ev_pipe (filedes)

