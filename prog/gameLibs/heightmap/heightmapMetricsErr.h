// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

struct HeightmapErrorRet
{
  float maxError = 0;
  float rms = 0;
  int cnt = 0;
  enum
  {
    HAS_UNDERWATER = 1,
    HAS_LAND = 2,
    SHOULD_HAVE_UNDERWATER = 4,
    SHOULD_HAVE_LAND = 8,
    SHORE = 16,
    DISAPPEARED_UNDERWATER = HAS_LAND | SHOULD_HAVE_UNDERWATER,
    DISAPPEARED_LAND = SHOULD_HAVE_LAND | HAS_UNDERWATER
  };
  enum
  {
    DISAPPEARED_UNDERWATER_MASK = HAS_UNDERWATER | DISAPPEARED_UNDERWATER,
    DISAPPEARED_LAND_MASK = HAS_LAND | DISAPPEARED_LAND
  };
  uint8_t flags = 0;
  bool isShore2() const
  {
    return (flags & DISAPPEARED_UNDERWATER_MASK) == DISAPPEARED_UNDERWATER || (flags & DISAPPEARED_LAND_MASK) == DISAPPEARED_LAND;
  }
  bool isShore() const { return flags & SHORE; }
  void calcShore()
  {
    flags &= ~SHORE;
    if (isShore2())
      flags |= SHORE;
  }
  float getRMSSq() const { return cnt > 0 ? (rms / cnt) : 0; }
  float getRMS() const { return sqrtf(getRMSSq()); }
};

static inline float add_error_no_rms(HeightmapErrorRet &error, float triVal, float hmapVal, float water_level)
{
  float err = fabsf(triVal - hmapVal);
  if (triVal > water_level && hmapVal > water_level)
    error.flags |= error.HAS_LAND | error.SHOULD_HAVE_LAND;
  else if (triVal < water_level && hmapVal < water_level)
    error.flags |= error.HAS_UNDERWATER | error.SHOULD_HAVE_UNDERWATER;
  else
  {
    uint8_t fl = 0;
    if (triVal < water_level)
      fl |= error.HAS_UNDERWATER;

    if (triVal > water_level)
      fl |= error.HAS_LAND;

    if (hmapVal < water_level)
      fl |= error.SHOULD_HAVE_UNDERWATER;

    if (hmapVal > water_level)
      fl |= error.SHOULD_HAVE_LAND;

    if (fl == error.DISAPPEARED_UNDERWATER || fl == error.DISAPPEARED_LAND)
      fl |= error.SHORE;

    if (fl == error.DISAPPEARED_UNDERWATER)
      err *= 1.5f;

    error.flags |= fl;
  }
  error.maxError = max(error.maxError, err);
  return err;
}

static inline void add_error(HeightmapErrorRet &error, float triVal, float hmapVal, float water_level)
{
  error.rms += sqr(add_error_no_rms(error, triVal, hmapVal, water_level));
}

static inline void add_error(HeightmapErrorRet &error, const HeightmapErrorRet &err)
{
  error.flags |= err.flags;
  if (err.maxError > error.maxError)
    error.maxError = err.maxError;
  error.rms += err.rms;
  error.cnt += err.cnt;
}


static inline bool operator<(const HeightmapErrorRet &a, const HeightmapErrorRet &b)
{
  if (a.isShore() != b.isShore())
    return uint8_t(a.isShore()) < uint8_t(b.isShore());
  return a.maxError * a.maxError + a.getRMSSq() < b.maxError * b.maxError + b.getRMSSq();
  // return a.maxError + a.getRMSSq() < b.maxError + b.getRMSSq();
}
