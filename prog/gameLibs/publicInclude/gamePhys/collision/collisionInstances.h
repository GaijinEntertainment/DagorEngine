//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_TMatrix.h>
#include <vecmath/dag_vecMathDecl.h>
#include <rendInst/rendInstDesc.h>
#include <gamePhys/collision/collisionObject.h>
#include <dag/dag_vector.h>

class CollisionResource;

namespace dacoll
{
struct ScaledBulletInstance
{
  float timeOfDeath;
  CollisionObject obj;
  rendinst::RendInstDesc desc;
  Point3 scale;
  CollisionObjectUserData userData;

  ScaledBulletInstance(CollisionObject original_obj, vec3f scl, const rendinst::RendInstDesc &in_desc, float time_of_death);

public:
  ScaledBulletInstance(const ScaledBulletInstance &) = delete;
  ~ScaledBulletInstance();
  ScaledBulletInstance &operator=(const ScaledBulletInstance &rhs) = delete;
  void updateUserDataPtr();
};

class CollisionInstances
{
public:
  CollisionInstances(const CollisionObject &obj) : originalObj(obj) {}

  CollisionInstances(const CollisionInstances &) = delete;
  CollisionInstances(CollisionInstances &&) = delete; // Relocatable
  ~CollisionInstances();

  CollisionInstances &operator=(CollisionInstances &&) = delete; // This need to be implemented only if you need to erase from
                                                                 // container
  CollisionInstances &operator=(const CollisionInstances &) = delete;

  void clear();
  void clearInstances();
  void unlink() { prevNotEmpty = nextNotEmpty = NOT_IN_LIST; }
  bool hasPhysBodyInstances() const { return !bulletInstances.empty(); }

  void removeCollisionObject(const rendinst::RendInstDesc &desc);
  CollisionObject updateTm(const rendinst::RendInstDesc &desc, const TMatrix &tm, TMatrix *out_ntm = nullptr,
    bool update_scale = false, bool instant = false);
  CollisionObject getCollisionObject(const rendinst::RendInstDesc &desc, TMatrix &tm, bool instant = false)
  {
    return updateTm(desc, tm, &tm, /*update_scale*/ true, instant);
  }
  CollisionObject updateTm(const rendinst::RendInstDesc &desc, const Point3 &vel, const Point3 &omega);

  bool update(float cur_time); // Return false if empty

private:
  ScaledBulletInstance *getScaledInstance(const rendinst::RendInstDesc &desc, vec3f scl, bool &out_created);

protected:
  static constexpr int NOT_IN_LIST = -1;
  // We can probably get away with 16 bit indexes but there is no padding in this struct so there is no point
  int prevNotEmpty = NOT_IN_LIST, nextNotEmpty = NOT_IN_LIST;

  dag::Vector<ScaledBulletInstance> bulletInstances;
  CollisionObject originalObj;

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
} // namespace dacoll

DAG_DECLARE_RELOCATABLE(dacoll::ScaledBulletInstance);
DAG_DECLARE_RELOCATABLE(dacoll::CollisionInstances);
