//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daNet/daNetTypes.h>
#include <generic/dag_tab.h>
#include <daNet/bitStream.h>
#include <EASTL/vector_set.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/unique_ptr.h>
#include "connection.h"
#include "netbase.h"

namespace danet
{
class BitStream;
}
struct Packet;

namespace net
{

class Connection;
class IMessage;
class Object;
struct WaitMsg;
struct RTTStat;
class EncryptionCtx;
class MessageClass;
struct MessageNetDesc;

class INetworkObserver
{
public:
  virtual ~INetworkObserver() = default;
  virtual void onConnect(Connection &conn) = 0;
  virtual void onDisconnect(Connection *conn, DisconnectionCause cause) = 0;
  virtual bool onPacket(const Packet *) { return false; } // return true if packet was processed

private:
  friend class CNetwork;
};

struct NetStatRecord;

static constexpr int PROTO_VERSION_UNKNOWN = 0;

// Class that ties together replication networking & daNet networking.
// It's also implements messaging which replication lacking
class CNetwork
{
public:
  CNetwork(INetDriver *drv_, INetworkObserver *obsrv, uint16_t protov = PROTO_VERSION_UNKNOWN, uint64_t session_rand = 0,
    scope_query_cb_t &&scope_query_ = scope_query_cb_t());
  ~CNetwork();

  void update(int cur_time_ms, uint8_t replication_channel = 0);

  bool sendto(int cur_time, ecs::EntityId to_eid, const IMessage &msg, IConnection *receiver,
    const MessageNetDesc *msg_net_desc = nullptr);

  Connection *getServerConnection() { return serverConnection.get(); }
  const eastl::vector<eastl::unique_ptr<Connection>> &getClientConnections() const // Warning: vector might contain NULLs!
  {
    return clientConnections;
  }

  void addConnection(Connection *conn, unsigned idx);
  void destroyConnection(unsigned idx, DisconnectionCause cause);
  bool debugVerifyNetConnectionPtr(void *conn) const; // work only for truly network connections (i.e. not blackholes)

  void stopAll(DisconnectionCause cause);

  bool isServer() const { return bServer; }
  bool isClient() const { return !bServer; }

  INetDriver *getDriver() { return drv.get(); }
  void enableComponentFiltering(ConnectionId id, bool on);
  bool isComponentFilteringEnabled(ConnectionId id);
  void dumpStats();
  bool readReplayKeyFrame(const danet::BitStream &bs);

private:
  void receivePackets(int cur_time_ms, uint8_t replication_channel);
  void onPacket(const Packet *pkt, int cur_time_ms, uint8_t replication_channel, int &numEntitiesDestroyed);
  void syncStateUpdates(int cur_time, uint8_t channel);
  void updateConnections(int cur_time);
  void flushClientWaitMsgs(ecs::entity_id_t server_id);
  Connection *getConnection(unsigned idx);

private:
  bool bServer; // cached value of 'drv->isServer()'
  eastl::unique_ptr<INetDriver, DestroyDeleter<INetDriver>> drv;
  INetworkObserver *observer;
  eastl::unique_ptr<Connection> serverConnection;
  eastl::vector<eastl::unique_ptr<Connection>> clientConnections; // might contain NULLs
  eastl::unique_ptr<EncryptionCtx> encryptionCtx;
  Tab<RTTStat> rttstat; // rolling average per connection (server). Warning: doesn't include queue correcton therefore very rough!
  scope_query_cb_t scope_query;
  void *debugConnVtbl = nullptr;
  struct ObjMsg
  {
    const MessageClass *msgCls;
    DaNetTime rcvTime;
    danet::BitStream bs;
    void apply(const net::Object &robj, net::Connection &from, CNetwork &cnet, const danet::BitStream &data) const;
  };
  struct ClientWaitMsg
  {
    ecs::entity_id_t server_id;
    eastl::fixed_vector<ObjMsg, 1> msgs;
    bool operator<(const ClientWaitMsg &rhs) const { return server_id < rhs.server_id; }
  };
  eastl::vector_set<ClientWaitMsg> clientWaitMsgs; // client side messages that waiting for objects to be created. TODO: timeout
  uint32_t protocolVersion;
#if DAGOR_DBGLEVEL > 0
  eastl::vector_set<NetStatRecord> rxNetStat, txNetStat;
#endif
};

} // namespace net
