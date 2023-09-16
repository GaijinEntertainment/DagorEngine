#include <daNet/internalIp.h>

#include <osApiWrappers/dag_miscApi.h>
#if _TARGET_PC_WIN | _TARGET_XBOX
#include <Winsock2.h>
#include <WS2tcpip.h>
#define CLOSESOCKET closesocket
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#if _TARGET_C3

#else
#define CLOSESOCKET close
#endif
typedef int SOCKET;
#define INVALID_SOCKET -1

#endif

namespace danet
{

bool get_internal_ip(char *buf, size_t buf_size)
{
  bool ret = false;
#if _TARGET_PC_WIN
  WSADATA wd;
  if (WSAStartup(2, &wd) == 0)
#endif
  {
    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s != INVALID_SOCKET)
    {
      sockaddr_in addr;
      addr.sin_addr.s_addr = ::string_to_ip("8.8.8.8"); // just any public address
      addr.sin_port = 1234;                             // we don't care which one to choose
      addr.sin_family = AF_INET;
      if (connect(s, (sockaddr *)&addr, sizeof(addr)) == 0)
      {
        socklen_t size = sizeof(addr);
        if (getsockname(s, (sockaddr *)&addr, &size) == 0)
        {
#if _TARGET_PC_WIN // inet_ntop() not supported on Windows XP
          char *a = inet_ntoa(addr.sin_addr);
          ret = a != NULL;
          if (a)
            strncpy(buf, a, buf_size);
#else
          ret = inet_ntop(AF_INET, &addr.sin_addr, buf, buf_size) != NULL;
#endif
          buf[buf_size - 1] = '\0';
        }
      }
      CLOSESOCKET(s);
    }
#if _TARGET_PC_WIN
    WSACleanup();
#endif
  }
  return ret;
}


} // namespace danet
