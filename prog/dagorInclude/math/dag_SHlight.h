//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_SHmath.h>
#include <math/dag_Point3.h>
#include <string.h>


//! Lighting as Spherical harmonics (SH3)
class SH3Lighting
{
public:
  Color3 sh[SPHHARM_NUM3];


  static const SH3Lighting ZERO;


  SH3Lighting() {}


  SH3Lighting(const Color3 &sh00)
  {
    clear();
    sh[SPHHARM_00] = sh00;
  }


  void clear() { memset(sh, 0, sizeof(sh)); }


  // Returns average diffuse (clamped N*L) lighting color.
  Color3 getAverageDiffuseLighting() const { return sh[SPHHARM_00] * (SPHHARM_COEF_0 * PI); }


  // Returns maximum difference of diffuse lighting from average one.
  // NOTE: returned value is positive, but difference is +/- from average.
  // Use it together with getAverageDiffuseLighting() to light sprites/particles.
  Color3 getMaxDiffuseLightingDifference() const
  {
    Color3 sq = sh[SPHHARM_1m1] * sh[SPHHARM_1m1] + sh[SPHHARM_10] * sh[SPHHARM_10] + sh[SPHHARM_1p1] * sh[SPHHARM_1p1];

    return Color3(sqrtf(sq.r), sqrtf(sq.g), sqrtf(sq.b)) * (SPHHARM_COEF_1 * TWOPI / 3);
  }


  Color3 getDiffuseLighting(const Point3 &normal) const;

  Color3 getSample(const Point3 &n) const;
};


//! Lighting as packed Spherical harmonics (SH3)
class SH3StoredLighting
{
public:
  typedef Color3 Color;

  void store(const Color3 lt[SPHHARM_NUM3])
  {
    for (int i = 0; i < SPHHARM_NUM3; ++i)
      store(lt[i], sh[i]);
  }

  void copyTo(SH3Lighting &lt)
  {
    for (int i = 0; i < SPHHARM_NUM3; ++i)
      restore(sh[i], lt.sh[i]);
  }

  void interpolate(SH3StoredLighting &p0, SH3StoredLighting &p1, real t)
  {
    real k0 = 1 - t;
    for (int i = 0; i < SPHHARM_NUM3; ++i)
      blend(sh[i], p0.sh[i], k0, p1.sh[i], t);
  }

  void interpolate(SH3StoredLighting &p0, SH3StoredLighting &p1, SH3StoredLighting &p2, real u, real v)
  {
    real k0 = 1 - u - v;
    for (int i = 0; i < SPHHARM_NUM3; ++i)
      blend(sh[i], p0.sh[i], k0, p1.sh[i], u, p2.sh[i], v);
  }

  bool differs(SH3StoredLighting &p, real thr)
  {
    for (int i = 0; i < SPHHARM_NUM3; ++i)
      if (differs(sh[i], p.sh[i], thr))
        return true;
    return false;
  }

  void dump();

protected:
  void store(const Color3 &from, Color &to) { to = from; }

  void restore(const Color &from, Color3 &to) { to = from; }

  void blend(Color &res, const Color &p0, real w0, const Color &p1, real w1) { res = p0 * w0 + p1 * w1; }

  void blend(Color &res, const Color &p0, real w0, const Color &p1, real w1, const Color &p2, real w2)
  {
    res = p0 * w0 + p1 * w1 + p2 * w2;
  }

  bool differs(const Color &p0, const Color &p1, real thr) { return length(p1 - p0) > thr; }

  Color sh[SPHHARM_NUM3];
};


//! Lighting as Spherical harmonics (SH2) + 2 directional lights
struct SHDirLighting
{
  static const SHDirLighting ZERO;

  SHDirLighting() { clear(); }

  //! Spherical harmonics for ambient and non-primary lights (SH2)
  Color3 shAmb[4];

  //! directions of 2 lights and their colors
  Point3 dir1, dir2;
  Color3 col1, col2;

  //! reserved elements and label to distinct from SH3Lighting
  real _resv1, _resv2, label;

public:
  void clear()
  {
    memset(this, 0, sizeof(*this));
    label = realQNaN;
  }
};


//! Unified lighting, type can be deduced in runtime
class SHUnifiedLighting
{
public:
  //! reinterpreat as SH3Lighting
  const SH3Lighting &getSH3() const { return (const SH3Lighting &)sh; }
  SH3Lighting &getSH3() { return (SH3Lighting &)sh; }

  //! reinterpreat as SHDirLighting
  const SHDirLighting &getDirLt() const { return (const SHDirLighting &)sh; }
  SHDirLighting &getDirLt() { return (SHDirLighting &)sh; }

  //! determine type using NaN is dirLt.label
  bool isSHDirLighting() const { return check_nan(getDirLt().label); }

protected:
  Color3 sh[9];
};
