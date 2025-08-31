// Copyright (C) Gaijin Games KFT.  All rights reserved.

// TCP/UDP client using Winsock 2.2

#include <util/dag_globDef.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sockets/dag_netClient.h>
#include <sockets/dag_netPacket.h>
#include <ioSys/dag_dataBlock.h>
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <ioSys/dag_memIo.h>
#include <debug/dag_hwExcept.h>
#include "ns_private.h"
#include <process.h>
#include <memory/dag_mem.h>

#define TRACE_PACKETS 0

//
// This code assumes that at the transport level, the system only supports
// one stream protocol (TCP) and one datagram protocol (UDP).  Therefore,
// specifying a socket type of SOCK_STREAM is equivalent to specifying TCP
// and specifying a socket type of SOCK_DGRAM is equivalent to specifying UDP.
//

#define DEFAULT_FAMILY   PF_INET     // Accept either IPv4 or IPv6
#define DEFAULT_SOCKTYPE SOCK_STREAM // TCP
#define DEFAULT_PORT     "5001"      // Arbitrary, albiet a historical test port

#define MAX_STATIC_PACKET_SIZE 2048

class WinSock2NetworkClient : public NetSocketsClient
{
public:
  INetSocketsClientProcessor *proc;
  HANDLE clientThread, clientThreadUdp;
  Tab<String> regFileWC;
  bool useShares, rawMode;
  WinCritSec connCC, sendCC;
  volatile bool running;
  bool serverIsLocal;
  char *udpBuf;
  int udpBufSize;
  bool tcpNoDelayMode;

  // WinSock32 specific members
  char portBuf[128], adrBuf[128], portBufUDP[128];
  char hostName[NI_MAXHOST];
  int family, socketType;
  const char *port, *portUdp, *ServerAddress;

  ADDRINFO hints, *addrInfo, *AI;
  SOCKET connSock, connSockUdp;

  WinSock2NetworkClient() :
    proc(NULL),
    clientThread(NULL),
    clientThreadUdp(NULL),
    running(false),
    serverIsLocal(false),
    family(DEFAULT_FAMILY),
    socketType(DEFAULT_SOCKTYPE),
    port(DEFAULT_PORT),
    portUdp(NULL),
    ServerAddress(NULL),
    regFileWC(strmem_ptr()),
    udpBuf(NULL),
    udpBufSize(0)
  {
    regFileWC.reserve(16);
    connSock = NULL;
    connSockUdp = NULL;
    useShares = true;
    rawMode = false;
    tcpNoDelayMode = false;
  }

  ~WinSock2NetworkClient()
  {
    G_ASSERT(!running);
    if (udpBuf)
    {
      memfree(udpBuf, midmem);
      udpBuf = NULL;
      udpBufSize = 0;
    }
  }

  virtual void setUseRegistryShares(bool on) { useShares = on; }

  void destroy()
  {
    setProcessor(NULL);
    delete this;
  }

  void setProcessor(INetSocketsClientProcessor *nssp)
  {
    if (proc)
    {
      proc->detach();
      proc->client = NULL;
    }
    proc = nssp;
    if (proc)
    {
      proc->client = this;
      proc->attach();
    }
  }

  void setup(const DataBlock &blk)
  {
    // parse config
    const char *s;
    s = blk.getStr("family", "PF_INET");
    if (stricmp(s, "PF_INET") == 0)
      family = PF_INET;
    else if (stricmp(s, "PF_INET6") == 0)
      family = PF_INET6;
    else if (stricmp(s, "PF_UNSPEC") == 0)
      family = PF_UNSPEC;
    else
      DAG_FATAL("family=%s", s);

    s = blk.getStr("socket_type", "TCP");
    if (stricmp(s, "TCP") == 0)
      socketType = SOCK_STREAM;
    else if (stricmp(s, "UDP") == 0)
      socketType = SOCK_DGRAM;
    else
      DAG_FATAL("socket_type=%s", s);

    s = blk.getStr("address", ServerAddress);
    if (s)
    {
      if (stricmp(s, "local_host") == 0)
        ServerAddress = NULL;
      else
      {
        strcpy(adrBuf, s);
        ServerAddress = adrBuf;
      }
    }

    s = blk.getStr("port", port);
    if (s)
    {
      strcpy(portBuf, s);
      port = portBuf;
    }
    s = blk.getStr("port_udp", portUdp);
    if (s)
    {
      strcpy(portBufUDP, s);
      portUdp = portBufUDP;
      DEBUG_CTX("additional UDP port %s is scheduled", portUdp);
    }

    tcpNoDelayMode = blk.getBool("tcpNoDelayMode", false);

    if (udpBuf)
      memfree(udpBuf, midmem);

    udpBufSize = blk.getInt("max_udp_packet_size", 1024);
    if (udpBufSize < 0)
      udpBufSize = 0;
    udpBuf = (char *)memalloc(udpBufSize, midmem);

    if (useShares)
    {
      regFileWC.resize(1);
      regFileWC[0] = String(0, "%s/*", blk.getStr("registry", ""));
      for (int i = 0; i < 16; i++)
      {
        const char *reg = blk.getStr(String(32, "registry_%d", i), NULL);
        if (!reg)
          continue;

        append_items(regFileWC, 1);
        regFileWC.back() = String(0, "%s/*", reg);
      }
    }
  }
  void enumerateServers(Tab<NetServerInfo> &svr)
  {
    svr.clear();
    if (TRACE_PACKETS)
      debug("---{ Enumerate servers start");
    WIN32_FIND_DATA find_data;
    HANDLE h;

    G_ASSERT(useShares);

    for (int i = 0; i < regFileWC.size(); i++)
    {
      h = FindFirstFileA(regFileWC[i], &find_data);
      if (h == INVALID_HANDLE_VALUE)
      {
        int ret = GetLastError();
        debug("[*] found nothing with wildcard: <%s>, error %p: %s", (char *)regFileWC[i], ret, NetSockets::decode_error(ret));
      }
      else
      {
        do
        {
          if (!(find_data.dwFileAttributes &
                (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_TEMPORARY | FILE_ATTRIBUTE_ENCRYPTED | FILE_ATTRIBUTE_OFFLINE |
                  FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_REPARSE_POINT)))
          {
            bool found = false;
            for (int j = 0; j < svr.size(); j++)
              if (stricmp(svr[j].address, find_data.cFileName) == 0)
              {
                found = true;
                break;
              }
            if (found)
              continue;

            int l = append_items(svr, 1);
            if (!getServerInfo(find_data.cFileName, port, family, socketType, svr[l]))
              erase_items(svr, l, 1);
          }
        } while (FindNextFile(h, &find_data));
        FindClose(h);
      }
    }

    if (TRACE_PACKETS)
      debug("---} Enumerate servers done");
  }

  bool connectToServer(const char *server_addr, bool raw_mode)
  {
    G_ASSERT(!running);

    running = false;

    // Initialize WinSock
    int i, ret, addrLen;
    char addrName[NI_MAXHOST];
    struct sockaddr_storage Addr;

    if (strlen(server_addr) >= sizeof(adrBuf))
    {
      DEBUG_CTX("--- too long server_adr: <%s>", server_addr);
      return false;
    }
    if (stricmp(server_addr, "local_host") == 0)
      ServerAddress = NULL;
    else
    {
      strcpy(adrBuf, server_addr);
      ServerAddress = adrBuf;
    }

    // Ask for Winsock version 2.2.
    if (!NetSockets::winsock2_init())
      return false;

    //
    // By not setting the AI_PASSIVE flag in the hints to getaddrinfo, we're
    // indicating that we intend to use the resulting address(es) to connect
    // to a service.  This means that when the Server parameter is NULL,
    // getaddrinfo will return one entry per allowed protocol family
    // containing the loopback address for that family.
    //

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = family;
    hints.ai_socktype = socketType;
    ret = getaddrinfo(ServerAddress, port, &hints, &addrInfo);
    if (ret != 0)
    {
      DEBUG_CTX("--- Cannot resolve address [%s] and port [%s], error %d: %s", ServerAddress, port, ret, gai_strerror(ret));
      NetSockets::winsock2_term();
      return false;
    }

    //
    // Try each address getaddrinfo returned, until we find one to which
    // we can sucessfully connect.
    //
    for (AI = addrInfo; AI != NULL; AI = AI->ai_next)
    {
      // Open a socket with the correct address family for this address.
      connSock = socket(AI->ai_family, AI->ai_socktype, AI->ai_protocol);
      if (connSock == INVALID_SOCKET)
      {
        DEBUG_CTX("--- Error Opening socket, error %d: %s", WSAGetLastError(), NetSockets::decode_error(WSAGetLastError()));
        continue;
      }


      {
        WinAutoLock lock(connCC);
        if (connect(connSock, AI->ai_addr, (int)AI->ai_addrlen) != SOCKET_ERROR)
          break;
      }

      i = WSAGetLastError();
      if (getnameinfo(AI->ai_addr, (int)AI->ai_addrlen, addrName, sizeof(addrName), NULL, 0, NI_NUMERICHOST) != 0)
        strcpy(addrName, "<unknown>");

      DEBUG_CTX("--- connect() to %s failed with error %d: %s", addrName, i, NetSockets::decode_error(i));
      close_socket();
    }

    if (AI == NULL)
    {
      DEBUG_CTX("--- Fatal error: unable to connect to the server %s", ServerAddress ? ServerAddress : "local_host");
      NetSockets::winsock2_term();
      return false;
    }

    //
    // This demonstrates how to determine to where a socket is connected.
    //
    addrLen = sizeof(Addr);
    if (getpeername(connSock, (LPSOCKADDR)&Addr, &addrLen) == SOCKET_ERROR)
    {
      DEBUG_CTX("--- getpeername() failed with error %d: %s", WSAGetLastError(), NetSockets::decode_error(WSAGetLastError()));
    }
    else
    {
      if (getnameinfo((LPSOCKADDR)&Addr, addrLen, addrName, sizeof(addrName), NULL, 0, NI_NUMERICHOST) != 0)
        strcpy(addrName, "<unknown>");
      debug("CLIENT: Connected to %s, port %d (%p, %d)", addrName, ntohs(SS_PORT(&Addr)), this, connSock);
    }

    // We are done with the address info chain, so we can free it.
    freeaddrinfo(addrInfo);

    //
    // Find out what local address and port the system picked for us.
    //
    addrLen = sizeof(Addr);
    if (getsockname(connSock, (LPSOCKADDR)&Addr, &addrLen) == SOCKET_ERROR)
    {
      debug("--- getsockname() failed with error %d: %s", WSAGetLastError(), NetSockets::decode_error(WSAGetLastError()));
    }
    else
    {
      if (getnameinfo((LPSOCKADDR)&Addr, addrLen, addrName, sizeof(addrName), NULL, 0, NI_NUMERICHOST) != 0)
        strcpy(addrName, "<unknown>");
      debug("CLIENT: Using local address %s, port %d", addrName, ntohs(SS_PORT(&Addr)));
    }

    if (portUdp)
    {
      memset(&hints, 0, sizeof(hints));
      hints.ai_family = family;
      hints.ai_socktype = SOCK_DGRAM;
      ret = getaddrinfo(ServerAddress, portUdp, &hints, &addrInfo);
      if (ret != 0)
      {
        DEBUG_CTX("--- Cannot resolve address [%s] and port [%s], error %d: %s", ServerAddress, port, ret, gai_strerror(ret));
        NetSockets::winsock2_term();
        return false;
      }

      //
      // Try each address getaddrinfo returned, until we find one to which
      // we can sucessfully connect.
      //
      for (AI = addrInfo; AI != NULL; AI = AI->ai_next)
      {
        // Open a socket with the correct address family for this address.
        connSockUdp = socket(AI->ai_family, AI->ai_socktype, AI->ai_protocol);
        if (connSockUdp == INVALID_SOCKET)
        {
          DEBUG_CTX("--- Error Opening UDP socket, error %d: %s", WSAGetLastError(), NetSockets::decode_error(WSAGetLastError()));
          continue;
        }


        {
          WinAutoLock lock(connCC);
          if (connect(connSockUdp, AI->ai_addr, (int)AI->ai_addrlen) != SOCKET_ERROR)
            break;
        }

        i = WSAGetLastError();
        if (getnameinfo(AI->ai_addr, (int)AI->ai_addrlen, addrName, sizeof(addrName), NULL, 0, NI_NUMERICHOST) != 0)
          strcpy(addrName, "<unknown>");

        DEBUG_CTX("--- connect() to %s failed with error %d: %s", addrName, i, NetSockets::decode_error(i));
        close_socket();
      }

      if (AI == NULL)
      {
        DEBUG_CTX("--- Fatal error: unable to connect to the server %s", ServerAddress ? ServerAddress : "local_host");
        NetSockets::winsock2_term();
        return false;
      }

      //
      // This demonstrates how to determine to where a socket is connected.
      //
      addrLen = sizeof(Addr);
      if (getpeername(connSockUdp, (LPSOCKADDR)&Addr, &addrLen) == SOCKET_ERROR)
      {
        DEBUG_CTX("--- getpeername() failed with error %d: %s", WSAGetLastError(), NetSockets::decode_error(WSAGetLastError()));
      }
      else
      {
        if (getnameinfo((LPSOCKADDR)&Addr, addrLen, addrName, sizeof(addrName), NULL, 0, NI_NUMERICHOST) != 0)
          strcpy(addrName, "<unknown>");
        debug("CLIENT: Connected to %s, port %d", addrName, ntohs(SS_PORT(&Addr)));
      }

      // We are done with the address info chain, so we can free it.
      freeaddrinfo(addrInfo);

      //
      // Find out what local address and port the system picked for us.
      //
      addrLen = sizeof(Addr);
      if (getsockname(connSockUdp, (LPSOCKADDR)&Addr, &addrLen) == SOCKET_ERROR)
      {
        debug("--- getsockname() failed with error %d: %s", WSAGetLastError(), NetSockets::decode_error(WSAGetLastError()));
      }
      else
      {
        if (getnameinfo((LPSOCKADDR)&Addr, addrLen, addrName, sizeof(addrName), NULL, 0, NI_NUMERICHOST) != 0)
          strcpy(addrName, "<unknown>");
        debug("CLIENT: Using local address %s, port %d", addrName, ntohs(SS_PORT(&Addr)));
      }
    }


    if (connSock && tcpNoDelayMode)
    {
      debug("setting TCP_NODELAY mode...");
      BOOL trueVal = TRUE;
      if (setsockopt(connSock, IPPROTO_TCP, TCP_NODELAY, (char *)&trueVal, sizeof(BOOL)) == SOCKET_ERROR)
        debug("cannot set socket option: TCP_NODELAY");
      else
        debug("OK");
    };

    if (portUdp && raw_mode)
    {
      raw_mode = false;
      debug("CLIENT: UDP doesn't support raw mode!");
    }


    // asynchronous execution (another thread)
    running = true;
    rawMode = raw_mode;

    clientThread = (HANDLE)_beginthreadex(NULL, 256 << 10, startThread, this, 0, NULL);
    if (!clientThread)
    {
      running = false;
      return false;
    }
    if (portUdp)
    {
      clientThreadUdp = (HANDLE)_beginthreadex(NULL, 256 << 10, startThreadUdp, this, 0, NULL);
      if (!clientThreadUdp)
      {
        running = false;
        return false;
      }
    }

    return true;
  }

  bool disconnectFromServer(bool _fatal)
  {
    if (!running && !(connSock || connSockUdp))
      return true;

    running = false;
    if (connSock)
    {
      shutdown(connSock, SD_SEND);
      connSock = NULL;
    }
    if (connSockUdp)
    {
      shutdown(connSockUdp, SD_SEND);
      connSockUdp = NULL;
    }

    if (connSock || connSockUdp)
    {
      debug("shutdown");
      close_socket();
    }

    if (!waitThreadTermination(500))
    {
      DEBUG_CTX("--- can't wait for client termination for more than %d ms", 500);
      if (!waitThreadTermination(3500))
      {
        if (_fatal)
          DAG_FATAL("can't wait for client termination for more than %d ms", 4000);
        else
        {
          DEBUG_CTX("can't wait for client termination for more than %d ms", 4000);
          return false;
        }
      }
    }

    NetSockets::winsock2_term();
    return true;
  }

  bool isConnected() { return running; }
  bool isServerLocal()
  {
    if (!running)
      return false;
    return serverIsLocal;
  }

  int sendPacket(const NetMsgPacket *p)
  {
    if (rawMode)
      DAG_FATAL("packet");
    if (TRACE_PACKETS)
      debug("CLIENT: send packet (size=%d gen=%p queryId=%d", p->hdr.size, p->hdr.generation, p->queryId);
    return sendBytes(p, p->hdr.size + sizeof(p->hdr), true);
  }
  int sendPacketUdp(const NetMsgPacket *p)
  {
    if (TRACE_PACKETS)
      debug("CLIENT: send UDP packet (size=%d gen=%p queryId=%d", p->hdr.size, p->hdr.generation, p->queryId);
    return send(connSockUdp, (const char *)p, sizeof(p->hdr) + p->hdr.size, 0);
  }
  int sendRawBytes(const void *data, int byte_num, bool whole_block)
  {
    if (TRACE_PACKETS)
      debug("CLIENT: send raw bytes (size=%d)", byte_num);
    return sendBytes(data, byte_num, whole_block);
  }

  void close_socket()
  {
    WinAutoLock lock(connCC);
    if (connSock)
    {
      closesocket(connSock);
      connSock = NULL;
    }
    if (connSockUdp)
    {
      closesocket(connSockUdp);
      connSockUdp = NULL;
    }
  }

  int clientLoop()
  {
    char msg_buf[MAX_STATIC_PACKET_SIZE + sizeof(NetMsgPacketHeader)];
    NetMsgPacket *p;
    NetMsgPacketHeader ph;
    int ret;

    if (proc)
      proc->connected();
    sendCheckLocalRequest();

    while (running)
    {
      ret = recvBytes(&ph, sizeof(ph), true);
      if (ret <= 0 || !running)
        break;

      if (ph.size < MAX_STATIC_PACKET_SIZE)
      {
        // short messages are mapped onto msg buffer
        p = NetMsgPacket::create(msg_buf, ph.size);
      }
      else
      {
        // long messages are allocated from heap
        p = NetMsgPacket::create(ph.size, midmem);
      }
      p->hdr = ph;

      if (ph.size > 0)
      {
        ret = recvBytes(p->data, ph.size, true);
        if (ret <= 0 || !running)
          break;
      }

      // dumpPacket ( p );

      if (ph.generation == NGEN_Query)
      {
        // received Short Query Packet
        if (TRACE_PACKETS)
          debug("CLIENT: received query (size=%d query=%d)", ph.size, p->queryId);
        if (proc)
          proc->recvQuery(*(NetShortQueryPacket *)p);

        if (ph.size >= MAX_STATIC_PACKET_SIZE)
          delete p;
      }
      else if (ph.generation == NGEN_Response)
      {
        // received Response Packet
        if (TRACE_PACKETS)
          debug("CLIENT: received response (size=%d query=%d)", ph.size, p->queryId);

        if (p->queryId < NSQP_SERVER_SPEC_QUERY_NUM)
          recvResponsePacket(*(NetShortQueryPacket *)p);
        else if (proc)
          proc->recvResponse(*(NetShortQueryPacket *)p);

        if (ph.size >= MAX_STATIC_PACKET_SIZE)
          delete p;
      }
      else
      {
        if (TRACE_PACKETS)
          debug("CLIENT: receive packet (size=%d gen=%p queryId=%d", p->hdr.size, p->hdr.generation, p->queryId);
        if (proc)
          proc->recvPacket(p, ph.size < MAX_STATIC_PACKET_SIZE);
        else if (ph.size >= MAX_STATIC_PACKET_SIZE)
          delete p;
      }
    }

    running = false;
    if (proc)
      proc->disconnected();

    return 0;
  }

  int clientLoopUdp()
  {
    NetShortQueryPacket *p;
    int ret;

    while (running && connSockUdp)
    {
      ret = recv(connSockUdp, udpBuf, udpBufSize, 0);
      if (ret == SOCKET_ERROR)
      {
        DEBUG_CTX("recv() failed with error %d: %s", WSAGetLastError(), NetSockets::decode_error(WSAGetLastError()));
        closesocket(connSockUdp);
        break;
      }
      //
      // We are not likely to see this with UDP, since there is no
      // 'connection' established.
      //
      if (ret == 0)
      {
        DEBUG_CTX("Server closed connection");
        closesocket(connSockUdp);
        break;
      }

      p = (NetShortQueryPacket *)udpBuf;
      if (p->hdr.size + sizeof(p->hdr) != ret)
        DEBUG_CTX("broken packet arrived: p->size=%d p->gen=%d size=%d", p->hdr.size, p->hdr.generation, ret);
      else
      {
        if (proc)
          proc->datagramReceived(*p);
      }
    }
    DEBUG_CTX("udp loop exited!");
    return 0;
  }

  int clientLoopRaw()
  {
    char msg_buf[MAX_STATIC_PACKET_SIZE];
    int ret;

    if (proc)
      proc->connected();

    while (running)
    {
      fd_set rfd, efd;

      FD_ZERO(&rfd);
      FD_ZERO(&efd);

      FD_SET(connSock, &rfd);
      FD_SET(connSock, &efd);

      timeval timeout;
      timeout.tv_sec = 0;
      timeout.tv_usec = 50;

      if (select(1, &rfd, NULL, &efd, &timeout) < 0)
        break;

      if (rfd.fd_count > 0)
      {
        ret = recvBytes(msg_buf, sizeof(msg_buf), false);
        if (ret <= 0 || !running)
          break;
        proc->recvRawBytes(msg_buf, ret);
      }

      int buffLen = proc->fillSendBuffer(msg_buf, MAX_STATIC_PACKET_SIZE);
      if (buffLen > 0)
      {
        ret = sendBytes(msg_buf, buffLen, true);
        if (ret <= 0 || !running)
          break;
      }

      if (buffLen < 0)
      {
        running = false;
      }
    }

    running = false;
    if (proc)
      proc->disconnected();

    return 0;
  }

  void recvResponsePacket(NetShortQueryPacket &p)
  {
    switch (p.queryId)
    {
      case NSQP_GetInfo:
      {
        // forward this packet to processor
        if (proc)
          proc->recvResponse(p);
      }
      break;
      case NSQP_CheckLocal:
      {
        serverIsLocal = p.dwData[1];
        debug("received response on NSQP_CheckLocal: %d", serverIsLocal);
      }
      break;
    }
  }
  void sendCheckLocalRequest()
  {
    DynamicMemGeneralSaveCB cwr(tmpmem, 0, 2 << 10);
    NetMsgPacketHeader hdr;
    NetMsgPacket *rp;

    hdr.size = 0;
    hdr.generation = NGEN_Query;

    cwr.write(&hdr, sizeof(hdr));
    cwr.writeInt(NSQP_CheckLocal);
    cwr.writeString(getenv("COMPUTERNAME"));

    rp = NetMsgPacket::create(cwr.data(), cwr.size() - sizeof(hdr));
    sendPacket(rp);
  }

  bool waitThreadTermination(int timeout)
  {
    DWORD dw;
    if (!clientThread && !clientThreadUdp)
      return true;

    while (GetExitCodeThread(clientThread, &dw) && timeout > 0)
    {
      if (dw != STILL_ACTIVE)
        break;
      Sleep(100);
      timeout -= 100;
    }
    CloseHandle(clientThread);
    clientThread = NULL;

    if (clientThreadUdp)
    {
      while (GetExitCodeThread(clientThreadUdp, &dw) && timeout > 0)
      {
        if (dw != STILL_ACTIVE)
          break;
        Sleep(100);
        timeout -= 100;
      }
      CloseHandle(clientThreadUdp);
      clientThreadUdp = NULL;
    }

    return timeout > 0;
  }

  static unsigned int __stdcall startThread(void *p)
  {
    WinSock2NetworkClient *cli = (WinSock2NetworkClient *)p;
    int ret = 0, id = DagorHwException::setHandler("client tcp");
    ::win32_set_thread_name("Client-TCP");

    DAGOR_TRY
    {
      // guarded thread procedure
      ret = cli->rawMode ? cli->clientLoopRaw() : cli->clientLoop();
    }
    DAGOR_CATCH(DagorException e)
    {
#ifdef DAGOR_EXCEPTIONS_ENABLED
      DagorHwException::reportException(e, true, "except_thread");
#endif
    }
    DagorHwException::removeHandler(id);
    return ret;
  }
  static unsigned int __stdcall startThreadUdp(void *p)
  {
    WinSock2NetworkClient *cli = (WinSock2NetworkClient *)p;
    int ret = 0, id = DagorHwException::setHandler("client udp");
    ::win32_set_thread_name("Client-UDP");

    DAGOR_TRY
    {
      // guarded thread procedure
      ret = cli->clientLoopUdp();
    }
    DAGOR_CATCH(DagorException e)
    {
#ifdef DAGOR_EXCEPTIONS_ENABLED
      DagorHwException::reportException(e, true, "except_thread");
#endif
    }
    DagorHwException::removeHandler(id);
    return ret;
  }

  // Low-level send/receive
  int recvBytes(void *_p, int buf_sz, bool whole_block)
  {
    if (!connSock)
    {
      DEBUG_CTX("--- recvBytes ( %p, %d ), while socket not connected", _p, buf_sz);
      return -1;
    }

    int ret, ret_sz = 0;
    char *p = (char *)_p;

    while (buf_sz > 0)
    {
      ret = recv(connSock, p, buf_sz > MAX_PACKET_BURST_SIZE ? MAX_PACKET_BURST_SIZE : buf_sz, 0);

      if (ret == SOCKET_ERROR)
      {
        if (running)
        {
          DEBUG_CTX("--- %p.recv(%d) failed with error %d: %s", this, connSock, WSAGetLastError(),
            NetSockets::decode_error(WSAGetLastError()));
        }
        close_socket();
        return -1;
      }
      if (ret == 0)
      {
        close_socket();
        return 0;
      }

      buf_sz -= ret;
      ret_sz += ret;
      p += ret;

      if (!whole_block || !running)
        break;
    }
    return ret_sz;
  }
  int sendBytes(const void *_p, int sz, bool whole_block)
  {
    if (!connSock)
    {
      DEBUG_CTX("--- %p.sendBytes ( %d, %p, %d ), while socket not connected", this, connSock, _p, sz);
      if (running)
        disconnectFromServer(false);
      return -1;
    }
    WinAutoLock lock(sendCC);

    int ret, ret_sz = 0, unit_sz = sz;
    char *p = (char *)_p;
    if (unit_sz > MAX_PACKET_BURST_SIZE)
      unit_sz = MAX_PACKET_BURST_SIZE;

    while (sz > 0)
    {
      ret = send(connSock, (char *)p, unit_sz, 0);
      if (ret == SOCKET_ERROR)
      {
        DEBUG_CTX("--- send() failed: error %d: %s", WSAGetLastError(), NetSockets::decode_error(WSAGetLastError()));
        close_socket();
        return -1;
      }

      sz -= ret;
      ret_sz += ret;
      p += ret;
      if (unit_sz > sz)
        unit_sz = sz;

      if (!whole_block || !running)
        break;
    }
    return ret_sz;
  }

  void dumpPacket(const NetMsgPacket *p)
  {
    static int packet_n = 0;
    FILE *fp = fopen(String(128, "c%08d_sz%p_gen%p_qid%d.msg", packet_n++, p->hdr.size, p->hdr.generation, p->queryId), "wb");
    if (fp)
    {
      fwrite(p, sizeof(p->hdr) + p->hdr.size, 1, fp);
      fclose(fp);
    }
  }
};

NetSocketsClient *NetSocketsClient::make()
{
  NetSockets::load_debug_info();
  return new (midmem) WinSock2NetworkClient;
}

//
// get unified server information (connect/send query/receive info/disconnect)
//
class EnumeratorClientProcessor : public INetSocketsClientProcessor
{
public:
  volatile bool answered;
  NetServerInfo *info;

  const char *getUserFriendlyName() { return "Server Enumerator"; }

  void destroy() {}
  void attach() { answered = false; }
  void detach() { answered = false; }

  void recvPacket(NetMsgPacket *p, bool packet_static)
  {
    if (!packet_static)
      delete p;
  }

  void recvQuery(NetShortQueryPacket &p) {}

  void recvResponse(NetShortQueryPacket &p)
  {
    if (p.queryId != NSQP_GetInfo)
      return;

    if (info)
    {
      InPlaceMemLoadCB crd(p.dwData + 1, p.hdr.size - 4);

      info->clientsNum = crd.readInt() - 1;
      info->state = crd.readInt();
      crd.readString(info->userFriendlyName);
    }

    answered = true;
  }
  void datagramReceived(NetShortQueryPacket &p) {}
  void recvRawBytes(const void *p, int len) {}

  void connected() {}
  void disconnected() {}
};

bool NetSocketsClient::getServerInfo(const char *address, const char *port, int family, int socketType, NetServerInfo &info)
{
  NetSockets::load_debug_info();

  WinSock2NetworkClient *conn_test_client = new (tmpmem) WinSock2NetworkClient;
  EnumeratorClientProcessor conn_test_proc;

  conn_test_client->family = family;
  conn_test_client->socketType = socketType;
  conn_test_client->port = conn_test_client->portBuf;
  strcpy(conn_test_client->portBuf, port);

  conn_test_proc.info = &info;
  conn_test_client->setProcessor(&conn_test_proc);

  conn_test_client->connectToServer(address, false);
  if (conn_test_client->isConnected())
  {
    NetShortQueryPacket qp(NSQP_GetInfo);
    int attempts = 80, ms_left = 0;

    conn_test_client->sendPacket(&qp);

    while (attempts && !conn_test_proc.answered)
    {
      Sleep(20);
      attempts--;
      ms_left += 20;
    }

    conn_test_client->disconnectFromServer(false);
    conn_test_client->setProcessor(NULL);

    info.address = address;
    conn_test_client->destroy();
    return (attempts > 0);
  }

  conn_test_client->setProcessor(NULL);
  debug("ENUMERATOR: not connected! server is missing?  %s:%s", address, port);
  conn_test_client->destroy();
  return false;
}
