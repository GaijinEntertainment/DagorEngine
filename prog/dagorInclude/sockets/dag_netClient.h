//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <sockets/dag_netData.h>
#include <generic/dag_tab.h>

// forward declarations for external classes
class DataBlock;


// forward declarations for internal classes
struct NetMsgPacket;
struct NetShortQueryPacket;


// Interface for server request processor
class INetSocketsClientProcessor
{
public:
  class NetSocketsClient *client;

  virtual void destroy() = 0;

  virtual void attach() = 0;
  virtual void detach() = 0;

  virtual void recvPacket(NetMsgPacket *p, bool packet_static) = 0;
  virtual void recvQuery(NetShortQueryPacket &p) = 0;
  virtual void recvResponse(NetShortQueryPacket &p) = 0;

  virtual void datagramReceived(NetShortQueryPacket &p) = 0;

  virtual void recvRawBytes(const void *p, int len) = 0;
  virtual int fillSendBuffer(char *data, int size) { return 0; };

  virtual void connected() = 0;
  virtual void disconnected() = 0;

  virtual char *getUserFriendlyName() = 0;
};

// Interface for generic socket client
class NetSocketsClient
{
public:
  virtual void destroy() = 0;

  virtual void setProcessor(INetSocketsClientProcessor *nssp) = 0;

  virtual void setup(const DataBlock &blk) = 0;
  virtual void enumerateServers(Tab<NetServerInfo> &svr) = 0;

  virtual bool connectToServer(const char *server_addr, bool raw = false) = 0;
  virtual bool disconnectFromServer(bool _fatal = false) = 0;

  virtual int sendPacket(const NetMsgPacket *p) = 0;
  virtual int sendPacketUdp(const NetMsgPacket *p) = 0;

  virtual int sendRawBytes(const void *data, int byte_num, bool whole_block = true) = 0;

  virtual bool isConnected() = 0;
  virtual bool isServerLocal() = 0;

  virtual void setUseRegistryShares(bool on) = 0;

  static NetSocketsClient *make();
  static bool getServerInfo(const char *address, const char *port, int family, int socketType, NetServerInfo &info);
};
