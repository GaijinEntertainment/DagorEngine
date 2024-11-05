// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameMath/quantization.h>
#include <gamePhys/common/loc.h>
#include <EASTL/tuple.h>
#include <EASTL/type_traits.h>
#include "phys/physConsts.h"
#include <daNet/bitStream.h>
#include <daECS/core/entityId.h>

#define QUANTIZE_PHYS_SNAPSHOTS 1

// Max quantization error ~= 1.95mm
struct QuantizedWorldPosXZScale
{
  constexpr operator float() const { return 4096; }
};
struct QuantizedWorldPosYScale
{
  constexpr operator float() const { return 512.f; }
};
typedef gamemath::QuantizedPos<22, 19, 22, QuantizedWorldPosXZScale, QuantizedWorldPosYScale, QuantizedWorldPosXZScale>
  QuantizedWorldPos;
#define HIDDEN_QWPOS_XYZ -(QuantizedWorldPosXZScale() - 1), -(QuantizedWorldPosYScale() - 1), -(QuantizedWorldPosXZScale() - 1)

template <typename QuantizedWorldQuatType>
struct QuantizedWorldLoc final : public QuantizedWorldPos, public QuantizedWorldQuatType
{
  static constexpr uint64_t POS_CLAMPED_BIT = 1ULL << 63;
  static constexpr typename QuantizedWorldQuatType::IntType inMotionBit =
    QuantizedWorldQuatType::FreeQuatBits
      ? typename QuantizedWorldQuatType::IntType(1) << (POW2_ALIGN(QuantizedWorldQuatType::TotalQuatBits, CHAR_BIT) - 1)
      : 0;
  QuantizedWorldLoc() = default;
  QuantizedWorldLoc(const Point3 &p, const Quat &q, bool in_motion) : QuantizedWorldQuatType(q)
  {
    bool posClamped = false;
    QuantizedWorldPos::packPos(p, &posClamped);
    G_FAST_ASSERT(!(qpos & POS_CLAMPED_BIT));
    if (posClamped)
      qpos |= POS_CLAMPED_BIT;
    if (in_motion)
      QuantizedWorldQuatType::qquat |= inMotionBit;
  }
  QuantizedWorldLoc(const gamephys::Loc &loc, bool in_motion) : QuantizedWorldLoc(Point3::xyz(loc.P), loc.O.getQuat(), in_motion) {}
  bool inMotion() const { return (inMotionBit == 0) || (QuantizedWorldQuatType::qquat & inMotionBit) != 0; }
  eastl::tuple<Point3, Quat, bool> unpackTuple() const { return eastl::make_tuple(unpackPos(), this->unpackQuat(), inMotion()); }
  TMatrix unpackTM() const
  {
    TMatrix tm = makeTM(this->unpackQuat());
    tm.setcol(3, unpackPos());
    return tm;
  }
};
typedef QuantizedWorldLoc<gamemath::QuantizedQuat23> QuantizedWorldLocSmall;
typedef QuantizedWorldLoc<gamemath::QuantizedQuat32> QuantizedWorldLocDefault;

template <typename QL = QuantizedWorldLocDefault, typename QV = gamemath::UnitVecPacked32, bool bFastSpeed = true>
struct CachedQuantizedInfoT
{
  int atTick = -1;
  QL quantLoc;                          // cached quantized currentState.location (+ optional 'inMotion' bit)
  QuantizedWorldLocSmall quantLocSmall; // same but less bits for orientation
  QV velDirPacked;
  union
  {
    int16_t quantSpeed;
    uint16_t quantSpeedU;
  };

  CachedQuantizedInfoT() : quantSpeed(0) {}
  void invalidate() { atTick = -1; }
  bool isInvalid() const { return atTick < 0; }

  template <typename Q>
  static bool serializeQLoc(danet::BitStream &bs, const Q &qloc, const DPoint3 &rpos)
  {
    if (!(qloc.qpos & Q::POS_CLAMPED_BIT))
      bs.Write(qloc.qpos);
    else
    {
      // POS_CLAMPED_BIT corresponds to sign bit (MSB) of second float (y). Little endianess assumed.
      bs.Write(Point3(rpos.x, -fabsf((float)rpos.y), rpos.z));
      bs.Write(rpos.y < 0);
      bs.AlignWriteToByteBoundary(); // Note: 7 bits are wasted
    }
    bs.WriteBits((const uint8_t *)&qloc.qquat, POW2_ALIGN(qloc.TotalQuatBits, CHAR_BIT));
    return qloc.inMotion();
  }

  template <typename Q = QL>
  static bool deserializeQLoc(
    const danet::BitStream &bs, Point3 &out_pos, Quat &out_quat, bool &out_in_motion, bool *out_ext2_bit = nullptr)
  {
    G_UNUSED(out_ext2_bit);
    union
    {
      int64_t qpos;
      Point3 rpos;
    } upos;
    if (!bs.Read(upos.qpos))
      return false;
    bool rawpos = (upos.qpos & Q::POS_CLAMPED_BIT) != 0;
    if (rawpos)
    {
      G_ASSERTF(upos.rpos.y < FLT_MIN, "%f", upos.rpos.y);
      bool ySign = false;
      if (!bs.Read(upos.rpos.z) || !bs.Read(ySign))
        return false;
      bs.AlignReadToByteBoundary();
      if (!ySign)
        upos.rpos.y = -upos.rpos.y;
    }
    Q qloc;
    if (!bs.ReadBits((uint8_t *)&qloc.qquat, POW2_ALIGN(Q::TotalQuatBits, CHAR_BIT)))
      return false;
    if constexpr (Q::FreeQuatBits >= 2)
      if (out_ext2_bit)
        *out_ext2_bit = (qloc.qquat & (typename Q::IntType(1) << (POW2_ALIGN(Q::TotalQuatBits, CHAR_BIT) - 2))) != 0;
    qloc.qpos = upos.qpos;
    if (!rawpos)
      eastl::tie(out_pos, out_quat, out_in_motion) = qloc.unpackTuple();
    else
    {
      out_pos = upos.rpos;
      out_quat = qloc.unpackQuat();
      out_in_motion = qloc.inMotion();
    }
    return true;
  }

  template <typename T>
  void update(const T &phys, ecs::EntityId, const void * = nullptr)
  {
    float speed = phys.currentState.velocity.length();
    bool inMotion;
    if (bFastSpeed)
    {
      float speedUnit = speed / PHYS_MAX_QUANTIZED_FAST_SPEED;
      quantSpeedU = eastl::min((int)(speedUnit * (USHRT_MAX + 1) + 0.5f), USHRT_MAX);
      inMotion = quantSpeedU != 0;
    }
    else
    {
      float speedDiff = speed - PHYS_MAX_QUANTIZED_SLOW_SPEED;
      if (speedDiff <= 0.f)
      {
        quantSpeed = eastl::min(int(speed / PHYS_MAX_QUANTIZED_SLOW_SPEED * float(SCHAR_MAX + 1) + 0.5f), SCHAR_MAX);
        inMotion = quantSpeed != 0;
      }
      else
      {
        constexpr float speedScale = PHYS_MAX_QUANTIZED_FAST_SPEED - PHYS_MAX_QUANTIZED_SLOW_SPEED;
        quantSpeed = -eastl::min(int(speedDiff / speedScale * float(SHRT_MAX + 1) + 0.5f), SHRT_MAX);
        inMotion = true;
      }
      G_ASSERTF(quantSpeed <= SCHAR_MAX, "%d %.16f", quantSpeed, speed);
    }
    quantLoc = QL(phys.currentState.location, inMotion);
    if constexpr (!eastl::is_same<decltype(quantLoc), decltype(quantLocSmall)>::value)
      quantLocSmall = QuantizedWorldLocSmall(phys.currentState.location, inMotion);
    else
    {
      quantLocSmall.qpos = quantLoc.qpos;
      quantLocSmall.qquat = quantLoc.qquat;
    }
    if (quantLoc.inMotion() || quantLocSmall.inMotion())
      velDirPacked.pack(phys.currentState.velocity);
    atTick = phys.currentState.atTick;
  }

  void serializeVelocity(danet::BitStream &bs) const
  {
    G_ASSERT(quantLoc.inMotion() || quantLocSmall.inMotion());
    if (bFastSpeed)
      bs.Write(quantSpeedU);
    else
    {
      if (quantSpeed < 0)
        bs.Write(uint8_t(quantSpeed >> 8)); // hi
      bs.Write(uint8_t(quantSpeed));        // lo
    }
    if (quantSpeed)
      bs.WriteBits((const uint8_t *)&velDirPacked.val, velDirPacked.TotalBits);
  }

  static bool deserializeVelocity(const danet::BitStream &bs, bool in_motion, Point3 &out_vel)
  {
    auto zeroVel = [&out_vel]() {
      out_vel.zero();
      return true;
    };
    if (!in_motion)
      return zeroVel();
    QV velDirPacked;
    float speed;
    if (bFastSpeed)
    {
      uint16_t speedPacked = 0;
      if (!bs.Read(speedPacked))
        return false;
      if (!speedPacked)
        return zeroVel();
      else if (!bs.ReadBits((uint8_t *)&velDirPacked, velDirPacked.TotalBits))
        return false;
      speed = speedPacked / float(USHRT_MAX) * PHYS_MAX_QUANTIZED_FAST_SPEED;
    }
    else
    {
      union
      {
        int16_t speedPacked;
        struct
        {
          int8_t lo, hi;
        };
      } spu;
      if (!bs.Read(spu.lo))
        return false;
      if (spu.lo >= 0)
      {
        spu.hi = 0;
        G_ASSERT(spu.speedPacked <= SCHAR_MAX);
      }
      else
      {
        spu.hi = spu.lo;
        if (!bs.Read(spu.lo))
          return false;
        G_ASSERT(spu.speedPacked < 0);
      }
      if (!spu.speedPacked)
        return zeroVel();
      else if (!bs.ReadBits((uint8_t *)&velDirPacked, velDirPacked.TotalBits))
        return false;
      if (spu.speedPacked >= 0)
        speed = spu.speedPacked / float(SCHAR_MAX) * PHYS_MAX_QUANTIZED_SLOW_SPEED;
      else
      {
        constexpr float speedScale = PHYS_MAX_QUANTIZED_FAST_SPEED - PHYS_MAX_QUANTIZED_SLOW_SPEED;
        speed = (-spu.speedPacked / float(SHRT_MAX) * speedScale) + PHYS_MAX_QUANTIZED_SLOW_SPEED;
      }
    }
    out_vel = velDirPacked.unpack() * speed;
    return true;
  }
};
