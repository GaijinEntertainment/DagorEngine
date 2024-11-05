// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/collision/rendinstContactResultWrapper.h>
#include <gamePhys/collision/rendinstCollisionUserInfo.h>
#include <gamePhys/phys/rendinstDestr.h>
#include <rendInst/treeDestr.h>
#include <math/dag_mathUtils.h>

void WrapperRendinstContactResultCB::addSingleResult(contact_data_t &cp, obj_user_data_t *userPtrA, obj_user_data_t *userPtrB,
  const RendinstCollisionUserInfo *user_info, gamephys::CollisionObjectInfo *object_info)
{
  auto &c = WrapperContactResultCB::addSingleResult(cp, userPtrA, userPtrB);
  c.objectInfo = object_info;
  c.riPool = (user_info && !user_info->desc.isRiExtra()) ? user_info->desc.pool : -1;
}

void WrapperRendinstContactResultCB::addSingleResult(contact_data_t &cp, obj_user_data_t *userPtrA, obj_user_data_t *userPtrB)
{
  const RendinstCollisionUserInfo *userInfo = (const RendinstCollisionUserInfo *)userPtrB;
  G_ASSERT(!userInfo || userInfo->physObjectType == _MAKE4C('RIUD'));
  Point3 boxWidth = userInfo ? userInfo->bbox.width() : Point3(0.f, 0.f, 0.f);
  bool immortal = userInfo ? userInfo->immortal || (boxWidth.y > 4.f || boxWidth.x > 4.f || boxWidth.z > 4.f) : true;
  bool isSecondLayer = userInfo && userInfo->desc.layer == 1;
  bool forceBushBehaviour = userInfo && userInfo->bushBehaviour;
  bool forceTreeBehaviour = userInfo && userInfo->treeBehaviour;
  if (userInfo && (userInfo->isTree || forceBushBehaviour || forceTreeBehaviour))
  {
    const rendinstdestr::TreeDestr &treeDestr = rendinstdestr::get_tree_destr();

    const bool isSmallTree = (boxWidth.y < treeDestr.heightThreshold && boxWidth.x * 0.5f < treeDestr.radiusThreshold);

    if (!forceTreeBehaviour && (isSecondLayer || isSmallTree || forceBushBehaviour) && processTreeBehaviour)
    {
      rendinstdestr::create_tree_rend_inst_destr(userInfo->desc, true, Point3(0.f, 0.f, 0.f), Point3(0.f, 0.f, 0.f), false, false, 0.f,
        0.f, NULL, true);
    }
    else
    {
      userInfo->collided = true;
      addSingleResult(cp, userPtrA, userPtrB, userInfo, userInfo->objInfoData);
    }
  }
  else if (isSecondLayer)
  {
    // Do nothing with these guys.
    return;
  }
  else if (userInfo && userInfo->isDestr)
  {
    userInfo->collided = true;
    addSingleResult(cp, userPtrA, userPtrB, userInfo, userInfo->objInfoData);
  }
  else if (immortal)
  {
    addSingleResult(cp, userPtrA, userPtrB, userInfo);
  }
  else
  {
    Point3 pos = originalRIPos;
    Point3 axis = normalizeDef((cp.wposB - cp.wpos) % Point3(0.f, 1.f, 0.f), Point3(1.f, 0.f, 0.f));
    // stash damage for apply it later dtor to avoid intermixing rendinst ro/rw locks
    riGenDamage.push_back({pos, axis});
  }
}

void WrapperRendinstContactResultCB::applyRiGenDamage() const
{
  for (auto &rgd : riGenDamage)
    rendinst::doRIGenDamage(rgd.pos, frameNo, rendinstdestr::get_ri_damage_effect_cb(), rgd.axis);
}
