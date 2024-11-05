// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdint.h>
#include <generic/dag_enumBitMask.h>
#include <daECS/core/entityId.h>
#include <daNet/disconnectionCause.h>
#include <generic/dag_span.h>
#include <dag/dag_vector.h>
#include <EASTL/string.h>

class String;

namespace net
{
class Object;
class IConnection;
class Connection;
class IMessage;
class MessageClass;
struct MessageNetDesc;

enum class ServerFlags : uint16_t
{
  None = 0,
  Encryption = 1,
  Dev = 2,
  // resv    = 4,
  DynTick = 8,
  // resv    = 16,
  // resv    = 32,
};
DAGOR_ENABLE_ENUM_BITMASK(ServerFlags)

ServerFlags get_server_flags();

enum ConnFlags : uint32_t
{
  CF_NONE = 0,
  CF_PENDING = 1 << 0, // Not yet authenticated
  // resv    =  1<<1,
  // resv    =  1<<2,
  // resv    =  1<<3,
};

struct ConnectParams
{
  dag::Vector<eastl::string> serverUrls;
  dag::Vector<uint8_t> encryptKey;
  dag::Vector<uint8_t> authKey;
  eastl::string sessionId;
};
} // namespace net

uint32_t get_current_server_route_id();
const char *get_server_route_host(uint32_t route_id);
uint32_t get_server_route_count();
void switch_server_route(uint32_t route_id);
void send_echo_msg(uint32_t route_id);

bool is_server(); // Please note, that single (no network) mode is also considered 'server'
bool is_true_net_server();
bool has_network();
net::IConnection *get_server_conn();
net::IConnection *get_client_connection(int id);
dag::Span<net::IConnection *> get_client_connections(); // Note: might contain NULLs

void net_disconnect(net::IConnection &conn, DisconnectionCause cause = DC_CONNECTION_CLOSED);

#ifndef NET_SEND_NET_MSG_DECLARED
#define NET_SEND_NET_MSG_DECLARED
// Dst connection is deduced automatically (based of routing & recipient filter of message)
// Return number of successfull sends
int send_net_msg(ecs::EntityId eid, net::IMessage &&msg, const net::MessageNetDesc *msg_net_desc = nullptr);
#endif

int get_no_packets_time_ms(); // time, in milliseconds, since last packet from server was received

bool net_init_early();
bool net_init_late_client(net::ConnectParams &&connect_params);
void net_init_late_server();
void net_create_time_if_not_exist();
void net_update();
void net_sleep_server();
void net_destroy(bool final = false);
void net_on_before_emgr_clear();
void net_stop();
