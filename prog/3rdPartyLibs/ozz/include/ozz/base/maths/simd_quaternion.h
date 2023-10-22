//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) Guillaume Blanc                                              //
//                                                                            //
// Permission is hereby granted, free of charge, to any person obtaining a    //
// copy of this software and associated documentation files (the "Software"), //
// to deal in the Software without restriction, including without limitation  //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,   //
// and/or sell copies of the Software, and to permit persons to whom the      //
// Software is furnished to do so, subject to the following conditions:       //
//                                                                            //
// The above copyright notice and this permission notice shall be included in //
// all copies or substantial portions of the Software.                        //
//                                                                            //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR //
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   //
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    //
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER //
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    //
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        //
// DEALINGS IN THE SOFTWARE.                                                  //
//                                                                            //
//----------------------------------------------------------------------------//

#ifndef OZZ_OZZ_BASE_MATHS_SIMD_QUATERNION_H_
#define OZZ_OZZ_BASE_MATHS_SIMD_QUATERNION_H_

#include "ozz/base/maths/simd_math.h"

#include <cmath>

// Implement simd quaternion.
namespace ozz {
namespace math {
// Declare the Quaternion type.
struct SimdQuaternion {
  SimdFloat4 xyzw;

  // Returns the identity quaternion.
  static OZZ_INLINE SimdQuaternion identity() {
    const SimdQuaternion quat = {simd_float4::w_axis()};
    return quat;
  }

  // the angle in radian.
  static OZZ_INLINE SimdQuaternion FromAxisAngle(_SimdFloat4 _axis,
                                                 _SimdFloat4 _angle);

  // Returns a normalized quaternion initialized from an axis and angle cosine
  // representation.
  // Assumes the axis part (x, y, z) of _axis_angle is normalized.
  // _angle.x is the angle cosine in radian, it must be within [-1,1] range.
  static OZZ_INLINE SimdQuaternion FromAxisCosAngle(_SimdFloat4 _axis,
                                                    _SimdFloat4 _cos);

  // Returns the quaternion that will rotate vector _from into vector _to,
  // around their plan perpendicular axis.The input vectors don't need to be
  // normalized, they can be null also.
  static OZZ_INLINE SimdQuaternion FromVectors(_SimdFloat4 _from,
                                               _SimdFloat4 _to);

  // Returns the quaternion that will rotate vector _from into vector _to,
  // around their plan perpendicular axis. The input vectors must be normalized.
  static OZZ_INLINE SimdQuaternion FromUnitVectors(_SimdFloat4 _from,
                                                   _SimdFloat4 _to);

  // Returns a normalized quaternion initialized from an axis angle
  // representation.
  // Assumes the axis part (x, y, z) of _axis_angle is normalized. _angle.x is
};

// Returns the multiplication of _a and _b. If both _a and _b are normalized,
// then the result is normalized.
OZZ_INLINE SimdQuaternion operator*(const SimdQuaternion& _a,
                                    const SimdQuaternion& _b) {
  // Original quaternion multiplication can be swizzled in a simd friendly way
  // if w is negated, and some w multiplications parts (1st/last) are swaped.
  //
  //        p1            p2            p3            p4
  //    _a.w * _b.x + _a.x * _b.w + _a.y * _b.z - _a.z * _b.y
  //    _a.w * _b.y + _a.y * _b.w + _a.z * _b.x - _a.x * _b.z
  //    _a.w * _b.z + _a.z * _b.w + _a.x * _b.y - _a.y * _b.x
  //    _a.w * _b.w - _a.x * _b.x - _a.y * _b.y - _a.z * _b.z
  // ... becomes ->
  //    _a.w * _b.x + _a.x * _b.w + _a.y * _b.z - _a.z * _b.y
  //    _a.w * _b.y + _a.y * _b.w + _a.z * _b.x - _a.x * _b.z
  //    _a.w * _b.z + _a.z * _b.w + _a.x * _b.y - _a.y * _b.x
  // - (_a.z * _b.z + _a.x * _b.x + _a.y * _b.y - _a.w * _b.w)
  const SimdFloat4 p1 =
      Swizzle<3, 3, 3, 2>(_a.xyzw) * Swizzle<0, 1, 2, 2>(_b.xyzw);
  const SimdFloat4 p2 =
      Swizzle<0, 1, 2, 0>(_a.xyzw) * Swizzle<3, 3, 3, 0>(_b.xyzw);
  const SimdFloat4 p13 =
      MAdd(Swizzle<1, 2, 0, 1>(_a.xyzw), Swizzle<2, 0, 1, 1>(_b.xyzw), p1);
  const SimdFloat4 p24 =
      NMAdd(Swizzle<2, 0, 1, 3>(_a.xyzw), Swizzle<1, 2, 0, 3>(_b.xyzw), p2);
  const SimdQuaternion quat = {Xor(p13 + p24, simd_int4::mask_sign_w())};
  return quat;
}

// Returns the conjugate of _q. This is the same as the inverse if _q is
// normalized. Otherwise the magnitude of the inverse is 1.f/|_q|.
OZZ_INLINE SimdQuaternion Conjugate(const SimdQuaternion& _q) {
  const SimdQuaternion quat = {Xor(_q.xyzw, simd_int4::mask_sign_xyz())};
  return quat;
}

// Returns the negate of _q. This represent the same rotation as q.
OZZ_INLINE SimdQuaternion operator-(const SimdQuaternion& _q) {
  const SimdQuaternion quat = {Xor(_q.xyzw, simd_int4::mask_sign())};
  return quat;
}

// Returns the normalized quaternion _q.
OZZ_INLINE SimdQuaternion Normalize(const SimdQuaternion& _q) {
  const SimdQuaternion quat = {Normalize4(_q.xyzw)};
  return quat;
}

// Returns the normalized quaternion _q if the norm of _q is not 0.
// Otherwise returns _safer.
OZZ_INLINE SimdQuaternion NormalizeSafe(const SimdQuaternion& _q,
                                        const SimdQuaternion& _safer) {
  const SimdQuaternion quat = {NormalizeSafe4(_q.xyzw, _safer.xyzw)};
  return quat;
}

// Returns the estimated normalized quaternion _q.
OZZ_INLINE SimdQuaternion NormalizeEst(const SimdQuaternion& _q) {
  const SimdQuaternion quat = {NormalizeEst4(_q.xyzw)};
  return quat;
}

// Returns the estimated normalized quaternion _q if the norm of _q is not 0.
// Otherwise returns _safer.
OZZ_INLINE SimdQuaternion NormalizeSafeEst(const SimdQuaternion& _q,
                                           const SimdQuaternion& _safer) {
  const SimdQuaternion quat = {NormalizeSafeEst4(_q.xyzw, _safer.xyzw)};
  return quat;
}

// Tests if the _q is a normalized quaternion.
// Returns the result in the x component of the returned vector. y, z and w are
// set to 0.
OZZ_INLINE SimdInt4 IsNormalized(const SimdQuaternion& _q) {
  return IsNormalized4(_q.xyzw);
}

// Tests if the _q is a normalized quaternion.
// Uses the estimated normalization coefficient, that matches estimated math
// functions (RecpEst, MormalizeEst...).
// Returns the result in the x component of the returned vector. y, z and w are
// set to 0.
OZZ_INLINE SimdInt4 IsNormalizedEst(const SimdQuaternion& _q) {
  return IsNormalizedEst4(_q.xyzw);
}

OZZ_INLINE SimdQuaternion SimdQuaternion::FromAxisAngle(_SimdFloat4 _axis,
                                                        _SimdFloat4 _angle) {
  assert(AreAllTrue1(IsNormalizedEst3(_axis)) && "axis is not normalized.");
  const SimdFloat4 half_angle = _angle * simd_float4::Load1(.5f);
  const SimdFloat4 half_sin = SinX(half_angle);
  const SimdFloat4 half_cos = CosX(half_angle);
  const SimdQuaternion quat = {SetW(_axis * SplatX(half_sin), half_cos)};
  return quat;
}

OZZ_INLINE SimdQuaternion SimdQuaternion::FromAxisCosAngle(_SimdFloat4 _axis,
                                                           _SimdFloat4 _cos) {
  const SimdFloat4 one = simd_float4::one();
  const SimdFloat4 half = simd_float4::Load1(.5f);

  assert(AreAllTrue1(IsNormalizedEst3(_axis)) && "axis is not normalized.");
  assert(AreAllTrue1(And(CmpGe(_cos, -one), CmpLe(_cos, one))) &&
         "cos is not in [-1,1] range.");

  const SimdFloat4 half_cos2 = (one + _cos) * half;
  const SimdFloat4 half_sin2 = one - half_cos2;
  const SimdFloat4 half_sincos2 = SetY(half_cos2, half_sin2);
  const SimdFloat4 half_sincos = Sqrt(half_sincos2);
  const SimdFloat4 half_sin = SplatY(half_sincos);
  const SimdQuaternion quat = {SetW(_axis * half_sin, half_sincos)};
  return quat;
}

// Returns to an axis angle representation of quaternion _q.
// Assumes quaternion _q is normalized.
OZZ_INLINE SimdFloat4 ToAxisAngle(const SimdQuaternion& _q) {
  assert(AreAllTrue1(IsNormalizedEst4(_q.xyzw)) && "_q is not normalized.");
  const SimdFloat4 x_axis = simd_float4::x_axis();
  const SimdFloat4 clamped_w = Clamp(-x_axis, SplatW(_q.xyzw), x_axis);
  const SimdFloat4 half_angle = ACosX(clamped_w);

  // Assuming quaternion is normalized then s always positive.
  const SimdFloat4 s = SplatX(SqrtX(NMAdd(clamped_w, clamped_w, x_axis)));
  // If s is close to zero then direction of axis is not important.
  const SimdInt4 low = CmpLt(s, simd_float4::Load1(1e-3f));
  return Select(low, x_axis,
                SetW(_q.xyzw * RcpEstNR(s), half_angle + half_angle));
}

OZZ_INLINE SimdQuaternion SimdQuaternion::FromVectors(_SimdFloat4 _from,
                                                      _SimdFloat4 _to) {
  // http://lolengine.net/blog/2014/02/24/quaternion-from-two-vectors-final
  const SimdFloat4 norm_from_norm_to =
      SqrtX(Length3Sqr(_from) * Length3Sqr(_to));
  const float norm_from_norm_to_x = GetX(norm_from_norm_to);
  if (norm_from_norm_to_x < 1.e-6f) {
    return SimdQuaternion::identity();
  }

  const SimdFloat4 real_part = norm_from_norm_to + Dot3(_from, _to);
  SimdQuaternion quat;
  if (GetX(real_part) < 1.e-6f * norm_from_norm_to_x) {
    // If _from and _to are exactly opposite, rotate 180 degrees around an
    // arbitrary orthogonal axis. Axis normalization can happen later, when we
    // normalize the quaternion.
    float from[4];
    ozz::math::StorePtrU(_from, from);
    quat.xyzw = std::abs(from[0]) > std::abs(from[2])
                    ? ozz::math::simd_float4::Load(-from[1], from[0], 0.f, 0.f)
                    : ozz::math::simd_float4::Load(0.f, -from[2], from[1], 0.f);
  } else {
    // This is the general code path.
    quat.xyzw = SetW(Cross3(_from, _to), real_part);
  }
  return Normalize(quat);
}

OZZ_INLINE SimdQuaternion SimdQuaternion::FromUnitVectors(_SimdFloat4 _from,
                                                          _SimdFloat4 _to) {
  // http://lolengine.net/blog/2014/02/24/quaternion-from-two-vectors-final
  assert(ozz::math::AreAllTrue1(
             And(IsNormalizedEst3(_from), IsNormalizedEst3(_to))) &&
         "Input vectors must be normalized.");

  const SimdFloat4 real_part =
      ozz::math::simd_float4::x_axis() + Dot3(_from, _to);
  if (GetX(real_part) < 1.e-6f) {
    // If _from and _to are exactly opposite, rotate 180 degrees around an
    // arbitrary orthogonal axis.
    // Normalization isn't needed, as from is already.
    float from[4];
    ozz::math::StorePtrU(_from, from);
    SimdQuaternion quat = {
        std::abs(from[0]) > std::abs(from[2])
            ? ozz::math::simd_float4::Load(-from[1], from[0], 0.f, 0.f)
            : ozz::math::simd_float4::Load(0.f, -from[2], from[1], 0.f)};
    return quat;
  } else {
    // This is the general code path.
    SimdQuaternion quat = {SetW(Cross3(_from, _to), real_part)};
    return Normalize(quat);
  }
}

// Computes the transformation of a Quaternion and a vector _v.
// This is equivalent to carrying out the quaternion multiplications:
// _q.conjugate() * (*this) * _q
// w component of the returned vector is undefined.
OZZ_INLINE SimdFloat4 TransformVector(const SimdQuaternion& _q,
                                      _SimdFloat4 _v) {
  // http://www.neil.dantam.name/note/dantam-quaternion.pdf
  // _v + 2.f * cross(_q.xyz, cross(_q.xyz, _v) + _q.w * _v)
  const SimdFloat4 cross1 = MAdd(SplatW(_q.xyzw), _v, Cross3(_q.xyzw, _v));
  const SimdFloat4 cross2 = Cross3(_q.xyzw, cross1);
  return _v + cross2 + cross2;
}
}  // namespace math
}  // namespace ozz
#endif  // OZZ_OZZ_BASE_MATHS_SIMD_QUATERNION_H_
