// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <ecs/anim/anim.h>
#include <ecs/phys/collRes.h>
#include <ecs/phys/ragdoll.h>
#include <ecs/phys/physEvents.h>
#include <gamePhys/collision/collisionLib.h>
#include <ecs/delayedAct/actInThread.h>
#include <phys/dag_physDecl.h>
#include <phys/dag_physics.h>
#include <daECS/net/time.h>

#define PHYS_ECS_EVENT ECS_REGISTER_EVENT
PHYS_ECS_EVENTS
#undef PHYS_ECS_EVENT
typedef PhysRagdoll ragdoll_t;

class RagdollCTM final : public ecs::ComponentTypeManager
{
public:
  typedef PhysRagdoll component_type;

  void create(void *d, ecs::EntityManager &mgr, ecs::EntityId eid, const ecs::ComponentsMap &, ecs::component_index_t) override
  {
    *(ecs::PtrComponentType<PhysRagdoll>::ptr_type(d)) = nullptr;
    AnimV20::AnimcharBaseComponent &animchar = g_entity_mgr->getRW<AnimV20::AnimcharBaseComponent>(eid, ECS_HASH("animchar"));

    PhysRagdoll *ragdoll = PhysRagdoll::create(animchar.getPhysicsResource(), dacoll::get_phys_world());
    G_ASSERT_RETURN(ragdoll, );
    *(ecs::PtrComponentType<PhysRagdoll>::ptr_type(d)) = ragdoll;

    ragdoll->setRecalcWtms(true);
    ragdoll->setContinuousCollisionMode(mgr.getOr(eid, ECS_HASH("ragdoll__use_ccd"), false));

    if (mgr.getOr(eid, ECS_HASH("ragdoll__active"), false))
    {
      ragdoll->setStartAddLinVel(mgr.getOr(eid, ECS_HASH("ragdoll__start_vel"), ZERO<Point3>()));
      const int interactLayer = mgr.getOr(eid, ECS_HASH("ragdoll__interactLayer"), 1);
      const int interactMask = mgr.getOr(eid, ECS_HASH("ragdoll__interactMask"), (int)0xffffffff);
      ragdoll->startRagdoll(interactLayer, interactMask, &animchar.getNodeTree());
      animchar.setPostController(ragdoll);
    }
  }

  void destroy(void *d) override
  {
    PhysRagdoll *__restrict *__restrict a = (ecs::PtrComponentType<PhysRagdoll>::ptr_type(d));
    G_ASSERT_RETURN(a, );
    auto &cres = *a;
    if (cres) // can be null if onLoaded wasn't called (entity destroyed during loading)
      del_it(cres);
  }
};
G_STATIC_ASSERT(!ecs::ComponentTypeInfo<PhysRagdoll>::can_be_tracked);

// todo:replce to InplaceCreator (same as Animchar)
ECS_REGISTER_MANAGED_TYPE(PhysRagdoll, nullptr, RagdollCTM);
ECS_AUTO_REGISTER_COMPONENT_DEPS(PhysRagdoll, "ragdoll", nullptr, 0, "animchar");

ECS_REGISTER_RELOCATABLE_TYPE(ProjectileImpulse, nullptr);
ECS_AUTO_REGISTER_COMPONENT(ProjectileImpulse, "projectile_impulse", nullptr, 0);

static inline int get_body_index_by_name(dag::ConstSpan<PhysicsResource::Body> bodies, const char *name)
{
  for (int i = 0; i < bodies.size(); i++)
    if (strcmp(name, bodies[i].name) == 0)
      return i;
  return -1;
}

static void add_collision_nodes(const CollisionResource &collres, PhysRagdoll &ragdoll, bool ragdoll__isSingleBody)
{
  PhysicsResource *phys = ragdoll.getPhysRes();
  if (!phys)
    return;
  dag::ConstSpan<CollisionNode> allNodes = collres.getAllNodes();
  dag::ConstSpan<PhysicsResource::Body> bodies = phys->getBodies();
  ragdoll.resizeCollMap(allNodes.size());
  if (ragdoll__isSingleBody)
  {
    G_ASSERT_RETURN(bodies.size() == 1, );
    for (int nodeId = 0; nodeId < allNodes.size(); ++nodeId)
      ragdoll.addCollNode(nodeId, 0);
    return;
  }
  for (int nodeId = 0; nodeId < allNodes.size(); ++nodeId)
  {
    const char *name = allNodes[nodeId].name;
    for (int i = 0; i < bodies.size(); i++)
    {
      if (!strcmp(name, bodies[i].name))
      {
        int ref = i;
        if (!bodies[i].aliasName.empty())
          ref = get_body_index_by_name(bodies, bodies[i].aliasName);
        G_ASSERTF_BREAK(ref >= 0, "Unknown ragdoll body node alias [%s]", bodies[i].aliasName);
        ragdoll.addCollNode(nodeId, ref);
        break;
      }
    }
  }
}

ECS_BEFORE(start_async_phys_sim_es)
inline void ragdoll_start_es_event_handler(const ParallelUpdateFrameDelayed &info, ProjectileImpulse &projectile_impulse,
  ragdoll_t &ragdoll, CollisionResource &collres, Point4 *ragdoll__massMatrix, bool &ragdoll__applyParams,
  bool ragdoll__isSingleBody = false, float projectile_impulse__impulseSaveDeltaTime = 0.5f,
  float projectile_impulse__cinematicArtistryMult = 0.01f, float projectile_impulse__cinematicArtistrySpeedMult = 0.1f,
  float projectile_impulse__maxSingleImpulse = 0.01f, float projectile_impulse__maxVelocity = -1.0f,
  float projectile_impulse__maxOmega = -1.0f, int projectile_impulse__maxCount = -1)
{
  PhysSystemInstance *physSys = ragdoll.getPhysSys();
  if (!ragdoll__applyParams || !physSys || !physSys->getBodyCount())
    return;

  add_collision_nodes(collres, ragdoll, ragdoll__isSingleBody);
  if (ragdoll__isSingleBody && ragdoll__massMatrix)
  {
    G_ASSERT_RETURN(physSys->getBodyCount() == 1, );
    const Point4 &mm = *ragdoll__massMatrix;
    physSys->getBody(0)->setMassMatrix(mm.x, mm.y, mm.z, mm.w);
  }
  const float curTime = info.curTime;
  if (!projectile_impulse.data.empty())
  {
    Point3 impulse(0.f, 0.f, 0.f);
    const int minIndex =
      projectile_impulse__maxCount > 0 ? max(0, (int)projectile_impulse.data.size() - projectile_impulse__maxCount) : 0;
    for (int i = projectile_impulse.data.size() - 1; i >= minIndex; i--)
    {
      const auto &t = projectile_impulse.data[i];
      if (t.first < curTime - projectile_impulse__impulseSaveDeltaTime)
        break;
      Point3 currentImpulse = t.second.impulse * projectile_impulse__cinematicArtistryMult;
      if (currentImpulse.lengthSq() > sqr(projectile_impulse__maxSingleImpulse))
        currentImpulse *= safediv(projectile_impulse__maxSingleImpulse, currentImpulse.length());
      ragdoll.applyImpulse(t.second.nodeId, t.second.pos, currentImpulse);
      impulse += t.second.impulse;
    }
    projectile_impulse.data.clear();
    impulse *= projectile_impulse__cinematicArtistrySpeedMult;
    const float velocityMagnitude = impulse.length();
    if (projectile_impulse__maxVelocity > 0.f && velocityMagnitude > projectile_impulse__maxVelocity)
      impulse *= safediv(projectile_impulse__maxVelocity, velocityMagnitude);
    for (int i = 0; i < physSys->getBodyCount(); ++i)
    {
      PhysBody *b = physSys->getBody(i);
      b->setVelocity(b->getVelocity() + impulse);
    }
  }

  if (projectile_impulse__maxOmega > 0.f)
    for (int i = 0; i < physSys->getBodyCount(); ++i)
    {
      const Point3 omega = physSys->getBody(i)->getAngularVelocity();
      const float omegaMagnitude = omega.length();
      if (omegaMagnitude > projectile_impulse__maxOmega)
        physSys->getBody(i)->setAngularVelocity(omega * safediv(projectile_impulse__maxOmega, omegaMagnitude));
    }

  ragdoll__applyParams = false;
}

ECS_REQUIRE(eastl::true_type isAlive)
inline void ragdoll_alive_es_event_handler(const EventOnPhysImpulse &evt, ProjectileImpulse &projectile_impulse,
  float projectile_impulse__impulseSaveDeltaTime = 1.f)
{
  const float curTime = evt.get<0>();
  int collNodeId = evt.get<1>();
  const Point3 pos = evt.get<2>();
  const Point3 impulse = evt.get<3>();
  if (!check_nan(pos) && !check_nan(impulse) && lengthSq(impulse) < 1e10f)
    save_projectile_impulse(curTime, projectile_impulse, collNodeId, pos, impulse, projectile_impulse__impulseSaveDeltaTime);
  else
    logerr("EventOnPhysImpulse with invalid data for alive body pos = %f %f %f impulse = %f %f %f ", pos.x, pos.y, pos.z, impulse.x,
      impulse.y, impulse.z);
}

ECS_REQUIRE(eastl::false_type isAlive)
inline void ragdoll_dead_es_event_handler(const EventOnPhysImpulse &evt, ragdoll_t &ragdoll, ProjectileImpulse &projectile_impulse,
  float projectile_impulse__impulseSaveDeltaTime = 1.f, float projectile_impulse__cinematicArtistryMultDead = 5.f)
{
  const float curTime = evt.get<0>();
  int collNodeId = evt.get<1>();
  const Point3 pos = evt.get<2>();
  const Point3 impulse = evt.get<3>();
  if (!check_nan(pos) && !check_nan(impulse) && lengthSq(impulse) < 1e10f)
  {
    if (!ragdoll.isCanApplyNodeImpulse())
      save_projectile_impulse(curTime, projectile_impulse, collNodeId, pos, impulse, projectile_impulse__impulseSaveDeltaTime);
    else if (projectile_impulse__cinematicArtistryMultDead != 0)
      ragdoll.applyImpulse(collNodeId, pos, impulse * projectile_impulse__cinematicArtistryMultDead);
  }
  else
    logerr("EventOnPhysImpulse with invalid data for dead body pos = %f %f %f impulse = %f %f %f ", pos.x, pos.y, pos.z, impulse.x,
      impulse.y, impulse.z);
}
