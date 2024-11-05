// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/phys/weaponState.h>
#include <daNet/bitStream.h>
#include <daNet/dag_netUtils.h>

WeaponState::WeaponState() :
  overheatCurrentTimer(0.0f),
  nextShotAtTime(0.0f),
  lastShotAtTime(0.0f),
  shootingCharge(0.0f),
  jettisoned(false),
  scheduledReloadTick(0),
  scheduledShotCount(0),
  jettisoning(false),
  flags(0u),
  chosenBulletType(0u),
  misfireCount(0u),
  shotStep(0.0f)
{}

void WeaponState::serialize(danet::BitStream &bs) const
{
  bool bulletsCountNotEqualsToOne = bulletSets.size() != 1;
  bs.Write(bulletsCountNotEqualsToOne);
  if (bulletsCountNotEqualsToOne)
    bs.Write((uint8_t)bulletSets.size());
  for (const auto &bulletSet : bulletSets)
  {
    bs.Write(bulletSet.bulletsLeft);
    bs.Write(bulletSet.bulletsBorn);
  }
  bs.Write(jettisoned);
  bs.Write(flags);
  if (flags & WS_IS_BASE_GUN)
    return;
  if (!(flags & WS_IS_REUPDATEABLE_GUN))
    return;
  bs.Write(overheatCurrentTimer);
  bs.Write(nextShotAtTime);
  bs.Write(lastShotAtTime);
  bs.Write(PACK_U8(shootingCharge, 1.f));
  bs.Write(scheduledShotCount);
  bs.Write(jettisoning);
  bs.Write(chosenBulletType);
  bs.Write(!scheduledBulletSets.empty());
  if (!scheduledBulletSets.empty())
  {
    bs.Write((uint8_t)scheduledBulletSets.size());
    for (const auto &bulletSet : scheduledBulletSets)
      bs.Write(bulletSet);
  }
  bs.Write(scheduledReloadTick);
  bs.Write(shotStep);
  bs.Write(misfireCount);
}

bool WeaponState::deserialize(const danet::BitStream &bs)
{
  bool bulletsCountNotEqualsToOne = false;
  bool isOk = bs.Read(bulletsCountNotEqualsToOne);
  uint8_t bulletsSetsCount = 1u;
  if (bulletsCountNotEqualsToOne)
    isOk = isOk && bs.Read(bulletsSetsCount);
  if (bulletsSetsCount <= bulletSets.capacity())
  {
    bulletSets.resize(bulletsSetsCount);
    for (auto &bulletSet : bulletSets)
    {
      isOk = isOk && bs.Read(bulletSet.bulletsLeft);
      isOk = isOk && bs.Read(bulletSet.bulletsBorn);
    }
  }
  isOk = isOk && bs.Read(jettisoned);
  isOk = isOk && bs.Read(flags);
  if (flags & WS_IS_BASE_GUN)
    return isOk;
  if (!(flags & WS_IS_REUPDATEABLE_GUN))
    return isOk;
  isOk = isOk && bs.Read(overheatCurrentTimer);
  isOk = isOk && bs.Read(nextShotAtTime);
  isOk = isOk && bs.Read(lastShotAtTime);

  uint8_t ui8 = 0;
  isOk = isOk && bs.Read(ui8);
  shootingCharge = netutils::UNPACK(ui8, 1.f);

  isOk = isOk && bs.Read(scheduledShotCount);
  isOk = isOk && bs.Read(jettisoning);
  isOk = isOk && bs.Read(chosenBulletType);
  bool hasScheduledBulletSets = false;
  isOk = isOk && bs.Read(hasScheduledBulletSets);
  if (hasScheduledBulletSets)
  {
    uint8_t scheduledBulletSetsCount = 0u;
    isOk = isOk && bs.Read(scheduledBulletSetsCount);
    if (scheduledBulletSetsCount <= scheduledBulletSets.capacity())
    {
      scheduledBulletSets.resize(scheduledBulletSetsCount);
      for (auto &bulletSet : scheduledBulletSets)
        isOk = isOk && bs.Read(bulletSet);
    }
  }
  isOk = isOk && bs.Read(scheduledReloadTick);
  isOk = isOk && bs.Read(shotStep);
  isOk = isOk && bs.Read(misfireCount);

  return isOk;
}

bool WeaponState::operator==(const WeaponState &a) const
{
  if (bulletSets.size() != a.bulletSets.size())
    return false;
  for (uint32_t i = 0; i < bulletSets.size(); ++i)
    if (bulletSets[i].bulletsLeft != a.bulletSets[i].bulletsLeft || bulletSets[i].bulletsBorn != a.bulletSets[i].bulletsBorn)
      return false;
  if (jettisoned != a.jettisoned || flags != a.flags)
    return false;

  if (flags & WS_IS_BASE_GUN)
    return true;

  if (!(flags & WS_IS_REUPDATEABLE_GUN))
    return true;

  static constexpr float timeTreshold = 1.0e-2f;
  if (chosenBulletType == a.chosenBulletType && rabs(overheatCurrentTimer - a.overheatCurrentTimer) < timeTreshold &&
      rabs(nextShotAtTime - a.nextShotAtTime) < timeTreshold && rabs(lastShotAtTime - a.lastShotAtTime) < timeTreshold &&
      scheduledShotCount == a.scheduledShotCount && jettisoning == a.jettisoning && scheduledReloadTick == a.scheduledReloadTick &&
      shotStep == a.shotStep && misfireCount == a.misfireCount)
    ;
  else
    return false;

  if (scheduledBulletSets.size() != a.scheduledBulletSets.size())
    return false;
  for (uint32_t i = 0; i < scheduledBulletSets.size(); ++i)
    if (scheduledBulletSets[i] != a.scheduledBulletSets[i])
      return false;

  return true;
}