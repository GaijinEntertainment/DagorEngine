// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <rendInst/riexSync.h>
#include <EASTL/vector_set.h>
#include <EASTL/vector_map.h>
#include <EASTL/bitvector.h>
#include <daECS/net/netbase.h>
#include <daECS/net/msgDecl.h>
#include <daECS/net/msgSink.h>
#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstAccess.h>
#include <rendInst/rendInstExtra.h>

namespace riexsync
{
static void net_rcv_ri_pools_update(const net::IMessage *msg);
} // namespace riexsync

// IMPORTANT: pool updates MUST be ordered, otherwise empty snapshot, that players usually receive during entering the match,
// might come later than pool updates and override them. RiexSyncUpdateMsg can be unordered
ECS_NET_DECL_MSG(RiexSyncUpdateMsg, danet::BitStream);
ECS_NET_IMPL_MSG(RiexSyncUpdateMsg, net::ROUTING_SERVER_TO_CLIENT, &net::broadcast_rcptf, RELIABLE_ORDERED, 0, net::MF_DEFAULT_FLAGS,
  ECS_NET_NO_DUP, &riexsync::net_rcv_ri_pools_update);

namespace riexsync
{
static int prev_ri_extra_map_size = -1;
static bool is_pool_sync_sent = true;


void ensure_all_ri_extra_pools_are_synced()
{
  // check, if rend inst pool list has changed, and updates synced rendinsts,
  // if nothing has changed (most cases) this call is free
  if (rendinst::getRIExtraMapSize() != prev_ri_extra_map_size)
  {
    prev_ri_extra_map_size = rendinst::getRIExtraMapSize();
    is_pool_sync_sent = false;
    riexsync::sync_all_ri_extra_pools();
  }
}

static void net_rcv_ri_pools_update(const net::IMessage *msg)
{
  const RiexSyncUpdateMsg *riMsg = msg->cast<RiexSyncUpdateMsg>();
  G_ASSERT(riMsg);
  ensure_all_ri_extra_pools_are_synced();
  const bool ok = riexsync::deserialize_synced_ri_extra_pools(riMsg->get<0>());
  G_ASSERT(ok && riMsg->get<0>().GetNumberOfUnreadBits() == 0);
  G_UNUSED(ok);
}

// passing nullptr will send a broadcast to everyone and mark all synced
static void send_initial_snapshot_internal(net::IConnection *conn)
{
  const bool isBroadcast = conn == nullptr;
  ensure_all_ri_extra_pools_are_synced();
  RiexSyncUpdateMsg msg(danet::BitStream(1024, framemem_ptr()));
  riexsync::serialize_synced_ri_extra_pools(msg.get<0>(), /* full snapshot */ true, /* skip if empty */ false,
    /* mark all synced */ isBroadcast);
  net::MessageNetDesc md = msg.getMsgClass();
  md.rcptFilter = isBroadcast ? &net::broadcast_rcptf : &net::direct_connection_rcptf;
  msg.connection = conn;
  send_net_msg(net::get_msg_sink(), eastl::move(msg), &md);
}

void send_initial_snapshot_to(net::IConnection &conn) { send_initial_snapshot_internal(&conn); }

void broadcast_initial_snapshot() { send_initial_snapshot_internal(nullptr); }

void update()
{
  if (!sync_active)
    return;
  ensure_all_ri_extra_pools_are_synced();

  if (riexsync::is_server())
  {
    if (!is_pool_sync_sent)
    {
      // do not reserve memory, there might be nothing to send
      RiexSyncUpdateMsg msg = RiexSyncUpdateMsg(danet::BitStream(framemem_ptr()));
      if (riexsync::serialize_synced_ri_extra_pools(msg.get<0>(), /* full snapshot */ false, /* skip if empty */ true,
            /* mark all synced */ true))
        send_net_msg(::net::get_msg_sink(), eastl::move(msg));
      is_pool_sync_sent = true;
    }
  }
}

void shutdown()
{
  riexsync::clear_synced_ri_extra_pools();
  prev_ri_extra_map_size = -1;
  is_pool_sync_sent = true;
  sync_active = false;
}

} // namespace riexsync
