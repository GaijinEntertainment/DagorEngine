// Copyright (C) Gaijin Games KFT.  All rights reserved.

// TCP/UDP server using Winsock 2.2

#include <util/dag_globDef.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sockets/dag_netServer.h>
#include <sockets/dag_netPacket.h>
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_tab.h>
#include <debug/dag_debug.h>
#include <ioSys/dag_memIo.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <debug/dag_hwExcept.h>
#include "ns_private.h"
#include <process.h>
#include <util/dag_string.h>
#include <memory/dag_mem.h>

#define TRACE_PACKETS 0

#define DEFAULT_FAMILY   PF_INET     // Accept either IPv4 or IPv6
#define DEFAULT_SOCKTYPE SOCK_STREAM // TCP
#define DEFAULT_PORT     "5001"      // Arbitrary, albiet a historical test port

#define MAX_STATIC_PACKET_SIZE 2048

//
// Implementation of server on Windows Sockets 2.0
//
class WinSock2NetworkServer : public NetSocketsServer
{
  struct StartConnThreadData
  {
    WinSock2NetworkServer *svr;
    INetSocketsServerProcessor *me;
    const char *host_name;
    int socket;
  };

public:
  INetSocketsServerProcessor *proc;
  HANDLE serverThread, workThread;
  Tab<String> regFile;
  bool useShares, rawMode;
  int clientsNum;
  volatile bool running;
  int refCount;
  WinCritSec refCC, connCC;
  WinCritSec sendCC[NET_MAX_CONNECTIONS];
  char *udpBuf;
  int udpBufSize;

  // WinSock32 specific members
  char portBuf[128], portBufUDP[128], adrBuf[128];
  char hostName[NI_MAXHOST];
  int family, socketType;
  char *port, *portUdp, *address;

  int numSocks, udpSocksStart;
  ADDRINFO hints, *addrInfo, *AI;
  SOCKET servSock[FD_SETSIZE], connSock[NET_MAX_CONNECTIONS];
  fd_set sockSet;

public:
  WinSock2NetworkServer() :
    proc(NULL),
    running(false),
    serverThread(NULL),
    workThread(NULL),
    family(DEFAULT_FAMILY),
    socketType(DEFAULT_SOCKTYPE),
    port(DEFAULT_PORT),
    portUdp(NULL),
    address(NULL),
    refCount(1),
    regFile(strmem_ptr()),
    udpSocksStart(0),
    udpBuf(NULL),
    udpBufSize(0)
  {
    memset(servSock, 0, sizeof(servSock));
    memset(connSock, 0, sizeof(connSock));
    clientsNum = 0;
    regFile.reserve(16);
    useShares = true;
    rawMode = false;
  }

  ~WinSock2NetworkServer()
  {
    DEBUG_CTX("destroying server: running=%d", running);
    G_ASSERT(!running);
  }

  virtual void setUseRegistryShares(bool on) { useShares = on; }

  void addRef()
  {
    if (!this)
      return;
    WinAutoLock lock(refCC);
    refCount++;
    // debug("SERVER::addRef: refCount=%d", refCount);
  }
  bool delRef()
  {
    if (!this)
      return false;
    WinAutoLock lock(refCC);
    refCount--;
    // debug("SERVER::delRef: refCount=%d", refCount);
    return refCount == 0;
  }
  int getRefCount()
  {
    if (!this)
      return 1;
    WinAutoLock lock(refCC);
    return refCount;
  }
  void destroy()
  {
    if (!this)
      return;
    if (delRef())
      destroyServer();
  }
  void destroyServer()
  {
    if (!this)
      return;
    setProcessor(NULL);
    delete this;
  }

  void setProcessor(INetSocketsServerProcessor *nssp)
  {
    if (proc)
    {
      proc->detach();
      proc->server = NULL;
    }
    proc = nssp;
    if (proc)
    {
      proc->server = this;
      proc->attach();
    }
  }

  bool start(const DataBlock &blk)
  {
    G_ASSERT(!running);

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

    rawMode = blk.getBool("tcpRaw", false);

    /*    String qcn ( getenv("COMPUTERNAME"));
        if ( getenv ( "USERDNSDOMAIN" ))
          qcn += String(".") + getenv ( "USERDNSDOMAIN" ); */

    s = blk.getStr("address", getenv("COMPUTERNAME"));
    if (s)
    {
      if (stricmp(s, "local_host") == 0)
        address = NULL;
      else
      {
        strcpy(adrBuf, s);
        address = adrBuf;
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

    udpBufSize = blk.getInt("max_udp_packet_size", 1024);
    if (udpBufSize < 0)
      udpBufSize = 0;
    udpBuf = (char *)memalloc(udpBufSize, midmem);

    // Initialize WinSock
    int val, i;

    // Ask for Winsock version 2.2.
    if (!NetSockets::winsock2_init())
      return false;

    if (port == NULL)
    {
      return false;
    }

    //
    // By setting the AI_PASSIVE flag in the hints to getaddrinfo, we're
    // indicating that we intend to use the resulting address(es) to bind
    // to a socket(s) for accepting incoming connections.  This means that
    // when the address parameter is NULL, getaddrinfo will return one
    // entry per allowed protocol family containing the unspecified address
    // for that family.
    //
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = family;
    hints.ai_socktype = socketType;
    hints.ai_flags = AI_PASSIVE;
    val = getaddrinfo(address, port, &hints, &addrInfo);
    if (val != 0)
    {
      DEBUG_CTX("--- getaddrinfo (%s) failed with error %d: %s", address, val, gai_strerror(val));
      NetSockets::winsock2_term();
      return false;
    }

    //
    // For each address getaddrinfo returned, we create a new socket,
    // bind that address to it, and create a queue to listen on.
    //
    for (i = 0, AI = addrInfo; AI != NULL; AI = AI->ai_next, i++)
    {
      // Highly unlikely, but check anyway.
      if (i >= FD_SETSIZE)
      {
        debug("--- getaddrinfo returned more addresses than we could use.");
        break;
      }

      // This example only supports PF_INET and PF_INET6.
      if ((AI->ai_family != PF_INET) && (AI->ai_family != PF_INET6))
        continue;

      // Open a socket with the correct address family for this address.
      servSock[i] = socket(AI->ai_family, AI->ai_socktype, AI->ai_protocol);
      if (servSock[i] == INVALID_SOCKET)
      {
        DEBUG_CTX("--- socket() failed with error %d: %s", WSAGetLastError(), NetSockets::decode_error(WSAGetLastError()));
        continue;
      }

      //
      // bind() associates a local address and port combination
      // with the socket just created. This is most useful when
      // the application is a server that has a well-known port
      // that clients know about in advance.
      //
      if (bind(servSock[i], AI->ai_addr, (int)AI->ai_addrlen) == SOCKET_ERROR)
      {
        DEBUG_CTX("--- bind() failed with error %d: %s", WSAGetLastError(), NetSockets::decode_error(WSAGetLastError()));
        continue;
      }

      //
      // So far, everything we did was applicable to TCP as well as UDP.
      // However, there are certain fundamental differences between stream
      // protocols such as TCP and datagram protocols such as UDP.
      //
      // Only connection orientated sockets, for example those of type
      // SOCK_STREAM, can listen() for incoming connections.
      //
      if (socketType == SOCK_STREAM)
      {
        if (listen(servSock[i], 5) == SOCKET_ERROR)
        {
          DEBUG_CTX("--- listen() failed with error %d: %s", WSAGetLastError(), NetSockets::decode_error(WSAGetLastError()));
          continue;
        }
      }

      debug("SERVER STARTED (%s:%s) on port %s", (socketType == SOCK_STREAM) ? "TCP" : "UDP",
        (AI->ai_family == PF_INET) ? "PF_INET" : "PF_INET6", port);
    }

    freeaddrinfo(addrInfo);

    if (i == 0)
    {
      DEBUG_CTX("--- Fatal error: unable to serve on any address.");
      NetSockets::winsock2_term();
      return false;
    }
    numSocks = i;
    udpSocksStart = numSocks;

    if (portUdp)
    {
      //
      // By setting the AI_PASSIVE flag in the hints to getaddrinfo, we're
      // indicating that we intend to use the resulting address(es) to bind
      // to a socket(s) for accepting incoming connections.  This means that
      // when the address parameter is NULL, getaddrinfo will return one
      // entry per allowed protocol family containing the unspecified address
      // for that family.
      //
      memset(&hints, 0, sizeof(hints));
      hints.ai_family = family;
      hints.ai_socktype = SOCK_DGRAM;
      hints.ai_flags = AI_PASSIVE;
      val = getaddrinfo(address, portUdp, &hints, &addrInfo);
      if (val != 0)
      {
        DEBUG_CTX("--- getaddrinfo (%s) failed with error %d: %s", address, val, gai_strerror(val));
        NetSockets::winsock2_term();
        return false;
      }

      //
      // For each address getaddrinfo returned, we create a new socket,
      // bind that address to it, and create a queue to listen on.
      //
      for (i = 0, AI = addrInfo; AI != NULL; AI = AI->ai_next, i++)
      {
        // Highly unlikely, but check anyway.
        if (numSocks + i >= FD_SETSIZE)
        {
          debug("--- getaddrinfo returned more addresses than we could use.");
          break;
        }

        // This example only supports PF_INET and PF_INET6.
        if ((AI->ai_family != PF_INET) && (AI->ai_family != PF_INET6))
          continue;

        // Open a socket with the correct address family for this address.
        servSock[numSocks + i] = socket(AI->ai_family, AI->ai_socktype, AI->ai_protocol);
        if (servSock[numSocks + i] == INVALID_SOCKET)
        {
          DEBUG_CTX("--- socket() failed with error %d: %s", WSAGetLastError(), NetSockets::decode_error(WSAGetLastError()));
          continue;
        }

        //
        // bind() associates a local address and port combination
        // with the socket just created. This is most useful when
        // the application is a server that has a well-known port
        // that clients know about in advance.
        //
        if (bind(servSock[numSocks + i], AI->ai_addr, (int)AI->ai_addrlen) == SOCKET_ERROR)
        {
          DEBUG_CTX("--- bind() failed with error %d: %s", WSAGetLastError(), NetSockets::decode_error(WSAGetLastError()));
          continue;
        }

        debug("SERVER STARTED (%s:%s) on port %s", "UDP", (AI->ai_family == PF_INET) ? "PF_INET" : "PF_INET6", portUdp);
      }

      freeaddrinfo(addrInfo);
      numSocks = numSocks + i;
    }

    bool sync_exec = blk.getBool("sync_exec", false);
    DEBUG_CTX("synchronous execution: %d", sync_exec);
    if (!sync_exec)
    {
      // asynchronous execution (another thread)
      running = true;

      serverThread = (HANDLE)_beginthreadex(NULL, 256 << 10, startThread, this, 0, NULL);
      if (!serverThread)
      {
        running = false;
        return false;
      }
    }

    if (useShares)
    {
      // register server
      regFile.resize(1);
      regFile[0] = String(0, "%s/%s", blk.getStr("registry", "."), address ? address : "local_host");

      for (i = 0; i < 16; i++)
      {
        const char *reg = blk.getStr(String(32, "registry_%d", i), NULL);
        if (!reg)
          continue;

        append_items(regFile, 1);
        regFile.back() = String(0, "%s/%s", reg, address ? address : "local_host");
      }

      for (i = 0; i < regFile.size(); i++)
      {
        debug("  registered name: %s", (char *)regFile[i]);

        FILE *fp = fopen(regFile[i], "wb");
        if (fp)
        {
          const char *p = proc ? proc->getUserFriendlyName() : "=READY=";
          int len = i_strlen(p) + 1;

          fwrite(p, 1, len, fp);
          fclose(fp);
        }
      }
    }

    if (sync_exec)
    {
      // synchronous execution
      running = true;
      serverLoop();
      // stop (true);
    }
    return true;
  }

  bool stop(bool _fatal)
  {
    int i;

    DEBUG_CTX("stopping server: running=%d, _fatal=%d", running, _fatal);
    running = false;

    if (useShares)
    {
      for (i = 0; i < regFile.size(); i++)
      {
        debug("  unregistered name: %s", (char *)regFile[i]);
        unlink(regFile[i]);
      }
    }

    for (i = 0; i < NET_MAX_CONNECTIONS; i++)
      if (connSock[i])
        close_socket(i);

    if (!waitThreadTermination(500))
    {
      DEBUG_CTX("--- can't wait for server termination for more than %d ms", 500);
      if (!waitThreadTermination(9500))
        if (_fatal)
          DAG_FATAL("can't wait for server termination for more than %d ms", 10000);
        else
        {
          DEBUG_CTX("can't wait for server termination for more than %d ms", 10000);
          return false;
        }
    }
    if (udpBuf)
    {
      memfree(udpBuf, midmem);
      udpBuf = NULL;
      udpBufSize = 0;
    }

    NetSockets::winsock2_term();

    return true;
  }

  bool waitEndSession(int timeout)
  {
    while (running && timeout > 0)
    {
      Sleep(100);
      timeout -= 100;
    }

    return timeout > 0;
  }

  bool waitThreadTermination(int timeout)
  {
    DWORD dw;
    if (!serverThread && !workThread)
      return true;

    while (getRefCount() > 1 && timeout > 0)
    {
      Sleep(50);
      timeout -= 50;
    }
    if (serverThread)
      CloseHandle(serverThread);
    if (workThread)
      CloseHandle(workThread);
    serverThread = workThread = NULL;

    return timeout > 0;
  }

  int sendPacket(int s, const NetMsgPacket *p)
  {
    if (TRACE_PACKETS)
      debug("SERVER: send packet (size=%d gen=%p queryId=%d", p->hdr.size, p->hdr.generation, p->queryId);
    // dumpPacket ( p );
    return sendBytes(s, p, sizeof(p->hdr) + p->hdr.size, true);
  }
  int sendPacket(void *adr, int adr_len, const NetMsgPacket *p)
  {
    if (TRACE_PACKETS)
      debug("SERVER: send UDP packet (size=%d gen=%p queryId=%d", p->hdr.size, p->hdr.generation, p->queryId);
    int ret = sendto(servSock[udpSocksStart], (const char *)p, sizeof(p->hdr) + p->hdr.size, 0, (LPSOCKADDR)adr, adr_len);
    if (ret == SOCKET_ERROR)
    {
      DEBUG_CTX("send() failed with error %d: %s\n", WSAGetLastError(), NetSockets::decode_error(WSAGetLastError()));
      ret = 0;
    }
    return ret;
  }
  int sendRawBytes(int s, const void *data, int byte_num, bool whole_block)
  {
    if (TRACE_PACKETS)
      debug("CLIENT: send raw bytes (size=%d)", byte_num);
    return sendBytes(s, data, byte_num, whole_block);
  }

  bool isRunning() { return running; }

  void close_socket(int s)
  {
    if (s < 0 || s >= NET_MAX_CONNECTIONS || !connSock[s])
    {
      DEBUG_CTX("--- close_socket ( %d ), while socket not connected", s);
      return;
    }
    WinAutoLock lock(connCC);
    debug("close socket %d", s);
    closesocket(connSock[s]);
    connSock[s] = NULL;
    clientsNum--;
  }

  int serverLoop()
  {
    SOCKADDR_STORAGE from;
    int val, fromLen, i;

    FD_ZERO(&sockSet);

    NetSockets::winsock2_init();

    while (running)
    {
      fromLen = sizeof(from);

      //
      // For connection orientated protocols, we will handle the
      // packets comprising a connection collectively.  For datagram
      // protocols, we have to handle each datagram individually.
      //

      //
      // Check to see if we have any sockets remaining to be served
      // from previous time through this loop.  If not, call select()
      // to wait for a connection request or a datagram to arrive.
      //
      for (i = 0; i < numSocks; i++)
        if (FD_ISSET(servSock[i], &sockSet))
          break;

      if (i == numSocks)
      {
        for (i = 0; i < numSocks; i++)
          FD_SET(servSock[i], &sockSet);

        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        val = select(numSocks, &sockSet, 0, 0, &tv);
        if (val == SOCKET_ERROR)
        {
          DEBUG_CTX("--- select() failed with error %d: %s", WSAGetLastError(), NetSockets::decode_error(WSAGetLastError()));
          /*
          NetSockets::winsock2_term();
          DEBUG_CTX("--- SERVER emergency exit");
          return -1;
          */
          continue;
        }
        if (val == 0)
          continue;
      }
      for (i = 0; i < numSocks; i++)
        if (FD_ISSET(servSock[i], &sockSet))
        {
          FD_CLR(servSock[i], &sockSet);
          break;
        }

      if (socketType == SOCK_STREAM && i < udpSocksStart)
      {
        //
        // Since this socket was returned by the select(), we know we
        // have a connection waiting and that this accept() won't block.
        //

        int scon;
        {
          WinAutoLock lock(connCC);

          scon = getAvailableConnection();
          if (scon == -1)
          {
            DEBUG_CTX("--- all connections used");
            continue;
          }

          clientsNum++;
          connSock[scon] = accept(servSock[i], (LPSOCKADDR)&from, &fromLen);
          if (!running)
            break;
          if (connSock[scon] == INVALID_SOCKET)
          {
            clientsNum--;
            connSock[scon] = NULL;
            DEBUG_CTX("--- accept() failed with error %d: %s", WSAGetLastError(), NetSockets::decode_error(WSAGetLastError()));
            NetSockets::winsock2_term();
            DEBUG_CTX("--- SERVER emergency exit");
            return -1;
          }
        }

        if (getnameinfo((LPSOCKADDR)&from, fromLen, hostName, sizeof(hostName), NULL, 0, NI_NUMERICHOST) != 0)
          strcpy(hostName, "<unknown>");

        //
        // This sample server only handles connections sequentially.
        // To handle multiple connections simultaneously, a server
        // would likely want to launch another thread or process at this
        // point to handle each individual connection.  Alternatively,
        // it could keep a socket per connection and use select()
        // on the fd_set to determine which to read from next.
        //
        // Here we just loop until this connection terminates.
        //

        if (proc)
        {
          if (!proc->isStreamConnAsync())
            proc->streamConnectionAccepted(hostName, scon);
          else
          {
            StartConnThreadData *data = new (tmpmem) StartConnThreadData;
            data->svr = this;
            data->me = proc;
            data->host_name = hostName;
            data->socket = scon;

            HANDLE h = (HANDLE)_beginthreadex(NULL, 256 << 10, startStreamConnThread, data, 0, NULL);
            if (h)
              CloseHandle(h);
          }
        }
      }
      else if (i >= udpSocksStart)
      {
        //
        // Since UDP maintains message boundaries, the amount of data
        // we get from a recvfrom() should match exactly the amount of
        // data the client sent in the corresponding sendto().
        //
        int bytesRead = recvfrom(servSock[i], (char *)udpBuf, udpBufSize, 0, (LPSOCKADDR)&from, &fromLen);
        if (bytesRead == SOCKET_ERROR)
        {
          DEBUG_CTX("recvfrom() failed with error %d: %s", WSAGetLastError(), NetSockets::decode_error(WSAGetLastError()));
          closesocket(servSock[i]);
          continue;
        }
        if (bytesRead == 0)
        {
          // This should never happen on an unconnected socket, but...
          DEBUG_CTX("recvfrom() returned zero, aborting");
          closesocket(servSock[i]);
          break;
        }

        val = getnameinfo((LPSOCKADDR)&from, fromLen, hostName, sizeof(hostName), NULL, 0, NI_NUMERICHOST);
        if (val != 0)
        {
          DEBUG_CTX("getnameinfo() failed with error %d: %s", val, NetSockets::decode_error(val));
          strcpy(hostName, "<unknown>");
        }

        NetShortQueryPacket *p = (NetShortQueryPacket *)udpBuf;
        if (p->hdr.size + sizeof(p->hdr) != bytesRead)
          DEBUG_CTX("broken packet arrived: p->size=%d p->gen=%d size=%d", p->hdr.size, p->hdr.generation, bytesRead);
        else
        {
          if (proc)
            proc->datagramReceived(hostName, &from, fromLen, *p);
        }
      }
    }

    NetSockets::winsock2_term();

    DEBUG_CTX("SERVER: graceful exit");
    return 0;
  }
  void streamConnectionLoop(int s)
  {
    char msg_buf[MAX_STATIC_PACKET_SIZE + sizeof(NetMsgPacketHeader)];
    NetMsgPacket *p;
    NetMsgPacketHeader ph;
    int ret;

    while (running)
    {
      ret = recvBytes(s, &ph, sizeof(NetMsgPacketHeader), true);
      if (ret <= 0 || !running)
      {
        DEBUG_CTX("ret=%d running=%d", ret, running);
        break;
      }

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
        ret = recvBytes(s, p->data, ph.size, true);
        if (ret <= 0 || !running)
        {
          DEBUG_CTX("ret=%d running=%d ph.size=%d", ret, running, ph.size);
          break;
        }
      }

      if (ph.generation == NGEN_Query)
      {
        // received Short Query Packet
        if (TRACE_PACKETS)
          debug("SERVER: received query (size=%d query=%d)", ph.size, p->queryId);
        if (p->queryId < NSQP_SERVER_SPEC_QUERY_NUM)
          recvShortQueryPacket(s, *(NetShortQueryPacket *)p);
        else if (proc)
          proc->recvQuery(s, *(NetShortQueryPacket *)p);

        if (ph.size >= MAX_STATIC_PACKET_SIZE)
          delete p;
      }
      else if (ph.generation == NGEN_Response)
      {
        // received Response Packet
        if (TRACE_PACKETS)
          debug("SERVER: received response (size=%d query=%d)", ph.size, p->queryId);
        if (p->queryId < NSQP_SERVER_SPEC_QUERY_NUM)
          recvResponsePacket(s, *(NetShortQueryPacket *)p);
        else if (proc)
          proc->recvResponse(s, *(NetShortQueryPacket *)p);

        if (ph.size >= MAX_STATIC_PACKET_SIZE)
          delete p;
      }
      else
      {
        // forward packet to processor
        if (TRACE_PACKETS)
          debug("SERVER: received packet (size=%d gen=%d)", ph.size, ph.generation);
        if (proc)
          proc->recvPacket(s, p, ph.size < MAX_STATIC_PACKET_SIZE);
        else if (ph.size >= MAX_STATIC_PACKET_SIZE)
          delete p;
      }
    }
  }

  void streamConnectionLoopRaw(int s)
  {
    char msg_buf[MAX_STATIC_PACKET_SIZE];
    int ret;

    while (running)
    {
      ret = recvBytes(s, msg_buf, sizeof(msg_buf), false);
      if (ret <= 0 || !running)
      {
        DEBUG_CTX("ret=%d running=%d", ret, running);
        break;
      }
      proc->recvRawBytes(s, msg_buf, ret);
    }
  }

  int getAvailableConnection()
  {
    WinAutoLock lock(connCC);
    for (int i = 0; i < NET_MAX_CONNECTIONS; i++)
      if (!connSock[i])
        return i;
    return -1;
  }
  void recvShortQueryPacket(int s, NetShortQueryPacket &p)
  {
    switch (p.queryId)
    {
      case NSQP_GetInfo:
      {
        DynamicMemGeneralSaveCB cwr(tmpmem, 0, 2 << 10);
        NetMsgPacketHeader hdr;
        NetMsgPacket *rp;

        hdr.size = 0;
        hdr.generation = NGEN_Response;

        cwr.write(&hdr, sizeof(hdr));
        cwr.writeInt(p.queryId);
        storeServerInfo(cwr);

        rp = NetMsgPacket::create(cwr.data(), cwr.size() - sizeof(hdr));

        sendPacket(s, rp);
      }
      break;
      case NSQP_CheckLocal:
      {
        InPlaceMemLoadCB crd(p.dwData + 1, p.hdr.size - 4);
        String client_cn, server_cn(getenv("COMPUTERNAME"));
        crd.readString(client_cn);

        NetShortQueryPacket qp(NSQP_CheckLocal, true);

        if (strcmp(client_cn, server_cn) == 0)
          qp.dwData[1] = 1;
        else
          qp.dwData[1] = 0;

        if (proc)
          proc->clientIsLocal(s, qp.dwData[1]);
        sendPacket(s, &qp);
      }
      break;
    }
  }
  void recvResponsePacket(int s, NetShortQueryPacket &p) {}

  void storeServerInfo(IGenSave &cwr)
  {
    // save clients num
    cwr.write(&clientsNum, sizeof(int));
    // save server state
    cwr.writeInt(0 /*proc->getServerState()*/);
    // save server name
    cwr.writeString(proc ? proc->getUserFriendlyName() : NULL);
  }
  bool startProcessorWorkThread(int stk_size)
  {
    if (!proc)
      return false;
    workThread = (HANDLE)_beginthreadex(NULL, stk_size, startProcWorkThread, proc, 0, NULL);
    if (!workThread)
    {
      DEBUG_CTX("cannot create thread!");
      return false;
    }
    return true;
  }

  static unsigned int __stdcall startThread(void *p)
  {
    WinSock2NetworkServer *svr = (WinSock2NetworkServer *)p;
    int ret = 0, id = DagorHwException::setHandler("server loop");
    ::win32_set_thread_name("Server-Main");

    DAGOR_TRY
    {
      // guarded thread procedure
      ret = svr->serverLoop();
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

  static unsigned int __stdcall startProcWorkThread(void *p)
  {
    INetSocketsServerProcessor *proc = (INetSocketsServerProcessor *)p;
    WinSock2NetworkServer *svr = (WinSock2NetworkServer *)proc->server;
    int id = DagorHwException::setHandler("server worker");
    ::win32_set_thread_name("Server-Work");

    DAGOR_TRY
    {
      // guarded thread procedure
      svr->addRef();

      proc->processorWorkThread();

      svr->destroy();
    }
    DAGOR_CATCH(DagorException e)
    {
#ifdef DAGOR_EXCEPTIONS_ENABLED
      DagorHwException::reportException(e, true, "except_thread");
#endif
    }
    DagorHwException::removeHandler(id);
    return 0;
  }

  static unsigned int __stdcall startStreamConnThread(void *p)
  {
    StartConnThreadData data = *(StartConnThreadData *)p;
    delete (StartConnThreadData *)p;
    int id = DagorHwException::setHandler(String(128, "server StreamConnThread %d", (int)data.socket));
    ::win32_set_thread_name("Server-Conn");

    DAGOR_TRY
    {
      // guarded thread procedure

      data.svr->addRef();

      data.me->streamConnectionAccepted(data.host_name, data.socket);
      if (data.svr->rawMode)
        data.svr->streamConnectionLoopRaw(data.socket);
      else
        data.svr->streamConnectionLoop(data.socket);
      data.me->streamConnectionClosed(data.host_name, data.socket);
      if (data.svr->connSock[data.socket])
        data.svr->close_socket(data.socket);

      if (data.svr->delRef())
        data.svr->destroyServer();
    }
    DAGOR_CATCH(DagorException e)
    {
#ifdef DAGOR_EXCEPTIONS_ENABLED
      DagorHwException::reportException(e, false, "except_thread");
#endif

      if (data.svr->connSock[data.socket])
        data.svr->close_socket(data.socket);
    }
    DagorHwException::removeHandler(id);
    return 0;
  }

  // Low-level send/receive
  int recvBytes(int s, void *_p, int buf_sz, bool whole_block)
  {
    if (s < 0 || s >= NET_MAX_CONNECTIONS || !connSock[s])
    {
      DEBUG_CTX("--- recvBytes ( %d, %p, %d ), while socket not connected", s, _p, buf_sz);
      return -1;
    }

    int ret, ret_sz = 0;
    char *p = (char *)_p;

    while (buf_sz > 0)
    {
      ret = recv(connSock[s], (char *)p, buf_sz > MAX_PACKET_BURST_SIZE ? MAX_PACKET_BURST_SIZE : buf_sz, 0);

      if (ret == SOCKET_ERROR)
      {
        if (running)
        {
          DEBUG_CTX("--- recv() failed with error %d: %s", WSAGetLastError(), NetSockets::decode_error(WSAGetLastError()));
        }
        close_socket(s);
        return -1;
      }
      if (ret == 0)
      {
        close_socket(s);
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

  int sendBytes(int s, const void *_p, int sz, bool whole_block)
  {
    if (s < 0 || s >= NET_MAX_CONNECTIONS || !connSock[s])
    {
      DEBUG_CTX("--- sendBytes ( %d, %p, %d ), while socket not connected", s, _p, sz);
      return -1;
    }
    WinAutoLock lock(sendCC[s]);

    int ret, ret_sz = 0, unit_sz = sz;
    char *p = (char *)_p;
    if (unit_sz > MAX_PACKET_BURST_SIZE)
      unit_sz = MAX_PACKET_BURST_SIZE;

    while (sz > 0)
    {
      ret = send(connSock[s], (char *)p, unit_sz, 0);
      if (ret == SOCKET_ERROR)
      {
        DEBUG_CTX("--- send() failed: error %d: %s", WSAGetLastError(), NetSockets::decode_error(WSAGetLastError()));
        close_socket(s);
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
    FILE *fp = fopen(String(128, "s%08d_sz%p_gen%p_qid%d.msg", packet_n++, p->hdr.size, p->hdr.generation, p->queryId), "wb");
    if (fp)
    {
      fwrite(p, sizeof(p->hdr) + p->hdr.size, 1, fp);
      fclose(fp);
    }
  }
};

NetSocketsServer *NetSocketsServer::make()
{
  NetSockets::load_debug_info();
  return new (midmem) WinSock2NetworkServer;
}
