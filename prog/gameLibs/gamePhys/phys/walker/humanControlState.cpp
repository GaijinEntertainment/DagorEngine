// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/phys/walker/humanControlState.h>
#include <daNet/bitStream.h>
#include <daNet/daNetTypes.h>
#include <gameMath/quantization.h>
#include <EASTL/tuple.h>

static inline eastl::pair<Point3, gamemath::UnitVecPacked32> set_ctrl_dir(const Point3 &dir)
{
  G_ASSERT(!check_nan(dir));
  gamemath::UnitVecPacked32 dirp(normalizeDef(dir, Point3(1, 0, 0)));
  return eastl::make_pair(dirp.unpack(), dirp);
}
void HumanControlState::setWishShootDir(const Point3 &dir) { eastl::tie(wishShootDir, wishShootDirPacked) = set_ctrl_dir(dir); }
void HumanControlState::setWishLookDir(const Point3 &dir) { eastl::tie(wishLookDir, wishLookDirPacked) = set_ctrl_dir(dir); }
void HumanControlState::setWishShootLookDir(const Point3 &dir)
{
  setWishShootDir(dir);
  wishLookDir = wishShootDir;
  wishLookDirPacked = wishShootDirPacked;
}
void HumanControlState::setWorldWalkDir(const Point2 &wd, const Point2 &look_dir)
{
  Point2 localDir = Point2(wd * look_dir, wd * Point2(-look_dir.y, look_dir.x));
  setWalkDir(localDir);
}
void HumanControlState::setWalkDir(const Point2 &wd)
{
  G_ASSERT(!check_nan(wd));
  float lenSq = wd.lengthSq();
  if (float_nonzero(lenSq))
  {
    float len = sqrtf(lenSq);
    gamemath::YawPacked<uint32_t, WALK_DIR_BITS> walkYaw(wd / len);
    setWalkDirBits(walkYaw.val);
    walkDir = walkYaw.unpack();
    setWalkSpeed(len);
  }
  else
  {
    walkPacked = 0;
    gamemath::YawPacked<uint32_t, WALK_DIR_BITS> walkYaw(0);
    walkDir = walkYaw.unpack();
  }
}

#define HUMAN_CONTROLS_DBG_BORDERS 0

template <typename T>
static inline void write_bit_or_data(danet::BitStream &bs, bool cmp, const T &data, size_t bits)
{
  if (cmp)
    bs.Write(false);
  else
  {
    bs.Write(true);
    bs.WriteBits((const uint8_t *)&data, bits);
  }
  if (HUMAN_CONTROLS_DBG_BORDERS)
    bs.WriteBits((const uint8_t *)&data, bits);
}
template <typename T>
static inline void write_bit_or_data(danet::BitStream &bs, bool cmp, const T &data)
{
  constexpr size_t bits = BYTES_TO_BITS(sizeof(T));
  write_bit_or_data(bs, cmp, data, bits);
}
template <typename T>
static inline bool read_bit_or_data(const danet::BitStream &bs, T &data, const T &def, size_t bits)
{
  bool bit = false;
  bool isOk = bs.Read(bit);
  if (bit)
    isOk &= bs.ReadBits((uint8_t *)&data, bits);
  else
    memcpy(&data, &def, sizeof(data));

  if (HUMAN_CONTROLS_DBG_BORDERS)
  {
    T verData;
    memset(&verData, 0, sizeof(T));
    isOk &= bs.ReadBits((uint8_t *)&verData, bits);
    G_ASSERT(!isOk || memcmp(&data, &verData, sizeof(T)) == 0);
  }
  return isOk;
}
template <typename T>
static inline bool read_bit_or_data(const danet::BitStream &bs, T &data, const T &def)
{
  constexpr size_t bits = BYTES_TO_BITS(sizeof(T));
  return read_bit_or_data(bs, data, def, bits);
}

void HumanControlState::serializeDelta(danet::BitStream &bs, const HumanControlState &base) const
{
  if (HUMAN_CONTROLS_DBG_BORDERS)
  {
    bs.Write((uint8_t)17);
    bs.Write(base.producedAtTick);
  }

  write_bit_or_data(bs, uint16_t(producedAtTick) == uint16_t(base.producedAtTick + 1), uint16_t(producedAtTick));
  write_bit_or_data(bs, packedState == base.packedState, packedState);
  if (packedState & PS_HAS_EXT_STATE)
    write_bit_or_data(bs, extendedState == base.extendedState, extendedState, EXT_BITS);
  write_bit_or_data(bs, walkPacked == base.walkPacked, walkPacked);
  write_bit_or_data(bs, wishShootDirPacked == base.wishShootDirPacked, wishShootDirPacked);
  write_bit_or_data(bs, wishLookDirPacked == wishShootDirPacked, wishLookDirPacked); // almost always eq to shoot
  if (isControlBitSet(HCT_SHOOT))
    write_bit_or_data(bs, shootPos == base.shootPos, shootPos);

#if HUMAN_USE_UNIT_VERSION
  write_bit_or_data(bs, unitVersion == base.unitVersion, unitVersion);
#endif

  if (HUMAN_CONTROLS_DBG_BORDERS)
    bs.Write((uint8_t)13);
}
bool HumanControlState::deserializeDelta(const danet::BitStream &bs, const HumanControlState &base, int cur_tick)
{
  bool isOk = true;

  if (HUMAN_CONTROLS_DBG_BORDERS)
  {
    uint8_t pr = 0;
    isOk &= bs.Read(pr);
    G_ASSERT(pr == 17);

    int baseTick = -1;
    isOk &= bs.Read(baseTick);
    G_ASSERT(baseTick == base.producedAtTick);
  }

  uint16_t producedAtTick16 = 0;
  isOk &= read_bit_or_data(bs, producedAtTick16, uint16_t(base.producedAtTick + 1));
  isOk &= read_bit_or_data(bs, packedState, base.packedState);
  if (packedState & PS_HAS_EXT_STATE)
    isOk &= read_bit_or_data(bs, extendedState, base.extendedState, EXT_BITS);
  isOk &= read_bit_or_data(bs, walkPacked, base.walkPacked);
  isOk &= read_bit_or_data(bs, wishShootDirPacked, base.wishShootDirPacked);
  isOk &= read_bit_or_data(bs, wishLookDirPacked, wishShootDirPacked); // yes, by default use 'shoot' for look
  if (isControlBitSet(HCT_SHOOT))
    isOk &= read_bit_or_data(bs, shootPos, base.shootPos);
  else
    shootPos.reset();

#if HUMAN_USE_UNIT_VERSION
  isOk &= read_bit_or_data(bs, unitVersion, base.unitVersion);
#endif

  if (HUMAN_CONTROLS_DBG_BORDERS)
  {
    uint8_t foo = 0;
    isOk &= bs.Read(foo);
    G_ASSERT(foo == 13);
  }

  producedAtTick = (cur_tick & 0xFFFF0000) | producedAtTick16;
  if ((producedAtTick & 0x8000) > (cur_tick & 0x8000))
    producedAtTick -= USHRT_MAX + 1;

  wishShootDir = wishShootDirPacked.unpack();
  wishLookDir = wishLookDirPacked.unpack();
  walkDir = gamemath::YawPacked<uint32_t, WALK_DIR_BITS>(getWalkDirBits()).unpack();
  isOk &= (int)getChosenWeapon() < EWS_NUM;

  return isOk;
}
