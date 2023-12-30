//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <limits.h>
#include <stdint.h>
#include <math/dag_mathBase.h>
#include <math/dag_mathAng.h>
#include <math/dag_Point3.h>
#include <math/dag_Point2.h>
#include <math/dag_Quat.h>
#include <debug/dag_assert.h>
#include <EASTL/type_traits.h>
#include <EASTL/algorithm.h>
#include <debug/dag_log.h>
#include <debug/dag_debug.h>

#if !defined(DEBUG_QUANTIZATION_CLAMPING) && DAGOR_DBGLEVEL > 0 && defined(_DEBUG_TAB_) && _TARGET_PC_WIN
#define DEBUG_QUANTIZATION_CLAMPING 1
#endif

namespace gamemath
{
// This is necessary hacks because C++ doesn't allow float arguments in templates
struct PiScale
{
  constexpr operator float() const { return PI; }
};
struct HalfPiScale
{
  constexpr operator float() const { return HALFPI; }
};
struct NoScale
{
  constexpr operator float() const { return 1.f; }
};
struct InvertedSqrt2Scale
{
  constexpr operator float() const { return M_SQRT1_2; }
};

#define FVAL_BITS_MASK(PackType, Bits)        (PackType((PackType(1) << (Bits)) - 1))
#define FVAL_BITS_MASK_SIGNED(PackType, Bits) ((PackType(1) << ((Bits)-1)) - 1)
#define FVAL_SIGN_BIT(PackType, Bits)         (PackType(1) << ((Bits)-1))

template <typename RT, typename F = float>
inline RT pack_scalar_signed(F val, const size_t bits, F scale = 1.0f, bool *out_clamped = nullptr)
{
  bool sign = val < 0;
  F unitVal = (sign ? -val : val) / scale;
  if (out_clamped)
    *out_clamped |= unitVal > 1 + FLT_EPSILON;
#if DEBUG_QUANTIZATION_CLAMPING
  else if (unitVal > 1 + FLT_EPSILON)
    logerr("%s: value %f is out of range [%f..%f]", __FUNCTION__, val, -scale, scale);
#endif
  const RT bitsMaskSigned = FVAL_BITS_MASK_SIGNED(RT, bits);
  const RT signBit = FVAL_SIGN_BIT(RT, bits);
  RT qval = eastl::min(RT(unitVal * F(bitsMaskSigned + 1) + F(0.5)), bitsMaskSigned);
  return (qval && sign) ? (qval ^ signBit) : qval;
}

template <typename RT, typename F = float>
inline F unpack_scalar_signed(RT val, const size_t bits, F scale = 1.0f)
{
  const RT bitsMaskSigned = FVAL_BITS_MASK_SIGNED(RT, bits);
  const RT signBit = FVAL_SIGN_BIT(RT, bits);

  F fval = F(val & bitsMaskSigned) / F(bitsMaskSigned) * scale;
  return (val & signBit) ? -fval : fval;
}

template <typename RT, typename F = float>
inline RT pack_scalar_unsigned(F val, const size_t bits, F scale = 1.0f, bool *out_clamped = nullptr)
{
  F unitVal = val / scale;
  if (out_clamped)
    *out_clamped |= unitVal > 1 + FLT_EPSILON;
#if DEBUG_QUANTIZATION_CLAMPING
  else if (unitVal > 1 + FLT_EPSILON)
    logerr("%s: value %f is out of range [0..%f]", __FUNCTION__, val, scale);
#endif

  const RT bitsMask = FVAL_BITS_MASK(RT, bits);
  return eastl::min(RT(unitVal * F(bitsMask + 1) + F(0.5)), RT(bitsMask));
}

template <typename RT, typename F = float>
inline F unpack_scalar_unsigned(RT val, const size_t bits, F scale = 1.0f)
{
  const RT bitsMask = FVAL_BITS_MASK(RT, bits);
  return F(val & bitsMask) / F(bitsMask) * scale;
}

template <size_t Bits, typename ScaleWrapper, typename F = float>
struct FValQuantizer
{
  using RT = typename eastl::type_select<(Bits >= 32), uint64_t, uint32_t>::type;

  static inline RT packSigned(F val, bool *out_clamped = nullptr)
  {
    return pack_scalar_signed<RT, F>(val, Bits, ScaleWrapper(), out_clamped);
  }
  static inline F unpackSigned(RT val) { return unpack_scalar_signed<RT, F>(val, Bits, ScaleWrapper()); }

  static inline RT packUnsigned(F val, bool *out_clamped = nullptr)
  {
    return pack_scalar_unsigned<RT, F>(val, Bits, ScaleWrapper(), out_clamped);
  }
  static inline F unpackUnsigned(RT val) { return unpack_scalar_unsigned<RT, F>(val, Bits, ScaleWrapper()); }
};


// From "A Survey of Efficient Representationsfor Independent Unit Vectors"
// (http://jcgt.org/published/0003/02/01/paper.pdf)
// "Map the sphere to an octahedron, project down into the z=0 plane,
// and then reflect the -z-hemisphere over the appropriate diagonal"
inline Point2 p3_to_oct(Point3 v)
{
  Point2 p = Point2::xy(v) * safeinv(fabsf(v.x) + fabsf(v.y) + fabsf(v.z));
  if (v.z <= 0)
    p = Point2((1.f - fabsf(p.y)) * fsel(p.x, +1.f, -1.f), (1.f - fabsf(p.x)) * fsel(p.y, +1.f, -1.f));
  return p;
}

inline Point3 oct_to_p3(Point2 oct)
{
  Point3 p;
  p.x = oct.x;
  p.y = oct.y;
  p.z = 1.f - fabsf(p.x) - fabsf(p.y);
  if (p.z < 0)
    p = Point3((1.f - fabsf(p.y)) * fsel(p.x, +1.f, -1.f), (1.f - fabsf(p.x)) * fsel(p.y, +1.f, -1.f), p.z);
  return normalize(p);
}

template <typename T>
__forceinline T pack_unit_vec(Point3 v, const size_t bits)
{
  const size_t halfBits = bits / 2;

  Point2 oct = p3_to_oct(v);

  if (halfBits >= 32) // Due to forceinline we will have compile-time branches here for the compile-time bits
    return T((pack_scalar_signed<uint64_t>(oct.x, halfBits) << halfBits) | pack_scalar_signed<uint64_t>(oct.y, halfBits));
  else
    return T((pack_scalar_signed<uint32_t>(oct.x, halfBits) << halfBits) | pack_scalar_signed<uint32_t>(oct.y, halfBits));
}

template <typename T>
__forceinline Point3 unpack_unit_vec(T val, const size_t bits)
{
  const size_t halfBits = bits / 2;

  Point2 oct;
  if (halfBits >= 32) // Due to forceinline we will have compile-time branches here for the compile-time bits
  {
    oct.x = unpack_scalar_signed<uint64_t>(val >> halfBits, halfBits);
    oct.y = unpack_scalar_signed<uint64_t>(val, halfBits);
  }
  else
  {
    oct.x = unpack_scalar_signed<uint32_t>(val >> halfBits, halfBits);
    oct.y = unpack_scalar_signed<uint32_t>(val, halfBits);
  }

  return oct_to_p3(oct);
}

template <typename T, size_t Bits>
struct UnitVecPacked
{
  G_STATIC_ASSERT(sizeof(T) * CHAR_BIT >= Bits);
  G_STATIC_ASSERT(Bits % 2 == 0);
  static constexpr size_t TotalBits = Bits;
  static constexpr size_t HalfBits = Bits / 2;
  T val = 0;
  UnitVecPacked() = default;
  UnitVecPacked(T v) : val(v) {}
  UnitVecPacked(const Point3 &v) { pack(v); }
  void pack(const Point3 &v) { val = pack_unit_vec<T>(v, Bits); }
  Point3 unpack() const { return unpack_unit_vec<T>(val, Bits); }
  bool operator==(const UnitVecPacked &rhs) const { return val == rhs.val; }
  bool operator!=(const UnitVecPacked &rhs) const { return val != rhs.val; }
};
typedef UnitVecPacked<uint32_t, 32> UnitVecPacked32;
typedef UnitVecPacked<uint32_t, 24> UnitVecPacked24;
typedef UnitVecPacked<uint16_t, 16> UnitVecPacked16;

template <typename T, size_t Bits>
struct YawPacked
{
  static constexpr int YawBits = Bits;
  T val;
  YawPacked(T v) : val(v) {}
  YawPacked(const Point2 &dir2d) { pack(dir2d); }
  bool operator==(const YawPacked &rhs) const { return val == rhs.val; }
  void pack(const Point2 &dir2d) { val = FValQuantizer<Bits, PiScale>::packSigned(atan2f(dir2d.y, dir2d.x)); }
  Point2 unpack() const
  {
    float azf = FValQuantizer<Bits, PiScale>::unpackSigned(val);
    float xSine, xCos;
    sincos(azf, xSine, xCos);
    return Point2(xCos, xSine);
  }
};
typedef YawPacked<uint16_t, 16> YawPacked16;

template <typename T, size_t Bits>
struct PitchPacked
{
  static constexpr int PitchBits = Bits;
  T val;
  PitchPacked(T v) : val(v) {}
  PitchPacked(float y) { pack(y); }
  bool operator==(const PitchPacked &rhs) const { return val == rhs.val; }
  void pack(float y) { val = FValQuantizer<Bits, HalfPiScale>::packSigned(safe_asin(y)); }
  float unpack() const
  {
    float fval = FValQuantizer<Bits, HalfPiScale>::unpackSigned(val);
    return sinf(fval);
  }
};
typedef PitchPacked<uint16_t, 16> PitchPacked16;

template <size_t XBits, size_t YBits, size_t ZBits, class XScale, class YScale, class ZScale>
struct QuantizedPos
{
  typedef typename eastl::type_select<((XBits + YBits + ZBits) > 32), uint64_t, uint32_t>::type PosType;
  PosType qpos = 0;
  G_STATIC_ASSERT((XBits + YBits + ZBits) <= sizeof(PosType) * CHAR_BIT);

  QuantizedPos() = default;
  QuantizedPos(const QuantizedPos &) = default;
  QuantizedPos(PosType pq) : qpos(pq) {}
  QuantizedPos(const Point3 &p, bool *out_clamped = nullptr) { packPos(p, out_clamped); }
  void packPos(const Point3 &p, bool *out_clamped = nullptr)
  {
    PosType posPacked = FValQuantizer<XBits, XScale>::packSigned(p.x, out_clamped);
    posPacked |= PosType(FValQuantizer<ZBits, ZScale>::packSigned(p.z, out_clamped)) << XBits;
    posPacked |= PosType(FValQuantizer<YBits, YScale>::packSigned(p.y, out_clamped)) << (XBits + ZBits);
    qpos = posPacked;
  }

  Point3 unpackPos() const
  {
    Point3 posP3;
    posP3.x = FValQuantizer<XBits, XScale>::unpackSigned(qpos & PosType((1ull << XBits) - 1));
    posP3.z = FValQuantizer<ZBits, ZScale>::unpackSigned((qpos >> XBits) & PosType((1ull << ZBits) - 1));
    posP3.y = FValQuantizer<YBits, YScale>::unpackSigned((qpos >> (XBits + ZBits)) & PosType((1ull << YBits) - 1));
    return posP3;
  }

  void reset() { qpos = 0; }
  QuantizedPos &operator=(const QuantizedPos &qp) = default;
  bool operator==(const QuantizedPos &qp) const { return qpos == qp.qpos; }
  bool operator!=(const QuantizedPos &qp) const { return qpos != qp.qpos; }
};

// Encoded by "smallest three" technique that described in https://gafferongames.com/post/snapshot_compression/
// This gives better precision then encoding of euler angles
template <size_t Bits> // Bits per component (totally used 3xBits+2, e.g. 32 for 10 Bits)
struct QuantizedQuat
{
  static constexpr int TotalQuatBits = 3 * Bits + 2;
  typedef typename eastl::type_select<(TotalQuatBits > 32), uint64_t, uint32_t>::type IntType;
  static constexpr int FreeQuatBits = sizeof(IntType) * CHAR_BIT - TotalQuatBits;

  IntType qquat = 0;

  QuantizedQuat() = default;
  QuantizedQuat(const QuantizedQuat &) = default;
  QuantizedQuat(IntType q) : qquat(q) {}
  QuantizedQuat(const Quat &q)
  {
    IntType valPacked = 0;
    unsigned mi = 0;
    float mx = fabsf(q[0]);
    for (unsigned i = 1; i < 4; ++i) // search for max component
    {
      float fqi = fabsf(q[i]);
      if (fqi > mx)
      {
        mx = fqi;
        mi = i;
      }
    }
    bool sign = q[mi] < 0.f;
    for (unsigned i = 0, j = 0; i < 4; ++i)
      if (i != mi)
      {
        valPacked |= (IntType)FValQuantizer<Bits, InvertedSqrt2Scale>::packSigned(sign ? -q[i] : q[i]) << (j * Bits);
        ++j;
      }
    valPacked |= IntType((mi + 1) & 3) << (3 * Bits); // increment max index to make '0' encode identity quat (0,0,0,1)
    qquat = valPacked;
  }
  Quat unpackQuat() const
  {
    unsigned mi = (qquat >> (Bits * 3)) & 3;
    mi = (mi - 1) & 3;
    Quat q;
    float qv = 1.f;
    for (unsigned i = 0, j = 0; i < 4; ++i)
      if (i != mi)
      {
        float v = FValQuantizer<Bits, InvertedSqrt2Scale>::unpackSigned((qquat >> (j * Bits)) & IntType((1LL << Bits) - 1));
        q[i] = v;
        qv -= sqr(v);
        ++j;
      }
    // Note: negative qv mostly likely caused by serialization of non-normalized quaternion or ill-formed data
    q[mi] = (qv <= FLT_MIN) ? 0.f : sqrtf(qv);
    return normalize(q);
  }

  void reset() { qquat = 0; }
  bool operator==(const QuantizedQuat &qq) const { return qquat == qq.qquat; }
  bool operator!=(const QuantizedQuat &qq) const { return qquat != qq.qquat; }
  QuantizedQuat &operator=(const QuantizedQuat &qq) = default;
};

typedef QuantizedQuat<7> QuantizedQuat23;  // 1 free bit
typedef QuantizedQuat<10> QuantizedQuat32; // 0 free bits
typedef QuantizedQuat<12> QuantizedQuat38; // 2 free bits
typedef QuantizedQuat<15> QuantizedQuat47; // 1 free bit
typedef QuantizedQuat<17> QuantizedQuat53; // 3 free bit
typedef QuantizedQuat<20> QuantizedQuat62; // 2 free bits

} // namespace gamemath
