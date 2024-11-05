// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/collision/rendinstCollisionUserInfo.h>

#include <gamePhys/phys/rendinstDestr.h>
#include <gamePhys/phys/rendinstPhys.h>
#include <rendInst/treeDestr.h>
#include <rendInst/rendInstExtra.h>


RendinstCollisionUserInfo::RendinstImpulseThresholdData::RendinstImpulseThresholdData(float impulse,
  const rendinst::RendInstDesc &ri_desc, float at_time, const rendinst::CollisionInfo &coll_info) :
  CachedCollisionObjectInfo(impulse, ri_desc, at_time), collInfo(coll_info)
{}

RendinstCollisionUserInfo::RendinstImpulseThresholdData::~RendinstImpulseThresholdData()
{
  if (!alive)
    rendinstdestr::destroyRendinst(riDesc, true /*add_restorable*/, Point3(0.f, 0.f, 0.f), Point3(0.f, 0.f, 0.f), atTime, &collInfo,
      rendinstdestr::get_destr_settings().createDestr);
}

float RendinstCollisionUserInfo::RendinstImpulseThresholdData::onImpulse(float impulse, const Point3 &dir, const Point3 &pos,
  float point_vel, const Point3 &collision_normal, uint32_t flags, int32_t user_data, gamephys::ImpulseLogFunc log_func)
{
  if (riDesc.isRiExtra() && impulse > 0.0f && rendinst::get_ri_phys_settings().impulseCallbacksEnabled)
    rendinst::onRiExtraImpulse(riDesc.getRiExtraHandle(), impulse, dir, pos, collision_normal, user_data);

  if (flags & CIF_NO_DAMAGE)
    return impulse;

  collInfo.userData = user_data;

  if (riDesc.isRiExtra() && collInfo.hp > 0.f)
  {
    if (riDesc.pool < 0)
      return 0;

    float absorbedImpulse = impulse;
    if (impulse > 0 && alive)
    {
      float damage;
      if (collInfo.destrImpulse > 0)
        damage = collInfo.initialHp * (impulse / collInfo.destrImpulse);
      else
        damage = impulse * rendinstdestr::get_destr_settings().destrImpulseHitPointsMult;
      absorbedImpulse = damage;
      if (log_func)
        log_func(rendinst::get_rendinst_res_name_from_col_info(collInfo), impulse, damage, collInfo.hp);
      if (rendinst::applyDamageRIGenExtra(riDesc, damage, &absorbedImpulse, true))
        alive = false;
      absorbedImpulse = safediv(absorbedImpulse, damage) * impulse;
    }
    return absorbedImpulse;
  }

  float absorbedImpulse = min(impulse, thresImpulse);
  if (absorbedImpulse > 0.f && impulse >= thresImpulse && alive)
    alive = false;
  thresImpulse = min(originalThreshold, thresImpulse - absorbedImpulse);
  CachedCollisionObjectInfo::onImpulse(impulse, dir, pos, point_vel, collision_normal, flags, user_data);
  return absorbedImpulse;
}

float RendinstCollisionUserInfo::RendinstImpulseThresholdData::getDestructionImpulse() const { return thresImpulse; }

bool RendinstCollisionUserInfo::RendinstImpulseThresholdData::isRICollision() const { return true; }

RendinstCollisionUserInfo::TreeRendinstImpulseThresholdData::TreeRendinstImpulseThresholdData(float impulse,
  const rendinst::RendInstDesc &ri_desc, float at_time, const rendinst::CollisionInfo &coll_info) :
  CachedCollisionObjectInfo(impulse, ri_desc, at_time), lastPointVel(0.f), lastOmega(0.f), collInfo(coll_info)
{
  finalImpulse.zero();
  invRiTm = inverse(coll_info.tm);
}

RendinstCollisionUserInfo::TreeRendinstImpulseThresholdData::~TreeRendinstImpulseThresholdData()
{
  if (!alive)
  {
    const rendinstdestr::TreeDestr &treeDestr = rendinstdestr::get_tree_destr();
    rendinstdestr::create_tree_rend_inst_destr(riDesc, true, finalPos, finalImpulse, true, lastPointVel < treeDestr.minSpeed,
      lastOmega, atTime, &collInfo, true);
  }
}

float RendinstCollisionUserInfo::TreeRendinstImpulseThresholdData::onImpulse(float impulse, const Point3 &dir, const Point3 &pos,
  float point_vel, const Point3 &collision_normal, uint32_t flags, int32_t user_data, gamephys::ImpulseLogFunc)
{
  if (flags & CIF_NO_DAMAGE)
    return impulse;

  G_ASSERTF(!check_nan(impulse) && impulse < 1e8f, "onImpulse %f, originalThreshold %f", impulse, originalThreshold);
  Point3 localPos = invRiTm * pos;
  float absorbedImpulse = min(impulse * max(1.f, length(localPos)), thresImpulse);
  finalImpulse += -dir * absorbedImpulse;
  if (absorbedImpulse > 0.f && impulse >= thresImpulse && alive)
    alive = false;
  lastPointVel = point_vel;
  thresImpulse = min(originalThreshold, thresImpulse - absorbedImpulse);
  lastOmega = safediv(point_vel, localPos.y);
  finalPos = pos;
  CachedCollisionObjectInfo::onImpulse(impulse, dir, pos, point_vel, collision_normal, flags, user_data);
  return min(absorbedImpulse, impulse);
}

float RendinstCollisionUserInfo::TreeRendinstImpulseThresholdData::getDestructionImpulse() const { return thresImpulse; }

bool RendinstCollisionUserInfo::TreeRendinstImpulseThresholdData::isTreeCollision() const { return true; }

RendinstCollisionUserInfo::RendinstCollisionUserInfo(const rendinst::RendInstDesc &from_desc) :
  desc(from_desc),
  isTree(false),
  immortal(false),
  isDestr(false),
  objInfoData(NULL),
  collided(false),
  bushBehaviour(false),
  treeBehaviour(false)
{
  physObjectType = _MAKE4C('RIUD');
}
