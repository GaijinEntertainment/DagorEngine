#include <gamePhys/phys/weaponState.h>
#include <daNet/bitStream.h>
#include <daNet/dag_netUtils.h>

void WeaponState::serialize(danet::BitStream &bs) const
{
  bs.Write(bulletsLeft);
  bs.Write(jettisoned);
  bs.Write(flags);
  if (flags & WS_IS_BASE_GUN)
    return;
  bs.Write(bulletsBorn);
  if (!(flags & WS_IS_REUPDATEABLE_GUN))
    return;
  bs.Write(overheatCurrentTimer);
  bs.Write(nextShotAtTime);
  bs.Write(lastShotAtTime);
  bs.Write(PACK_U8(shootingCharge, 1.f));
  bs.Write(scheduledShotCount);
  bs.Write(jettisoning);
  bs.Write(chosenBulletType);
  bs.Write(scheduledReloadTick);
  bs.Write(shotStep);
  bs.Write(misfireCount);
}

bool WeaponState::deserialize(const danet::BitStream &bs)
{
  bool isOk = bs.Read(bulletsLeft);
  isOk = isOk && bs.Read(jettisoned);
  isOk = isOk && bs.Read(flags);
  if (flags & WS_IS_BASE_GUN)
    return isOk;
  isOk = isOk && bs.Read(bulletsBorn);
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
  isOk = isOk && bs.Read(scheduledReloadTick);
  isOk = isOk && bs.Read(shotStep);
  isOk = isOk && bs.Read(misfireCount);
  return isOk;
}
