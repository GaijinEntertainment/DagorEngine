#pragma once
#include <rendInst/rendInstGen.h>
#include <gamePhys/collision/rendinstCollision.h>

template <typename T>
struct RICollisionCB : public rendinst::RendInstCollisionCB
{
  T &callback;

  RICollisionCB(T &in_callback) : callback(in_callback) {}

  CollisionObject processCollisionInstance(const rendinst::RendInstCollisionCB::CollisionInfo &info)
  {
    dacoll::CollisionInstances *instance = dacoll::get_collision_instances_by_handle(info.handle);
    G_ASSERTF(instance, "Cannot find collision for ri at " FMT_P3, P3D(info.tm.getcol(3)));
    if (!instance || !dacoll::is_ri_instance_enabled(instance, info.desc))
      return CollisionObject();

    CollisionObject cobj = instance->updateTm(info.desc, info.tm);

    callback.collMatId = rendinst::getRIGenMaterialId(info.desc);
    return cobj;
  }

  virtual void addCollisionCheck(const rendinst::RendInstCollisionCB::CollisionInfo &info)
  {
    if (!info.handle)
      return;

    CollisionObject riObj = processCollisionInstance(info);
    callback.onContact(riObj, info.desc);
  }

  virtual void addTreeCheck(const rendinst::RendInstCollisionCB::CollisionInfo &info)
  {
    if (!info.handle)
      return;

    CollisionObject riObj = processCollisionInstance(info);
    callback.onContact(riObj, info.desc);
  }
};
