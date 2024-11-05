// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_ffd44.h>

void FFDDeformer44::noDeform()
{
  for (int i = 0; i < PT_COUNT; i++)
    pt[i] = getDefaultPoint(i);
}

void FFDDeformer44::crashDeform(float scale, bool preserve_only_corners)
{
  noDeform();
  for (int i = 0; i < 4; i++)
  {
    Point3 norm;
    norm.x = i == 0 ? 1 : (i == 3 ? -1 : 0);
    for (int j = 0; j < 4; j++)
    {
      norm.y = j == 0 ? 1 : (j == 3 ? -1 : 0);
      for (int k = 0; k < 4; k++)
      {
        norm.z = k == 0 ? 1 : (k == 3 ? -1 : 0);
        float fabsfNorm = fabsf(norm.x) + fabsf(norm.y) + fabsf(norm.z);
        if (fabsfNorm > (preserve_only_corners ? 2.1 : 1.1))
          continue;
        if (fabsfNorm < 0.9)
          continue;
        Point3 normalizedNorm = normalize(norm);
        pt[gridIndex(i, j, k)] += normalizedNorm * scale / fabsfNorm;
      }
    }
  }
}

FFDDeformer44::FFDDeformer44()
{
  setBox(BBox3(Point3(0, 0, 0), Point3(1, 1, 1)));
  noDeform();
}

Point3 FFDDeformer44::deform(const Point3 &p, bool in_bound_only) const
{
  Point3 q(0, 0, 0), pp;

  // Transform into lattice space
  pp = transformTo(p);
  // maybe skip the point if it is outside the source volume.
  if (in_bound_only)
  {
    for (int i = 0; i < 3; i++)
    {
      if (pp[i] < -REAL_EPS || pp[i] > 1.0f + REAL_EPS)
        return p;
    }
  }

  // Compute the deformed point as a weighted average of all
  // 64 control points.
  Point3 pp2(pp.x * pp.x, pp.y * pp.y, pp.z * pp.z);
  Point3 ss(1 - pp.x, 1 - pp.y, 1 - pp.z);
  Point3 ss2(ss.x * ss.x, ss.y * ss.y, ss.z * ss.z);
  float bpolyk[3][4] = {{ss2.x * ss.x, 3.0f * ss2.x * pp.x, 3.0f * ss.x * pp2.x, pp2.x * pp.x},
    {ss2.y * ss.y, 3.0f * ss2.y * pp.y, 3.0f * ss.y * pp2.y, pp2.y * pp.y},
    {ss2.z * ss.z, 3.0f * ss2.z * pp.z, 3.0f * ss.z * pp2.z, pp2.z * pp.z}};
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      for (int k = 0; k < 4; k++)
      {
        q += pt[gridIndex(i, j, k)] * bpolyk[0][i] * bpolyk[1][j] * bpolyk[2][k];
      }
    }
  }
  // Transform out of lattice space back into object space.
  return transformFrom(q);
}
