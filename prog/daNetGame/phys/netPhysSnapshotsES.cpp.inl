// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "netPhysSnapshots.h"
#include "physUtils.h"
#include <daECS/net/msgDecl.h>
#include <daECS/net/msgSink.h>
#include <daECS/net/connection.h>
#include <EASTL/algorithm.h>
#include <EASTL/type_traits.h>
#include <EASTL/numeric_limits.h>
#include <math/dag_bits.h>
#include "net/net.h"
#include <daECS/net/replay.h>
#include <ecs/core/entityManager.h>
#include <daECS/net/object.h>
#include <daECS/net/network.h>
#include <util/dag_convar.h>
#include <memory/dag_framemem.h>
#include "game/dasEvents.h"
#include "game/gameEvents.h"
#include "game/player.h"
#include "game/team.h"
#include <statsd/statsd.h>
#include <stdlib.h>
#include <stddef.h> // offsetof
#include <daECS/net/netbase.h>
#include <ecs/core/utility/ecsRecreate.h>
#include <daECS/net/msgDecl.h>
#include <daNet/getTime.h>
#include "net/netStat.h"
#include "net/time.h"
#include "net/dedicated.h" // get_server_tick_time_us
#include "net/plosscalc.h"
#include "net/authEvents.h"
#include <daECS/net/recipientFilters.h>

#ifdef _MSC_VER
#pragma warning(disable : 4506) // warning C4506: no definition for inline function 'bool PhysActor<X,Y>::deserializePhysSnapshot(...
#pragma warning(disable : 4701) // warning C4701: potentially uninitialized local variable 'vPlayerLookAt|vPlayerLookDir' used
#endif

namespace physreg
{
extern size_t snapshotSizeMax;
extern StaticTab<const net::MessageClass *, (int)PhysType::NUM> snapshotMsgCls; //< parallel to snapshotMsgCb
extern StaticTab<void (*)(const net::IMessage *msg), (int)PhysType::NUM> snapshotMsgCb;
} // namespace physreg

static const int net_lod_zones_freq[] = {0, 0, 1, 3, 0}; // pow2-1: 100%, 100%, 50%, 25%
static ConVarI sv_force_serializetype("net.sv_force_serialize_type", -1, -1, (int)PhysSnapSerializeType::HIDDEN, nullptr);

class PhysSnapshotsMsgSender
{
public:
  PhysSnapshotsMsgSender(float ct) : bs(framemem_ptr()), curTime(ct) { reserveHeader(); } //-V730
  bool haveSpaceToWrite(danet::BitStream &tmpBs, int sizeLimit)
  {
    // try to fit in MTU (except first write in case if snapshot is bigger then MTU)
    return written <= UCHAR_MAX && !(written && bs.GetNumberOfBytesUsed() + tmpBs.GetNumberOfBytesUsed() > sizeLimit);
  }

  void write(const danet::BitStream &tmpBs)
  {
    G_ASSERT((tmpBs.GetNumberOfBitsUsed() & 7) == 0); // byte aligned
    bs.Write((const char *)tmpBs.GetData(), tmpBs.GetNumberOfBytesUsed());
    written++;
  }

  void send(send_msg_t sendf, ecs::EntityId receiver, game::Player *plr, net::IConnection &conn)
  {
    if (uint16_t *send_seq = (plr ? plr->getPhysSnapshotsSeqRW() : nullptr))
    {
      bs.Write(send_seq[1]++);
      if (!sentSomething)
      {
        bs.Write((uint8_t)eastl::min(int(dedicated::get_server_tick_time_us() / 1000.f + 0.5f), UCHAR_MAX));
        bs.Write((uint8_t)eastl::min(int(plr->getControlsPlossCalc().calcPacketLoss() * (UCHAR_MAX + 1) + 0.5f), UCHAR_MAX)); //-V1004
        if (dedicated::is_dynamic_tickrate_enabled())
          bs.Write((uint8_t)eastl::min(phys_get_tickrate(), UCHAR_MAX));
        sentSomething = true;
      }
    }
    G_ASSERTF((unsigned)(written - 1) <= UCHAR_MAX, "%d", written);
    bs.WriteAt((uint8_t)(written - 1), 0);

    sendf(receiver, eastl::move(bs), conn, /*reliable*/ false);

    reserveHeader();
  }

  void reserveHeader()
  {
    written = 0;
    bs.SetWriteOffset(0);
    bs.Write((uint8_t)0); // written
    bs.Write(curTime);
  }

  uint16_t written;
  bool sentSomething;
  float curTime;
  danet::BitStream bs;
};

template <typename Callable>
static __forceinline void base_phys_actor_eid_ecs_query(ecs::EntityId eid, Callable c);

ECS_REQUIRE(ecs::Tag msg_sink)
ECS_TAG(netClient)
static inline void net_phys_receive_snapshots_es_event_handler(const ecs::EventNetMessage &evt)
{
  const net::IMessage *msg = evt.get<0>().get();
  G_ASSERT(!has_network() || msg->connection);
  for (const net::MessageClass *&cls : physreg::snapshotMsgCls)
    if (cls == &msg->getMsgClass())
      return physreg::snapshotMsgCb[&cls - physreg::snapshotMsgCls.cbegin()](msg);
}

enum class PhysActorSyncState // Note: shall fit in 2 bits
{
  Normal,
  Asleep,
  Hidden,
  Invalid // can't happen
};

static void send_phys_snapshots_impl(net::IConnection &conn,
  danet::BitStream &tmpBs,
  PhysSnapshotsMsgSender &unreliableSender,
  float curTime,
  ecs::EntityId msg_sink_eid,
  dag::ConstSpan<SendRecordsTypeRange> sr_type_ranges,
  dag::ConstSpan<PhysActorSendRecord> send_records,
  game::Player *plr = nullptr,
  int *clientNetFlags = nullptr,
  int plrTeam = TEAM_UNASSIGNED,
  const vec4f *vPlayerLookAt = nullptr,
  const vec4f *vPlayerLookDir = nullptr)
{
  int connMTU = conn.getMTU();
  int connId = conn.getId();
  const uint16_t send_seq_value = plr ? plr->getPhysSnapshotsSeqRW()[0]++ : 0;

  if (!conn.isResponsive())
  {
    if (clientNetFlags && !(*clientNetFlags & CNF_CONN_UNRESPONSIVE))
    {
      debug("%s conn #%d became UNresponsive", __FUNCTION__, connId);
      *clientNetFlags |= CNF_CONN_UNRESPONSIVE;
    }
    return;
  }

  bool connBecameResponsive = clientNetFlags && (*clientNetFlags & CNF_CONN_UNRESPONSIVE);
  if (connBecameResponsive)
  {
    debug("%s conn #%d became responsive", __FUNCTION__, connId);
    *clientNetFlags &= ~CNF_CONN_UNRESPONSIVE;
  }

  uint32_t rangeStart = 0;
  bool connRecordingReplay = !clientNetFlags || (*clientNetFlags & CNF_REPLAY_RECORDING);
  unreliableSender.sentSomething = false;
  for (auto &sr_range : sr_type_ranges)
  {
    PhysSnapSerializeType pstBehind =
      (sr_range.physType == PhysType::HUMAN) ? PhysSnapSerializeType::MIN : PhysSnapSerializeType::NORMAL;
    auto serializePhysSnapshot = sr_range.serializePhysSnapshot;
    auto &syncStates = (*BasePhysActor::allSyncStates[(int)sr_range.physType])[connId];
    uint32_t curRangeStart = rangeStart;
    rangeStart += sr_range.size;
    for (auto &sr : make_span_const(send_records).subspan(curRangeStart, sr_range.size))
    {
      // Note: intentionally do not check net scope here since most of phys actors are always in scope anyway.
      // The only exception right now are CR fridges but it will work correctly since in scope check performed within network send

      // Do not send snapshots to player which controls this entity. He receives AAS already (unless replay recording active)
      if (!connRecordingReplay && net::ConnectionId(sr.connControlledBy) == connId)
        continue;

      // Only replay conn might have no player
      PhysSnapSerializeType pst = plr ? PhysSnapSerializeType::FARAWAY : PhysSnapSerializeType::CLOSEST;
      bool skipUpdate = false;
      PhysActorSyncState curSyncState = ((sr.flags & PHYS_ASLEEP) == 0) ? PhysActorSyncState::Normal : PhysActorSyncState::Asleep;
      if (vPlayerLookAt)
      {
        vec4f vpos = ((sizeof(PhysActorSendRecord) % 16) == 0 && (offsetof(PhysActorSendRecord, pos) % 16) == 0) ? v_ld(&sr.pos.x)
                                                                                                                 : v_ldu(&sr.pos.x);
        vec4f dirToActor = v_sub(vpos, *vPlayerLookAt);
        vec4f lenToEntitySq = v_length3_sq(dirToActor);
        int zitN = __popcount(v_signmask(v_cmp_gt(lenToEntitySq, sr_range.netLodZones)));
        int freqMinusOne = net_lod_zones_freq[zitN];

        DAECS_EXT_FAST_ASSERT(zitN <= (size_t)PhysSnapSerializeType::HIDDEN);
        DAECS_EXT_ASSERT(is_pow_of2(freqMinusOne + 1));

        pst = (PhysSnapSerializeType)zitN;

        if (pst == PhysSnapSerializeType::HIDDEN)
        {
          bool should_be_min_visible = game::is_teams_friendly(plrTeam, sr.team)  // teammates are visible regardless of it's distance
                                       || (sr.flags & PHYS_ALWAYS_SEND_SHANPSHOT) // marks are visible on a huge distance
            ;
          if (!should_be_min_visible)
            g_entity_mgr->sendEventImmediate(sr.eid, QueryPhysSnapshotMustBeMinVisible(plrTeam, &should_be_min_visible));
          if (should_be_min_visible)
            pst = PhysSnapSerializeType::MIN;
        }

        if (pst != PhysSnapSerializeType::HIDDEN)
        {
          // Humans in vehicles are synced for it's gun & look direction
          if ((sr.flags & IN_VEHICLE) && pst > PhysSnapSerializeType::NORMAL)
            continue;

          // Lower send quality & frequency send for entities that behind player's avatars (i.e. frustum approx)
          if (pst != PhysSnapSerializeType::MIN && vPlayerLookDir)
            if (v_test_vec_x_lt_0(v_dot3_x(*vPlayerLookDir, dirToActor)))
            {
              if (pst == PhysSnapSerializeType::CLOSEST)
                pst = PhysSnapSerializeType::NORMAL;
              else if (!connRecordingReplay)
              {
                freqMinusOne = eastl::max(1, freqMinusOne);
                pst = pstBehind;
              }
            }

          if (sr.getTickrateType() == PhysTickRateType::Normal)
            skipUpdate = freqMinusOne && (send_seq_value & freqMinusOne);
          else // PhysTickRateType::LowFreq
            skipUpdate = (sr.flags & TICK_NOT_CHANGED) != 0 || (freqMinusOne && (sr.physTick % eastl::min(freqMinusOne + 1, 3)));
        }
        else
          curSyncState = PhysActorSyncState::Hidden;
      }

#ifdef _DEBUG_TAB_
      if (sv_force_serializetype.get() >= 0)
        pst = (PhysSnapSerializeType)sv_force_serializetype.get();
#endif

      PhysActorSyncState prevSyncState = PhysActorSyncState::Normal;
      static constexpr unsigned RECSPERBYTE = 4; // CHAR_BIT/2 bits
      unsigned stByteIdx = sr.netPhysId / RECSPERBYTE;
      // If connection became responsive - resend all reliable state changes (asleep/hidden)
      if (uint8_t stbyte = (stByteIdx >= syncStates.size() || connBecameResponsive) ? 0 : syncStates[stByteIdx])
      {
        prevSyncState = PhysActorSyncState((stbyte >> (sr.netPhysId % RECSPERBYTE * 2)) % RECSPERBYTE);
        DAECS_EXT_FAST_ASSERT(prevSyncState != PhysActorSyncState::Invalid);
      }

      if (curSyncState != prevSyncState)
      {
        if (DAGOR_UNLIKELY(stByteIdx >= syncStates.size()))
          syncStates.resize(stByteIdx + 1);
        unsigned bshift = sr.netPhysId % RECSPERBYTE * 2;
        syncStates[stByteIdx] = (syncStates[stByteIdx] & ~(0b11 << bshift)) | (unsigned(curSyncState) << bshift);
      }

      if ((curSyncState == PhysActorSyncState::Normal) ? skipUpdate : (curSyncState == prevSyncState))
        continue;

      {
        tmpBs.ResetWritePointer();
        if (curSyncState != PhysActorSyncState::Normal)
          tmpBs.Write(curTime);
        DAECS_EXT_ASSERT(sr.netPhysId != USHRT_MAX);
        tmpBs.WriteCompressed(sr.netPhysId);
        unsigned pstWriteOffs = tmpBs.GetWriteOffset();
        DAECS_EXT_ASSERT((pstWriteOffs % CHAR_BIT) == 0); // byte aligned
        tmpBs.reserveBits(CHAR_BIT);
        tmpBs.GetData()[pstWriteOffs / CHAR_BIT] = (int)eastl::min(pst, PhysSnapSerializeType::MIN) << (CHAR_BIT - PST_BITS);
        tmpBs.SetWriteOffset(pstWriteOffs + PST_BITS);
        (sr.actor.*serializePhysSnapshot)(tmpBs, pst);
        DAECS_EXT_FAST_ASSERT(tmpBs.GetNumberOfBitsUsed() > (pstWriteOffs + PST_BITS));
      }

      if (curSyncState == PhysActorSyncState::Normal)
      {
        if (!unreliableSender.haveSpaceToWrite(tmpBs, connMTU))
          unreliableSender.send(sr_range.sendCb, msg_sink_eid, plr, conn);
        unreliableSender.write(tmpBs);
      }
      else // send reliably
      {
        // Note: we send here message to actor itself (instead of msg_sink) as it's only way to guarantee that message will be
        // delivered
        sr_range.sendCb(sr.eid, eastl::move(tmpBs), conn, /*reliable*/ true);
      }
    } // send records in range loop
    if (unreliableSender.written)
      unreliableSender.send(sr_range.sendCb, msg_sink_eid, plr, conn);
  } // send records ranges loop
}

template <typename Callable>
inline void player_snapshots_ecs_query(Callable c);

static void send_phys_snapshots(float curTime,
  ecs::EntityId msg_sink_eid,
  dag::ConstSpan<SendRecordsTypeRange> sr_type_ranges,
  dag::ConstSpan<PhysActorSendRecord> send_records)
{
  danet::BitStream tmpBs(framemem_ptr());
  // We serialize time instead of tick since different entities might have different timeStep (hence tick would be different too)
  PhysSnapshotsMsgSender unreliableSender(curTime);

  player_snapshots_ecs_query([&](ECS_REQUIRE_NOT(ecs::Tag playerIsBot) ecs::EntityId eid, game::Player &player, int connid,
                               int &clientNetFlags, int team = TEAM_UNASSIGNED) {
    net::IConnection *cptr = (connid != net::INVALID_CONNECTION_ID) ? get_client_connection(connid) : nullptr;

    if (!cptr || !(clientNetFlags & CNF_REPLICATE_PHYS_ACTORS))
      return;

    const Point3 *playerLookAt = nullptr, *playerLookDir = nullptr;
    vec4f vPlayerLookAt, vPlayerLookDir;
    bool spectating = false;
    ecs::EntityId plrLookAtEid = game::player_get_looking_at_entity(eid, spectating);
    playerLookAt = game::player_get_looking_at(plrLookAtEid, spectating, &playerLookDir);
    if (playerLookAt)
    {
      vPlayerLookAt = v_ldu(&playerLookAt->x);
      if (playerLookDir)
      {
        vPlayerLookDir = v_ldu(&playerLookDir->x);
        // Compensate for potential 3rd person view offset
        // To consider: check for player's 3rd/1st view somehow and use camera.offset accordingly
        do
        {
          float frustumOffset;
          if (ECS_GET_OR(plrLookAtEid, isInVehicle, false))
            frustumOffset = 10.f;
          else if (ECS_GET_OR(plrLookAtEid, isTpsView, false))
            frustumOffset = 2.f;
          else
            break;
          vPlayerLookAt = v_sub(vPlayerLookAt, v_mul(vPlayerLookDir, v_splats(frustumOffset)));
        } while (0);
      }
    }

    send_phys_snapshots_impl(*cptr, tmpBs, unreliableSender, curTime, msg_sink_eid, sr_type_ranges, send_records, &player,
      &clientNetFlags, team, (playerLookAt ? &vPlayerLookAt : nullptr), (playerLookDir ? &vPlayerLookDir : nullptr));
  });

  if (net::IConnection *cptr = net::get_replay_connection())
    send_phys_snapshots_impl(*cptr, tmpBs, unreliableSender, curTime, msg_sink_eid, sr_type_ranges, send_records);
}

extern net::CNetwork *get_net_internal();

void net_send_phys_snapshots(float curTime, float /*dt*/)
{
  net::CNetwork *netw = is_server() ? get_net_internal() : nullptr;
  if (!netw)
    return;

  GatherRecordsToSend evtCtx;
  evtCtx.sendRecords.reserve(128);
  g_entity_mgr->broadcastEventImmediate(GatherNetPhysRecordsToSend(&evtCtx));

  DAECS_EXT_ASSERT(net::get_msg_sink());
  send_phys_snapshots(curTime, net::get_msg_sink(), evtCtx.sendRecordsRanges, evtCtx.sendRecords);
}

FixedPlossCalc &phys_get_snapshots_ploss_calc()
{
  static auto plossCalc = eastl::make_unique<FixedPlossCalc>(); // yes, this is created on heap, singleton (needed only by client)
  return *plossCalc.get();
}

static void report_snapshots_ploss(net::sequence_t snap_seq)
{
  static net::sequence_t lastReportedCtrlSeq = 0;

  FixedPlossCalc &plossCalc = phys_get_snapshots_ploss_calc();
  plossCalc.pushSequence(snap_seq);
  if (net::seq_diff(snap_seq, lastReportedCtrlSeq) > plossCalc.HISTORY_SIZE)
  {
    lastReportedCtrlSeq = snap_seq;
    if (const char *addr = get_server_route_host(get_current_server_route_id()))
    {
      eastl::string country_code;
      g_entity_mgr->broadcastEventImmediate(NetAuthGetCountryCodeEvent{&country_code});
      statsd::profile("snapshots_rx_packet_loss_percent", plossCalc.calcPacketLoss() * 100.f, // 1 lost packet from 9*30 is 0.37%
        {{"addr", addr}, {"country", country_code.c_str()}});
    }
  }
}

void net_rcv_phys_snapshots(const net::IMessage &,
  bool to_msg_sink,
  const danet::BitStream &bs,
  net::IConnection &conn,
  phys_eids_list_t &eids_list,
  phys_actor_resolve_by_eid_t resolve_eid_cb,
  deserialize_snapshot_cb_t deserialize_snap_cb,
  process_snapshot_no_entity_cb_t &snapshot_no_entity_cb)
{
  float tsDelay = get_timespeed_accumulated_delay_sec();

  uint8_t numMinusOne = 0;
  float atTime = -1.f;
  if (to_msg_sink)
    G_VERIFY(bs.Read(numMinusOne));
  G_VERIFY(bs.Read(atTime));
  G_ASSERT_RETURN(atTime >= 0, );

  char *snapBuf = (char *)((intptr_t(alloca(physreg::snapshotSizeMax + sizeof(void *))) + sizeof(void *) - 1) & ~(sizeof(void *) - 1));
  for (int i = 0, n = numMinusOne + 1; i < n; ++i)
  {
    uint16_t netPhysId = -1;
    G_VERIFY(bs.ReadCompressed(netPhysId));

    auto pst = PhysSnapSerializeType::NORMAL;
    G_VERIFY(bs.ReadBits((uint8_t *)&pst, PST_BITS));

    BasePhysSnapshot &snap = *reinterpret_cast<BasePhysSnapshot *>(snapBuf);
    if (deserialize_snap_cb(bs, pst, snap, conn))
    {
      snap.atTime = atTime;
      // Might be null if snapshot for entity received before it's creation (due to different network channel)
      if (BasePhysActor *actor = (netPhysId < eids_list.size()) ? resolve_eid_cb(eids_list[netPhysId]) : nullptr)
      {
        float curDelay =
          (tsDelay < 1e-6f) ? 0.f : PhysUpdateCtx::ctx.getInterpDelay(actor->tickrateType, actor->getPhys().getTimeStep());
        actor->savePhysSnapshot(eastl::move(snap), curDelay);
      }
      else
        snapshot_no_entity_cb(netPhysId, eastl::move(snap));
    }
    else
    {
      G_ASSERTF(0, "failed to deserialize snapshot for actor eid=%d/netPhysId=%d",
        (netPhysId < eids_list.size()) ? (ecs::entity_id_t)eids_list[netPhysId] : 0, netPhysId);
      break;
    }
  }

  if (!to_msg_sink)
    return;

  uint16_t snap_seq = 0;
  if (bs.Read(snap_seq))
    report_snapshots_ploss(snap_seq);

  uint8_t serverTickMs = 0, rxCtrlLoss = 0;
  if (bs.Read(serverTickMs) && bs.Read(rxCtrlLoss))
  {
    if ((net::get_server_flags() & net::ServerFlags::Dev) == net::ServerFlags::None)
      netstat::set(netstat::CT_SERVER_TICK, serverTickMs);
    else
      netstat::set(netstat::CT_DEV_SERVER_TICK, serverTickMs);
    netstat::set(netstat::CT_CTRL_PLOSS, float(rxCtrlLoss) / UCHAR_MAX * 1000.);
  }

  uint8_t serverTickRate = 0;
  if (bs.Read(serverTickRate) && phys_set_tickrate(serverTickRate))
    debug("Change game tickrate to %d", serverTickRate);
}
