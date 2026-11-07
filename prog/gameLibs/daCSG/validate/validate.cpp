// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "mesh/bvh.h"
#include "mesh/solidUtil.h"
#include "mesh/unionFind.h"
#include "validate/intersect.h"
#include <gameLibs/daCSG/validate_api.h>
#include <math/dag_check_nan.h>
#include <EASTL/algorithm.h>
#include <float.h>
#include <math.h>

namespace dacsg
{
// a skin: one triangle in both orientations, which quality counts instead
static bool skin(const Solid &s, uint32_t f, uint32_t g)
{
  for (int j = 0; j < 3; ++j)
    if (s.faceVert(g, j) == s.faceVert(f, 0))
      return s.faceVert(g, (j + 1) % 3) == s.faceVert(f, 2) && s.faceVert(g, (j + 2) % 3) == s.faceVert(f, 1);
  return false;
}

static void plane_sides(const DPoint3 &n, double d, const DPoint3 *tri, double tolerance, double side[3])
{
  for (int i = 0; i < 3; ++i)
  {
    side[i] = n * tri[i] - d;
    if (fabs(side[i]) <= tolerance)
      side[i] = 0;
  }
}

static bool in_plane(const double side[3]) { return side[0] == 0 && side[1] == 0 && side[2] == 0; }

static bool crosses_plane(const double side[3])
{
  return (side[0] > 0 || side[1] > 0 || side[2] > 0) && (side[0] < 0 || side[1] < 0 || side[2] < 0);
}

struct LineInterval
{
  double lo = DBL_MAX, hi = -DBL_MAX;
  DPoint3 loPoint, hiPoint;

  void add(const DPoint3 &p, const DPoint3 &dir)
  {
    const double t = dir * p;
    if (t < lo)
    {
      lo = t;
      loPoint = p;
    }
    if (t > hi)
    {
      hi = t;
      hiPoint = p;
    }
  }
};

static LineInterval plane_section(const DPoint3 *tri, const double side[3], const DPoint3 &dir)
{
  LineInterval r;
  for (int i = 0; i < 3; ++i)
  {
    const int j = (i + 1) % 3;
    if (side[i] == 0)
      r.add(tri[i], dir);
    else if ((side[i] > 0 && side[j] < 0) || (side[i] < 0 && side[j] > 0))
      r.add(tri[i] + (tri[j] - tri[i]) * (side[i] / (side[i] - side[j])), dir);
  }
  return r;
}

static bool coplanar_overlap(const DPoint3 *a, const DPoint3 *b, const DPoint3 &n, double tolerance)
{
  const DPoint3 u = unit_or_zero(fabs(n.x) < 0.9 ? DPoint3(1, 0, 0) % n : DPoint3(0, 1, 0) % n);
  const DPoint3 v = n % u;
  double flat[2][3][2];
  for (int i = 0; i < 3; ++i)
  {
    flat[0][i][0] = u * a[i];
    flat[0][i][1] = v * a[i];
    flat[1][i][0] = u * b[i];
    flat[1][i][1] = v * b[i];
  }
  for (const auto &tri : flat)
    for (int k = 0; k < 3; ++k)
    {
      const double ex = tri[(k + 1) % 3][0] - tri[k][0], ey = tri[(k + 1) % 3][1] - tri[k][1];
      const double len = sqrt(ex * ex + ey * ey);
      if (!(len > 0))
        continue;
      double lo[2] = {DBL_MAX, DBL_MAX}, hi[2] = {-DBL_MAX, -DBL_MAX};
      for (int t = 0; t < 2; ++t)
        for (int i = 0; i < 3; ++i)
        {
          const double s = (flat[t][i][1] * ex - flat[t][i][0] * ey) / len;
          lo[t] = min(lo[t], s);
          hi[t] = max(hi[t], s);
        }
      if (min(hi[0], hi[1]) - max(lo[0], lo[1]) <= tolerance)
        return false;
    }
  return true;
}

struct FacePlane
{
  DPoint3 n;
  double d;
};

static FacePlane face_plane(const DPoint3 *p)
{
  const DPoint3 n = unit_or_zero((p[1] - p[0]) % (p[2] - p[0]));
  return FacePlane{n, n * p[0]};
}

// pa = face_plane(a), pb = face_plane(b)
static bool planes_intersect(const DPoint3 *a, const FacePlane &pa, const DPoint3 *b, const FacePlane &pb, double tolerance,
  DPoint3 &from, DPoint3 &to)
{
  if (lengthSq(pa.n) == 0 || lengthSq(pb.n) == 0)
    return false;
  double sideA[3], sideB[3];
  plane_sides(pb.n, pb.d, a, tolerance, sideA);
  plane_sides(pa.n, pa.d, b, tolerance, sideB);
  const bool coplanar = in_plane(sideA) || in_plane(sideB);
  if (!coplanar && (!crosses_plane(sideA) || !crosses_plane(sideB)))
    return false;
  from = (a[0] + a[1] + a[2]) / 3;
  to = (b[0] + b[1] + b[2]) / 3;
  if (coplanar)
    return coplanar_overlap(a, b, in_plane(sideA) ? pb.n : pa.n, tolerance);
  const DPoint3 dir = unit_or_zero(pa.n % pb.n);
  if (lengthSq(dir) == 0)
    return coplanar_overlap(a, b, pa.n, tolerance);
  const LineInterval ia = plane_section(a, sideA, dir), ib = plane_section(b, sideB, dir);
  if (!(min(ia.hi, ib.hi) - max(ia.lo, ib.lo) > tolerance))
    return false;
  from = ia.lo > ib.lo ? ia.loPoint : ib.loPoint;
  to = ia.hi < ib.hi ? ia.hiPoint : ib.hiPoint;
  return true;
}

bool triangles_intersect(const DPoint3 *a, const DPoint3 *b, double tolerance, DPoint3 &from, DPoint3 &to)
{
  return planes_intersect(a, face_plane(a), b, face_plane(b), tolerance, from, to);
}

double half_solid_angle(const DPoint3 &p, const DPoint3 &a, const DPoint3 &b, const DPoint3 &c)
{
  const DPoint3 u = a - p, v = b - p, w = c - p;
  const double lu = length(u), lv = length(v), lw = length(w);
  return atan2(u * (v % w), lu * lv * lw + (u * v) * lw + (v * w) * lu + (w * u) * lv);
}

bool winds_around(double half_angles) { return half_angles > 3.141592653589793; }

static void face_points(const Solid &s, uint32_t f, DPoint3 *p)
{
  for (int k = 0; k < 3; ++k)
    p[k] = s.pos[s.faceVert(f, k)];
}

// keeps the first hits by face pair
static void keep_first(dag::Vector<FacePairIntersection> &kept, const FacePairIntersection &hit)
{
  auto at = eastl::upper_bound(kept.begin(), kept.end(), hit, [](const FacePairIntersection &x, const FacePairIntersection &y) {
    return x.faceA != y.faceA ? x.faceA < y.faceA : x.faceB < y.faceB;
  });
  if (kept.size() == ValidationReport::MAX_REPORTED_INTERSECTIONS)
  {
    if (at == kept.end())
      return;
    kept.pop_back();
  }
  kept.insert(at, hit);
}

static void add_error(eastl::string &text, uint32_t count, const char *what)
{
  if (!count)
    return;
  text.append_sprintf("%s%u %s", text.empty() ? "" : ", ", count, what);
}

ValidationReport validate(const Solid &s, uint32_t flags)
{
  ValidationReport r;
  if (s.halfedge.size() != 3 * s.faceCount())
  {
    r.error.sprintf("%u halfedges for %u faces", uint32_t(s.halfedge.size()), s.faceCount());
    return r;
  }
  for (const DPoint3 &p : s.pos)
    if (!check_finite(p.x) || !check_finite(p.y) || !check_finite(p.z))
      ++r.nonFiniteVerts;

  dag::Vector<uint32_t> pair;
  const EdgeCounts edges = pair_halfedges(s.halfedge, s.pos.size(), pair);
  uint32_t stalePairs = 0;
  for (uint32_t h = 0; h < s.halfedge.size(); ++h)
    if (pair[h] != NO_INDEX && pair[h] != s.halfedge[h].pairedHalfedge)
      ++stalePairs;
  r.unpairedHalfedges = edges.unpaired + edges.overPaired;
  r.misorientedEdges = edges.misoriented;
  r.misalignedArrays = int(!s.provenance.empty() && s.provenance.size() != s.pos.size()) +
                       int(!s.facePlaneKey.empty() && s.facePlaneKey.size() != s.faceCount());

  const uint32_t faceCount = s.faceCount();
  const bool geometry = edges.badIndex == 0 && r.nonFiniteVerts == 0;
  if (geometry)
  {
    // touching shells duplicate the vertex, spec.md 3.2
    uint32_t fans = 0;
    dag::Vector<uint8_t> seen(edges.closed() ? s.halfedge.size() : 0, 0);
    for (uint32_t h0 = 0; h0 < seen.size(); ++h0)
      for (uint32_t h = h0; !seen[h]; h = pair[prev_he(h)])
      {
        seen[h] = 1;
        fans += h == h0;
      }
    UnionFind shells; // on pair's storage
    shells.parent.swap(pair);
    shells.init(s.vertCount());
    for (uint32_t f = 0; f < faceCount; ++f)
    {
      shells.join(s.faceVert(f, 0), s.faceVert(f, 1));
      shells.join(s.faceVert(f, 0), s.faceVert(f, 2));
    }
    dag::Vector<uint32_t> shellOf;
    const uint32_t shellCount = shells.compress(shellOf);
    const auto shell = [&](uint32_t f) { return shellOf[s.faceVert(f, 0)]; };
    dag::Vector<double> shellVolume(shellCount, 0.0);
    dag::Vector<BBoxD> shellBox(shellCount);
    for (uint32_t v = 0; v < s.vertCount(); ++v)
      shellBox[shellOf[v]] += s.pos[v];
    dag::Vector<uint8_t> used(s.pos.size(), 0);
    for (uint32_t f = 0; f < faceCount; ++f)
    {
      for (int k = 0; k < 3; ++k)
        used[s.faceVert(f, k)] = 1;
      const DPoint3 cross = face_cross(s, f);
      if (!(lengthSq(cross) > 0))
        ++r.zeroAreaFaces;
      shellVolume[shell(f)] += s.pos[s.faceVert(f, 0)] * cross;
    }
    dag::Vector<FaceKey> keys;
    face_keys(s, keys);
    for (uint32_t i = 1; i < keys.size(); ++i)
      r.duplicateFaces += keys[i].sameVerts(keys[i - 1]) && keys[i].odd == keys[i - 1].odd;
    const int usedVerts = eastl::count(used.begin(), used.end(), 1);
    r.eulerCharacteristic = usedVerts - int(edges.edges) + int(faceCount);
    r.extraFans = seen.empty() ? 0 : fans - usedVerts;
    r.signedVolume = signed_volume(s);

    // an inward shell is a cavity: the other shells wind around a point of it
    for (uint32_t f = 0; f < faceCount; ++f)
      if (const uint32_t own = shell(f); shellVolume[own] < 0)
      {
        shellVolume[own] = 0;
        const DPoint3 p = (s.pos[s.faceVert(f, 0)] + s.pos[s.faceVert(f, 1)] + s.pos[s.faceVert(f, 2)]) / 3;
        double halfAngles = 0;
        // a shell whose box misses p winds zero
        for (uint32_t g = 0; g < faceCount; ++g)
          if (shell(g) != own && !bboxes_disjoint(shellBox[shell(g)], BBoxD{{p, p}}, 0))
            halfAngles += half_solid_angle(p, s.pos[s.faceVert(g, 0)], s.pos[s.faceVert(g, 1)], s.pos[s.faceVert(g, 2)]);
        r.strayInwardShells += !winds_around(halfAngles);
      }

    double tolerance = 0;
    if (flags & VALIDATE_SELF_INTERSECTION)
      r.selfIntersectionTested = auto_tolerance(points_bbox(s.pos), tolerance, r.error);
    if (r.selfIntersectionTested)
    {
      Bvh bvh;
      bvh.build(s);
      dag::Vector<TriPair> pairs;
      // repeated faces make the candidates quadratic: refuse them
      const uint32_t limit = 64 * faceCount + 65536;
      bvh.selfOverlapsUnsorted(pairs, limit);
      r.selfIntersectionTested = pairs.size() <= limit;
      if (!r.selfIntersectionTested)
      {
        r.error.append_sprintf("over %u candidate pairs for %u faces", limit, faceCount);
        pairs.clear();
      }
      dag::Vector<FacePlane> planes(pairs.empty() ? 0 : faceCount);
      for (uint32_t f = 0; f < planes.size(); ++f)
      {
        DPoint3 p[3];
        face_points(s, f, p);
        planes[f] = face_plane(p);
      }
      for (const TriPair &p : pairs)
      {
        if (skin(s, p.first, p.second))
          continue;
        DPoint3 a[3], b[3];
        face_points(s, p.first, a);
        face_points(s, p.second, b);
        FacePairIntersection hit;
        if (!planes_intersect(a, planes[p.first], b, planes[p.second], tolerance, hit.from, hit.to))
          continue;
        ++r.selfIntersectingPairs;
        hit.faceA = p.first;
        hit.faceB = p.second;
        keep_first(r.firstIntersections, hit);
      }
    }
  }

  add_error(r.error, r.nonFiniteVerts, "non-finite vertices");
  add_error(r.error, edges.badIndex, "halfedges with a missing vertex");
  add_error(r.error, r.unpairedHalfedges, "unpaired halfedges");
  add_error(r.error, stalePairs, "stale pairedHalfedge entries");
  add_error(r.error, r.misorientedEdges, "misoriented edges");
  add_error(r.error, r.duplicateFaces, "duplicate faces");
  add_error(r.error, r.zeroAreaFaces, "zero-area faces");
  add_error(r.error, r.misalignedArrays, "misaligned provenance and facePlaneKey arrays");
  add_error(r.error, r.selfIntersectingPairs, "self-intersecting face pairs");
  add_error(r.error, r.strayInwardShells, "inward shells outside every shell");
  add_error(r.error, r.extraFans, "extra vertex fans");
  if (geometry && faceCount && !(r.signedVolume > 0))
    r.error.append_sprintf("%ssigned volume %g", r.error.empty() ? "" : ", ", r.signedVolume);
  r.valid = r.error.empty();
  return r;
}
} // namespace dacsg
