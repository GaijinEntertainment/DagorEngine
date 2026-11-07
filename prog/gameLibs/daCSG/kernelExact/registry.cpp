// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "kernelExact/arrangement.h"
#include <EASTL/sort.h>
#include <math.h>

namespace dacsg::exact
{
// Equal points round to the same or an adjacent integer triple of units: a unit is at least four times the error,
// so the rounding of a coordinate past 2^48 units cannot push them two apart.
static uint64_t bucket_key(const DPoint3 &approx, double per_unit, int dx, int dy, int dz)
{
  return mix_cell(uint64_t(llround(approx.x * per_unit)) + dx, uint64_t(llround(approx.y * per_unit)) + dy,
    uint64_t(llround(approx.z * per_unit)) + dz);
}

struct Buckets
{
  KeyChains chains;
  double perUnit = 1;

  void add(const Arrangement &a, uint32_t id) { chains.add(bucket_key(a.points[id].approx, perUnit, 0, 0, 0), id); }

  uint32_t find_equal(const Arrangement &a, const ExactPoint &p) const
  {
    for (int dx = -1; dx <= 1; ++dx)
      for (int dy = -1; dy <= 1; ++dy)
        for (int dz = -1; dz <= 1; ++dz)
          for (uint32_t id = chains.first(bucket_key(p.approx, perUnit, dx, dy, dz)); id != NO_INDEX; id = chains.next[id])
            if (point_equal(p, a.points[id]))
              return id;
    return NO_INDEX;
  }
};

void assign_ids(Arrangement &a)
{
  if (a.candidates.empty())
    return;
  double width = a.unit;
  for (dag::ConstSpan<ExactPoint> set : {make_span_const(a.points), make_span_const(a.candidates)})
    for (const ExactPoint &p : set)
      width = eastl::max(width, 2 * p.err);
  Buckets buckets;
  buckets.perUnit = 1 / (2 * width);
  const uint32_t most = a.points.size() + a.candidates.size();
  buckets.chains.init(most, most);
  for (uint32_t id = 0; id < a.points.size(); ++id)
    buckets.add(a, id);
  dag::Vector<uint32_t> order(a.candidates.size()), idOf(a.candidates.size(), NO_INDEX);
  for (uint32_t i = 0; i < order.size(); ++i)
    order[i] = i;
  eastl::sort(order.begin(), order.end(), [&](uint32_t x, uint32_t y) {
    return key_less(a.candidates[x], a.candidates[y]) || (!key_less(a.candidates[y], a.candidates[x]) && x < y);
  });
  for (uint32_t i = 0; i < order.size(); ++i)
  {
    const uint32_t c = order[i];
    if (i > 0 && !key_less(a.candidates[order[i - 1]], a.candidates[c]))
    {
      idOf[c] = idOf[order[i - 1]];
      continue;
    }
    const ExactPoint &p = a.candidates[c];
    uint32_t id = buckets.find_equal(a, p);
    if (id == NO_INDEX)
    {
      id = a.points.size();
      a.points.push_back(p);
      buckets.add(a, id);
      ++a.pointsByKind[int(p.kind)];
    }
    idOf[c] = id;
  }
  for (Segment &s : a.segments)
    for (uint32_t &p : s.p)
      if (p & CAND)
        p = idOf[p & ~CAND];
  // find_crossings refills it for the next call
  a.candidates.clear();
}

bool tri_closure_contains(const Arrangement &a, uint32_t tri, const ExactPoint &p)
{
  const Tri &t = a.tris[tri];
  const Group &g = a.groups[t.group];
  if (sign_point_plane(p, g.plane) != 0)
    return false;
  const int s = t.orient * g.sign;
  for (int k = 0; k < 3; ++k)
    if (s * orient2d(a.points[t.v[k]], a.points[t.v[(k + 1) % 3]], p, g.axis, t.group) < 0)
      return false;
  return true;
}

void insert_points(const Arrangement &a, dag::Vector<uint64_t> &tri_point_pairs)
{
  dag::Vector<uint32_t> hits, leaves;
  for (uint32_t id = 0; id < a.points.size(); ++id)
  {
    const ExactPoint &p = a.points[id];
    const DPoint3 e(p.err, p.err, p.err);
    const bbox3f box = outward_box(p.approx - e, p.approx + e);
    const uint32_t own = id < a.pointLeaf.size() ? a.pointLeaf[id] : NO_INDEX;
    a.leafTree.overlapping(box, leaves);
    for (uint32_t l : leaves)
    {
      // a convex prim holds its own vertex at corners only, which finish_tri_points adds
      if (l == own && a.leafConvex[l])
        continue;
      a.leafBvh[l].overlapping(box, hits);
      for (uint32_t local : hits)
      {
        const uint32_t tri = local + a.leafTriBase[l];
        const Tri &t = a.tris[tri];
        const bool corner = t.v[0] == id || t.v[1] == id || t.v[2] == id;
        if (corner || tri_closure_contains(a, tri, p))
          tri_point_pairs.push_back(uint64_t(tri) << 32 | id);
      }
    }
  }
}

void finish_tri_points(Arrangement &a, dag::Vector<uint64_t> &tri_point_pairs)
{
  for (uint32_t t = 0; t < a.tris.size(); ++t)
    for (uint32_t v : a.tris[t].v)
      tri_point_pairs.push_back(uint64_t(t) << 32 | v);
  eastl::sort(tri_point_pairs.begin(), tri_point_pairs.end());
  a.triPointStart.assign(a.tris.size() + 1, 0);
  a.triPoint.clear();
  for (uint32_t i = 0; i < tri_point_pairs.size(); ++i)
  {
    if (i > 0 && tri_point_pairs[i] == tri_point_pairs[i - 1])
      continue;
    a.triPoint.push_back(uint32_t(tri_point_pairs[i]));
    ++a.triPointStart[uint32_t(tri_point_pairs[i] >> 32) + 1];
  }
  for (uint32_t t = 0; t < a.tris.size(); ++t)
    a.triPointStart[t + 1] += a.triPointStart[t];
  tri_point_pairs.clear();
}

void index_segments(Arrangement &a)
{
  eastl::sort(a.segments.begin(), a.segments.end(), [](const Segment &x, const Segment &y) {
    return x.tri != y.tri ? x.tri < y.tri : x.p[0] != y.p[0] ? x.p[0] < y.p[0] : x.p[1] < y.p[1];
  });
  a.triSegStart.assign(a.tris.size() + 1, 0);
  for (const Segment &s : a.segments)
    ++a.triSegStart[s.tri + 1];
  for (uint32_t t = 0; t < a.tris.size(); ++t)
    a.triSegStart[t + 1] += a.triSegStart[t];
}
} // namespace dacsg::exact
