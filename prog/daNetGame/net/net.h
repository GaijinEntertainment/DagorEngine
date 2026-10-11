// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdint.h>
#include <generic/dag_enumBitMask.h>
#include <daECS/core/entityId.h>
#include <daNet/disconnectionCause.h>
#include <generic/dag_span.h>
#include <dag/dag_vector.h>
#include <EASTL/string.h>
#include <EASTL/functional.h>
#include <osApiWrappers/dag_miscApi.h>
#include <daECS/net/authority.h>
#include <debug/dag_assert.h>

class String;

namespace net
{
class Object;
class IConnection;
class Connection;
class CNetwork;
class IMessage;
class MessageClass;
struct MessageNetDesc;

// Compat name; maps to net authority thread (or unset).
bool is_this_thread_net_em_owner();

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

struct ConnectParams
{
  dag::Vector<eastl::string> serverUrls;
  dag::Vector<uint8_t> encryptKey;
  dag::Vector<uint8_t> authKey;
  eastl::string sessionId;
  eastl::string relayStunRequestAddr; // if set, UDP punch to this relay addr is queued on connect
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
int send_net_msg(ecs::EntityManager &mgr, ecs::EntityId eid, net::IMessage &&msg, const net::MessageNetDesc *msg_net_desc = nullptr);
int send_net_msg(ecs::EntityId eid, net::IMessage &&msg, const net::MessageNetDesc *msg_net_desc = nullptr);

// Untargeted send: no EntityManager parameter -- routing is per-NetContext (GET_NET_CTX is thread-aware).
int send_net_msg(net::IMessage &&msg, const net::MessageNetDesc *msg_net_desc = nullptr);
#endif

net::CNetwork *get_net_or_null_unchecked(); // do not call directly; use GET_NET()

// Active session's CNetwork or nullptr. No owner-thread assert.
#define GET_NET() (get_net_or_null_unchecked())

bool send_msg_to_client(net::IMessage &&msg, int client_conn_id); // may run off-owner (unchecked)
void debug_verify_net_connection_ptr(net::IConnection *conn);     // may run off-owner; debug-only

void disconnect_from_relay();
bool establish_relay_connection(const char *relay_url);
bool set_relay_connection_handler(void (*relayConnectionHandler)(bool));
// Runs at net_update and net_destroy. fn's bool is false when connectGen moved since the call.
void on_net_thread(eastl::function<void(bool)> fn);

eastl::string get_received_stun_system_address_str();
void request_udp_punch_via_relay(const char *relay_addr);

int get_no_packets_time_ms(); // time, in milliseconds, since last packet from server was received

void net_create_time_if_not_exist();
void net_update(ecs::EntityManager &mgr);
void net_sleep_server();
// Returns true when teardown ran; false on wrong-thread no-op or nothing to destroy.
bool net_destroy(ecs::EntityManager &mgr, bool final = false);
// Returns true when broadcast fired; false on wrong-thread no-op or no ctx for mgr.
bool net_on_about_to_clear_all_entities(ecs::EntityManager &mgr);
void net_stop();

enum class NetworkVariant : int
{
  None = 0,
  MainNet = 1,
  UserNet = 2,
};

// Like dedicated::is_dedicated(): constexpr default for DNG; Eden links a runtime
// body when built with -DDAGOR_NET_VARIANT_RUNTIME=1.
// non_main_net_possible: Eden can run UserNet / forbidden-thread checks; DNG folds them away.
#if DAGOR_NET_VARIANT_RUNTIME
NetworkVariant network_variant();
bool is_main_thread_network();
inline constexpr bool non_main_net_possible() { return true; }
#else
inline constexpr NetworkVariant network_variant() { return NetworkVariant::MainNet; }
inline constexpr bool is_main_thread_network() { return true; }
inline constexpr bool non_main_net_possible() { return false; }
#endif
