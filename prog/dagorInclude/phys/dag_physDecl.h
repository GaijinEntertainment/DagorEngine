//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_compilerDefs.h>

//
// Check for proper defines for physics engine selection
//
#if defined(USE_BULLET_PHYSICS) + defined(USE_JOLT_PHYSICS) == 0
#error No physics engine specified. Define USE_BULLET_PHYSICS, USE_JOLT_PHYSICS
#elif defined(USE_BULLET_PHYSICS) + defined(USE_JOLT_PHYSICS) != 1
#error Define only one USE_#_PHYSICS directive
#endif


//
// Define alias for classes of each physics engine
//
#if defined(USE_BULLET_PHYSICS)

#define PHYSICS_ENGINE_NAME  "Bullet"
#define init_physics_engine  init_bullet_physics_engine
#define close_physics_engine close_bullet_physics_engine

#define PhysWorld   BulletPhysWorld
#define PhysBody    BulletPhysBody
#define PhysRagdoll BulletPhysRagdoll
#define PhysRayCast BulletPhysRayCast

#define IPhysBodyCollisionCallback IBulletPhysBodyCollisionCallback
#define PhysBodyPreSolveCallback   BulletPhysBodyPreSolveCallback
#define IPhysContactData           IBulletPhysContactData

#define PhysCollision            BulletPhysCollision
#define PhysBoxCollision         BulletPhysBoxCollision
#define PhysSphereCollision      BulletPhysSphereCollision
#define PhysCapsuleCollision     BulletPhysCapsuleCollision
#define PhysCylinderCollision    BulletPhysCylinderCollision
#define PhysPlaneCollision       BulletPhysPlaneCollision
#define PhysHeightfieldCollision BulletPhysHeightfieldCollision
#define PhysTriMeshCollision     BulletPhysTriMeshCollision
#define PhysConvexHullCollision  BulletPhysConvexHullCollision
#define PhysCompoundCollision    BulletPhysCompoundCollision

#define PhysJoint             BulletPhysJoint
#define PhysRagdollBallJoint  BulletPhysRagdollBallJoint
#define PhysRagdollHingeJoint BulletPhysRagdollHingeJoint
#define Phys6DofJoint         BulletPhys6DofJoint
#define Phys6DofSpringJoint   BulletPhys6DofSpringJoint
#define PhysFixedJoint        BulletPhysFixedJoint

#define PhysSystemInstance BulletPhysSystemInstance

class btRigidBody;

#elif defined(USE_JOLT_PHYSICS)

#define PHYSICS_ENGINE_NAME  "Jolt"
#define init_physics_engine  init_jolt_physics_engine
#define close_physics_engine close_jolt_physics_engine

#define PhysWorld   JoltPhysWorld
#define PhysBody    JoltPhysBody
#define PhysRagdoll JoltPhysRagdoll
#define PhysRayCast JoltPhysRayCast

#define IPhysBodyCollisionCallback IJoltPhysBodyCollisionCallback
#define PhysBodyPreSolveCallback   JoltPhysBodyPreSolveCallback
#define IPhysContactData           IJoltPhysContactData

#define PhysCollision            JoltPhysCollision
#define PhysBoxCollision         JoltPhysBoxCollision
#define PhysSphereCollision      JoltPhysSphereCollision
#define PhysCapsuleCollision     JoltPhysCapsuleCollision
#define PhysCylinderCollision    JoltPhysCylinderCollision
#define PhysPlaneCollision       JoltPhysPlaneCollision
#define PhysHeightfieldCollision JoltPhysHeightfieldCollision
#define PhysTriMeshCollision     JoltPhysTriMeshCollision
#define PhysConvexHullCollision  JoltPhysConvexHullCollision
#define PhysCompoundCollision    JoltPhysCompoundCollision

#define PhysJoint             JoltPhysJoint
#define PhysRagdollBallJoint  JoltPhysRagdollBallJoint
#define PhysRagdollHingeJoint JoltPhysRagdollHingeJoint
#define Phys6DofJoint         JoltPhys6DofJoint
#define Phys6DofSpringJoint   JoltPhys6DofSpringJoint
#define PhysFixedJoint        JoltPhysFixedJoint

#define PhysSystemInstance JoltPhysSystemInstance


#endif

// external forwards
class Point3;

//
// Forward declarations for all physics classes
//
class PhysWorld;
class PhysBody;
class PhysRayCast;

class IPhysBodyCollisionCallback;
class IPhysContactData;

class PhysCollision;
class PhysBoxCollision;
class PhysSphereCollision;
class PhysCapsuleCollision;
class PhysCylinderCollision;
class PhysPlaneCollision;
class PhysHeightfieldCollision;
class PhysTriMeshCollision;
class PhysConvexHullCollision;
class PhysCompoundCollision;

class PhysJoint;
class PhysRagdollBallJoint;
class PhysRagdollHingeJoint;

class PhysSystemInstance;
struct PhysBodyCreationData;

// PreSolveCallback runs just after collision was detected and before physics solver is invoked, there's a
// chance to modify contact data, right now one can only modify normal, but it can be extended in
// the future.
// 'other' represents other object we're colliding with (btCollisionObject* in this case), there's a reason why
// it can't be 'PhysBody*', that's because other body might not be allocated through this system, it can come from
// static level geometry for example, but we still want to track collisions with it, so we just pass native body type
// here.
// 'contact_data' represents custom user data associated with contact and is initially NULL, caller can
// dervive from IBulletPhysContactData and allocate it via 'new', physics engine will call 'delete' on it
// once the contact is removed.
typedef void (*PhysBodyPreSolveCallback)(PhysBody *body, void *other, const Point3 &pos, Point3 &norm,
  IPhysContactData *&contact_data);

// action with execution delayed to end of update (after PhysWorld::startSim() finishes)
struct AfterPhysUpdateAction
{
#if USE_BULLET_PHYSICS
  btRigidBody *body;
  AfterPhysUpdateAction(btRigidBody *rb = nullptr) : body(rb) {}
#endif
  virtual ~AfterPhysUpdateAction() {}
  virtual void doAction(PhysWorld &phys_world, bool immediate) = 0;
};

template <typename T, typename P, class... Args>
inline void exec_or_add_after_phys_action(P &physWorld, Args &&...args)
{
  if (DAGOR_LIKELY(physWorld.canExecActionNow()))
    T(static_cast<Args &&>(args)...).doAction(physWorld, /*immediate*/ true);
  else // sim in progress -> add DA
    physWorld.addAfterPhysAction(new T(static_cast<Args &&>(args)...));
}
