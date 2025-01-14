//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_mathBase.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>

// angles names:
// heading - y rotate; attitude - z; bank - x

// converts Euler angles to quaternion
void euler_to_quat(real heading, real attitude, real bank, Quat &quat);
void euler_heading_to_quat(real heading, Quat &quat);
void euler_attitude_to_quat(real attitude, Quat &quat);
void euler_bank_to_quat(real bank, Quat &quat);
void euler_heading_attitude_to_quat(real heading, real attitude, Quat &quat);
void euler_attitude_bank_to_quat(real attitude, real bank, Quat &quat);
// gets Euler angles from quaternion
void quat_to_euler(const Quat &quat, real &heading, real &attitude, real &bank);

// gets Euler angles from quaternion with some approximation. max |error|<1.35e-5. 30% faster
void quat_to_euler_fast(const Quat &quat, real &heading, real &attitude, real &bank);
// gets Euler angles from matrix
void matrix_to_euler(const TMatrix &tm, real &heading, real &attitude, real &bank);

// gets local rotate angel for a 1 axis (1 - x, 2 - y, 3 - z)
real get_axis_angle(const TMatrix &tm, int axis);

// returns angle between two vectors
real vectors_angle(const Point3 &v1, const Point3 &v2);

// converts directon in Cartesian CS into spheric CS angles
Point2 dir_to_sph_ang(const Point3 &dir);

// converts direction to quaternion which points in such direction
Quat dir_to_quat(const Point3 &dir);

// converts direction to quaternion which points in such direction, when "up" is not necesserily 0,1,0.
Quat dir_and_up_to_quat(const Point3 &dir, const Point3 &up);

// converts angles in spheric CS into cartesian direction
// angles.x -- azimuth, angles.y -- zenith
Point3 sph_ang_to_dir(const Point2 &angles);

Quat axis_angle_to_quat(Point3 axis, float ang);
void quat_to_axis_angle(const Quat &q, Point3 &axis, float &ang);


/////////////////////////////////////
// Normalization and interp of angles
#define INLINE __forceinline

INLINE real norm_ang_deg(real a) { return fmodf(a, 360.f); }

// normalize angle to the range [0;360)
// regular norm_ang_deg returns (-360;+360)
INLINE real norm_ang_deg_positive(real a)
{
  const float res = fmodf(a, 360.f);
  const bool neg = a < 0.0f;
  return res + neg * 360.0f;
}

INLINE real norm_s_ang_deg(real a)
{
  a = fmodf(a, 360.f);
  return a + fsel(a - 180.f, -360.f, fsel(a + 180.f, 0, 360.f));
}

INLINE real clamp_s_ang(real a, real min_a, real max_a)
{
  real normMin = norm_s_ang(min_a - a);
  real normMax = norm_s_ang(max_a - a);
  return (normMin <= 0.f && normMax >= 0.f) ? a : (abs(normMax) - abs(normMin) >= 0.f ? min_a : max_a);
}

INLINE real interp_ang_deg(real a, real b, real t)
{
  return lerp(a, b, t) - fsel(180.0f - fabsf(a - b), 0, 360.0f * fsel(a - b, 1.0f - t, t));
}

INLINE real interp_s_ang_deg(real a, real b, real t) { return interp_ang_deg(norm_s_ang_deg(a), norm_s_ang_deg(b), t); }

INLINE real interp_ang(real a, real b, real t) { return lerp(a, b, t) - fsel(PI - fabsf(a - b), 0, TWOPI * fsel(a - b, 1.0f - t, t)); }

INLINE real restrict_ang_deg(bool full_circle, real a, real min_angle, real max_angle)
{
  if (full_circle)
    return norm_s_ang_deg(a);

  float midA = (min_angle + max_angle) * 0.5f;
  return midA + norm_s_ang_deg(norm_s_ang_deg(a) - norm_s_ang_deg(midA));
}

INLINE real renorm_ang(real ang, real pivot_ang)
{
  real delta = pivot_ang - ang;
  return rabs(delta) > PI ? ang + (delta > 0.f ? 1.f : -1.f) * TWOPI : ang;
}

inline Point2 dir_to_angles(Point3 dir) { return Point2(-atan2f(dir.z, dir.x), atan2f(dir.y, sqrtf(dir.x * dir.x + dir.z * dir.z))); }

inline Point2 dir_normalized_to_angles(Point3 dir)
{
  G_ASSERTF(fabsf(1.f - dir.lengthSq()) < 2e-4f, "len((%.9f,%.9f,%.9f))=%.9f", P3D(dir), dir.length());
  return Point2(-atan2f(dir.z, dir.x), safe_asin(dir.y));
}

inline Point3 angles_to_dir(Point2 angles)
{
  float xSine, xCos;
  float ySine, yCos;
  sincos(-angles.x, xSine, xCos);
  sincos(angles.y, ySine, yCos);
  return Point3(xCos * yCos, ySine, xSine * yCos);
}

inline float dir_to_yaw(Point2 dir) { return atan2f(dir.y, dir.x); }

inline float dir_to_yaw(Point3 dir) { return atan2f(dir.z, dir.x); }

inline Point2 yaw_to_2d_dir(float yaw)
{
  Point2 res;
  sincos(yaw, res.y, res.x);
  return res;
}

Point3 basis_aware_angles_to_dir(const Point2 &angles, const Point3 &up, const Point3 &fwd);
Point2 basis_aware_dir_to_angles(const Point3 &dir, const Point3 &up, const Point3 &fwd);
Point2 basis_aware_clamp_angles_by_dir(const Point2 &angles, const Point4 max_angles, const Point3 &dir, const Point3 &up,
  const Point3 &fwd);

bool is_direction_clockwise(float angle_1, float angle_2);
bool is_angle_in_sector(float test_angle, const Point2 &sector);
bool is_sector_intersects_sector(const Point2 &sector_1, const Point2 &sector_2);

bool is_direction_clockwise_deg(float angle_1_deg, float angle_2_deg);
bool is_angle_in_sector_deg(float test_angle_deg, const Point2 &sector_deg);
bool is_sector_intersects_sector_deg(const Point2 &sector_1_deg, const Point2 &sector_2_deg);

#undef INLINE
