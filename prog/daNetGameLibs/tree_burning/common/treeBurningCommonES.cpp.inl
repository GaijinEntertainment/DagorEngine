// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <ecs/rendInst/riExtra.h>
#include <daECS/core/coreEvents.h>
#include <daECS/net/netEvents.h>
#include <daECS/net/netEvent.h>
#include <game/gameEvents.h>
#include <gamePhys/phys/rendinstDestr.h>
#include <memory/dag_framemem.h>
#include <net/channel.h>
#include <perfMon/dag_statDrv.h>
#include <rendInst/rendInstCollision.h>
#include <rendInst/rendInstAccess.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraAccess.h>
#include <rendInst/rendInstGen.h>
#include <phys/quantization.h>
#include "treeBurningCommon.h"

ECS_REGISTER_EVENT(EventDestroyBurnedTree);
ECS_REGISTER_EVENT(EventCreateTreeBurningChain);
ECS_REGISTER_EVENT(EventAddTreeBurningChainSource);

struct EventDestroyBurnedTreesInRadius : public ecs::Event
{
  ECS_BROADCAST_EVENT_DECL(EventDestroyBurnedTreesInRadius)
  Point3 pos;
  float radius;
  EventDestroyBurnedTreesInRadius(const Point3 &pos, float radius) :
    ECS_EVENT_CONSTRUCTOR(EventDestroyBurnedTreesInRadius), pos(pos), radius(radius)
  {}
};

ECS_TAG(server)
static void tree_burning_destroy_burned_tree_es(const EventDestroyBurnedTree &evt)
{
  rendinst::riex_handle_t handle = evt.get<0>();
  if (handle == rendinst::RIEX_HANDLE_NULL)
    return;

  rendinst::RendInstDesc riDesc(handle);
  rendinst::CollisionInfo collInfo = rendinst::getRiGenDestrInfo(riDesc);
  Point3_vec4 treeCenter;
  v_stu(&treeCenter.x, rendinst::getRIGenExtraBSphere(handle));
  Point3 impulse = Point3(0, 0, 0);

  rendinstdestr::create_tree_rend_inst_destr(riDesc, true /*add_restorable*/, treeCenter, impulse, true /*create_phys*/,
    true /*constrained_phys*/, 0.f, 0.f, &collInfo, true /*create_destr*/, false /*from_damage*/);
}

ECS_REGISTER_EVENT(EventDestroyBurnedTreesInRadius);

static void tree_burning_destroy_burned_trees_es(const EventDestroyBurnedTreesInRadius &evt)
{
  rendinst::riex_collidable_t riHandles;
  rendinst::gatherRIGenExtraCollidable(riHandles, BSphere3(evt.pos, evt.radius), true /*read_lock*/);

  for (rendinst::riex_handle_t handle : riHandles)
  {
    // I think we can remove any riExtra with posInst==true, they should be trees and only burned trees are riExtra and have collision.
    // If someday it becomes incorrect, we can store burned tree handles and check them here.
    if (rendinst::isRIExtraGenPosInst(rendinst::handle_to_ri_type(handle)))
    {
      rendinst::RendInstDesc riDesc(handle);
      rendinst::CollisionInfo collInfo = rendinst::getRiGenDestrInfo(riDesc);
      if (!collInfo.treeBehaviour) [[unlikely]]
      {
        logerr("Can't destroy burned tree '%s' without treeBehaviour flag", rendinst::getRIGenExtraName(riDesc.pool));
        continue;
      }
      Point3_vec4 treeCenter;
      v_stu(&treeCenter.x, rendinst::getRIGenExtraBSphere(handle));
      Point3 impulse = normalize(treeCenter - evt.pos) * 10;

      rendinstdestr::create_tree_rend_inst_destr(riDesc, true /*add_restorable*/, treeCenter, impulse, true /*create_phys*/,
        true /*constrained_phys*/, 0.f, 0.f, &collInfo, true /*create_destr*/, false /*from_damage*/);
    }
  }
}


static void net_rcv_tree_burning_initial_sync_msg(const net::IMessage *msg)
{
  TIME_PROFILE(net_rcv_tree_burning_initial_sync_msg);
  auto burntTreesMsg = msg->cast<TreeBurningInitialSyncMsg>();
  G_ASSERT(burntTreesMsg);
  const danet::BitStream &bs = burntTreesMsg->get<0>();
  deserialize_tree_burning_data(bs);
}

ECS_NET_IMPL_MSG(TreeBurningInitialSyncMsg,
  net::ROUTING_SERVER_TO_CLIENT,
  &net::direct_connection_rcptf,
  RELIABLE_UNORDERED,
  NC_DEFAULT,
  net::MF_DEFAULT_FLAGS,
  ECS_NET_NO_DUP,
  &net_rcv_tree_burning_initial_sync_msg);

constexpr float PACK_SCALE_MUL = 256.f;

void serialize_tree_burning_data(danet::BitStream &bs, const BurntTreesRiExHandles &burnt_trees)
{
  G_ASSERT(burnt_trees.size());
  bs.WriteCompressed(burnt_trees.size());
  for (rendinst::riex_handle_t riHandle : burnt_trees)
  {
    rendinst::RendInstDesc riDesc = rendinst::get_restorable_desc(rendinst::RendInstDesc(riHandle));
    G_ASSERT(riDesc.cellIdx < 0);
    bs.WriteCompressed(uint16_t(-riDesc.cellIdx - 1));
    bs.WriteCompressed(uint16_t(riDesc.pool));
    bs.WriteCompressed(riDesc.offs);

    mat44f tm;
    rendinst::getRIGenExtra44(riHandle, tm);
    vec3f pos, scl;
    quat4f rot;
    v_mat4_decompose(tm, pos, rot, scl);
    Point3_vec4 p;
    v_st(&p.x, pos);
    QuantizedWorldPos qpos(p);
    Quat q;
    v_stu(&q.x, rot);
    gamemath::QuantizedQuat32 qrot(q);
    uint16_t qscl = v_extract_x(scl) * PACK_SCALE_MUL;
    bs.Write(qpos);
    bs.Write(qrot);
    bs.Write(qscl);
  }
}

static danet::BitStream pending_burnt_trees_data;

void deserialize_tree_burning_data(const danet::BitStream &bs)
{
  if (!rendinst::isRiExtraLoaded())
  {
    pending_burnt_trees_data = bs;
    return;
  }
  dag::Vector<rendinst::riex_handle_t, framemem_allocator> burnt_trees;
  uint32_t treeCount;
  bs.ReadCompressed(treeCount);
  burnt_trees.reserve(treeCount);
  for (uint32_t i = 0; i < treeCount; i++)
  {
    uint16_t cellIdx, pool;
    uint32_t offset;
    bs.ReadCompressed(cellIdx);
    bs.ReadCompressed(pool);
    bs.ReadCompressed(offset);

    QuantizedWorldPos qpos;
    gamemath::QuantizedQuat32 qrot;
    uint16_t qscl;
    bs.Read(qpos);
    bs.Read(qrot);
    bs.Read(qscl);
    mat44f tm;
    Point3 pos = qpos.unpackPos();
    Quat rot = qrot.unpackQuat();
    float scl = qscl / PACK_SCALE_MUL;
    v_mat44_compose(tm, v_ldu(&pos.x), v_ldu(&rot.x), v_splats(scl));
    rendinst::riex_handle_t handle = rendinst::addRIGenExtra44(pool, tm, true, cellIdx, offset);
    G_ASSERT_CONTINUE(handle != rendinst::RIEX_HANDLE_NULL);
    burnt_trees.push_back(handle);
  }
  apply_tree_burning_data_for_render(burnt_trees);
}

static void deserialize_pending_tree_burning_data_es(const EventRendinstsLoaded &)
{
  if (!pending_burnt_trees_data.GetNumberOfUnreadBits())
    return;
  G_ASSERT(rendinst::isRiExtraLoaded());
  deserialize_tree_burning_data(pending_burnt_trees_data);
  pending_burnt_trees_data.Clear();
}

static void clear_tree_burning_data_on_game_term_es(const EventOnGameTerm &) { pending_burnt_trees_data.Clear(); }
