//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_TMatrix.h>
#include <rendInst/rendInstGen.h>
#include <gamePhys/collision/collisionObject.h>
#include <dag/dag_vector.h>

class CollisionResource;

#if ENABLE_APEX
#define APEX_VIRTUAL  virtual
#define APEX_OVERRIDE override
#else
#define APEX_VIRTUAL
#define APEX_OVERRIDE
#endif

namespace dacoll
{
// Collision Instances Collision Object (probably need to rename whole thing to Rendinst Collision Instances)
#if ENABLE_APEX
typedef CollisionObject CICollisionObject;
#else
typedef CollisionObjectBullet CICollisionObject;
#endif

struct ScaledBulletInstance
{
  float timeOfDeath;
  CollisionObjectBullet obj;
  rendinst::RendInstDesc desc;
  Point3 scale;
  CollisionObjectUserData userData;

  ScaledBulletInstance(CICollisionObject original_obj, const Point3 &scl, const rendinst::RendInstDesc &in_desc, float time_of_death);

public:
  ScaledBulletInstance(const ScaledBulletInstance &) = delete;
  ~ScaledBulletInstance();
  ScaledBulletInstance &operator=(const ScaledBulletInstance &rhs) = delete;
  void updateUserDataPtr();
};

// Note: this class is not ABI combatible across Apex/non-Apex library build.
// Don't use it unless you know what are you doing
class CollisionInstances
{
public:
  CollisionInstances(const CICollisionObject &obj) : originalObj(obj) {}

  CollisionInstances(const CollisionInstances &) = delete;
  CollisionInstances(CollisionInstances &&);
  APEX_VIRTUAL ~CollisionInstances();

  CollisionInstances &operator=(CollisionInstances &&) = delete; // This need to be implemented only if you need to erase from
                                                                 // container
  CollisionInstances &operator=(const CollisionInstances &) = delete;

  void clear();
  APEX_VIRTUAL void clearInstances();
  void unlink();
  bool empty() const { return bulletInstances.empty(); }

  void removeCollisionObject(const rendinst::RendInstDesc &desc);
  void enableDisableCollisionObject(const rendinst::RendInstDesc &desc, bool flag);
  bool isCollisionObjectEnabled(const rendinst::RendInstDesc &desc) const;
  CollisionObject updateTm(const rendinst::RendInstDesc &desc, const TMatrix &tm, TMatrix *out_ntm = nullptr,
    bool update_scale = false, bool instant = false);
  CollisionObject getCollisionObject(const rendinst::RendInstDesc &desc, TMatrix &tm, bool instant = false)
  {
    return updateTm(desc, tm, &tm, /*update_scale*/ true, instant);
  }
  CollisionObject updateTm(const rendinst::RendInstDesc &desc, const Point3 &vel, const Point3 &omega);

  APEX_VIRTUAL physx::PxRigidActor *getPhysXCollisionObject(const Point3 &, const TMatrix &, const rendinst::RendInstDesc &, bool)
  {
    return NULL;
  }
  APEX_VIRTUAL bool deletePhysXCollisionObject(const rendinst::RendInstDesc &) { return false; }

  APEX_VIRTUAL bool update(float dt); // Return false if empty

private:
  const ScaledBulletInstance *getScaledInstance(const rendinst::RendInstDesc &desc, const Point3 &scale, bool &out_created);

protected:
  // We can probably get away with 16 bit indexes but there is no padding in this struct so there is no point
  int prevNotEmpty = -1, nextNotEmpty = -1;

  dag::Vector<ScaledBulletInstance> bulletInstances;
  CICollisionObject originalObj;

  // Be default all instances are enabled and we want to keep a compact list of disabled instances.
  // These instances can become disabled so they'll not interact *physically* with other objects,
  // for instance it's beneficial to disable these instances when we want to process collision of
  // this object in an alternative way (physobj, of physbody)
  dag::Vector<rendinst::RendInstDesc> disabledInstances;

  friend void *register_collision_cb(const CollisionResource *collRes, const char *debug_name);
  friend void unregister_collision_cb(void *&handle);
  friend CollisionInstances *get_collision_instances_by_handle(void *handle);
  friend void push_non_empty_ri_instance_list(CollisionInstances &ci);
  friend void remove_empty_ri_instance_list(CollisionInstances &ci);
  friend void update_ri_instances(float dt);
};

inline bool CollisionInstances::update(float cur_time)
{
  for (auto it = bulletInstances.begin(); it != bulletInstances.end();)
  {
    if (cur_time >= it->timeOfDeath)
    {
      it = bulletInstances.erase_unsorted(it);
      if (it != bulletInstances.end() && cur_time < it->timeOfDeath)
        it->updateUserDataPtr();
    }
    else
      ++it;
  }
  return !bulletInstances.empty();
}

void remove_physx_collision_object(const rendinst::RendInstDesc &desc);
CollisionInstances *create_apex_coll_instance(const CollisionObject &obj);
} // namespace dacoll

DAG_DECLARE_RELOCATABLE(dacoll::ScaledBulletInstance);
DAG_DECLARE_RELOCATABLE(dacoll::CollisionInstances);

#undef APEX_VIRTUAL
