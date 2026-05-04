// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/collision/rendinstContactResultWrapper.h>
#include <gamePhys/collision/rendinstCollisionUserInfo.h>
#include <gamePhys/phys/rendinstDestr.h>
#include <rendInst/treeDestr.h>
#include <math/dag_mathUtils.h>
#include "riUserInfo.h"

void WrapperRendinstContactResultCB::addSingleResult(contact_data_t &cp, obj_user_data_t *userPtrA,
  const RendinstCollisionUserInfo &user_info, gamephys::CollisionObjectInfo *object_info)
{
  auto &c = WrapperContactResultCB::addSingleResult(cp, userPtrA, (obj_user_data_t *)user_info.prevUserPtrB);
  c.objectInfo = object_info;
  c.riDesc = user_info.desc;
}

void WrapperRendinstContactResultCB::addSingleResult(contact_data_t &cp, obj_user_data_t *userPtrA, obj_user_data_t *userPtrB)
{
  const RendinstCollisionUserInfo *userInfo = (const RendinstCollisionUserInfo *)userPtrB;
  G_ASSERT(userInfo && userInfo->physObjectType == _MAKE4C('RIUD'));
  Point3 boxWidth = userInfo->bbox.width();
  bool immortal = userInfo->immortal || (boxWidth.y > 4.f || boxWidth.x > 4.f || boxWidth.z > 4.f);
  bool isSecondLayer = userInfo->desc.layer == 1;
  bool forceBushBehaviour = userInfo->bushBehaviour;
  bool forceTreeBehaviour = userInfo->treeBehaviour;
  if (userInfo->isTree || forceBushBehaviour || forceTreeBehaviour)
  {
    const rendinstdestr::TreeDestr &treeDestr = rendinstdestr::get_tree_destr();

    const bool isSmallTree = (boxWidth.y < treeDestr.heightThreshold && boxWidth.x * 0.5f < treeDestr.radiusThreshold);

    if (!forceTreeBehaviour && (isSecondLayer || isSmallTree || forceBushBehaviour) && processTreeBehaviour)
    {
      rendinstdestr::create_tree_rend_inst_destr(userInfo->desc, true, Point3(0.f, 0.f, 0.f), Point3(0.f, 0.f, 0.f), false, false, 0.f,
        0.f, NULL, true, false);
    }
    else
    {
      userInfo->collided = true;
      addSingleResult(cp, userPtrA, *userInfo, userInfo->objInfoData);
    }
  }
  else if (isSecondLayer)
  {
    // Do nothing with these guys.
    return;
  }
  else if (userInfo->isDestr)
  {
    userInfo->collided = true;
    addSingleResult(cp, userPtrA, *userInfo, userInfo->objInfoData);
  }
  else if (immortal)
  {
    addSingleResult(cp, userPtrA, *userInfo);
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
