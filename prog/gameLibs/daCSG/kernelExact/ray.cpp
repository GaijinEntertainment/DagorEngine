// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "kernelExact/arrangement.h"
#include <math.h>

namespace dacsg::exact
{
// int64grid: the seed point's coordinates are integers over it
static constexpr double SEED_SCALE = 65536;

// a dyadic point strictly inside the projection of f, as an LPI-shaped point; false when f is too thin
static bool seed_point(const Arrangement &a, const Face &f, int axis, int sign, ExactPoint &q)
{
  const int u = (axis + 1) % 3, v = (axis + 2) % 3;
  double cu = 0, cv = 0;
  for (uint32_t id : f.v)
  {
    cu += a.points[id].approx[u] / 3;
    cv += a.points[id].approx[v] / 3;
  }
  q.err = 0;
  q.approx[axis] = 0;
  // shewchuk: every double is exact, so a face three ulps wide holds its centroid
  q.approx[u] = a.fp ? cu : round(cu * SEED_SCALE) / SEED_SCALE;
  q.approx[v] = a.fp ? cv : round(cv * SEED_SCALE) / SEED_SCALE;
  if (a.fp)
    q.fp = true; // a dyadic double triple is an exact input point for the shewchuk backend
  else
  {
    q.kind = PointKind::Lpi;
    q.lpi.w = BigInt<3>(int64_t(SEED_SCALE));
    q.lpi.c[axis] = BigInt<3>(0);
    q.lpi.c[u] = BigInt<3>(int64_t(round(cu * SEED_SCALE)));
    q.lpi.c[v] = BigInt<3>(int64_t(round(cv * SEED_SCALE)));
  }
  for (int k = 0; k < 3; ++k)
    if (orient2d(a.points[f.v[k]], a.points[f.v[(k + 1) % 3]], q, axis) != sign)
      return false;
  return true;
}

// orient2d(p, q, Q) when it is zero: the sign for Q moved by (e, e^2) in the (u, v) projection; the sign of a difference
// of two doubles is exact, so the approx serves both backends
static int sos_edge_sign(const ExactPoint &p, const ExactPoint &q, int axis)
{
  const int u = (axis + 1) % 3, v = (axis + 2) % 3;
  const double du = q.approx[u] - p.approx[u], dv = q.approx[v] - p.approx[v];
  return dv != 0 ? (dv < 0 ? 1 : -1) : (du > 0 ? 1 : -1);
}

// sign of the axis coordinate of plane t at q minus that of plane f, with the (e, e^2) move on a tie; q must be the
// seed_point LPI, whose components are one limb wide, as the grid path reads only limb[0]
static int beyond_sign(const Plane &t, const Plane &f, const ExactPoint &q, int axis)
{
  if (t.fp)
    return fp::beyond_sign(t, f, q.approx, axis);
  const int u = (axis + 1) % 3, v = (axis + 2) % 3;
  const int axisSigns = t.n[axis].sign() * f.n[axis].sign();
  const BigInt<3> at = mul<3>(t.n[u], BigInt<1>(q.lpi.c[u].limb[0])) + mul<3>(t.n[v], BigInt<1>(q.lpi.c[v].limb[0])) +
                       mul<3>(t.d, BigInt<1>(q.lpi.w.limb[0]));
  const BigInt<3> af = mul<3>(f.n[u], BigInt<1>(q.lpi.c[u].limb[0])) + mul<3>(f.n[v], BigInt<1>(q.lpi.c[v].limb[0])) +
                       mul<3>(f.d, BigInt<1>(q.lpi.w.limb[0]));
  if (const int s = (mul<4>(af, t.n[axis]) - mul<4>(at, f.n[axis])).sign())
    return s * axisSigns;
  if (const int s = (mul<4>(f.n[u], t.n[axis]) - mul<4>(t.n[u], f.n[axis])).sign())
    return s * axisSigns;
  return (mul<4>(f.n[v], t.n[axis]) - mul<4>(t.n[v], f.n[axis])).sign() * axisSigns;
}

bool seed_winding(const Arrangement &a, const Face &f, int32_t *w)
{
  const Group &g = a.groups[f.group];
  const int axis = g.axis, u = (axis + 1) % 3, v = (axis + 2) % 3;
  ExactPoint q;
  if (!seed_point(a, f, axis, g.sign, q))
    return false;
  for (uint32_t l = 0; l < a.leafTriBase.size(); ++l)
    w[l] = 0;
  DPoint3 lo = q.approx, hi = q.approx;
  lo[axis] = -1e30;
  hi[axis] = 1e30;
  lo[u] -= a.unit;
  lo[v] -= a.unit;
  hi[u] += a.unit;
  hi[v] += a.unit;
  dag::Vector<uint32_t> leaves, local;
  const bbox3f box = outward_box(lo, hi);
  a.leafTree.overlapping(box, leaves);
  for (uint32_t l : leaves)
  {
    a.leafBvh[l].overlapping(box, local);
    for (uint32_t t : local)
    {
      const Tri &tri = a.tris[t + a.leafTriBase[l]];
      const ExactPoint *p[3] = {&a.points[tri.v[0]], &a.points[tri.v[1]], &a.points[tri.v[2]]};
      // the orientation in the ray's projection, not in the group's own
      const int o = tri.orient * plane_sign(a.groups[tri.group].plane, axis);
      bool inside = true;
      for (int k = 0; k < 3 && inside; ++k)
      {
        int s = orient2d(*p[k], *p[(k + 1) % 3], q, axis);
        if (s == 0)
          s = sos_edge_sign(*p[k], *p[(k + 1) % 3], axis);
        inside = s == o;
      }
      // leaving the leaf counts +1, entering it -1: nested outward shells of one leaf wind past 1
      if (inside && beyond_sign(a.groups[tri.group].plane, g.plane, q, axis) * g.sign > 0)
        w[tri.leaf] += o * g.sign;
    }
  }
  return true;
}
} // namespace dacsg::exact
