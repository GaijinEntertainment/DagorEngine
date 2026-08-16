// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/collision/rendinstCollisionUserInfo.h>

#include <gamePhys/phys/rendinstDestr.h>
#include <gamePhys/phys/rendinstPhys.h>
#include <rendInst/treeDestr.h>
#include <rendInst/rendInstExtra.h>

#if DAGOR_DBGLEVEL > 0
void (*ri_coll_damage_log)(const rendinst::CollisionInfo &coll_info, float impulse, float damage) = nullptr;
void (*ri_coll_destr_result_log)(const gamephys::CollisionObjectInfo *obj_info, float applied, float absorbed,
  bool destroyed) = nullptr;
#endif

RendinstImpulseThresholdData::RendinstImpulseThresholdData(float impulse, const rendinst::RendInstDesc &ri_desc, float at_time,
  const rendinst::CollisionInfo &coll_info) :
  CachedCollisionObjectInfo(impulse, ri_desc, at_time), collInfo(coll_info)
{
  finalPos.zero();
  finalImpulse.zero();
}

RendinstImpulseThresholdData::~RendinstImpulseThresholdData()
{
  if (!alive)
    // for now don't apply absorbed impulse, as it makes behaviour worse
    rendinstdestr::destroyRendinst(riDesc, true /*add_restorable*/, Point3(0.f, 0.f, 0.f), Point3(0.f, 0.f, 0.f), atTime, &collInfo,
      true /*createDestr*/, nullptr, -1, 1, nullptr,
      rendinst::DestrOptionFlag::DestroyedByCollision | rendinst::DestrOptionFlag::AddDestroyedRi |
        rendinst::DestrOptionFlag::ForceDestroy);
}

float RendinstImpulseThresholdData::onImpulse(float impulse, const Point3 &dir, const Point3 &pos, float point_vel,
  const Point3 &collision_normal, uint32_t flags, int32_t user_data)
{
  if (riDesc.isRiExtra() && impulse > 0.0f && rendinst::get_ri_phys_settings().impulseCallbacksEnabled)
    rendinst::onRiExtraImpulse(riDesc.getRiExtraHandle(), impulse, dir, pos, collision_normal, user_data);

  if (flags & CIF_NO_DAMAGE)
    return impulse;

  collInfo.userData = user_data;

  float absorbedImpulse = 0.f;
  if (riDesc.isRiExtra() && collInfo.hp > 0.f)
  {
    if (riDesc.pool < 0)
      return 0;

    absorbedImpulse = impulse;
    if (impulse > 0 && alive)
    {
      float damage;
      if (collInfo.destrImpulse > 0)
        damage = collInfo.initialHp * (impulse / collInfo.destrImpulse);
      else
        damage = impulse * rendinstdestr::get_destr_settings().destrImpulseHitPointsMult;
      absorbedImpulse = damage;
      if (ri_coll_damage_log)
        ri_coll_damage_log(collInfo, impulse, damage);
      if (rendinst::applyDamageRIGenExtra(riDesc, damage, &absorbedImpulse, true))
        alive = false;
      absorbedImpulse = safediv(absorbedImpulse, damage) * impulse;
      if (ri_coll_destr_result_log)
        ri_coll_destr_result_log(this, impulse, absorbedImpulse, !alive);
    }
  }
  else
  {
    const bool wasAlive = alive;
    absorbedImpulse = min(impulse, thresImpulse);
    if (absorbedImpulse > 0.f && impulse >= thresImpulse && alive)
      alive = false;
    CachedCollisionObjectInfo::onImpulse(impulse, dir, pos, point_vel, collision_normal, flags, user_data);
    if (ri_coll_destr_result_log && wasAlive)
      ri_coll_destr_result_log(this, impulse, absorbedImpulse, !alive);
  }

  finalImpulse += -dir * absorbedImpulse;
  finalPos = pos;
  thresImpulse = min(originalThreshold, thresImpulse - absorbedImpulse);

  return absorbedImpulse;
}

TreeRendinstImpulseThresholdData::TreeRendinstImpulseThresholdData(float impulse, const rendinst::RendInstDesc &ri_desc, float at_time,
  const rendinst::CollisionInfo &coll_info) :
  CachedCollisionObjectInfo(impulse, ri_desc, at_time), lastPointVel(0.f), lastOmega(0.f), collInfo(coll_info)
{
  finalImpulse.zero();
}

TreeRendinstImpulseThresholdData::~TreeRendinstImpulseThresholdData()
{
  if (!alive)
  {
    const rendinstdestr::TreeDestr &treeDestr = rendinstdestr::get_tree_destr();
    rendinstdestr::create_tree_rend_inst_destr(riDesc, true, finalPos, finalImpulse, true, lastPointVel < treeDestr.minSpeed,
      lastOmega, atTime, &collInfo, true, false);
  }
}

float TreeRendinstImpulseThresholdData::onImpulse(float impulse, const Point3 &dir, const Point3 &pos, float point_vel,
  const Point3 &collision_normal, uint32_t flags, int32_t user_data)
{
  if (flags & CIF_NO_DAMAGE)
    return impulse;

  G_ASSERTF(!check_nan(impulse) && impulse < 1e8f, "onImpulse %f, originalThreshold %f", impulse, originalThreshold);
  const bool wasAlive = alive;
  Point3 localPos = inverse(collInfo.tm) * pos;
  const float leveredImpulse = impulse * max(1.f, length(localPos));
  float absorbedImpulse = min(leveredImpulse, thresImpulse);
  finalImpulse += -dir * absorbedImpulse;
  if (absorbedImpulse > 0.f && impulse >= thresImpulse && alive)
    alive = false;
  lastPointVel = point_vel;
  if (ri_coll_destr_result_log && wasAlive)
    ri_coll_destr_result_log(this, impulse, absorbedImpulse, !alive);
  thresImpulse = min(originalThreshold, thresImpulse - absorbedImpulse);
  lastOmega = safediv(point_vel, localPos.y);
  finalPos = pos;
  CachedCollisionObjectInfo::onImpulse(impulse, dir, pos, point_vel, collision_normal, flags, user_data);
  return min(absorbedImpulse, impulse);
}
