//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_globDef.h>
#include <phys/dag_physDecl.h>
#include <phys/dag_physUserData.h>

struct CollisionObjectUserData : public PhysObjectUserData
{
  int matId = -1;

  MAKE_PHYS_OBJ_TYPE(CollisionObjectUserData, _MAKE4C('CLOB'));
};

namespace physx
{
class PxRigidActor;
}
struct CollisionObject;

struct CollisionObjectBullet
{
  PhysBody *body = nullptr;
  CollisionObjectBullet() = default;
  CollisionObjectBullet(PhysBody *b, physx::PxRigidActor *physx_obj) : body(b)
  {
    G_ASSERT(!physx_obj);
    G_UNUSED(physx_obj);
  }
  CollisionObjectBullet(CollisionObject);
  explicit operator bool() const { return body != nullptr; }
  bool isValid() const { return body; }
  void clear_ptrs() { body = nullptr; }
};

struct CollisionObject
{
  PhysBody *body = nullptr;
  physx::PxRigidActor *physxObject = nullptr;

  CollisionObject() = default;
  CollisionObject(PhysBody *b, physx::PxRigidActor *p) : body(b), physxObject(p) {}
  /*imlicit*/ CollisionObject(CollisionObjectBullet cob) : body(cob.body) {}

  explicit operator bool() const { return body != nullptr; }
  bool isValid() const { return body; }
  void clear_ptrs()
  {
    body = nullptr;
    physxObject = nullptr;
  }
};

inline CollisionObjectBullet::CollisionObjectBullet(CollisionObject co) : body(co.body) { G_ASSERT(!co.physxObject); }
