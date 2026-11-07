// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "mesh/bvh.h"
#include "validate/intersect.h"
#include <gameLibs/daCSG/validate_api.h>
#include <math/random/dag_random.h>
#include <EASTL/algorithm.h>
#include <float.h>
#include <math.h>

namespace dacsg
{
struct Surface
{
  const Solid &s;
  Bvh bvh;
  BBoxD box;
  dag::Vector<DPoint3> corner;
  dag::Vector<double> areaSum; // prefix sums of the face areas

  Surface(const Solid &solid, bool with_bvh) : s(solid)
  {
    if (with_bvh)
      bvh.build(s);
    corner.resize(s.halfedge.size());
    areaSum.resize(s.faceCount());
    double sum = 0;
    for (uint32_t f = 0; f < s.faceCount(); ++f)
    {
      for (int k = 0; k < 3; ++k)
      {
        corner[3 * f + k] = s.pos[s.faceVert(f, k)];
        box += corner[3 * f + k];
      }
      sum += 0.5 * length(face_cross(s, f));
      areaSum[f] = sum;
    }
  }

  double area() const { return areaSum.empty() ? 0 : areaSum.back(); }

  // generalized winding number above 1/2
  bool inside(const DPoint3 &p) const
  {
    for (int i = 0; i < 3; ++i)
      if (p[i] < box.lim[0][i] || p[i] > box.lim[1][i])
        return false;
    double sum = 0;
    for (uint32_t i = 0; i < corner.size(); i += 3)
      sum += half_solid_angle(p, corner[i], corner[i + 1], corner[i + 2]);
    return winds_around(sum);
  }

  double distance(const DPoint3 &p) const
  {
    double d;
    return bvh.closest(s, p, d) == NO_INDEX ? DBL_MAX : sqrt(d);
  }

  DPoint3 sample(int &seed) const
  {
    const uint32_t f = eastl::min<uint32_t>(eastl::upper_bound(areaSum.begin(), areaSum.end(), _frnd(seed) * area()) - areaSum.begin(),
      areaSum.size() - 1);
    const double r = sqrt(_frnd(seed)), t = _frnd(seed);
    return corner[3 * f] * (1 - r) + corner[3 * f + 1] * (r * (1 - t)) + corner[3 * f + 2] * (r * t);
  }
};

static double rel_diff(double a, double b)
{
  const double lo = min(fabs(a), fabs(b)), d = fabs(a - b);
  return d == 0 ? 0 : lo > 0 ? d / lo : DBL_MAX;
}

CompareReport compare(const Solid &a, const Solid &b, const CompareParams &p)
{
  CompareReport r;
  for (const Solid *s : {&a, &b})
    if (s->halfedge.size() != 3 * s->faceCount())
      return (r.error.sprintf("%u halfedges for %u faces", uint32_t(s->halfedge.size()), s->faceCount()), r);
  if (!indices_valid(a) || !indices_valid(b))
  {
    r.error = "a solid references a missing vertex";
    return r;
  }
  const Surface sa(a, p.samples > 0), sb(b, p.samples > 0);
  r.volumeA = signed_volume(a);
  r.volumeB = signed_volume(b);
  r.volumeRelDiff = rel_diff(r.volumeA, r.volumeB);
  r.areaA = sa.area();
  r.areaB = sb.area();
  r.areaRelDiff = rel_diff(r.areaA, r.areaB);

  BBoxD box = sa.box;
  add_bbox(box, sb.box);
  double tolerance = p.tolerance;
  if (tolerance == 0 && !auto_tolerance(box, tolerance, r.error))
    return r;

  int seed = int(p.seed);
  for (uint32_t i = 0; i < p.samples && !box.isempty(); ++i, ++r.samples)
  {
    DPoint3 q;
    for (int k = 0; k < 3; ++k)
      q[k] = box.lim[0][k] + (box.lim[1][k] - box.lim[0][k]) * _frnd(seed);
    const bool insideA = sa.inside(q);
    if (insideA == sb.inside(q) || sa.distance(q) <= tolerance || sb.distance(q) <= tolerance)
      continue;
    if (r.disagreements < p.maxListedDisagreements)
      r.disagreementPoints.push_back(Disagreement{Point3(q), uint8_t(insideA ? 0 : 1)});
    ++r.disagreements;
  }

  if (a.faceCount() && b.faceCount() && p.samples)
  {
    double sum = 0;
    for (uint32_t i = 0; i < p.samples; ++i)
    {
      const double da = sb.distance(sa.sample(seed)), db = sa.distance(sb.sample(seed));
      r.surfaceDistMax = max(r.surfaceDistMax, max(da, db));
      sum += da + db;
    }
    r.surfaceDistMean = sum / (2.0 * p.samples);
  }
  else if (a.faceCount() || b.faceCount() || !p.samples)
    r.surfaceDistMax = r.surfaceDistMean = DBL_MAX;

  r.equal = r.volumeRelDiff <= p.maxVolumeRelDiff && r.areaRelDiff <= p.maxAreaRelDiff && r.disagreements <= p.maxDisagreements;
  return r;
}
} // namespace dacsg
