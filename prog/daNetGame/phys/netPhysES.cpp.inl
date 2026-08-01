// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "netPhys.h"
#include "net/dedicated.h"
#include "net/net.h" // is_true_net_server
#include <daECS/net/netbase.h>
#include "phys/physUtils.h"
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <ecs/game/generic/grid.h>
#include <daECS/net/message.h>
#include <daECS/net/recipientFilters.h>
#include <daECS/core/coreEvents.h>
#include <osApiWrappers/dag_events.h>
#include <util/dag_threadPool.h>
#include "phys/physEvents.h"
#include "game/player.h"
#include <gamePhys/phys/walker/humanPhys.h>
#include <ecs/phys/collRes.h>
#include <gamePhys/collision/collisionLib.h>
#include <gamePhys/collision/collisionResponse.h>
#include <gamePhys/collision/contactData.h>
#include <gamePhys/phys/physToSolverBody.h>
#include <gamePhys/phys/rendinstDestr.h>
#include "game/gameEvents.h"
#include "navMeshPhysProxy.h"
#include <memory/dag_framemem.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_mathUtils.h>
#include <ecs/phys/physBody.h>
#include <phys/dag_physics.h>
#include <statsd/statsd.h>
#include <daECS/delayedAct/actInThread.h>

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
static void get_current_local_delay_ecs_query(ecs::EntityManager &manager, Callable c);

float get_timespeed_accumulated_delay_sec()
{
  float delay = 0.f;
  get_current_local_delay_ecs_query(*g_entity_mgr,
    [&delay](float time_speed__accumulatedDelay) { delay = time_speed__accumulatedDelay; });
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

float phys_get_interp_delay_sec(PhysTickRateType tr_type, float time_step)
{
  return PhysUpdateCtx::ctx.getInterpDelay(tr_type, time_step);
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
  const char *getJobName(bool &) const override { return "ReserveOneThreadpoolWorkerJob"; }
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
static void get_phys_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable c);

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
    if (!gridHolder)
    {
      continue;
    }
    G_ASSERT(eastl::find(pair_collision__gridHolders.begin(), pair_collision__gridHolders.end(), gridHolder) ==
             pair_collision__gridHolders.end());
    pair_collision__gridHolders.push_back(gridHolder);
  }
}

template <typename Callable>
static bool collision_obj_eid_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable c);

void query_pair_collision_data(ecs::EntityId eid, PairCollisionData &data)
{
  // everything should be optional
  G_VERIFY(collision_obj_eid_ecs_query(*g_entity_mgr, eid,
    [&](const ecs::string *pair_collision__tag, const ecs::Tag *human, const ecs::Tag *airplane, const ecs::Tag *phys__kinematicBody,
      const ecs::Tag *phys__hasCustomMoveLogic, const ecs::Tag *humanAdditionalCollisionChecks, const ecs::Tag *nphysPairCollision,
      const CollisionResource *collres, const ecs::Tag *phys__inverseOmega, PhysBody *phys_body, bool havePairCollision = true,
      float phys__maxMassRatioForPushOnCollision = -1.f, bool pair_collision__ignoreWorldContacts = false,
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
      data.ignoreWorldContacts = pair_collision__ignoreWorldContacts;
      data.collres = collres;
      data.inverseOmega = phys__inverseOmega != nullptr;
      data.staticPhysBody = phys_body;
    }));
}

void move_collision_objects(IPhysBase &phys, const TMatrix &tm)
{
  for (const auto &co : phys.getCollisionObjects())
    if (dacoll::is_obj_active(co))
      dacoll::set_collision_object_tm(co, tm);
}

void net_phys_collision_es_impl(const UpdatePhysEvent &info,
  BasePhysActor &actor,
  const CollisionResource &collres,
  const GridHolders &grids_to_search,
  const ecs::HashedConstString &tag_to_check)
{
  if (!(actor.getRole() & (IPhysActor::URF_AUTHORITY | IPhysActor::URF_LOCAL_CONTROL)))
    return;

  IPhysBase &phys = actor.getPhys();
  dag::ConstSpan<CollisionObject> curColl = phys.getCollisionObjects();
  uint64_t curCollActive = phys.getActiveCollisionObjectsBitMask();
  const TMatrix &curPhysTm = phys.getCollisionObjectsMatrix();
  const TraceMeshFaces *curTraceCache = phys.getTraceHandle();
  bool worldContactsProcessed = false;
  PairCollisionData curBody;
  query_pair_collision_data(actor.eid, curBody);
  bool pairCollisionIgnoreWorldContacts = curBody.ignoreWorldContacts;
  const bool curIsAsleep = actor.isAsleep() && !curBody.hasCustomMoveLogic;
  mat44f vCurPhysTm;
  v_mat44_make_from_43cu_unsafe(vCurPhysTm, curPhysTm.array);
  vec4f vBounding = collres.getBoundingSphereXYZR();
  v_bsph_init(vBounding, vCurPhysTm, vBounding, v_splat_w(vBounding));
  BSphere3 bounding;
  v_stu_bsphere3(bounding, vBounding);
  // World-contact queries are centered at the collision objects poses (~ the phys pivot), so they
  // must cover the reach around the pivot: pivot-to-bounding-center distance plus the radius
  const float curWorldContactsRad = v_extract_x(v_add_x(v_distance_xyz_x(vBounding, vCurPhysTm.col3), v_splat_w(vBounding)));
  alignas(NavMeshPhysProxy) char nmeshProxyStorage[sizeof(NavMeshPhysProxy)];

  for_each_entity_in_grids(grids_to_search, bounding, GridEntCheck::BOUNDING, [&](ecs::EntityId eid, vec4f wbsph) {
    if (eid == actor.eid)
      return;
    if (!g_entity_mgr->has(eid, tag_to_check)) // they don't have pair collision
      return;

    BasePhysActor *testActor = get_phys_actor(eid);
    if (curIsAsleep && testActor && testActor->isAsleep() && !g_entity_mgr->has(eid, ECS_HASH("phys__hasCustomMoveLogic")))
      return;

    PairCollisionData testBody;
    query_pair_collision_data(eid, testBody);

    // don't check same collision pair twice
    if (ecs::entity_id_t(actor.eid) > ecs::entity_id_t(eid) && testBody.havePairCollision && testActor &&
        testActor->getRole() & (IPhysActor::URF_AUTHORITY | IPhysActor::URF_LOCAL_CONTROL))
    {
      if (testBody.pairCollisionTag && g_entity_mgr->has(actor.eid, ECS_HASH_SLOW(testBody.pairCollisionTag->c_str())))
        return;
    }

    // A test body is either dynamic (a phys actor or a navmesh proxy, via testPhys) or an immovable
    // static phys_body prop with no phys actor (e.g. an animchar scene object, via testBody.staticPhysBody).
    IPhysBase *testPhys = nullptr;
    if (testActor)
      testPhys = &testActor->getPhys();
    else if (testBody.nphysPairCollision)
      testPhys = new (&nmeshProxyStorage, _NEW_INPLACE) NavMeshPhysProxy(eid, info.dt);
    else if (!testBody.staticPhysBody)
      return; // not collidable: no phys actor, navmesh proxy or static phys body
    else if (curIsAsleep)
      return; // a static phys body can not wake us up

    QueryPhysActorsNotCollidable qcoll(actor.eid);
    g_entity_mgr->sendEventImmediate(eid, qcoll);
    if (!qcoll.shouldCollide)
      return;
    qcoll.otherEid = eid;
    g_entity_mgr->sendEventImmediate(actor.eid, qcoll);
    if (!qcoll.shouldCollide)
      return;

    CollisionObject staticCollObj(testBody.staticPhysBody, nullptr);
    dag::ConstSpan<CollisionObject> testColl = testPhys ? testPhys->getCollisionObjects() : make_span_const(&staticCollObj, 1);
    uint64_t testCollActive = testPhys ? testPhys->getActiveCollisionObjectsBitMask() : ~0ull;
    TMatrix testTm, staticBodyItm;
    if (testPhys)
      testTm = testPhys->getCollisionObjectsMatrix();
    else
    {
      testBody.staticPhysBody->getTm(testTm);
      mat44f vtm, vitm;
      v_mat44_make_from_43cu_unsafe(vtm, testTm.array);
      v_mat44_orthonormal_inverse43(vitm, vtm);
      v_mat_43cu_from_mat44(staticBodyItm.array, vitm);
    }
    Tab<gamephys::CollisionContactData> contacts(framemem_ptr());
    if (!dacoll::test_pair_collision(curColl, curCollActive, testColl, testCollActive, contacts,
          dacoll::TestPairFlags::Default & ~dacoll::TestPairFlags::CheckInWorld))
      return;

    int contactsNum = contacts.size();

    if (!worldContactsProcessed && !pairCollisionIgnoreWorldContacts)
    {
      dacoll::test_collision_world(curColl, curWorldContactsRad, contacts, curTraceCache);
      worldContactsProcessed = true;
    }
    const int contactsNum1 = contacts.size() - contactsNum;

    if (!pairCollisionIgnoreWorldContacts)
    {
      float worldContactsRad = v_extract_w(wbsph); // grid bounding radius fallback when no collres
      if (testBody.collres)
      {
        vec4f wbs = v_ldu_bsphere3(testTm, testBody.collres->getBoundingSphereS());
        vec3f tmPos = v_ldu_p3(&testTm.getcol(3).x);
        worldContactsRad = v_extract_x(v_distance_xyz_x(wbs, tmPos)) + v_extract_w(wbs);
      }
      dacoll::test_collision_world(testColl, worldContactsRad, contacts, testPhys ? testPhys->getTraceHandle() : nullptr);
    }
    const int contactsNum2 = contacts.size() - contactsNum1 - contactsNum;

    daphys::SolverBodyInfo curBodyInfo = gamephys::phys_to_solver_body(&phys);
    daphys::SolverBodyInfo testBodyInfo = testPhys ? gamephys::phys_to_solver_body(testPhys)
                                                   : daphys::SolverBodyInfo(testTm, staticBodyItm, ZERO<DPoint3>(), ZERO<DPoint3>(),
                                                       ZERO<DPoint3>(), ZERO<DPoint3>(), /*inv_mass*/ 0.0);
    if (curBody.inverseOmega)
      curBodyInfo.omega = -curBodyInfo.omega;
    if (testBody.inverseOmega)
      testBodyInfo.omega = -testBodyInfo.omega;

    if (testPhys && !curBody.isKinematicBody && !testBody.isKinematicBody)
    {
      const float maxMassRatio = max(curBody.maxMassRatioForPushOnCollision, testBody.maxMassRatioForPushOnCollision);
      if (maxMassRatio > 0.f)
      {
        // If one of the bodies is way heavier than the other don't push it
        // The code fixes cases like a human pushes a tank
        const float massRatio = safediv(phys.getMass(), testPhys->getMass());
        if (massRatio >= maxMassRatio)
        {
          curBodyInfo.invMass = 0.f;
          curBodyInfo.invMoi.zero();
        }
        if (safeinv(massRatio) >= maxMassRatio)
        {
          testBodyInfo.invMass = 0.f;
          testBodyInfo.invMoi.zero();
        }
      }
    }

    if (curBody.isKinematicBody)
    {
      curBodyInfo.invMass = 0.f;
      curBodyInfo.invMoi.zero();
    }

    if (testBody.isKinematicBody)
    {
      testBodyInfo.invMass = 0.f;
      testBodyInfo.invMoi.zero();
    }

    // Onesided collision ignoring:
    // Bot cannot affect players but playres can affect bots
    // So, this code should skip contacts with bots when it's called for player
    // but solve contacts when it's called for bot
    QueryPhysActorsOneSideCollidable curBodyOneSide(eid);
    QueryPhysActorsOneSideCollidable testBodyOneSide(actor.eid);
    g_entity_mgr->sendEventImmediate(eid, testBodyOneSide);
    g_entity_mgr->sendEventImmediate(actor.eid, curBodyOneSide);

    if (testBodyOneSide.oneSideCollidable)
    {
      testBodyInfo.invMass = 0.f;
      testBodyInfo.invMoi.zero();
    }
    if (curBodyOneSide.oneSideCollidable)
    {
      curBodyInfo.invMass = 0.f;
      curBodyInfo.invMoi.zero();
    }

    if (curBody.isAirplane)
    {
      curBodyInfo.commonImpulseLimit = DummyCustomPhys::aircraftCollisionImpulseSpeedMax * phys.getMass();
      move_collision_objects(phys, curPhysTm);
    }
    if (testPhys && testBody.isAirplane)
    {
      testBodyInfo.commonImpulseLimit = DummyCustomPhys::aircraftCollisionImpulseSpeedMax * testPhys->getMass();
      move_collision_objects(*testPhys, testTm);
    }

    carray<double, BasePhysActor::MAX_COLLISION_SUBOBJECTS> impulseLimits1;
    eastl::fill(impulseLimits1.data(), impulseLimits1.data() + impulseLimits1.size(), VERY_BIG_NUMBER);
    curBodyInfo.impulseLimits.set(make_span(impulseLimits1));
    carray<double, BasePhysActor::MAX_COLLISION_SUBOBJECTS> impulseLimits2;
    eastl::fill(impulseLimits2.data(), impulseLimits2.data() + impulseLimits2.size(), VERY_BIG_NUMBER);
    testBodyInfo.impulseLimits.set(make_span(impulseLimits2));

    // Contacts are laid out as [pair contacts | cur body's world contacts | test body's world
    // contacts]; contacts_to_solver_data indexes each span from 0, so re-base contactIndex to the
    // position of that span within the full contacts array.
    Tab<gamephys::SeqImpulseInfo> collisions(framemem_ptr());
    collisions.reserve(contactsNum * 3 + contactsNum1 + contactsNum2);
    auto appendCollisions = [&](const daphys::SolverBodyInfo *lhs, const daphys::SolverBodyInfo *rhs, int ofs, int cnt) {
      int prevSize = collisions.size();
      contacts_to_solver_data(lhs, rhs, make_span(contacts).subspan(ofs, cnt), collisions);
      for (int i = prevSize; i < collisions.size(); ++i)
        collisions[i].contactIndex += ofs;
    };
    appendCollisions(&curBodyInfo, &testBodyInfo, 0, contactsNum);
    appendCollisions(&curBodyInfo, nullptr, contactsNum, contactsNum1);
    appendCollisions(nullptr, &testBodyInfo, contactsNum + contactsNum1, contactsNum2);

    phys.prepareCollisions(curBodyInfo, testBodyInfo, true, 0.7f, make_span(contacts), make_span(collisions));
    if (testPhys)
      testPhys->prepareCollisions(curBodyInfo, testBodyInfo, false, 0.7f, make_span(contacts), make_span(collisions));

    Point3 lastCollisionPos = curPhysTm.getcol(3);
    float maxImpulse = 0.f;
    if (daphys::resolve_pair_velocity(curBodyInfo, testBodyInfo, make_span(collisions)))
    {
      phys.wakeUp();
      phys.rescheduleAuthorityApprovedSend();
      if (testPhys)
      {
        testPhys->wakeUp();
        testPhys->rescheduleAuthorityApprovedSend();
      }

      for (const gamephys::SeqImpulseInfo &collision : collisions)
      {
        if (collision.appliedImpulse <= maxImpulse)
          continue;
        maxImpulse = collision.appliedImpulse;
        lastCollisionPos = Point3::xyz(collision.pos);
      }

      Point3 bodyTestVel = Point3::xyz(testBodyInfo.vel);
      Point3 bodyCurVel = Point3::xyz(curBodyInfo.vel);
      g_entity_mgr->sendEvent(actor.eid,
        EventOnCollision(Point3::xyz(curBodyInfo.addVel), bodyCurVel, lastCollisionPos, eid, bodyTestVel, info.dt, -1.f));
      g_entity_mgr->sendEvent(eid,
        EventOnCollision(Point3::xyz(testBodyInfo.addVel), bodyTestVel, lastCollisionPos, actor.eid, bodyCurVel, info.dt, -1.f));
    }

    if (curBody.isHuman || testBody.isHuman)
    {
      // In case of human collision checks we must skip entities that have humanAdditionalCollisionChecks
      // Human already done all collision checks inside his physics
      if (curBody.humanAdditionalCollisionChecks || testBody.humanAdditionalCollisionChecks)
        return;
    }

    if (curBody.inverseOmega)
      curBodyInfo.addOmega = -curBodyInfo.addOmega;
    if (testBody.inverseOmega)
      testBodyInfo.addOmega = -testBodyInfo.addOmega;
    phys.applyVelOmegaDelta(curBodyInfo.addVel, curBodyInfo.addOmega);
    if (testPhys)
      testPhys->applyVelOmegaDelta(testBodyInfo.addVel, testBodyInfo.addOmega);

    daphys::resolve_pair_penetration(curBodyInfo, testBodyInfo, collisions, 2.0, 5);
    phys.applyPseudoVelOmegaDelta(curBodyInfo.pseudoVel * phys.getTimeStep(), curBodyInfo.pseudoOmega * phys.getTimeStep());
    if (testPhys)
      testPhys->applyPseudoVelOmegaDelta(testBodyInfo.pseudoVel * testPhys->getTimeStep(),
        testBodyInfo.pseudoOmega * testPhys->getTimeStep());

    phys.wakeUp();
    if (testPhys)
    {
      testPhys->wakeUp();
      if (!testActor) // testPhys can only be a NavMeshPhysProxy when there is no phys actor for eid
        static_cast<const NavMeshPhysProxy *>(testPhys)->applyChangesTo(eid);
    }

    if (is_server())
    {
      actor.onCollisionDamage(make_span(collisions), make_span(contacts), eid.index(), 1.0, info.curTime);
      if (testActor != nullptr)
        testActor->onCollisionDamage(make_span(collisions), make_span(contacts), actor.eid.index(), 1.0, info.curTime);
    }
  });

  if (!curIsAsleep)
    for (const CollisionObject &collObj : curColl)
      if (collObj.isValid())
        rendinstdestr::test_dynobj_to_ri_phys_collision(collObj, curWorldContactsRad);
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
