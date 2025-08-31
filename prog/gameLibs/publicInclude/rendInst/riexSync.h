//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>


namespace net
{
class IConnection;
}
namespace danet
{
class BitStream;
}

namespace riexsync
{
void clear_synced_ri_extra_pools();
void sync_all_ri_extra_pools();
void sync_ri_extra_pool(int pool_id);
void mark_all_loaded_pools_synced();
int get_base_synced_pools_count();
uint64_t calc_base_synced_pools_hash();
bool serialize_synced_ri_extra_pools(danet::BitStream &bs, bool full, bool skip_if_no_data, bool mark_all_synced);
bool deserialize_synced_ri_extra_pools(const danet::BitStream &bs);
int get_client_ri_pool_id(int server_pool_id);          // server -> client
int get_client_ri_pool_id(int server_pool_id, int def); // server -> client, def when not sycned
int get_server_ri_pool_id(int client_pool_id);          // client -> server
int get_server_ri_pool_id(int client_pool_id, int def); // client -> server, def when not sycned
bool is_server_ri_pool_sync_pending(int server_pool_id);

void update();
void send_initial_snapshot_to(net::IConnection &conn);
void broadcast_initial_snapshot();
void shutdown();
void ensure_all_ri_extra_pools_are_synced();

extern bool sync_active;
extern void (*pool_mapping_change_callback)();
bool is_server();
}; // namespace riexsync
