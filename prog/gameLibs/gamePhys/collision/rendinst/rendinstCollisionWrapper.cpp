// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <phys/dag_physDecl.h>
#include <phys/dag_physics.h>
#include <gamePhys/collision/collisionLib.h>
#include <gamePhys/collision/collisionObject.h>
#include <gamePhys/collision/collisionInstances.h>
#include <gamePhys/collision/rendinstCollision.h>
#include <gamePhys/collision/rendinstCollisionWrapper.h>
#include <gamePhys/collision/rendinstContactResultWrapper.h>
#include "riUserInfo.h"
#include "../collisionLibPrivate.h"


CollisionObject WrapperRendInstCollisionImplCB::processCollisionInstance(const rendinst::CollisionInfo &coll_info,
  CollisionObject &alternative_obj, TMatrix &normalized_tm)
{
  dacoll::CollisionInstances *instance = dacoll::get_collision_instances_by_handle(coll_info.handle);
  return instance ? instance->getCollisionObject(coll_info.desc, normalized_tm) : alternative_obj;
}

void WrapperRendInstCollisionImplCB::addCollisionCheck(const rendinst::CollisionInfo &coll_info)
{
  if (!coll_info.handle)
  {
    if (coll_info.bushBehaviour && !(resultCallback.processBushBehaviour && resultCallback.processTreeBehaviour))
      addNonDestrBushCheck(coll_info);
    return;
  }

  if (coll_info.treeBehaviour)
  {
    addTreeCheck(coll_info);
    return;
  }

  if (!dacoll::is_ri_instance_enabled(coll_info.desc))
    return;

  TMatrix normalizedTm = coll_info.tm;
  CollisionObject empty;
  CollisionObject cobj = processCollisionInstance(coll_info, empty, normalizedTm);
  if (!cobj) // PhysBody alloc failed?
    return;

  RendinstCollisionUserInfo userInfo(coll_info.desc);
  userInfo.bbox = coll_info.localBBox;
  userInfo.immortal = coll_info.isImmortal;
  userInfo.isTree = false;
  userInfo.isDestr = coll_info.isDestr;
  userInfo.tm = normalizedTm;
  userInfo.bushBehaviour = coll_info.bushBehaviour;
  userInfo.treeBehaviour = coll_info.treeBehaviour;
  if (shouldProvideCollisionInfo)
  {
    if (coll_info.isDestr)
    {
      CachedCollisionObjectInfo *objInfo = rendinstdestr::get_or_add_cached_collision_object(coll_info.desc, atTime, coll_info);
      if (restoreImpulse)
        objInfo->thresImpulse = objInfo->originalThreshold;
      userInfo.objInfoData = objInfo;
    }
    else if (coll_info.bushBehaviour && !(resultCallback.processBushBehaviour && resultCallback.processTreeBehaviour))
    {
      using namespace rendinstdestr;
      const TreeDestr &treeDestr = get_tree_destr();
      Point3 boxWidth = coll_info.localBBox.width();
      float destrImpulse;
      if (coll_info.destrImpulse > 0.f)
      {
        float widthScale = length(coll_info.tm.getcol(0)) * length(coll_info.tm.getcol(2));
        destrImpulse = coll_info.destrImpulse * widthScale;
      }
      else
      {
        const float height = boxWidth.y * coll_info.tm.getcol(1).y;
        destrImpulse = treeDestr.impulseThresholdMult * sqr(max(height - treeDestr.heightThreshold, 1.f));
      }
      destrImpulse *= treeDestr.getRadiusToImpulse(boxWidth.x * 0.5f);
      userInfo.objInfoData = get_or_add_cached_nondestr_bush_collision_object(coll_info.desc, destrImpulse, atTime);
    }
  }

  userInfo.prevUserPtrB = cobj.body->getUserData();
  cobj.body->setUserData(&userInfo);
  PhysWorld *physWorld = dacoll::get_phys_world();
  physWorld->fetchSimRes(true);
  physWorld->contactTestPair(objA.body, cobj.body, resultCallback);
  cobj.body->setUserData(userInfo.prevUserPtrB); // Restore

#if DAGOR_DBGLEVEL > 0
  if (physdbg::isInDebugMode(physWorld))
  {
    physdbg::renderOneBody(physWorld, cobj.body, physdbg::RenderFlag::BODIES);
    physdbg::renderOneBody(physWorld, objA.body, physdbg::RenderFlag::BODIES, 0xFF0000);
  }
#endif
}

void WrapperRendInstCollisionImplCB::addTreeCheck(const rendinst::CollisionInfo &coll_info)
{
  using namespace rendinstdestr;

  float ht = coll_info.localBBox[1].y;
  float rad = fabsf(coll_info.localBBox[0].x);
  if (!get_tree_collision())
  {
    CollisionObject &treeColl = get_tree_collision();
    treeColl = dacoll::create_coll_obj_from_shape(PhysCapsuleCollision(rad, ht + rad * 2, 1 /*Y*/), NULL, /*kinematic*/ false,
      /* add to world */ false, /* auto mask */ true);
  }

  if (!get_tree_collision())
    return;

  TMatrix normalizedTm = coll_info.tm;
  CollisionObject obj = processCollisionInstance(coll_info, get_tree_collision(), normalizedTm);
  if (!obj) // PhysBody alloc failed?
    return;

  RendinstCollisionUserInfo userInfo(coll_info.desc);
  userInfo.bbox = coll_info.localBBox;
  userInfo.immortal = coll_info.isImmortal;
  userInfo.isTree = true;
  userInfo.tm = normalizedTm;
  userInfo.bushBehaviour = coll_info.bushBehaviour;
  userInfo.treeBehaviour = coll_info.treeBehaviour;
  if (shouldProvideCollisionInfo)
  {
    const TreeDestr &treeDestr = get_tree_destr();

    const float height = coll_info.localBBox.width().y * coll_info.tm.getcol(1).y;
    float destrImpulse = treeDestr.impulseThresholdMult * sqr(max(height - treeDestr.heightThreshold, 1.f));
    if (coll_info.destrImpulse > 0.f)
    {
      float widthScale = length(coll_info.tm.getcol(0)) * length(coll_info.tm.getcol(2));
      destrImpulse = coll_info.destrImpulse * widthScale;
    }
    destrImpulse *= treeDestr.getRadiusToImpulse(coll_info.localBBox.width().x * 0.5f);
    CachedCollisionObjectInfo *objInfo = get_or_add_cached_tree_collision_object(coll_info.desc, destrImpulse, atTime, coll_info);
    if (restoreImpulse)
      objInfo->thresImpulse = objInfo->originalThreshold;
    userInfo.objInfoData = objInfo;
  }
  userInfo.prevUserPtrB = obj.body->getUserData();
  obj.body->setUserData((void *)&userInfo);
  PhysWorld *physWorld = dacoll::get_phys_world();
  physWorld->fetchSimRes(true);
  physWorld->contactTestPair(objA.body, obj.body, resultCallback);

#if DAGOR_DBGLEVEL > 0
  if (physdbg::isInDebugMode(physWorld))
  {
    physdbg::renderOneBody(physWorld, obj.body, normalizedTm, physdbg::RenderFlag::BODIES);
    physdbg::renderOneBody(physWorld, objA.body, physdbg::RenderFlag::BODIES, 0xFF0000);
  }
#endif
  obj.body->setUserData(userInfo.prevUserPtrB);
}

// Mirrors addTreeCheck's handle-independent capsule proxy, for a bush-flagged RI with no
// registered collision resource: get_tree_collision()'s shared capsule is sized from whichever
// tree hits it first, which can be far larger than a bush, so this uses its own shared capsule
// and resizes it to the current bush bbox before every test. Uses NonDestrBushImpulseThresholdData, not
// TreeRendinstImpulseThresholdData, so the RI is never popped via create_tree_rend_inst_destr.
void WrapperRendInstCollisionImplCB::addNonDestrBushCheck(const rendinst::CollisionInfo &coll_info)
{
  using namespace rendinstdestr;

  float ht = coll_info.localBBox[1].y;
  float rad = fabsf(coll_info.localBBox[0].x);
  if (!get_nondestr_bush_collision())
  {
    CollisionObject &bushColl = get_nondestr_bush_collision();
    bushColl = dacoll::create_coll_obj_from_shape(PhysCapsuleCollision(rad, ht + rad * 2, 1 /*Y*/), NULL, /*kinematic*/ false,
      /* add to world */ false, /* auto mask */ true);
  }

  if (!get_nondestr_bush_collision())
    return;

  TMatrix normalizedTm = coll_info.tm;
  CollisionObject obj = processCollisionInstance(coll_info, get_nondestr_bush_collision(), normalizedTm);
  if (!obj) // PhysBody alloc failed?
    return;

  RendinstCollisionUserInfo userInfo(coll_info.desc);
  userInfo.bbox = coll_info.localBBox;
  userInfo.immortal = coll_info.isImmortal;
  userInfo.isTree = false;
  userInfo.tm = normalizedTm;
  userInfo.bushBehaviour = coll_info.bushBehaviour;
  userInfo.treeBehaviour = coll_info.treeBehaviour;
  if (shouldProvideCollisionInfo)
  {
    const TreeDestr &treeDestr = get_tree_destr();
    Point3 boxWidth = coll_info.localBBox.width();
    float destrImpulse;
    if (coll_info.destrImpulse > 0.f)
    {
      float widthScale = length(coll_info.tm.getcol(0)) * length(coll_info.tm.getcol(2));
      destrImpulse = coll_info.destrImpulse * widthScale;
    }
    else
    {
      const float height = boxWidth.y * coll_info.tm.getcol(1).y;
      destrImpulse = treeDestr.impulseThresholdMult * sqr(max(height - treeDestr.heightThreshold, 1.f));
    }
    destrImpulse *= treeDestr.getRadiusToImpulse(boxWidth.x * 0.5f);
    userInfo.objInfoData = get_or_add_cached_nondestr_bush_collision_object(coll_info.desc, destrImpulse, atTime);
  }
  dacoll::set_vert_capsule_shape_size(obj, rad, ht);
  dacoll::set_collision_object_tm(obj, normalizedTm);
  userInfo.prevUserPtrB = obj.body->getUserData();
  obj.body->setUserData((void *)&userInfo);
  PhysWorld *physWorld = dacoll::get_phys_world();
  physWorld->fetchSimRes(true);
  physWorld->contactTestPair(objA.body, obj.body, resultCallback);

#if DAGOR_DBGLEVEL > 0
  if (physdbg::isInDebugMode(physWorld))
  {
    physdbg::renderOneBody(physWorld, obj.body, normalizedTm, physdbg::RenderFlag::BODIES);
    physdbg::renderOneBody(physWorld, objA.body, physdbg::RenderFlag::BODIES, 0xFF0000);
  }
#endif
  obj.body->setUserData(userInfo.prevUserPtrB);
}
