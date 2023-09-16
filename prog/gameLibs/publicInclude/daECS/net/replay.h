//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_span.h>
#include <daECS/net/connid.h>
#include <daECS/net/scope_query_cb.h>
#include <generic/dag_tab.h>
#include <util/dag_baseDef.h>

namespace danet
{
class BitStream;
}

namespace net
{

class IConnection;
class INetDriver;

struct BaseReplayFooterData
{
  static constexpr int MAGIC = ~_MAKE4C('GJRP');
  int magic;        // = MAGIC
  int footerSizeOf; // sizeof of this struct
  int lastWriteTime;
};

typedef Tab<uint8_t> (*get_replay_footer_data_cb_t)();
typedef void (*dump_replay_key_frame_cb_t)(danet::BitStream &bs, int cur_time);
typedef void (*restore_replay_key_frame_cb_t)(const danet::BitStream &bs);

IConnection *get_replay_connection();

// Server replay driver (created on client for playback)
INetDriver *create_replay_net_driver(const char *read_fname, uint16_t version, Tab<uint8_t> *out_footer_data = nullptr,
  restore_replay_key_frame_cb_t restore_replay_key_frame_cb = nullptr);

// Black-hole connection that will write all outgoing packets to replay file.
// 'write_fname' is in/out, should ends with 'XXXXXX' in which actual temp path will be written
Connection *create_replay_connection(ConnectionId id, char *write_fname, uint16_t version,
  get_replay_footer_data_cb_t get_footer_cb = nullptr, scope_query_cb_t &&scope_query = scope_query_cb_t(),
  dump_replay_key_frame_cb_t dump_replay_key_frame_cb = nullptr);

// Client replay driver (created on client to write all incoming packets to replay file)
INetDriver *create_replay_client_net_driver(INetDriver *drv, const char *write_fname, uint16_t version,
  get_replay_footer_data_cb_t get_footer_cb = nullptr);

bool load_replay_footer_data(const char *path, uint16_t version,
  eastl::function<void(Tab<uint8_t> const &, const uint32_t)> footer_data_cb);
bool replay_rewind(INetDriver *drv, int rewind_time);
void replay_save_keyframe(IConnection *conn, int cur_time);
uint32_t get_replay_proto_version(uint16_t net_proto_version);
} // namespace net
