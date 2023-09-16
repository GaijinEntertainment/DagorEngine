//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <recast.h>

#include <generic/dag_tab.h>
#include <math/dag_Point3.h>
#include <math/dag_mathUtils.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <math/dag_bounds3.h>

namespace recastcoll
{
bool traceray(const rcHeightfield *m_solid, const Point3 &start_in, Point3 &end_in_out);
float get_line_max_height(const rcHeightfield *solid, const Point3 &sp, const Point3 &sq, const float minSpace);
float get_line_transparency(const rcHeightfield *solid, const Point3 &sp, const Point3 &sq, const float maxHeight,
  const float minHeight);
bool get_compact_heightfield_height(const rcCompactHeightfield *chf, const Point3 &pt, const float range, const float hrange,
  float &height);
float build_box_on_sphere_cast(const rcHeightfield *m_solid, const Point3 &pt, float maxSize, IPoint2 step, BBox3 &box);
// Point2 precision = (float agentRad, float maxClimb). best precision = (chf->cs, chf->ch)
bool is_line_walkable(const rcCompactHeightfield *chf, const Point3 &sp, const Point3 &sq, const Point2 precision);

__forceinline IPoint3 convert_point(const rcHeightfield *m_solid, const Point3 &pt)
{
  const int w = m_solid->width - 1;
  const int h = m_solid->height - 1;
  const float cs = m_solid->cs;
  const float ch = m_solid->ch;
  const float *orig = m_solid->bmin;
  IPoint3 ret;
  ret.x = clamp((int)floorf((pt.x - orig[0]) / cs), 0, w);
  ret.y = max((int)floorf((pt.y - orig[1]) / ch), 0);
  ret.z = clamp((int)floorf((pt.z - orig[2]) / cs), 0, h);
  return ret;
}

template <typename T>
__forceinline Point3 convert_point(const T *context, const IPoint3 &pt)
{
  const float cs = context->cs;
  const float ch = context->ch;
  const float *orig = context->bmin;
  Point3 ret;
  ret.x = orig[0] + pt.x * cs;
  ret.y = orig[1] + pt.y * ch;
  ret.z = orig[2] + pt.z * cs;
  return ret;
}
} // namespace recastcoll
