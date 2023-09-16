#include <gamePhys/collision/rendinstContactResultWrapper.h>
#include <gamePhys/collision/rendinstCollisionUserInfo.h>
#include <gamePhys/phys/rendinstDestr.h>
#include <gamePhys/phys/treeDestr.h>
#include <math/dag_mathUtils.h>


float WrapperRendinstContactResultCB::addSingleResult(contact_data_t &cp, obj_user_data_t *userPtrA, obj_user_data_t *userPtrB,
  gamephys::CollisionObjectInfo *)
{
  const RendinstCollisionUserInfo *userInfo = (const RendinstCollisionUserInfo *)userPtrB;
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
      rendinstdestr::create_tree_rend_inst_destr(userInfo->desc, true, Point3(0.f, 0.f, 0.f), false, false, 0.f, 0.f, NULL, true);
    }
    else
    {
      userInfo->collided = true;
      WrapperContactResultCB::addSingleResult(cp, userPtrA, userPtrB, userInfo->objInfoData);
    }
  }
  else if (isSecondLayer)
  {
    // Do nothing with these guys.
    return 0;
  }
  else if (userInfo && userInfo->isDestr)
  {
    userInfo->collided = true;
    WrapperContactResultCB::addSingleResult(cp, userPtrA, userPtrB, userInfo->objInfoData);
  }
  else if (immortal)
  {
    WrapperContactResultCB::addSingleResult(cp, userPtrA, userPtrB, nullptr);
  }
  else
  {
    Point3 pos = originalRIPos;
    Point3 axis = normalizeDef((cp.wposB - cp.wpos) % Point3(0.f, 1.f, 0.f), Point3(1.f, 0.f, 0.f));
    // stash damage for apply it later dtor to avoid intermixing rendinst ro/rw locks
    riGenDamage.push_back({pos, axis});
  }
  return 0;
}

void WrapperRendinstContactResultCB::applyRiGenDamage() const
{
  for (auto &rgd : riGenDamage)
    rendinst::doRIGenDamage(rgd.pos, frameNo, rendinstdestr::get_ri_damage_effect_cb(), rgd.axis);
}
