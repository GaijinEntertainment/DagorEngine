//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <phys/dag_physDecl.h>

#if defined(USE_BULLET_PHYSICS)
#include <phys/dag_bulletPhysics.h>
#elif defined(USE_JOLT_PHYSICS)
#include <phys/dag_joltPhysics.h>
#else
#error New or undefined physics engine!
#endif


/*

// Common physics interface:


void init_physics_engine();

void close_physics_engine();


class PhysCollision
{
public:
  ~PhysCollision();
};


class IPhysBodyCollisionCallback
{
public:
  virtual void onCollision ( PhysBody *other, const Point3 &pt, const Point3 &norm,
                             int this_matId, int other_matId,
                             const Point3 &norm_imp, const Point3 &friction_imp,
                             const Point3 &this_old_vel, const Point3 &other_old_vel ) = 0;
};

class PhysBody
{
public:
  PhysBody(PhysWorld *w, float mass, const PhysCollision *coll, const TMatrix &tm = TMatrix::IDENT,
    const PhysBodyCreationData &s = {});
  ~PhysBody();

  void setTm(const TMatrix &local_to_world);
  void getTm(TMatrix &local_to_world) const;
  void setTmInstant(const TMatrix &local_to_world, bool upd_aabb = false);
  void getTmInstant(TMatrix &local_to_world) const;

  void activateBody ( bool active = true );
  bool isActive() const;

  void setGroupAndLayerMask(unsigned int group, unsigned int mask);
  unsigned int getInteractionLayer();

  void setVelocity(const Point3 &vel);
  Point3 getVelocity();
  void setAngularVelocity(const Point3 &omega);
  Point3 getAngularVelocity();

  // Zero mass makes body static.
  void setMassMatrix(real mass, real Ixx, real Iyy, real Izz);
  real getMass() const;
  void getMassMatrix(real &mass, real &Ixx, real &Iyy, real &Izz);
  void getInvMassMatrix(real &mass, real &Ixx, real &Iyy, real &Izz);

  void setCollision(PhysCollision *collision, bool allow_fast_inaccurate_coll_tm=true);

  void setMaterialId(int id);

  void setCollisionCallback ( IPhysBodyCollisionCallback *cb );
  IPhysBodyCollisionCallback *getCollisionCallback ();
};



class PhysWorld
{
public:
  PhysWorld(real default_static_friction, real default_dynamic_friction,
    real default_restitution, real default_softness);

  ~PhysWorld();

  inline void simulate(real dt) { startSim(dt); fetchSimResults(true); }
  void startSim(real dt);
  bool fetchSimResults(bool wait);

  void setGravity(const Point3 &gravity);
  Point3 getGravity() const;

  int getDefaultMaterialId();
  int createNewMaterialId();

  void setMaterialPairInteraction(int mat_id1, int mat_id2, bool collide);
};

*/
