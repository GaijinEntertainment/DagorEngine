// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/** \addtogroup de3Common
  @{
*/

#include <heightMapLand/dag_hmlTraceRay.h>

/**
Ray tracer class for effects.
Ray collids with heightmap and FRT.
*/

class FrtSceneEffectRayTracer : public IEffectRayTracer
{
  FastRtDumpManager &frt;

public:
  /**
  \brief Constructor

  \param[in] _frt FRT dump manager.
  \param[in] _hmap heightmap land dump manager.

  @see FastRtDumpManager HeightmapLandDumpManager
  */
  FrtSceneEffectRayTracer(FastRtDumpManager &_frt, void *) : frt(_frt) {}

  /**
  \brief Traces ray.

  \param[in] pos point, ray traces from.
  \param[in] dir tracing ray direction.
  \param[out] dist to save distance from pos to contact point to.
  \param[out] out_normal to save colliding object's normal in contact point to.

  @see Point3
  */
  virtual bool traceRay(const Point3 &pos, const Point3 &dir, float &dist, Point3 *out_normal = NULL)
  {
    real ldir = length(dir);

    real t = dist * ldir;
    bool hit;
    int pmid;

    if (out_normal)
    {
      hit = frt.tracerayNormalized(pos, dir / ldir, t, pmid, *out_normal);
    }
    else
    {
      Point3 norm;
      hit = frt.tracerayNormalized(pos, dir / ldir, t);
    }

    if (hit)
      dist = t / ldir;

    return hit;
  }
};
/** @} */
