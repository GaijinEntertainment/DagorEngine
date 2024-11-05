//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <scene/dag_frtdump.h>
#include <sceneRay/dag_sceneRay.h>

// RT flags setup
inline void FastRtDump::setCustomFlags(int custom)
{
  rt->setUseFlagMask(customUsageMask[custom]);
  rt->setSkipFlagMask(customSkipMask[custom]);
}

inline void FastRtDump::setDefaultFlags()
{
  rt->setUseFlagMask(customUsageMask[CUSTOM_Default]);
  rt->setSkipFlagMask(customSkipMask[CUSTOM_Default]);
}

inline bool FastRtDump::checkFaceFlags(int custom, int face_flags)
{
  return (customUsageMask[custom] & face_flags) && !(customSkipMask[custom] & face_flags);
}

// pmid-oriented version of StaticSceneRayTracer interface
inline int FastRtDump::traceray(const Point3 &p, const Point3 &dir, real &t, int &out_pmid)
{
  if (!active)
    return -1;
  int f = rt->traceray(p, dir, t);
  if (f != -1)
    out_pmid = pmid[f];
  return f;
}

inline int FastRtDump::tracerayNormalized(const Point3 &p, const Point3 &normDir, real &t, int &out_pmid)
{
  if (!active)
    return -1;
  int f = rt->tracerayNormalized(p, normDir, t);
  if (f != -1)
    out_pmid = pmid[f];
  return f;
}

inline int FastRtDump::clipCapsule(const Capsule &cap, Point3 &cp1, Point3 &cp2, real &md, const Point3 &movedirNormalized,
  int &out_pmid)
{
  if (!active)
    return -1;
  int f = rt->clipCapsule(cap, cp1, cp2, md, movedirNormalized);
  if (f != -1)
    out_pmid = pmid[f];
  return f;
}


// pmid-oriented version of StaticSceneRayTracer interface (also normal is returned)
inline int FastRtDump::traceray(const Point3 &p, const Point3 &dir, real &t, int &out_pmid, Point3 &out_norm)
{
  if (!active)
    return -1;
  int f = rt->traceray(p, dir, t);
  if (f != -1)
  {
    out_pmid = pmid[f];
    out_norm = rt->facebounds(f).n;
  }
  return f;
}
inline int FastRtDump::tracerayNormalized(const Point3 &p, const Point3 &normDir, real &t, int &out_pmid, Point3 &out_norm)
{
  if (!active)
    return -1;
  int f = rt->tracerayNormalized(p, normDir, t);
  if (f != -1)
  {
    out_pmid = pmid[f];
    out_norm = rt->facebounds(f).n;
  }
  return f;
}


inline int FastRtDump::clipCapsule(const Capsule &cap, Point3 &cp1, Point3 &cp2, real &md, const Point3 &movedirNormalized,
  int &out_pmid, Point3 &out_norm)
{
  if (!active)
    return -1;
  rt->setUseFlagMask(customUsageMask[CUSTOM_Default]);
  rt->setSkipFlagMask(0);
  int f = rt->clipCapsule(cap, cp1, cp2, md, movedirNormalized);
  setDefaultFlags();

  if (f != -1)
  {
    out_pmid = pmid[f];
    out_norm = rt->facebounds(f).n;
  }
  return f;
}

// Custom (usage flags) pmid-oriented version of StaticSceneRayTracer interface
inline int FastRtDump::traceray(int custom, const Point3 &p, const Point3 &dir, real &t, int &out_pmid)
{
  if (!active)
    return -1;
  setCustomFlags(custom);
  int f = rt->traceray(p, dir, t);
  setDefaultFlags();

  if (f != -1)
    out_pmid = pmid[f];
  return f;
}

inline int FastRtDump::tracerayNormalized(int custom, const Point3 &p, const Point3 &normDir, real &t, int &out_pmid)
{
  if (!active)
    return -1;
  setCustomFlags(custom);
  int f = rt->tracerayNormalized(p, normDir, t);
  setDefaultFlags();

  if (f != -1)
    out_pmid = pmid[f];
  return f;
}

inline int FastRtDump::clipCapsule(int custom, const Capsule &cap, Point3 &cp1, Point3 &cp2, real &md, const Point3 &movedirNormalized,
  int &out_pmid)
{
  if (!active)
    return -1;
  setCustomFlags(custom);
  int f = rt->clipCapsule(cap, cp1, cp2, md, movedirNormalized);
  setDefaultFlags();

  if (f != -1)
    out_pmid = pmid[f];
  return f;
}

// Custom (usage flags)  pmid-oriented version of StaticSceneRayTracer interface (also normal is returned)
inline int FastRtDump::traceray(int custom, const Point3 &p, const Point3 &dir, real &t, int &out_pmid, Point3 &out_norm)
{
  if (!active)
    return -1;
  setCustomFlags(custom);
  int f = rt->traceray(p, dir, t);
  setDefaultFlags();

  if (f != -1)
  {
    out_pmid = pmid[f];
    out_norm = rt->facebounds(f).n;
  }
  return f;
}
inline int FastRtDump::tracerayNormalized(int custom, const Point3 &p, const Point3 &normDir, real &t, int &out_pmid, Point3 &out_norm)
{
  if (!active)
    return -1;
  setCustomFlags(custom);
  int f = rt->tracerayNormalized(p, normDir, t);
  setDefaultFlags();

  if (f != -1)
  {
    out_pmid = pmid[f];
    out_norm = rt->facebounds(f).n;
  }
  return f;
}
inline int FastRtDump::clipCapsule(int custom, const Capsule &cap, Point3 &cp1, Point3 &cp2, real &md, const Point3 &movedirNormalized,
  int &out_pmid, Point3 &out_norm)
{
  if (!active)
    return -1;
  setCustomFlags(custom);
  int f = rt->clipCapsule(cap, cp1, cp2, md, movedirNormalized);
  setDefaultFlags();

  if (f != -1)
  {
    out_pmid = pmid[f];
    out_norm = rt->facebounds(f).n;
  }
  return f;
}

inline bool FastRtDump::rayhitNormalized(int custom, const Point3 &p, const Point3 &normDir, real t)
{
  if (!active)
    return false;
  setCustomFlags(custom);
  bool ret = rt->rayhitNormalized(p, normDir, t);
  setDefaultFlags();

  return ret;
}


//==============================================================================
inline bool FastRtDump::rayhitNormalized(const Point3 &p, const Point3 &norm_dir, real t)
{
  if (!active)
    return false;
  return rt->rayhitNormalized(p, norm_dir, t);
}
