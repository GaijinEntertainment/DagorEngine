//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daECS/core/entityId.h>
#include <daECS/core/component.h>
#include "connid.h"
#include "scope_query_cb.h"
#include <daNet/daNetEchoManager.h> // for EchoResponse
#include <daNet/packetPriority.h>
#include <daNet/daNetTypes.h>
#include <daNet/disconnectionCause.h>
#include <generic/dag_tab.h>
#include <generic/dag_enumBitMask.h>


namespace danet
{
class BitStream;
}
struct Packet;
namespace ecs
{
class EntityComponentRef;
}

namespace net
{

#define ECS_NET_CONNERRS                                                                                                  \
  CONNERR_VAL(CONNECTION_CLOSED)             /* Normal disconnect */                                                      \
  CONNERR_VAL(CONNECTION_LOST)               /* Connection was lost/timeouted (abnormal disconnect) */                    \
  CONNERR_VAL(CONNECT_FAILED)                /* Connect to server failed (client-only) */                                 \
  CONNERR_VAL(CONNECT_FAILED_PROTO_MISMATCH) /* Connection was rejected by server because of incompatible net protocol */ \
  CONNERR_VAL(SERVER_FULL)                   /* Server is full (no free incoming connections) */                          \
  CONNERR_VAL(WAS_KICKED_OUT)                /* Client was kicked by server */                                            \
  CONNERR_VAL(KICK_AFK)                      /* Kicked for being AFK */                                                   \
  CONNERR_VAL(KICK_EAC)                      /* Kicked by EAC */

enum class ConnErr
{
#define CONNERR_VAL(x) x,
  ECS_NET_CONNERRS
#undef CONNERR_VAL
};

class Object;
class IMessage;
struct MessageNetDesc;

enum class EncryptionKeyBits : uint32_t
{
  None = 0,
  Encryption = 1,
  Decryption = 2
};
DAGOR_ENABLE_ENUM_BITMASK(EncryptionKeyBits);

class IConnection
{
public:
  virtual ~IConnection() {}

  virtual ConnectionId getId() const = 0;

  virtual bool isEntityInScope(ecs::EntityId eid) const = 0;
  virtual bool setEntityInScopeAlways(ecs::EntityId eid) = 0;

  virtual danet::PeerQoSStat getPeerQoSStat() const = 0;
  virtual void disconnect(DisconnectionCause cause = DC_CONNECTION_CLOSED) = 0;

  virtual void setUserPtr(void *ptr) = 0;
  virtual void *getUserPtr() const = 0;

  virtual uint32_t getConnFlags() const = 0;
  virtual uint32_t &getConnFlagsRW() = 0;

  virtual void sendEcho(const char *, uint32_t) {}
  virtual bool send(int cur_time, const danet::BitStream &bs, PacketPriority prio, PacketReliability rel, uint8_t chn,
    int dup_delay_ms = 0) = 0;
  virtual bool isBlackHole() const = 0; // if this is true than this connection won't ack or reply to anything

  virtual void setEncryptionKey(dag::ConstSpan<uint8_t> key, EncryptionKeyBits ebits) = 0;

  virtual bool changeSendAddress(const char * /*new_host*/) { return false; }

  virtual void allowReceivePlaintext(bool /*allow*/){};

  virtual int getMTU() const = 0;
  virtual uint32_t getIP() const = 0;
  virtual const char *getIPStr() const = 0;
  virtual bool isResponsive() const = 0;
};

class ConnectionsIterator
{
  int i = 0;
  void advance();

public:
  ConnectionsIterator() { advance(); }
  explicit operator bool() const { return i >= 0; }
  IConnection &operator*() const;
  void operator++()
  {
    ++i;
    advance();
  }
};

class INetDriver
{
public:
  virtual ~INetDriver() {}
  virtual void destroy() { delete this; }
  virtual bool connect(const char * /*connecturl*/, uint16_t /*protov*/) { return false; }
  virtual eastl::optional<danet::EchoResponse> receiveEchoResponse() { return eastl::nullopt; }
  virtual Packet *receive(int cur_time_ms) = 0;
  virtual void free(Packet *pkt) = 0;
  virtual void shutdown(int wait_time_ms) = 0;
  virtual void stopAll(DisconnectionCause cause) = 0; // disconnect all with special error code
  virtual void *getControlIface() const = 0;
  virtual bool isServer() const = 0;
  virtual DaNetTime getLastReceivedPacketTime(ConnectionId) const { return 0; }
};

INetDriver *create_net_driver_listen(const char *listenurl, int max_connections, uint16_t *out_port = NULL); // server driver
INetDriver *create_net_driver_listen(const SocketDescriptor &sd, int max_connections);                       // server driver
INetDriver *create_net_driver_connect(const char *connecturl, uint16_t protov = 0);                          // client driver
Connection *create_net_connection(INetDriver *drv, ConnectionId id, scope_query_cb_t &&scope_query = scope_query_cb_t()); // generic
                                                                                                                          // net
                                                                                                                          // connection

INetDriver *create_stub_net_driver();
Connection *create_stub_connection();

// serialization
void serialize_comp_nameless(ecs::component_t name, const ecs::EntityComponentRef &cref, danet::BitStream &bs);
ecs::MaybeChildComponent deserialize_comp_nameless(ecs::component_t &name, const danet::BitStream &bs);

void write_eid(danet::BitStream &bs, ecs::EntityId eid);
bool read_eid(const danet::BitStream &bs, ecs::EntityId &eid); // return false if read from stream failed

}; // namespace net

#ifndef NET_SEND_NET_MSG_DECLARED
#define NET_SEND_NET_MSG_DECLARED
// Dst connection is deduced automatically (based of routing & recipient filter of message)
// Return number of successfull sends
int send_net_msg(ecs::EntityId eid, net::IMessage &&msg, const net::MessageNetDesc *msg_net_desc = nullptr);
#endif
// Returns true if app is running in server environment.
// Offline (no network mode) is also assumed to be "server".
bool is_server();
bool is_true_net_server();
bool has_network();
net::IConnection *get_client_connection(int id);
dag::Span<net::IConnection *> get_client_connections(); // Note: might contain NULLs
