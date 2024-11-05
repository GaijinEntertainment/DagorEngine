// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include "navMeshPhysProxy.h"
#include <phys/dag_physics.h>
#include <gamePhys/collision/collisionLib.h>
#include "game/gameEvents.h"

template <typename Callable>
static bool navphys_collision_data_ecs_query(ecs::EntityId, Callable);

void *NavMeshPhysProxy::vtblPtr = nullptr;

// Just one. This means that NavMeshPhysProxy to NavMeshPhysProxy collision isn't going to work as both will use the same object.
static CollisionObject nvmphys_capsule_coll;

NavMeshPhysProxy::NavMeshPhysProxy(ecs::EntityId eid, float dt)
{
  if (!vtblPtr)
    vtblPtr = ((void **)this)[0];
  timeStep = dt;
  pseudoVelDelta = ZERO<Point3>();

  if (!nvmphys_capsule_coll.isValid())
    nvmphys_capsule_coll = dacoll::add_dynamic_capsule_collision(TMatrix::IDENT, /*radius*/ 1.f, /*heigh*/ 1.f, /*user_ptr*/ nullptr,
      /*add_to_world*/ false);

  if (navphys_collision_data_ecs_query(eid,
        [this](const TMatrix &transform, const float nphys_coll__capsuleRadius, const float nphys_coll__capsuleHeight,
          const Point3 navmesh_phys__currentWalkVelocity, const float nphys__mass) {
          loc.fromTM(transform);
          mass = nphys__mass;
          velocity = navmesh_phys__currentWalkVelocity;
          radius = nphys_coll__capsuleRadius;
          height = nphys_coll__capsuleHeight;
        }))
  {
    return;
  }
  // fallback code path
  mass = 0.f;
  velocity.zero();
  radius = 0.f;
  height = 0.f;
}

float NavMeshPhysProxy::getMass() const { return mass; };

float NavMeshPhysProxy::getTimeStep() const { return timeStep; }

uint64_t NavMeshPhysProxy::getActiveCollisionObjectsBitMask() const
{
  return ~0ull; // all
};

dag::ConstSpan<CollisionObject> NavMeshPhysProxy::getCollisionObjects() const
{
  TMatrix tm;
  loc.toTM(tm);
  dacoll::set_collision_object_tm(nvmphys_capsule_coll, tm);
  dacoll::set_vert_capsule_shape_size(nvmphys_capsule_coll, radius, height);
  return dag::ConstSpan<CollisionObject>(&nvmphys_capsule_coll, 1);
};

TMatrix NavMeshPhysProxy::getCollisionObjectsMatrix() const
{
  TMatrix tm;
  dacoll::get_coll_obj_tm(nvmphys_capsule_coll, tm);
  return tm;
}

DPoint3 NavMeshPhysProxy::getCurrentStateVelocity() const { return DPoint3(velocity); }

Point3 NavMeshPhysProxy::getCenterOfMass() const { return Point3(0, height / 2, 0); };

DPoint3 NavMeshPhysProxy::calcInertia() const { return DPoint3(radius, height, radius) * mass; };

void NavMeshPhysProxy::applyVelOmegaDelta(const DPoint3 &add_vel, const DPoint3 &) { velocity += add_vel; }

void NavMeshPhysProxy::applyPseudoVelOmegaDelta(const DPoint3 &add_pos, const DPoint3 &)
{
  // Redistribute y component to x and z
  float len = add_pos.length();
  Point3 horPart = add_pos;
  horPart.y = 0;
  float horLen = horPart.length();
  float scale = safediv(len, horLen);
  horPart *= scale;

  pseudoVelDelta += horPart;
}

void clear_navmeshproxy_collision_es_event_handler(const EventOnGameShutdown &) { del_it(nvmphys_capsule_coll.body); }