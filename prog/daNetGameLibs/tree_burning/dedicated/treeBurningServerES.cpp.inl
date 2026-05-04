// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <daECS/net/msgSink.h>
#include <daECS/net/netEvents.h>
#include <rendInst/rendInstCollision.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraAccess.h>
#include <rendInst/rendInstAccess.h>
#include <rendInst/rendInstGen.h>
#include <daECS/net/netEvent.h>
#include <gamePhys/phys/rendinstDestr.h>
#include <game/gameEvents.h>
#include <game/riDestr.h>
#include <net/channel.h>
#include <net/net.h>
#include <tree_burning/common/treeBurningCommon.h>

static BurntTreesRiExHandles burnt_trees;

ECS_ON_EVENT(on_appear)
static void tree_burning_replace_tree_on_server_es(
  const ecs::Event &, ecs::EntityId eid, float tree_burning__ignite_radius, const TMatrix &transform)
{
  dag::Vector<rendinst::RendInstDesc, framemem_allocator> treeDescs;
  struct ForeachTreeCB final : public rendinst::ForeachCB
  {
    dag::Vector<rendinst::RendInstDesc, framemem_allocator> *descs;
    virtual void executeForPos(RendInstGenData * /* rgl */, const rendinst::RendInstDesc &desc, const TMatrix & /* tm */)
    {
      descs->push_back(desc);
    }
  } cb;
  cb.descs = &treeDescs;
  BSphere3 sph(transform.col[3], tree_burning__ignite_radius);
  rendinst::foreachRIGenInSphere(sph, rendinst::GatherRiTypeFlag::RiGenOnly | rendinst::GatherRiTypeFlag::RiGenCanopy, cb);
  bool replaced = false;
  for (const rendinst::RendInstDesc &treeDesc : treeDescs)
  {
    BBox3 canopyBox;
    rendinst::getRIGenCanopyBBox(treeDesc, TMatrix::IDENT, canopyBox);
    TMatrix tm = rendinst::getRIGenMatrix(treeDesc);
    int canopyType = getRIGenCanopyShape(treeDesc);
    rendinst::riex_handle_t handle = rendinst::replaceRIGenWithRIExtra(treeDesc);
    if (handle != rendinst::RIEX_HANDLE_NULL)
    {
      g_entity_mgr->sendEvent(eid,
        EventCreateTreeBurningChain(handle, tm, canopyType, canopyBox.lim[0], canopyBox.lim[1], transform.col[3]));
      burnt_trees.insert(handle);
      replaced = true;
    }
  }
  if (replaced)
    ridestr::increment_ridestr_version();
}

static void send_burnt_trees_to_new_client_es(const EventOnClientConnected &evt)
{
  if (burnt_trees.empty())
    return;
  net::IConnection *conn = get_client_connection(evt.get<0>());
  if (!conn)
    return;
  danet::BitStream bs(framemem_ptr());
  serialize_tree_burning_data(bs, burnt_trees);
  TreeBurningInitialSyncMsg msg(bs);
  msg.connection = conn;
  send_net_msg(net::get_msg_sink(), eastl::move(msg));
}

static void invalidate_riex_handle_cb(rendinst::riex_handle_t handle) { burnt_trees.erase(handle); }

static void tree_burning_register_server_callback_es(const EventOnGameAppStarted &)
{
  rendinst::registerRIGenExtraInvalidateHandleCb(invalidate_riex_handle_cb);
}

static void tree_burning_clear_server_burnt_trees_es(const EventOnGameTerm &) { burnt_trees.clear(); }

void apply_tree_burning_data_for_render(dag::ConstSpan<rendinst::riex_handle_t>) { G_ASSERT(false); } // stub
