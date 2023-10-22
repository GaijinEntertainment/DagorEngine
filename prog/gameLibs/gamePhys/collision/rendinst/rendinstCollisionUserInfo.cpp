#include <gamePhys/collision/rendinstCollisionUserInfo.h>

#include <gamePhys/phys/rendinstDestr.h>
#include <gamePhys/phys/treeDestr.h>
#include <rendInst/rendInstExtra.h>


RendinstCollisionUserInfo::RendinstImpulseThresholdData::RendinstImpulseThresholdData(float impulse,
  const rendinst::RendInstDesc &ri_desc, float at_time, const rendinst::CollisionInfo &coll_info) :
  CachedCollisionObjectInfo(impulse, ri_desc, at_time), collInfo(coll_info)
{}

RendinstCollisionUserInfo::RendinstImpulseThresholdData::~RendinstImpulseThresholdData()
{
  if (!alive)
    rendinstdestr::destroyRendinst(riDesc, true, Point3(0.f, 0.f, 0.f), Point3(0.f, 0.f, 0.f), atTime, &collInfo,
      rendinstdestr::get_ri_damage_effect_cb() != nullptr, rendinstdestr::get_ri_damage_effect_cb(),
      rendinstdestr::get_destr_settings().isClient);
}

float RendinstCollisionUserInfo::RendinstImpulseThresholdData::onImpulse(float impulse, const Point3 &dir, const Point3 &pos,
  float point_vel, int32_t user_data, gamephys::ImpulseLogFunc log_func)
{
  collInfo.userData = user_data;

  if (riDesc.isRiExtra() && collInfo.hp > 0.f)
  {
    if (riDesc.pool < 0)
      return 0;

    float absorbed_impulse = impulse;
    if (impulse > 0 && alive)
    {
      float damage;
      if (collInfo.destrImpulse > 0)
        damage = collInfo.initialHp * (impulse / collInfo.destrImpulse);
      else
        damage = impulse * rendinstdestr::get_destr_settings().destrImpulseHitPointsMult;
      absorbed_impulse = damage;
      if (log_func)
        log_func(rendinst::get_rendinst_res_name_from_col_info(collInfo), impulse, damage, collInfo.hp);
      if (rendinst::applyDamageRIGenExtra(riDesc, damage, &absorbed_impulse, true))
        alive = false;
      absorbed_impulse = safediv(absorbed_impulse, damage) * impulse;
    }
    return absorbed_impulse;
  }

  float diff = min(impulse, thresImpulse);
  if (diff > 0.f && impulse >= thresImpulse && alive)
    alive = false;
  thresImpulse = min(originalThreshold, thresImpulse - diff);
  CachedCollisionObjectInfo::onImpulse(impulse, dir, pos, point_vel, user_data);
  return diff;
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
    rendinstdestr::create_tree_rend_inst_destr(riDesc, true, finalImpulse, true, lastPointVel < treeDestr.minSpeed, lastOmega, atTime,
      &collInfo, true);
  }
}

float RendinstCollisionUserInfo::TreeRendinstImpulseThresholdData::onImpulse(float impulse, const Point3 &dir, const Point3 &pos,
  float point_vel, int32_t user_data, gamephys::ImpulseLogFunc)
{
  Point3 localPos = invRiTm * pos;
  float diff = min(impulse * max(1.f, length(localPos)), thresImpulse);
  finalImpulse += -dir * diff;
  if (diff > 0.f && impulse >= thresImpulse && alive)
    alive = false;
  lastPointVel = point_vel;
  thresImpulse -= diff;
  lastOmega = safediv(point_vel, localPos.y);
  CachedCollisionObjectInfo::onImpulse(impulse, dir, pos, point_vel, user_data);
  return min(diff, impulse);
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
