#include <phys/dag_physDecl.h>
#include <phys/dag_physics.h>

#include <gamePhys/collision/collisionLib.h>
#include <gamePhys/collision/collisionObject.h>
#include <gamePhys/collision/collisionInstances.h>
#include <gamePhys/collision/rendinstCollision.h>

#include <gamePhys/collision/rendinstCollisionWrapper.h>
#include <gamePhys/collision/rendinstContactResultWrapper.h>


CollisionObject WrapperRendInstCollisionImplCB::processCollisionInstance(const rendinst::CollisionInfo &coll_info,
  CollisionObject &alternative_obj, TMatrix &normalized_tm)
{
  dacoll::CollisionInstances *instance = dacoll::get_collision_instances_by_handle(coll_info.handle);
  return instance ? instance->getCollisionObject(coll_info.desc, normalized_tm) : alternative_obj;
}

void WrapperRendInstCollisionImplCB::addCollisionCheck(const rendinst::CollisionInfo &coll_info)
{
  if (!coll_info.handle)
    return;

  if (coll_info.treeBehaviour)
  {
    addTreeCheck(coll_info);
    return;
  }

  // TODO: optimize
  dacoll::CollisionInstances *instance = dacoll::get_collision_instances_by_handle(coll_info.handle);
  if (instance && !dacoll::is_ri_instance_enabled(instance, coll_info.desc))
    return;

  TMatrix normalizedTm = coll_info.tm;
  CollisionObject empty;
  CollisionObject cobj = processCollisionInstance(coll_info, empty, normalizedTm);

  RendinstCollisionUserInfo userInfo(coll_info.desc);
  userInfo.bbox = coll_info.localBBox;
  userInfo.immortal = coll_info.isImmortal;
  userInfo.isTree = false;
  userInfo.isDestr = coll_info.isDestr;
  userInfo.tm = normalizedTm;
  userInfo.bushBehaviour = coll_info.bushBehaviour;
  userInfo.treeBehaviour = coll_info.treeBehaviour;
  if (coll_info.isDestr && shouldProvideCollisionInfo)
  {
    CachedCollisionObjectInfo *objInfo = rendinstdestr::get_or_add_cached_collision_object(coll_info.desc, atTime, coll_info);
    if (restoreImpulse)
      objInfo->thresImpulse = objInfo->originalThreshold;
    objInfo->atTime = atTime;
    userInfo.objInfoData = objInfo;
  }

  void *prevUserPtr = cobj.body->getUserData();
  cobj.body->setUserData((void *)&userInfo);
  PhysWorld *physWorld = dacoll::get_phys_world();
  physWorld->fetchSimRes(true);
  physWorld->contactTestPair(objA.body, cobj.body, resultCallback);
#if DAGOR_DBGLEVEL > 0
  if (physdbg::isInDebugMode(physWorld))
  {
    physdbg::renderOneBody(physWorld, cobj.body, physdbg::RenderFlag::BODIES);
    physdbg::renderOneBody(physWorld, objA.body, physdbg::RenderFlag::BODIES, 0xFF0000);
  }
#endif
  cobj.body->setUserData(prevUserPtr);
}

void WrapperRendInstCollisionImplCB::addTreeCheck(const rendinst::CollisionInfo &coll_info)
{
  float ht = coll_info.localBBox[1].y;
  float rad = fabsf(coll_info.localBBox[0].x);
  if (!rendinstdestr::get_tree_collision())
  {
    CollisionObject &treeColl = rendinstdestr::get_tree_collision();
    treeColl = dacoll::create_coll_obj_from_shape(PhysCapsuleCollision(rad, ht + rad * 2, 1 /*Y*/), NULL, /*kinematic*/ false, false);
  }

  if (!rendinstdestr::get_tree_collision())
    return;

  TMatrix normalizedTm = coll_info.tm;
  CollisionObject obj = processCollisionInstance(coll_info, rendinstdestr::get_tree_collision(), normalizedTm);

  RendinstCollisionUserInfo userInfo(coll_info.desc);
  userInfo.bbox = coll_info.localBBox;
  userInfo.immortal = coll_info.isImmortal;
  userInfo.isTree = true;
  userInfo.tm = normalizedTm;
  userInfo.bushBehaviour = coll_info.bushBehaviour;
  userInfo.treeBehaviour = coll_info.treeBehaviour;
  if (shouldProvideCollisionInfo)
  {
    const rendinstdestr::TreeDestr &treeDestr = rendinstdestr::get_tree_destr();

    const float height = coll_info.localBBox.width().y * coll_info.tm.getcol(1).y;
    float destrImpulse = treeDestr.impulseThresholdMult * sqr(max(height - treeDestr.heightThreshold, 1.f));
    if (coll_info.destrImpulse > 0.f)
    {
      float widthScale = length(coll_info.tm.getcol(0)) * length(coll_info.tm.getcol(2));
      destrImpulse = coll_info.destrImpulse * widthScale;
    }
    destrImpulse *= treeDestr.getRadiusToImpulse(coll_info.localBBox.width().x * 0.5f);
    CachedCollisionObjectInfo *objInfo = rendinstdestr::get_cached_collision_object(coll_info.desc);
    if (!objInfo)
    {
      objInfo = new RendinstCollisionUserInfo::TreeRendinstImpulseThresholdData(destrImpulse, coll_info.desc, atTime, coll_info);
      rendinstdestr::add_cached_collision_object(objInfo);
    }
    if (restoreImpulse)
      objInfo->thresImpulse = objInfo->originalThreshold;
    objInfo->atTime = atTime;
    userInfo.objInfoData = objInfo;
    userInfo.objInfoData->collisionHardness = 0.3f;
  }
  void *prevUserPtr = obj.body->getUserData();
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
  obj.body->setUserData(prevUserPtr);
}
