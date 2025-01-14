// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "game/riDestr.h"
#include "game/gameEvents.h"
#include "main/level.h"
#include "net/net.h"
#include "net/recipientFilters.h"
#include <daECS/core/coreEvents.h>

#include <ecs/rendInst/riExtra.h>
#include <ecs/core/entityManager.h>
#include <daECS/net/msgDecl.h>
#include <ecs/anim/anim.h>
#include <ecs/game/generic/grid.h>
#include <daECS/net/msgSink.h>
#include <daECS/net/replay.h>
#include <util/dag_stlqsort.h>

#include <math/dag_frustum.h>

#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstAccess.h>
#include <rendInst/rendInstExtra.h>
#include <gamePhys/phys/rendinstDestr.h>
#include <gamePhys/phys/destructableObject.h>
#include <gamePhys/collision/collisionLib.h>
#include <gamePhys/collision/rendinstCollision.h>
#include <gamePhys/collision/physLayers.h>

#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include "net/dedicated.h"
#include "net/channel.h"
#include "phys/gridCollision.h"
#include "main/gameObjects.h"
#include "main/gameLoad.h"

#include "render/renderer.h"
#include "camera/sceneCam.h"
#include "sound/rendinstSound.h"

namespace ridestr
{
static void net_rcv_ri_destr_snapshot(const net::IMessage *msg);
static void net_rcv_ri_destr_update(const net::IMessage *msg);
static void net_rcv_ri_pools_update(const net::IMessage *msg);
} // namespace ridestr

ECS_NET_DECL_MSG(RiDestrSnapshotMsg, danet::BitStream);
ECS_NET_IMPL_MSG(RiDestrSnapshotMsg,
  net::ROUTING_SERVER_TO_CLIENT,
  &net::broadcast_rcptf,
  RELIABLE_ORDERED,
  NC_DEFAULT,
  net::MF_DEFAULT_FLAGS,
  ECS_NET_NO_DUP,
  &ridestr::net_rcv_ri_destr_snapshot);
ECS_NET_DECL_MSG(RiDestrUpdateMsg, danet::BitStream);
ECS_NET_IMPL_MSG(RiDestrUpdateMsg,
  net::ROUTING_SERVER_TO_CLIENT,
  &net::direct_connection_rcptf,
  RELIABLE_UNORDERED,
  NC_DEFAULT,
  net::MF_DEFAULT_FLAGS,
  ECS_NET_NO_DUP,
  &ridestr::net_rcv_ri_destr_update);
// IMPORTANT: pool updates MUST be ordered, otherwise empty snapshot, that players usually receive during entering the match,
// might come later than pool updates and override them. RiPoolDataUpdateMsg can be unordered
ECS_NET_DECL_MSG(RiPoolDataUpdateMsg, danet::BitStream);
ECS_NET_IMPL_MSG(RiPoolDataUpdateMsg,
  net::ROUTING_SERVER_TO_CLIENT,
  &net::broadcast_rcptf,
  RELIABLE_ORDERED,
  0,
  net::MF_DEFAULT_FLAGS,
  ECS_NET_NO_DUP,
  &ridestr::net_rcv_ri_pools_update);

template <typename Callable>
inline void destroyable_ents_ecs_query(Callable c);
template <typename Callable>
inline ecs::QueryCbResult destroyed_ladder_ecs_query(Callable c);
template <typename Callable>
inline void players_ecs_query(Callable c);

namespace ridestr
{
static dag::Vector<rendinstdestr::DestrUpdateDesc> unsynced_destr_data;
static uint16_t cur_ridestr_ver = 0, cached_ridestr_ver = 0, update_ridestr_ver = 0;
static danet::BitStream cached_serialized_initial_ridestr; // Full ridestr update cached on server
static int prev_ri_extra_map_size = -1;
static bool is_pool_sync_sent = true;

static void ensure_all_ri_extra_pools_are_synced()
{
  // check, if rend inst pool list has changed, and updates synced rendinsts,
  // if nothing has changed (most cases) this call is free
  if (rendinst::getRIExtraMapSize() != prev_ri_extra_map_size)
  {
    prev_ri_extra_map_size = rendinst::getRIExtraMapSize();
    is_pool_sync_sent = false;
    rendinstdestr::sync_all_ri_extra_pools();
  }
}

static void on_ridestr_changed_server(
  const rendinst::RendInstDesc &desc, const TMatrix &ri_tm, const Point3 &pos, const Point3 &impulse)
{
  G_ASSERT(is_server());
  if (sceneload::unload_in_progress)
    return;
  if (rendinstdestr::get_destr_settings().destrNewUpdateMessage)
  {
    rendinst::RendInstDesc restDesc = rendinst::get_restorable_desc(desc);
    if (!restDesc.isValid())
      return;

    G_ASSERT(abs(restDesc.cellIdx) <= SHRT_MAX);
    G_ASSERT(restDesc.pool <= USHRT_MAX);
    G_STATIC_ASSERT((rendinstdestr::QuantizedDestrImpulsePos::XYZBits & (CHAR_BIT - 1)) == 0); // perf
    G_STATIC_ASSERT((rendinstdestr::QuantizedDestrImpulseVec::XYZBits & (CHAR_BIT - 1)) == 0);
    mat44f tm, itm;
    v_mat44_make_from_43cu_unsafe(tm, ri_tm.array);
    v_mat44_inverse43(itm, tm);
    Point3_vec4 localImpPos;
    v_st(&localImpPos.x, v_mat44_mul_vec3p(itm, v_ldu(&pos.x)));

    bool clamped; // clamp max value without logerr
    rendinstdestr::DestrUpdateDesc &destrDesc = *(rendinstdestr::DestrUpdateDesc *)unsynced_destr_data.push_back_uninitialized();
    destrDesc.offs = restDesc.offs;
    destrDesc.poolIdx = uint16_t(restDesc.pool);
    destrDesc.cellIdx = int16_t(restDesc.cellIdx);
    destrDesc.riPos = ri_tm.getcol(3);
    destrDesc.serializedLocalPos.packPos(localImpPos, &clamped);
    destrDesc.serializedImpulse.packPos(impulse, &clamped);
    G_ASSERT(eastl::find(unsynced_destr_data.begin(), unsynced_destr_data.end() - 1, unsynced_destr_data.back()) ==
             unsynced_destr_data.end() - 1);
    // logerr("Impulse pos error %.3f impulse error %.3f localPos " FMT_P3 " imp " FMT_P3, impulse.lengthSq() != 0.f ? (pos - ri_tm *
    // destrDesc.serializedLocalPos.unpackPos()).length() : 0.f, (impulse - destrDesc.serializedImpulse.unpackPos()).length(),
    // P3D(localImpPos), P3D(impulse));
  }
  cur_ridestr_ver++;
}

static const Point3_vec4 ri_collision_check_margin = {0.16f, 0.16f, 0.16f};

static void on_ri_destroyed_destroy_and_update_ladders_around(const bbox3f &ri_bbox, const BBox3 &bbox)
{
  if (const GameObjects *game_objects = ECS_GET_SINGLETON_COMPONENT(GameObjects, game_objects))
  {
    if (game_objects->ladders)
    {
      const bool isServer = is_server();

      BBox3 lbbox = bbox;
      lbbox.inflate(1.f);
      bbox3f lbox = v_ldu_bbox3(lbbox);
      game_objects->ladders->boxCull<false, true>(lbox, 0, 0, [&](scene::node_index idx, mat44f_cref tm) {
        if (!v_bbox3_test_pt_inside(ri_bbox, tm.col3))
        {
          if (isServer)
          {
            const float pt1[] = {0.0f, -0.5f, 0.0f, 0.0f};
            const float pt2[] = {0.0f, 0.5f, 0.0f, 0.0f};
            vec4f top = v_mat44_mul_vec3p(tm, v_ldu(pt1));
            vec4f bot = v_mat44_mul_vec3p(tm, v_ldu(pt2));
            if (v_bbox3_test_pt_inside(lbox, top) || v_bbox3_test_pt_inside(lbox, bot))
            {
              alignas(16) TMatrix tm2;
              v_mat_43ca_from_mat44(tm2.m[0], tm);
              g_entity_mgr->broadcastEventImmediate(EventLadderUpdate(tm2));
            }
          }
          return;
        }

        alignas(16) TMatrix tm2;
        v_mat_43ca_from_mat44(tm2.m[0], tm);
        game_objects->ladders->destroy(idx);

        if (isServer)
        {
          g_entity_mgr->broadcastEventImmediate(EventLadderUpdate(tm2));

          destroyed_ladder_ecs_query([idx](ecs::EntityId eid, uint32_t &ladder__sceneIndex) {
            if (ladder__sceneIndex == idx)
            {
              ladder__sceneIndex = -1;
              g_entity_mgr->destroyEntity(eid);
              return ecs::QueryCbResult::Stop;
            }
            return ecs::QueryCbResult::Continue;
          });
        }
      });
    }
  }
}


static void calc_ri_bbox_and_check_radius(
  const BBox3 &box, const TMatrix &tm, bbox3f &out_ri_bbox, BBox3 &out_bbox, float *out_check_radius)
{
  out_ri_bbox = v_ldu_bbox3(box);
  v_bbox3_extend(out_ri_bbox, v_ld(&ri_collision_check_margin.x));

  if (out_check_radius)
    *out_check_radius = v_extract_x(v_bbox3_outer_rad_from_zero(out_ri_bbox));

  mat44f boxTm;
  v_mat44_make_from_43cu_unsafe(boxTm, tm.array);
  v_bbox3_init(out_ri_bbox, boxTm, out_ri_bbox);
  v_stu_bbox3(out_bbox, out_ri_bbox);
}


static void destroy_entities_in_radius(const TMatrix &tm, float check_radius)
{
  // 5us for 977 entities on i7-8700
  // It triggers on every rendinst being destroyed, not only necessary one
  // Can be refactored if we add rendinst->entity relation
  vec3f tm_pos = v_ldu(&tm.getcol(3).x), checkRadiusSq = v_splats(check_radius * check_radius);
  destroyable_ents_ecs_query([&](ecs::EntityId eid, const TMatrix &transform ECS_REQUIRE(ecs::Tag destroyable_with_rendinst)) {
    if (v_test_vec_x_lt(v_length3_sq(v_sub(tm_pos, v_ldu(&transform.getcol(3).x))), checkRadiusSq))
      g_entity_mgr->destroyEntity(eid);
  });
}


static void destroy_entities_in_box(const bbox3f &bbox)
{
  destroyable_ents_ecs_query([&](ecs::EntityId eid, const TMatrix &transform ECS_REQUIRE(ecs::Tag destroyable_with_rendinst)) {
    const vec3f pos = v_ldu(&transform.getcol(3).x);
    if (v_bbox3_test_pt_inside(bbox, pos))
      g_entity_mgr->destroyEntity(eid);
  });
}


static void on_ri_destroyed_net_client_cb(rendinst::riex_handle_t riex_handle, const TMatrix &tm, const BBox3 &box)
{
  G_ASSERT(!is_server());
  if (sceneload::unload_in_progress)
    return;

  bbox3f riBBox;
  BBox3 bbox;
  calc_ri_bbox_and_check_radius(box, tm, riBBox, bbox, nullptr);

  on_ri_destroyed_destroy_and_update_ladders_around(riBBox, bbox);
  g_entity_mgr->broadcastEvent(EventRendinstDestroyed(riex_handle, tm, box));
}

static void on_ri_destroyed_server_cb(rendinst::riex_handle_t riex_handle, const TMatrix &tm, const BBox3 &box)
{
  // Only server have authority to destroy entities that have replication component,
  // set another callback if you need client code to be executed on rendinst destruction
  G_ASSERT(is_server());
  if (sceneload::unload_in_progress)
    return;

  if (ECS_GET_OR(find_ri_extra_eid(riex_handle), ri_extra__isBeingReplaced, false))
    return;

  bbox3f riBBox;
  BBox3 bbox;
  float checkRadius;
  calc_ri_bbox_and_check_radius(box, tm, riBBox, bbox, &checkRadius);

  // loot
  for_each_entity_in_grid(ECS_HASH("loot"), bbox, GridEntCheck::BOUNDING, [](ecs::EntityId eid, vec3f) {
    if (g_entity_mgr->has(eid, ECS_HASH("disableDropOnRiDestroyd")))
      return;
    TMatrix &transform = g_entity_mgr->getRW<TMatrix>(eid, ECS_HASH("transform"));
    float t = 256.f;
    float epsilon = 0.003; // FLT_EPSILON did not solve the problem
    Point3 raytraceOrigin = transform.getcol(3);
    raytraceOrigin.y += epsilon;
    int traceFlags = dacoll::ETF_RI | dacoll::ETF_FRT | dacoll::ETF_LMESH;
    if (dacoll::traceray_normalized(raytraceOrigin, Point3(0, -1.f, 0), t, nullptr, nullptr, traceFlags))
    {
      raytraceOrigin.y -= t;
      transform.setcol(3, raytraceOrigin);
    }
  });

  on_ri_destroyed_destroy_and_update_ladders_around(riBBox, bbox);

  destroy_entities_in_box(riBBox);

  g_entity_mgr->broadcastEventImmediate(EventRendinstDestroyed(riex_handle, tm, box));
}

static void on_ri_destroyed_render_cb(rendinst::riex_handle_t, const TMatrix &tm, const BBox3 &box)
{
  IRenderWorld *renderer = get_world_renderer();
  if (renderer && !sceneload::unload_in_progress)
  {
    mat44f mat;
    v_mat44_make_from_43cu_unsafe(mat, tm.array);
    bbox3f vFullBBox;
    v_bbox3_init(vFullBBox, mat, v_ldu_bbox3(box));
    BBox3 fullBBox;
    v_stu_bbox3(fullBBox, vFullBBox);
    renderer->shadowsInvalidate(fullBBox);
    renderer->invalidateGI(box, tm, fullBBox);
  }
}


static void on_sweep_rendinst_server_cb(const rendinst::RendInstDesc &desc)
{
  // Only server have authority to destroy entities that have replication component,
  // set another callback if you need client code to be executed on rendinst destruction
  G_ASSERT(is_server());
  if (sceneload::unload_in_progress)
    return;

  const TMatrix tm = rendinst::getRIGenMatrix(desc);
  const BBox3 box = rendinst::getRIGenBBox(desc);
  if (desc.isRiExtra())
  {
    rendinstdestr::call_on_rendinst_destroyed_cb(desc.getRiExtraHandle(), tm, box);
    return;
  }

  bbox3f riBBox;
  BBox3 bbox;
  float checkRadius;
  calc_ri_bbox_and_check_radius(box, tm, riBBox, bbox, &checkRadius);

  BBox3 canopyBBox;
  if (rendinst::getRIGenCanopyBBox(desc, tm, canopyBBox))
    destroy_entities_in_box(v_ldu_bbox3(canopyBBox));
  else
    destroy_entities_in_radius(tm, checkRadius);
}


template <typename Callable>
inline void riextra_eid_ecs_query(ecs::EntityId eid, Callable c);

static void on_riex_destruction_cb(rendinst::riex_handle_t handle,
  bool /*is_dynamic*/,
  bool create_destr_effects,
  int32_t /*user_data*/,
  const Point3 &impulse,
  const Point3 &impulse_pos)
{
  riextra_eid_ecs_query(find_ri_extra_eid(handle),
    [handle, impulse, impulse_pos, create_destr_effects](
      const TMatrix &transform ECS_REQUIRE(ecs::Tag isRendinstDestr, eastl::false_type ri_extra__destroyed = false),
      bool ri_extra__destrFx = true) {
      if (ri_extra__destrFx)
        rendinstdestr::destroyRiExtra(handle, transform, create_destr_effects, impulse, impulse_pos);
    });
  rendinst::removeRIGenExtraFromGrid(handle);
}

static void net_rcv_ri_destr_snapshot(const net::IMessage *msg)
{
  TIME_PROFILE(net_rcv_ri_destr_snapshot);
  auto ridestrMsg = msg->cast<RiDestrSnapshotMsg>();
  G_ASSERT(ridestrMsg);
  ensure_all_ri_extra_pools_are_synced();
  const danet::BitStream &bs = ridestrMsg->get<0>();
  bool done = rendinstdestr::deserialize_synced_ri_extra_pools(bs);
  G_ASSERT(done);
  bs.AlignReadToByteBoundary();
  done &= rendinstdestr::deserialize_destr_data(bs, 0, rendinstdestr::get_destr_settings().riMaxSimultaneousDestrs);
  G_ASSERT(!done || bs.GetNumberOfUnreadBits() == 0);
  G_UNUSED(done);
}

static void net_rcv_ri_destr_update(const net::IMessage *msg)
{
  TIME_PROFILE(net_rcv_ri_destr_update);
  auto ridestrMsg = msg->cast<RiDestrUpdateMsg>();
  G_ASSERT(ridestrMsg);
  ensure_all_ri_extra_pools_are_synced();
  bool done = rendinstdestr::deserialize_destr_update(ridestrMsg->get<0>());
  G_ASSERT(!done || ridestrMsg->get<0>().GetNumberOfUnreadBits() == 0);
  G_UNUSED(done);
}

static void net_rcv_ri_pools_update(const net::IMessage *msg)
{
  const RiPoolDataUpdateMsg *riMsg = msg->cast<RiPoolDataUpdateMsg>();
  G_ASSERT(riMsg);
  ensure_all_ri_extra_pools_are_synced();
  const bool ok = rendinstdestr::deserialize_synced_ri_extra_pools(riMsg->get<0>());
  G_ASSERT(ok && riMsg->get<0>().GetNumberOfUnreadBits() == 0);
  G_UNUSED(ok);
}

void init(bool have_render)
{
  destructables::init(dgs_get_settings());
  rendinstdestr::init(is_server() ? &on_ridestr_changed_server : nullptr, true, //
    &rendinstdestr::create_tree_rend_inst_destr, &rendinstdestr::remove_tree_rendinst_destr, &rendinstsound::rendinst_tree_sound_cb,
    have_render ? +[] { return get_cam_itm().getcol(3); } : nullptr);

  if (have_render)
  {
    rendinstdestr::set_ri_damage_effect_cb((rendinst::ri_damage_effect_cb)&ri_destr_start_effect);
    rendinstdestr::set_on_rendinst_destroyed_cb(on_ri_destroyed_render_cb);
    rendinstdestr::get_mutable_destr_settings().hasStaticShadowsInvalidationCallback = true;
    rendinst::registerRiExtraDestructionCb(on_riex_destruction_cb);
  }

  rendinstdestr::get_mutable_destr_settings().hasSound = rendinstsound::have_sound();

  if (is_server())
  {
    rendinstdestr::set_on_rendinst_destroyed_cb(on_ri_destroyed_server_cb);
    rendinst::sweep_rendinst_cb = on_sweep_rendinst_server_cb;
  }
  else
    rendinstdestr::set_on_rendinst_destroyed_cb(on_ri_destroyed_net_client_cb);

  rendinst::registerRiExtraDestructionCb([](rendinst::riex_handle_t handle, bool /*is_dynamic*/, bool /*create_destr_effects*/,
                                           int32_t user_data, const Point3 & /*impulse*/, const Point3 & /*impulse_pos*/) {
    ecs::EntityId riexEid = find_ri_extra_eid(handle);
    ecs::EntityId offenderEid = user_data != -1 ? ecs::EntityId(user_data) : ECS_GET_OR(riexEid, riOffender, ecs::INVALID_ENTITY_ID);
    bool *ri_extra__destroyed = ECS_GET_NULLABLE_RW(bool, riexEid, ri_extra__destroyed);
    if (ri_extra__destroyed && *ri_extra__destroyed == false)
    {
      g_entity_mgr->sendEventImmediate(riexEid, EventRiExtraDestroyed(offenderEid));
      *ri_extra__destroyed = true;
    }
  });

  rendinstdestr::DestrSettings &destrSettings = rendinstdestr::get_mutable_destr_settings();
  destrSettings.isNetClient = !is_server();
  destrSettings.createDestr = have_render;
  destrSettings.hitPointsToDestrImpulseMult = ::dgs_get_game_params()->getBlockByNameEx("riDestr")->getReal(
    "hitPointsToDestrImpulseMult", destrSettings.hitPointsToDestrImpulseMult);
  destrSettings.destrImpulseHitPointsMult = ::dgs_get_game_params()->getBlockByNameEx("riDestr")->getReal("destrImpulseHitPointsMult",
    destrSettings.destrImpulseHitPointsMult);
  destrSettings.destrImpulseSendDist =
    ::dgs_get_game_params()->getBlockByNameEx("riDestr")->getReal("destrImpulseSendDist", destrSettings.destrImpulseSendDist);
  destrSettings.destrImpulseSendByDefault = ::dgs_get_game_params()->getBlockByNameEx("riDestr")->getBool("destrImpulseSendByDefault",
    destrSettings.destrImpulseSendByDefault);
  destrSettings.destrMaxUpdateSize =
    ::dgs_get_game_params()->getBlockByNameEx("riDestr")->getInt("destrMaxUpdateSize", destrSettings.destrMaxUpdateSize);
  destrSettings.riMaxSimultaneousDestrs =
    ::dgs_get_game_params()->getBlockByNameEx("riDestr")->getInt("riMaxSimultaneousDestrs", destrSettings.riMaxSimultaneousDestrs);
  destrSettings.destrNewUpdateMessage = true;

  rendinstdestr::tree_destr_load_from_blk(*::dgs_get_game_params()->getBlockByNameEx("riDestr")->getBlockByNameEx("treeDestr"));
}

void update_cached_serialized_ridestr()
{
  if (cur_ridestr_ver != cached_ridestr_ver)
  {
    cached_serialized_initial_ridestr.ResetWritePointer();
    rendinstdestr::serialize_destr_data(cached_serialized_initial_ridestr);
    cached_ridestr_ver = cur_ridestr_ver;
  }
}

void send_initial_ridestr(net::IConnection &conn)
{
  update_cached_serialized_ridestr();
  if (!cached_serialized_initial_ridestr.GetNumberOfBitsUsed())
    return;

  ensure_all_ri_extra_pools_are_synced();
  RiDestrSnapshotMsg msg(danet::BitStream(1024, framemem_ptr()));
  rendinstdestr::serialize_synced_ri_extra_pools(msg.get<0>(), /* full snapshot */ true, /* skip if empty */ false);
  msg.get<0>().WriteAlignedBytes(cached_serialized_initial_ridestr.GetData(),
    cached_serialized_initial_ridestr.GetNumberOfBytesUsed());
  net::MessageNetDesc md = msg.getMsgClass();
  md.rcptFilter = &net::direct_connection_rcptf;
  msg.connection = &conn;
  send_net_msg(net::get_msg_sink(), eastl::move(msg), &md);
}

static void flush_ridestr_update()
{
  ensure_all_ri_extra_pools_are_synced();
  if (!is_pool_sync_sent)
  {
    // do not reserve memory, there might be nothing to send
    RiPoolDataUpdateMsg msg = RiPoolDataUpdateMsg(danet::BitStream(framemem_ptr()));
    if (rendinstdestr::serialize_synced_ri_extra_pools(msg.get<0>(), /* full snapshot */ false, /* skip if empty */ true))
      send_net_msg(::net::get_msg_sink(), eastl::move(msg));
    is_pool_sync_sent = true;
  }
  if (unsynced_destr_data.empty())
    return;

  const rendinstdestr::DestrSettings &destrSettings = rendinstdestr::get_destr_settings();
  G_ASSERT(destrSettings.destrMaxUpdateSize <= UCHAR_MAX);
  uint32_t updateSize = min(unsynced_destr_data.size(), destrSettings.destrMaxUpdateSize);
  dag::Span<rendinstdestr::DestrUpdateDesc> update = make_span(unsynced_destr_data.data(), updateSize);
  // serialize function expect sorted elements grouped by sorted pools grouped by cells
  stlsort::sort_branchless(update.begin(), update.end());
  danet::BitStream bs((updateSize + 2) * 10, framemem_ptr());

  players_ecs_query([&](ecs::EntityId possessed, int connid ECS_REQUIRE_NOT(ecs::Tag playerIsBot)
                                                   ECS_REQUIRE(const ecs::auto_type &player, eastl::false_type disconnected)) {
    const TMatrix *possessedTm = ECS_GET_NULLABLE(TMatrix, possessed, transform);
    float cameraDistSq = sqr(destrSettings.destrImpulseSendDist);
    if (!possessedTm)
      cameraDistSq = destrSettings.destrImpulseSendByDefault ? VERY_BIG_NUMBER : 0.f;
    bs.Reset();
    RiDestrUpdateMsg msg(bs);
    const Point3 *cameraPos = possessedTm ? &possessedTm->getcol(3) : nullptr;
    rendinstdestr::serialize_destr_update(msg.get<0>(), cameraPos, cameraDistSq, update, destrSettings.destrImpulseSendByDefault,
      destrSettings.riMaxSimultaneousDestrs);
    msg.connection = get_client_connection(connid);
    send_net_msg(net::get_msg_sink(), eastl::move(msg));
  });
  if (net::IConnection *conn = net::get_replay_connection())
  {
    RiDestrUpdateMsg msg(danet::BitStream(512, framemem_ptr()));
    const Point3 *cameraPos = nullptr;
    rendinstdestr::serialize_destr_update(msg.get<0>(), cameraPos, VERY_BIG_NUMBER, update, destrSettings.destrImpulseSendByDefault,
      destrSettings.riMaxSimultaneousDestrs);
    msg.connection = conn;
    send_net_msg(net::get_msg_sink(), eastl::move(msg));
  }

  if (updateSize == unsynced_destr_data.size())
    unsynced_destr_data.clear();
  else
    unsynced_destr_data.erase(unsynced_destr_data.begin(), unsynced_destr_data.begin() + updateSize);
}

static void flush_dirty_ri_destr_msg()
{
  update_cached_serialized_ridestr();
  if (update_ridestr_ver == cur_ridestr_ver || !cached_serialized_initial_ridestr.GetNumberOfBitsUsed())
    return;

  update_ridestr_ver = cur_ridestr_ver;
  send_net_msg(net::get_msg_sink(), RiDestrSnapshotMsg(cached_serialized_initial_ridestr));
}

void update(float dt, const TMatrix4 &glob_tm)
{
  ensure_all_ri_extra_pools_are_synced();
  if (dedicated::is_dedicated())
  {
    rendinstdestr::update(dt, nullptr);
    if (rendinstdestr::get_destr_settings().destrNewUpdateMessage)
      flush_ridestr_update();
    else
      flush_dirty_ri_destr_msg();
  }
  else
  {
    const Frustum frustum(glob_tm);
    rendinstdestr::update(dt, &frustum);
  }
  destructables::update(dt);
}

void shutdown()
{
  rendinstdestr::shutdown();
  destructables::clear();
  cur_ridestr_ver = cached_ridestr_ver = 0;
  cached_serialized_initial_ridestr.Clear();
  clear_and_shrink(unsynced_destr_data);
  prev_ri_extra_map_size = -1;
  is_pool_sync_sent = true;
}

} // namespace ridestr

void rendinst_destr_es_event_handler(const EventLevelLoaded &) { rendinstdestr::startSession(dacoll::get_phys_world()); }

ECS_TAG(server)
ECS_REQUIRE(RiExtraComponent ri_extra)
ECS_REQUIRE_NOT(ecs::Tag undestroyableRiExtra)
void ri_extra_destroy_es_event_handler(const CmdDestroyRendinst &evt, ecs::EntityId eid)
{
  if (sceneload::unload_in_progress)
    return;

  const bool destroysEntity = evt.get<1>();
  if (destroysEntity)
    g_entity_mgr->destroyEntity(eid);
}
