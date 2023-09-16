//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

// forward declarations for external classes
class DataBlock;


// forward declarations for internal classes
struct NetMsgPacket;
struct NetShortQueryPacket;


// implementation restriction constants
enum
{
  NET_MAX_CONNECTIONS = 128,
};

// Interface for server request processor
class INetSocketsServerProcessor
{
public:
  class NetSocketsServer *server;

  virtual void destroy() = 0;

  virtual void attach() = 0;
  virtual void detach() = 0;

  virtual void recvPacket(int s, NetMsgPacket *p, bool packet_static) = 0;
  virtual void recvQuery(int s, NetShortQueryPacket &p) = 0;
  virtual void recvResponse(int s, NetShortQueryPacket &p) = 0;

  virtual void streamConnectionAccepted(const char *host_name, int s) = 0;
  virtual void streamConnectionClosed(const char *host_name, int s) = 0;

  virtual void datagramReceived(const char *host_name, void *from, int from_len, NetShortQueryPacket &p) = 0;

  virtual void recvRawBytes(int s, const void *p, int len) = 0;

  virtual bool isStreamConnAsync() = 0;

  virtual char *getUserFriendlyName() = 0;

  virtual void processorWorkThread() = 0;

  virtual void clientIsLocal(int s, bool local) = 0;
};

// Interface for generic socket server
class NetSocketsServer
{
public:
  virtual void destroy() = 0;

  virtual void setProcessor(INetSocketsServerProcessor *nssp) = 0;

  virtual bool start(const DataBlock &blk) = 0;
  virtual bool startProcessorWorkThread(int stk_size = 256 << 10) = 0;

  virtual bool stop(bool _fatal = false) = 0;
  virtual void close_socket(int s) = 0;

  virtual int sendPacket(int s, const NetMsgPacket *p) = 0;
  virtual int sendPacket(void *adr, int adr_len, const NetMsgPacket *p) = 0;

  virtual int sendRawBytes(int s, const void *data, int byte_num, bool whole_block = true) = 0;

  virtual bool isRunning() = 0;
  virtual bool waitEndSession(int timeout_ms = 0x7FFFFFFF) = 0;

  virtual void setUseRegistryShares(bool on) = 0;

  static NetSocketsServer *make();
};
