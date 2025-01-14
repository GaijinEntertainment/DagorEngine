// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "netPhys.h"
#include "net/dedicated.h"
#include "net/net.h" // is_true_net_server
#include <daECS/net/netbase.h>
#include "phys/physUtils.h"
#include <ecs/core/entityManager.h>
#include <ecs/game/generic/grid.h>
#include <daECS/net/message.h>
#include <daECS/net/recipientFilters.h>
#include <daECS/core/coreEvents.h>
#include <osApiWrappers/dag_events.h>
#include <util/dag_threadPool.h>
#include "phys/physEvents.h"
#include "game/player.h"
#include <gamePhys/phys/walker/humanPhys.h>
#include <gamePhys/collision/collisionLib.h>
#include <statsd/statsd.h>
#include <ecs/delayedAct/actInThread.h>

ECS_REGISTER_RELOCATABLE_TYPE(GridHolders, nullptr);
ECS_AUTO_REGISTER_COMPONENT(GridHolders, "pair_collision__gridHolders", nullptr, 0);

PhysUpdateCtx PhysUpdateCtx::ctx;

template <typename T>
static inline void do_send_auth_state_impl(IPhysActor *nu, T &&msg)
{
  auto actor = static_cast<const BasePhysActor *>(nu);
  net::IConnection *conn = rcptf::get_entity_ctrl_conn(actor->getEid());
  if (conn && conn->isResponsive())
    // Don't waste network bandwidth on sending AASes to humans within vehicles
    if (actor->physType != PhysType::HUMAN || !ECS_GET_OR(actor->getEid(), isInVehicle, false))
    {
      msg.connection = conn;
      send_net_msg(actor->getEid(), eastl::move(msg));
    }
}

void phys_send_auth_state(IPhysActor *nu, danet::BitStream &&state_data, float unit_time)
{
  do_send_auth_state_impl(nu, AAStateMsg(eastl::move(state_data), unit_time));

  auto actor = static_cast<BasePhysActor *>(nu);
  if (actor->physType == PhysType::HUMAN)
  {
    auto &physics = static_cast<HumanPhys &>(actor->getPhys());
    // we can calculate the metric with a 0.1% error, it's important because the metric may be small
    if (physics.numProcessedCT >= 1000)
    {
      unsigned processed = eastl::exchange(physics.numProcessedCT, 0u);
      unsigned forged = eastl::exchange(physics.numForgedCT, 0u);
      float forgedPct = static_cast<float>(forged) / static_cast<float>(processed);
      statsd::profile("net.forged_ctrls_pct", forgedPct);
    }
  }
}

void phys_send_part_auth_state(IPhysActor *nu, danet::BitStream &&state_data, int)
{
  do_send_auth_state_impl(nu, AAPartialStateMsg(eastl::move(state_data)));
}

template <typename T>
static inline void phys_broadcast_auth_state_impl(IPhysActor *nu, T &&msg)
{
  auto actor = static_cast<const BasePhysActor *>(nu);
  net::MessageNetDesc md = T::messageClass;
  md.rcptFilter = &net::broadcast_rcptf;
  send_net_msg(actor->getEid(), eastl::move(msg), &md);
}

void phys_send_broadcast_auth_state(IPhysActor *nu, danet::BitStream &&state_data, float unit_time)
{
  phys_broadcast_auth_state_impl(nu, AAStateMsg(eastl::move(state_data), unit_time));
}

void phys_send_broadcast_part_auth_state(IPhysActor *nu, danet::BitStream &&state_data, int)
{
  phys_broadcast_auth_state_impl(nu, AAPartialStateMsg(eastl::move(state_data)));
}

template <typename Callable>
static void get_current_local_delay_ecs_query(Callable c);

float get_timespeed_accumulated_delay_sec()
{
  float delay = 0.f;
  get_current_local_delay_ecs_query([&delay](float time_speed__accumulatedDelay) { delay = time_speed__accumulatedDelay; });
  return delay;
}

float phys_get_present_time_delay_sec(PhysTickRateType tr_type, float time_step, bool client_side)
{
  int ticks = (is_server() || !PHYS_ENABLE_INTERPOLATION || client_side) ? 0 : get_interp_delay_ticks(tr_type);
  return ticks * time_step + get_timespeed_accumulated_delay_sec();
}

void PhysUpdateCtx::update()
{
  additionalInterpDelay = is_true_net_server() ? 0.f : get_timespeed_accumulated_delay_sec();
  if (!is_server())
    for (int i = 0; i < interpDelayTicks.size(); ++i)
      interpDelayTicks[i] = get_interp_delay_ticks((PhysTickRateType)i);
}

int BasePhysActor::calcControlsTickDelta()
{
  if (getRole() != IPhysActor::ROLE_REMOTELY_CONTROLLED_AUTHORITY)
    return 0;
  const int maxTicksDelta = (int)ceilf(PHYS_MAX_CONTROLS_TICKS_DELTA_SEC / getPhys().getTimeStep());
  if (const int *controlsTickDelta = ECS_GET_NULLABLE(int, eid, net__controlsTickDelta))
    return eastl::min(*controlsTickDelta, maxTicksDelta);
  game::Player *plr = game::find_player_that_possess(eid);
  return plr ? eastl::min(plr->calcControlsTickDelta(), maxTicksDelta) : 0;
}

#if _TARGET_C1 | _TARGET_XBOXONE
struct ReserveOneThreadpoolWorkerJob final : public cpujobs::IJob
{
  os_event_t evt;
  ReserveOneThreadpoolWorkerJob() { os_event_create(&evt); }
  ~ReserveOneThreadpoolWorkerJob() { os_event_destroy(&evt); }
  void doJob() override
  {
    threadpool::wake_up_one(); // Wake up Jolt's sim job
    TIME_PROFILE_WAIT_DEV("tp_worker_reserve");
    os_event_wait(&evt, OS_WAIT_INFINITE);
  }
};
static InitOnDemand<ReserveOneThreadpoolWorkerJob, false> reserve_one_tp_worker_job;
void free_reserved_tp_worker() { os_event_set(&reserve_one_tp_worker_job->evt); }
#else
void free_reserved_tp_worker() {}
#endif

ECS_TAG(gameClient)
static void start_async_phys_sim_es(const ParallelUpdateFrameDelayed &evt)
{
#if _TARGET_C1 | _TARGET_XBOXONE
  // Reserve one threadpool worker until the end of PUFD in order to make sure that it has core to run on
  // (otherwise it might be preempted-out by other TP workers that do Jolt simulation)
  G_ASSERT(threadpool::get_current_worker_id() >= 0 && threadpool::get_num_workers() > 2);
  threadpool::add(reserve_one_tp_worker_job.demandInit());
  constexpr bool wake_tp = false; // Will be woken up by reserve_one_tp_worker_job
#else
  constexpr bool wake_tp = true;
#endif
  dacoll::phys_world_start_sim(evt.dt, wake_tp);
  dacoll::phys_world_set_invalid_fetch_sim_res_thread(/*cur thread*/ 0); // it's invalid to call fetchSimRes until end of this event
}

static inline void net_phys_update_es(const ecs::UpdateStageInfoAct &) { PhysUpdateCtx::ctx.update(); }

template <typename Callable>
static void get_phys_ecs_query(ecs::EntityId eid, Callable c);

BasePhysActor *get_phys_actor(ecs::EntityId eid) { return ECS_GET_OR(eid, base_net_phys_ptr, (BasePhysActorPtr) nullptr); }

static inline void player_controls_sequence_es_event_handler(const ecs::EventNetMessage &netMessage, game::Player &player)
{
  const net::IMessage *msg = netMessage.get<0>().get();
  if (auto *controlsSeq = msg->cast<ControlsSeq>())
    dedicated::statsd_report_ctrl_ploss(player.getControlsPlossCalc(), player.getLastReportedCtrlSeq(),
      uint16_t{controlsSeq->get<0>()});
}

ECS_ON_EVENT(on_appear)
void pair_collision_init_grid_holders_es_event_handler(
  const ecs::Event &, GridHolders &pair_collision__gridHolders, const ecs::StringList &pair_collision__gridNames)
{
  pair_collision__gridHolders.reserve(pair_collision__gridNames.size());
  for (const ecs::string &gridName : pair_collision__gridNames)
  {
    const GridHolder *gridHolder = find_grid_holder(ECS_HASH_SLOW(gridName.c_str()));
    G_ASSERT(eastl::find(pair_collision__gridHolders.begin(), pair_collision__gridHolders.end(), gridHolder) ==
             pair_collision__gridHolders.end());
    pair_collision__gridHolders.push_back(gridHolder);
  }
}

template <typename Callable>
static void collision_obj_eid_ecs_query(ecs::EntityId eid, Callable c);

void query_pair_collision_data(ecs::EntityId eid, PairCollisionData &data)
{
  collision_obj_eid_ecs_query(eid,
    [&](const ecs::string *pair_collision__tag, const ecs::Tag *human, const ecs::Tag *airplane, const ecs::Tag *phys__kinematicBody,
      const ecs::Tag *phys__hasCustomMoveLogic, const ecs::Tag *humanAdditionalCollisionChecks, const ecs::Tag *nphysPairCollision,
      bool havePairCollision = true, float phys__maxMassRatioForPushOnCollision = -1.f,
      int net_phys__collisionMaterialId = (int)PHYSMAT_INVALID, int net_phys__ignoreMaterialId = (int)PHYSMAT_INVALID) {
      data.havePairCollision = havePairCollision;
      data.pairCollisionTag = pair_collision__tag;
      data.isHuman = human != nullptr;
      data.isAirplane = airplane != nullptr;
      data.isKinematicBody = phys__kinematicBody != nullptr;
      data.hasCustomMoveLogic = phys__hasCustomMoveLogic != nullptr;
      data.humanAdditionalCollisionChecks = (humanAdditionalCollisionChecks != nullptr);
      data.nphysPairCollision = nphysPairCollision != nullptr;
      data.maxMassRatioForPushOnCollision = phys__maxMassRatioForPushOnCollision;
      data.collisionMaterialId = net_phys__collisionMaterialId;
      data.ignoreMaterialId = net_phys__ignoreMaterialId;
    });
}

ECS_NO_ORDER
static inline void deny_collision_by_ignore_list_es(QueryPhysActorsNotCollidable &evt, const ecs::EidList &collidable_ignore_list)
{
  if (evt.shouldCollide)
    if (eastl::find(collidable_ignore_list.begin(), collidable_ignore_list.end(), evt.otherEid) != collidable_ignore_list.end())
    {
      evt.shouldCollide = false;
      return;
    }
}

ECS_NO_ORDER
static inline void deny_collision_for_disabled_paircoll_es(QueryPhysActorsNotCollidable &evt, bool havePairCollision)
{
  if (!havePairCollision)
    evt.shouldCollide = false;
}
