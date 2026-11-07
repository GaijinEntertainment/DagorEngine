// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "cleanup/cleanup.h"
#include "cleanup/overlap.h"
#include "mesh/bvh.h"
#include "mesh/polygon.h"
#include "mesh/solidUtil.h"
#include "mesh/unionFind.h"
#include "validate/intersect.h"
#include <debug/dag_log.h>
#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
#include <float.h>
#include <math.h>

namespace dacsg
{
// a chain of flat faces along one line resolves one face per sweep from its ends
static constexpr int MAX_SWEEPS = 128;
static constexpr double DEG = 3.141592653589793 / 180;
static constexpr double CREASE_COS = 0.7071067811865476; // dihedral above 45 deg

enum FaceState : uint8_t
{
  LIVE,
  TOUCHED, // changed this sweep: stale pairs
  DEAD,
};

static DPoint3 tri_cross(const DPoint3 &a, const DPoint3 &b, const DPoint3 &c) { return (b - a) % (c - a); }

static bool share_edge(const uint32_t *a, const uint32_t *b)
{
  int common = 0;
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      common += a[i] == b[j];
  return common >= 2;
}

static double point_segment_dist_sq(const DPoint3 &p, const DPoint3 &a, const DPoint3 &b)
{
  const DPoint3 e = b - a;
  const double ee = e * e;
  return lengthSq(a + e * (ee > 0 ? clamp((p - a) * e / ee, 0.0, 1.0) : 0.0) - p);
}

// a face as an edit would leave it, with the outward planes of its edges
struct NewFace
{
  uint32_t v[3];
  DPoint3 p[3], edgeN[3];
  double edgeD[3];
  bbox3f box;
};

static void finish_face(NewFace &n)
{
  const DPoint3 normal = tri_cross(n.p[0], n.p[1], n.p[2]);
  for (int e = 0; e < 3; ++e)
  {
    n.edgeN[e] = unit_or_zero((n.p[(e + 1) % 3] - n.p[e]) % normal);
    n.edgeD[e] = n.edgeN[e] * n.p[e];
  }
  n.box = outward_box(min(min(n.p[0], n.p[1]), n.p[2]), max(max(n.p[0], n.p[1]), n.p[2]));
}

// b lies beyond an edge of a by more than tol but for its corners on that edge, so the two at most touch
static bool beyond_edge(const NewFace &a, const NewFace &b, double tol)
{
  for (int e = 0; e < 3; ++e)
  {
    int k = 0;
    while (k < 3 && (b.v[k] == a.v[e] || b.v[k] == a.v[(e + 1) % 3] || a.edgeN[e] * b.p[k] - a.edgeD[e] > tol))
      ++k;
    if (k == 3)
      return true;
  }
  return false;
}

// operations of one sweep read only LIVE faces
struct Cleaner
{
  Solid &s;
  const CleanupParams &params;
  const double tol, maxMoveSq, flipCos;
  CleanupStats &stats;
  const ChartTest *charts;
  dag::Vector<uint8_t> state;
  dag::Vector<uint32_t> valence, outHe, pair, candidates, fanA, fanB, oldFaces, nearQuery;
  dag::Vector<uint32_t> vertFirst, nameNext, nameFace; // per vertex a list of faces that may name it, one overlap sweep long
  // the pairing as the last one left it: what a sweep must pair again
  dag::Vector<uint32_t> open, loose, heStart, heMark, changed, repair, lostVerts; // loose: partners of rewritten halfedges
  uint32_t heGen = 0;
  dag::Vector<NewFace> newFaces;
  dag::Vector<TriPair> skins;
  Solid backup;
  dag::Vector<FaceShape> shape; // per face, current for a face no edit touched since
  bool shapeStale = true;       // an overlap renames vertices without touching faces: shapes and counts both go stale
  Bvh nearBvh;
  dag::Vector<bbox3f> faceBox;
  dag::Vector<uint32_t> nearPending, newId; // nearPending: new boxes since the tree's last refit
  uint32_t sweeps = 0, bvhSweep = NO_INDEX; // NO_INDEX: build anew
  // of the mesh as it is, from the last step that took them
  double knownVolume = 0;
  int knownGenus = 0;
  bool volumeKnown = false, genusKnown = false;
  // merge step state
  dag::Vector<uint32_t> order, faceStamp, vertStamp, visitStamp, boundaryOut, loopVerts, loopStart, newTris, tris, loopIds;
  dag::Vector<DPoint2> loopPts; // per item of loopVerts
  dag::Vector<double> loopArea;
  dag::Vector<dag::ConstSpan<DPoint2>> holeSpans;
  dag::Vector<TriPair> groups;
  uint32_t stamp = 0;
  dag::Vector<OverlapGroup> overlapGroups;
  OverlapEdit overlapEdit;
  dag::Vector<uint8_t> liveMask;

  Cleaner(Solid &solid, const CleanupParams &p, double tolerance, CleanupStats &st, const ChartTest *ct) :
    s(solid),
    params(p),
    tol(tolerance),
    maxMoveSq(sqr(max(p.maxMoveFactor * tolerance, 0.0))),
    flipCos(cos(p.flipDihedralDeg * DEG)),
    stats(st),
    charts(ct)
  {}

  void computeShapes()
  {
    shape.resize(s.faceCount());
    for (uint32_t f = 0; f < s.faceCount(); ++f)
      if (state[f] != DEAD)
        shape[f] = face_shape(s, f);
    shapeStale = false;
  }

  void rebuild()
  {
    uint32_t live = 0;
    // the near tree outlives a compaction while it holds a box for every face
    const bool keepTree = bvhSweep != NO_INDEX && faceBox.size() == state.size();
    newId.assign(keepTree ? state.size() : 0, NO_INDEX);
    for (uint32_t f = 0; f < state.size(); ++f)
      if (state[f] != DEAD)
      {
        for (int k = 0; k < 3; ++k)
          s.halfedge[3 * live + k] = s.halfedge[3 * f + k];
        s.face[live] = s.face[f];
        s.leafRef[live] = s.leafRef[f];
        if (!s.facePlaneKey.empty())
          s.facePlaneKey[live] = s.facePlaneKey[f];
        if (!shapeStale)
          shape[live] = shape[f];
        if (keepTree)
        {
          newId[f] = live;
          faceBox[live] = faceBox[f];
        }
        ++live;
      }
    s.halfedge.resize(3 * live);
    s.face.resize(live);
    s.leafRef.resize(live);
    if (!s.facePlaneKey.empty())
      s.facePlaneKey.resize(live);
    if (keepTree)
    {
      faceBox.resize(live);
      nearBvh.renumber(make_span_const(newId));
      for (uint32_t &f : nearPending)
        f = newId[f];
      nearPending.erase(eastl::remove(nearPending.begin(), nearPending.end(), NO_INDEX), nearPending.end());
    }
    else
    {
      bvhSweep = NO_INDEX;
      nearPending.clear();
    }
    pair_halfedges(s.halfedge, s.pos.size(), pair);
    open.clear();
    loose.clear();
    for (uint32_t h = 0; h < s.halfedge.size(); ++h)
    {
      s.halfedge[h].pairedHalfedge = pair[h];
      if (pair[h] == NO_INDEX)
        open.push_back(h);
    }
    state.assign(live, LIVE);
    countAll();
    if (shapeStale)
      computeShapes();
    shape.resize(live);
  }

  void countAll()
  {
    valence.assign(s.pos.size(), 0);
    outHe.assign(s.pos.size(), NO_INDEX);
    heStart.assign(s.halfedge.size(), NO_INDEX);
    for (uint32_t h = 0; h < s.halfedge.size(); ++h)
      if (state[h / 3] != DEAD)
      {
        heStart[h] = s.halfedge[h].startVert;
        ++valence[heStart[h]];
        outHe[heStart[h]] = h;
      }
  }

  // outHe: any live halfedge
  void countChanged()
  {
    lostVerts.clear();
    for (uint32_t f : changed)
      for (uint32_t h = 3 * f; h < 3 * f + 3; ++h)
      {
        const uint32_t was = heStart[h], now = state[f] == DEAD ? NO_INDEX : s.halfedge[h].startVert;
        if (was == now)
          continue;
        heStart[h] = now;
        if (was != NO_INDEX)
        {
          --valence[was];
          if (outHe[was] == h)
          {
            outHe[was] = NO_INDEX;
            lostVerts.push_back(was);
          }
        }
        if (now != NO_INDEX)
        {
          ++valence[now];
          if (outHe[now] == NO_INDEX)
            outHe[now] = h;
        }
      }
    if (lostVerts.empty())
      return;
    for (uint32_t f : changed)
      for (uint32_t h = 3 * f; h < 3 * f + 3 && state[f] != DEAD; ++h)
        if (outHe[s.halfedge[h].startVert] == NO_INDEX)
          outHe[s.halfedge[h].startVert] = h;
    for (uint32_t v : lostVerts)
      if (outHe[v] == NO_INDEX && valence[v]) // only untouched faces left: count all
      {
        countAll();
        return;
      }
  }

  // After a sweep of few edits only the halfedges of a touched face, those whose partner's face is no longer live
  // and those without a partner pair again; dead faces stay in place until the step ends, so every other pair
  // keeps its index. A chain of flat faces takes a sweep per face, and this keeps each such sweep local.
  void rebuildLocal()
  {
    dag::Vector<EdgeKey> keys;
    changed.clear();
    heStart.resize(s.halfedge.size(), NO_INDEX);
    // a face dead since an earlier sweep has no start left
    for (uint32_t f = 0; f < state.size(); ++f)
      if (state[f] == TOUCHED || (state[f] == DEAD && heStart[3 * f] != NO_INDEX))
        changed.push_back(f);
    // a changed face names its live partners
    heMark.resize(s.halfedge.size(), 0);
    ++heGen;
    repair.clear();
    const auto add = [&](uint32_t h) {
      if (h != NO_INDEX && heMark[h] != heGen && state[h / 3] != DEAD)
      {
        heMark[h] = heGen;
        repair.push_back(h);
      }
    };
    for (uint32_t f : changed)
      for (uint32_t h = 3 * f; h < 3 * f + 3; ++h)
      {
        add(s.halfedge[h].pairedHalfedge);
        if (state[f] == TOUCHED)
          add(h);
        else
          s.halfedge[h].pairedHalfedge = NO_INDEX;
      }
    for (uint32_t h : loose)
      add(h);
    for (uint32_t h : open)
      add(h);
    loose.clear();
    for (uint32_t h : repair)
    {
      Halfedge &e = s.halfedge[h];
      e.pairedHalfedge = NO_INDEX;
      if (e.startVert != e.endVert)
        keys.push_back(EdgeKey{edge_key(e.startVert, e.endVert), h});
    }
    eastl::sort(keys.begin(), keys.end());
    for (uint32_t i = 0; i < keys.size();)
    {
      uint32_t j = i + 1;
      while (j < keys.size() && keys[j].key == keys[i].key)
        ++j;
      const uint32_t a = keys[i].he, b = keys[j - 1].he;
      if (j - i == 2 && s.halfedge[a].startVert != s.halfedge[b].startVert)
      {
        s.halfedge[a].pairedHalfedge = b;
        s.halfedge[b].pairedHalfedge = a;
      }
      i = j;
    }
    open.clear();
    for (uint32_t h : repair)
      if (s.halfedge[h].pairedHalfedge == NO_INDEX)
        open.push_back(h);
    if (shapeStale)
      countAll();
    else
      countChanged();
    shape.resize(state.size());
    for (uint32_t f : changed)
    {
      if (f < faceBox.size() && state[f] == TOUCHED)
      {
        faceBox[f] = face_box(s, f);
        nearPending.push_back(f);
      }
      if (state[f] == TOUCHED && !shapeStale)
        shape[f] = face_shape(s, f);
      state[f] = state[f] == DEAD ? DEAD : LIVE;
    }
    if (shapeStale)
      computeShapes();
  }

  // twice the genus summed over the shells: 2 shells - (V - E + F) of the live faces; a step may not change it
  int genusTwice() const
  {
    UnionFind shell;
    shell.init(s.faceCount());
    dag::Vector<uint8_t> used(s.pos.size(), 0);
    int edges = 0, faces = 0, verts = 0, shells = 0;
    for (uint32_t h = 0; h < s.halfedge.size(); ++h)
    {
      if (state[h / 3] == DEAD)
        continue;
      const uint32_t t = s.halfedge[h].pairedHalfedge;
      edges += t == NO_INDEX || h < t;
      if (t != NO_INDEX)
        shell.join(h / 3, t / 3);
      used[s.halfedge[h].startVert] = 1;
    }
    for (uint32_t f = 0; f < s.faceCount(); ++f)
      if (state[f] != DEAD)
      {
        ++faces;
        shells += shell.find(f) == f;
      }
    for (uint8_t u : used)
      verts += u;
    return 2 * shells - (verts - edges + faces);
  }

  // false unless one closed fan of LIVE faces
  bool fan(uint32_t v, dag::Vector<uint32_t> &out) const
  {
    out.clear();
    uint32_t h = outHe[v];
    while (h != NO_INDEX && state[h / 3] == LIVE && out.size() < valence[v])
    {
      out.push_back(h);
      h = s.halfedge[prev_he(h)].pairedHalfedge;
      if (h != outHe[v])
        continue;
      return out.size() == valence[v];
    }
    return false;
  }

  void cornersWith(uint32_t f, uint32_t vert, const DPoint3 &at, DPoint3 p[3]) const
  {
    for (int k = 0; k < 3; ++k)
      p[k] = s.faceVert(f, k) == vert ? at : s.pos[s.faceVert(f, k)];
  }

  // two faces on the edge p-q lie on each other when their thirds c and d leave the edge the same way; the test
  // reads positions, so a sliver's noisy normal cannot hide a fold; a third within the tolerance of the edge's line
  // leaves the edge no way at all
  bool folded(const DPoint3 &p, const DPoint3 &q, const DPoint3 &c, const DPoint3 &d) const
  {
    const DPoint3 e = q - p;
    const double ee = e * e;
    if (!(ee > 0))
      return false;
    const DPoint3 u = (c - p) - e * (((c - p) * e) / ee), v = (d - p) - e * (((d - p) * e) / ee);
    return lengthSq(u) > tol * tol && lengthSq(v) > tol * tol && u * v > 0.999 * length(u) * length(v);
  }

  // the third vertex of the face across halfedge t, with vert moved to at
  DPoint3 thirdAcross(uint32_t t, uint32_t vert, const DPoint3 &at) const
  {
    DPoint3 q[3];
    cornersWith(t / 3, vert, at, q);
    return q[(t % 3 + 2) % 3];
  }

  // a face of the fan that would lie on its neighbour across an edge once vert is at at; a neighbour this sweep has
  // touched already has a stale pairing, so the edit waits for the next sweep
  bool foldsAfter(const dag::Vector<uint32_t> &fan_of, uint32_t vert, const DPoint3 &at, uint32_t skip0, uint32_t skip1) const
  {
    for (uint32_t h : fan_of)
    {
      const uint32_t f = h / 3;
      if (f == skip0 || f == skip1)
        continue;
      DPoint3 p[3];
      cornersWith(f, vert, at, p);
      for (int k = 0; k < 3; ++k)
      {
        const uint32_t t = s.halfedge[3 * f + k].pairedHalfedge;
        if (t == NO_INDEX || t / 3 == skip0 || t / 3 == skip1)
          continue;
        if (state[t / 3] != LIVE || folded(p[k], p[(k + 1) % 3], p[(k + 2) % 3], thirdAcross(t, vert, at)))
          return true;
      }
    }
    return false;
  }

  bool foldsAcross(uint32_t t, const DPoint3 &p, const DPoint3 &q, const DPoint3 &third, uint32_t vert, const DPoint3 &at) const
  {
    return t != NO_INDEX && (state[t / 3] != LIVE || folded(p, q, third, thirdAcross(t, vert, at)));
  }

  // squared distance of the moved corner at to the plane of each old face of the fan: a face that tilts is a changed
  // surface along its whole span even when the old vertex itself stays close to the new fan. A sliver's plane is
  // noise, so it reads the distance of its old centroid to the moved face instead
  double deviationSq(const dag::Vector<uint32_t> &fan_of, uint32_t vert, const DPoint3 &at, uint32_t skip0, uint32_t skip1) const
  {
    double d = 0;
    for (uint32_t h : fan_of)
      if (h / 3 != skip0 && h / 3 != skip1)
      {
        const FaceShape &fs = shape[h / 3];
        if (fs.height > tol && !is_sliver(fs, params))
        {
          d = max(d, sqr(unit_or_zero(face_cross(s, h / 3)) * (at - s.pos[vert])));
          continue;
        }
        DPoint3 p[3];
        cornersWith(h / 3, vert, at, p);
        const DPoint3 old = (s.pos[s.faceVert(h / 3, 0)] + s.pos[s.faceVert(h / 3, 1)] + s.pos[s.faceVert(h / 3, 2)]) / 3;
        d = max(d, point_triangle_dist_sq(old, p[0], p[1], p[2]));
      }
    return d;
  }

  // squared distance of the old position of vert to the fan with vert at at: a slide along the surface costs nothing,
  // a move across a gap costs the gap, whatever the centroids of the faces see
  double moveSq(const dag::Vector<uint32_t> &fan_of, uint32_t vert, const DPoint3 &at, uint32_t skip0, uint32_t skip1) const
  {
    double d = DBL_MAX;
    for (uint32_t h : fan_of)
      if (h / 3 != skip0 && h / 3 != skip1)
      {
        DPoint3 p[3];
        cornersWith(h / 3, vert, at, p);
        d = min(d, point_triangle_dist_sq(s.pos[vert], p[0], p[1], p[2]));
      }
    return d == DBL_MAX ? 0 : d;
  }

  // squared shift of the borders through vert moved to at; a slide along a straight border is free
  double borderMoveSq(const dag::Vector<uint32_t> &fan_of, uint32_t vert, const DPoint3 &at) const
  {
    if (!charts)
      return 0;
    uint32_t ends[2], count = 0;
    for (uint32_t h : fan_of)
      if (!charts->sameChart(s.leafRef[h / 3], s.leafRef[s.halfedge[h].pairedHalfedge / 3]))
        ends[min(count++, 1u)] = s.halfedge[h].endVert;
    if (count != 2)
      return count ? lengthSq(at - s.pos[vert]) : 0;
    const double d = sqrt(point_segment_dist_sq(s.pos[vert], s.pos[ends[0]], s.pos[ends[1]])) +
                     sqrt(point_segment_dist_sq(at, s.pos[ends[0]], s.pos[ends[1]]));
    return d * d;
  }

  NewFace newFace(const uint32_t v[3], uint32_t vert, const DPoint3 &at) const
  {
    NewFace n;
    for (int k = 0; k < 3; ++k)
      n.p[k] = (n.v[k] = v[k]) == vert ? at : s.pos[v[k]];
    finish_face(n);
    return n;
  }

  // the face f with vert renamed to as and moved to at; by default as it is
  NewFace newFace(uint32_t f, uint32_t vert = NO_INDEX, uint32_t as = NO_INDEX, const DPoint3 &at = DPoint3(0, 0, 0)) const
  {
    uint32_t v[3];
    for (int k = 0; k < 3; ++k)
      v[k] = s.faceVert(f, k) == vert ? as : s.faceVert(f, k);
    return newFace(v, as, at);
  }

  // the BVH of the sweep's start
  void updateNearBvh()
  {
    if (bvhSweep == sweeps)
      return;
    if (bvhSweep == NO_INDEX)
    {
      faceBox.resize(s.faceCount());
      for (uint32_t f = 0; f < s.faceCount(); ++f)
        faceBox[f] = face_box(s, f);
      nearBvh.build(make_span_const(faceBox));
    }
    else
      nearBvh.refit(make_span_const(faceBox), make_span_const(nearPending));
    nearPending.clear();
    bvhSweep = sweeps;
  }

  // edge planes first: in a dense fan most pairs end there, at a tenth of the full test
  bool crosses(const NewFace &a, const NewFace &b) const
  {
    DPoint3 from, to;
    return !share_edge(a.v, b.v) && v_bbox3_test_box_intersect(a.box, b.box) && !beyond_edge(a, b, tol) && !beyond_edge(b, a, tol) &&
           triangles_intersect(a.p, b.p, tol, from, to);
  }

  // the new faces of an edit may not cross each other, or add a crossing with a face near the path of the moved vertex
  // (from - to), where the surface changes most; the wide check takes every face near the new faces, as a moved vertex
  // tilts its faces out to their far edges. A face that crossed the old fan too is no new crossing, a pair on a shared
  // edge is the fold check's
  bool crossesAfter(const DPoint3 &from, const DPoint3 &to)
  {
    for (uint32_t i = 0; i < newFaces.size(); ++i)
      for (uint32_t j = i + 1; j < newFaces.size(); ++j)
        if (crosses(newFaces[i], newFaces[j]))
          return true;
    DPoint3 lo = min(from, to), hi = max(from, to);
    for (uint32_t i = 0; i < newFaces.size() && params.wideCrossingCheck; ++i)
      for (const DPoint3 &p : newFaces[i].p)
      {
        lo = min(lo, p);
        hi = max(hi, p);
      }
    updateNearBvh();
    const DPoint3 reach(sqrt(maxMoveSq) + tol, sqrt(maxMoveSq) + tol, sqrt(maxMoveSq) + tol);
    nearBvh.overlapping(outward_box(lo - reach, hi + reach), nearQuery);
    for (uint32_t f : nearQuery)
    {
      if (state[f] == DEAD || eastl::find(oldFaces.begin(), oldFaces.end(), f) != oldFaces.end())
        continue;
      const NewFace g = newFace(f);
      if (eastl::any_of(newFaces.begin(), newFaces.end(), [&](const NewFace &n) { return crosses(n, g); }) &&
          eastl::none_of(oldFaces.begin(), oldFaces.end(), [&](uint32_t o) { return crosses(newFace(o), g); }))
        return true;
    }
    return false;
  }

  bool keepsNormals(const dag::Vector<uint32_t> &fan_of, uint32_t vert, const DPoint3 &at, uint32_t skip0, uint32_t skip1) const
  {
    for (uint32_t h : fan_of)
    {
      const DPoint3 before = face_cross(s, h / 3);
      DPoint3 p[3];
      cornersWith(h / 3, vert, at, p);
      if (h / 3 != skip0 && h / 3 != skip1 && lengthSq(before) > 0 && !(before * tri_cross(p[0], p[1], p[2]) > 0))
        return false;
    }
    return true;
  }

  double moveError(const dag::Vector<uint32_t> &fan_of, const DPoint3 &t) const
  {
    double e = 0;
    for (uint32_t h : fan_of)
      e += fabs(face_cross(s, h / 3) * t);
    return e;
  }

  bool tryCollapse(uint32_t gone, uint32_t keep, const dag::Vector<uint32_t> &fan_gone, const dag::Vector<uint32_t> &fan_keep,
    uint32_t f0, uint32_t f1)
  {
    if (!(deviationSq(fan_gone, gone, s.pos[keep], f0, f1) <= maxMoveSq) ||
        !(moveSq(fan_gone, gone, s.pos[keep], f0, f1) <= maxMoveSq) || !(borderMoveSq(fan_gone, gone, s.pos[keep]) <= maxMoveSq) ||
        !keepsNormals(fan_gone, gone, s.pos[keep], f0, f1) || foldsAfter(fan_gone, gone, s.pos[keep], f0, f1))
      return false;
    for (uint32_t h : fan_gone)
      for (uint32_t k : fan_keep)
      {
        const uint32_t x = s.halfedge[h].endVert, y = s.halfedge[next_he(h)].endVert;
        const uint32_t a = s.halfedge[k].endVert, b = s.halfedge[next_he(k)].endVert;
        if ((a == x && b == y) || (a == y && b == x))
          return false;
        // the two faces newly paired across an apex edge may not lie on each other
        if ((x == b && folded(s.pos[keep], s.pos[x], s.pos[y], s.pos[a])) ||
            (y == a && folded(s.pos[keep], s.pos[y], s.pos[x], s.pos[b])))
          return false;
      }
    newFaces.clear();
    oldFaces.clear();
    for (uint32_t h : fan_gone)
      oldFaces.push_back(h / 3);
    for (uint32_t h : fan_gone)
      if (h / 3 != f0 && h / 3 != f1)
        newFaces.push_back(newFace(h / 3, gone, keep, s.pos[keep]));
    if (crossesAfter(s.pos[gone], s.pos[keep]))
      return false;
    for (uint32_t h : fan_gone)
    {
      const uint32_t f = h / 3;
      state[f] = f == f0 || f == f1 ? DEAD : TOUCHED;
      for (int k = 0; k < 3; ++k)
      {
        Halfedge &e = s.halfedge[3 * f + k];
        e.startVert = e.startVert == gone ? keep : e.startVert;
        e.endVert = e.endVert == gone ? keep : e.endVert;
      }
    }
    for (uint32_t k : fan_keep)
      state[k / 3] = state[k / 3] == LIVE ? TOUCHED : state[k / 3];
    return true;
  }

  // link condition: only the two apexes are shared
  bool collapseEdge(uint32_t he)
  {
    const uint32_t twin = s.halfedge[he].pairedHalfedge;
    if (twin == NO_INDEX || state[he / 3] != LIVE || state[twin / 3] != LIVE)
      return false;
    const uint32_t v = s.halfedge[he].startVert, w = s.halfedge[he].endVert;
    const uint32_t apex0 = s.halfedge[next_he(he)].endVert, apex1 = s.halfedge[next_he(twin)].endVert;
    if (apex0 == apex1 || !fan(v, fanA) || !fan(w, fanB))
      return false;
    uint32_t common = 0;
    for (uint32_t a : fanA)
      for (uint32_t b : fanB)
        if (s.halfedge[a].endVert == s.halfedge[b].endVert)
        {
          if (s.halfedge[a].endVert != apex0 && s.halfedge[a].endVert != apex1)
            return false;
          ++common;
        }
    if (common != 2)
      return false;
    const DPoint3 t = s.pos[w] - s.pos[v];
    if (moveError(fanA, t) <= moveError(fanB, t))
      return tryCollapse(v, w, fanA, fanB, he / 3, twin / 3) || tryCollapse(w, v, fanB, fanA, he / 3, twin / 3);
    return tryCollapse(w, v, fanB, fanA, he / 3, twin / 3) || tryCollapse(v, w, fanA, fanB, he / 3, twin / 3);
  }

  bool flat(const DPoint3 &n1, const DPoint3 &n2) const { return n1 * n2 > flipCos * length(n1) * length(n2); }

  // a flip keeps c; split and collapse moves c onto ab
  bool fixCap(uint32_t f, const FaceShape &fs, bool flip_only)
  {
    const uint32_t hab = 3 * f + fs.longest, twin = s.halfedge[hab].pairedHalfedge;
    if (twin == NO_INDEX || state[twin / 3] != LIVE)
      return false;
    const uint32_t g = twin / 3, a = s.halfedge[hab].startVert, b = s.halfedge[hab].endVert;
    const uint32_t c = s.halfedge[next_he(hab)].endVert, d = s.halfedge[next_he(twin)].endVert;
    if (c == d || !fan(c, fanA))
      return false;
    // the tests below are pure: cheapest per reject first
    const DPoint3 ng = face_cross(s, g), &pa = s.pos[a], &pb = s.pos[b], &pd = s.pos[d];
    DPoint3 to = s.pos[c];
    const DPoint3 n1 = tri_cross(to, pa, pd), n2 = tri_cross(pb, to, pd);
    // a sliver's own normal is noise: its apex must lie in the plane of g, on the far side of ab, or the flip folds;
    // the dihedral test takes the new faces, whose normals are sound, against g and against each other
    if (flip_only && (!same_look(s, f, g, charts) || fabs((to - pd) * ng) > tol * length(ng) || !(tri_cross(pa, pb, to) * ng > 0) ||
                       !flat(n1, ng) || !flat(n2, ng) || !flat(n1, n2)))
      return false;
    const bool split = !flip_only && (fs.height > tol || !flat(n1, n2));
    if (split)
      to = DPoint3(Point3(pa + (pb - pa) * fs.foot)); // the Mesh stores floats, spec 5.2
    if (!(tri_cross(to, pa, pd) * ng > 0 && tri_cross(pb, to, pd) * ng > 0))
      return false;
    const double worst = min(fs.minAngleDeg, shape[g].minAngleDeg);
    const FaceShape new1 = triangle_shape(to, pa, pd), new2 = triangle_shape(pb, to, pd);
    // the minimum angle must grow, or flips cycle; a sliver flip that leaves a sliver is no fix
    if (!(min(new1.minAngleDeg, new2.minAngleDeg) > worst) || (flip_only && (is_sliver(new1, params) || is_sliver(new2, params))))
      return false;
    if (split)
    {
      const double movedSq = min(moveSq(fanA, c, to, f, f),
        min(point_triangle_dist_sq(s.pos[c], to, pa, pd), point_triangle_dist_sq(s.pos[c], pb, to, pd)));
      if (!(deviationSq(fanA, c, to, f, f) <= maxMoveSq) || !(movedSq <= maxMoveSq) || !(borderMoveSq(fanA, c, to) <= maxMoveSq) ||
          !keepsNormals(fanA, c, to, f, f))
        return false;
    }
    // neither the moved fan nor the two new faces may lie on a neighbour
    const uint32_t ca = s.halfedge[prev_he(hab)].pairedHalfedge, bc = s.halfedge[next_he(hab)].pairedHalfedge;
    const uint32_t ad = s.halfedge[next_he(twin)].pairedHalfedge, db = s.halfedge[prev_he(twin)].pairedHalfedge;
    if (foldsAfter(fanA, c, to, f, g) || foldsAcross(ca, to, pa, pd, c, to) || foldsAcross(bc, pb, to, pd, c, to) ||
        foldsAcross(ad, pa, pd, to, c, to) || foldsAcross(db, pd, pb, to, c, to) ||
        eastl::any_of(fanA.begin(), fanA.end(), [&](uint32_t h) { return s.halfedge[h].endVert == d; }))
      return false;
    newFaces.clear();
    oldFaces.clear();
    oldFaces.push_back(g);
    for (uint32_t h : fanA)
      oldFaces.push_back(h / 3);
    for (uint32_t h : fanA)
      if (h / 3 != f)
        newFaces.push_back(newFace(h / 3, c, c, to));
    const uint32_t cad[3] = {c, a, d}, bcd[3] = {b, c, d};
    newFaces.push_back(newFace(cad, c, to));
    newFaces.push_back(newFace(bcd, c, to));
    if (crossesAfter(s.pos[c], to))
      return false;
    for (uint32_t h : {3 * f, 3 * f + 1, 3 * f + 2, 3 * g, 3 * g + 1, 3 * g + 2})
      loose.push_back(s.halfedge[h].pairedHalfedge);
    s.halfedge[3 * f] = Halfedge{c, a, NO_INDEX};
    s.halfedge[3 * f + 1] = Halfedge{a, d, NO_INDEX};
    s.halfedge[3 * f + 2] = Halfedge{d, c, NO_INDEX};
    s.halfedge[3 * g] = Halfedge{b, c, NO_INDEX};
    s.halfedge[3 * g + 1] = Halfedge{c, d, NO_INDEX};
    s.halfedge[3 * g + 2] = Halfedge{d, b, NO_INDEX};
    s.face[f] = s.face[g];
    s.leafRef[f] = s.leafRef[g];
    if (!s.facePlaneKey.empty())
      s.facePlaneKey[f] = s.facePlaneKey[g];
    // a moved apex lies on its planes no more
    if (!s.provenance.empty() && !(to == s.pos[c]))
      s.provenance[c] = Provenance();
    s.pos[c] = to;
    state[g] = TOUCHED;
    for (uint32_t h : fanA)
      state[h / 3] = TOUCHED;
    return true;
  }

  // a sliver first tries a flip; crease slivers take the cap rule
  bool fixFace(uint32_t f, bool sliver_step)
  {
    if (state[f] != LIVE)
      return false;
    const FaceShape &fs = shape[f];
    if (sliver_step && fixCap(f, fs, true))
      return true;
    bool cap = max(fs.height, tol) < min(fs.foot, 1 - fs.foot) * fs.edgeLen[fs.longest];
    const uint32_t twin = s.halfedge[3 * f + fs.longest].pairedHalfedge;
    if (!cap && sliver_step && twin != NO_INDEX)
    {
      const DPoint3 nf = face_cross(s, f), ng = face_cross(s, twin / 3);
      cap = nf * ng < CREASE_COS * length(nf) * length(ng);
    }
    const uint32_t shortEdge = 3 * f + fs.shortest;
    return (cap && fixCap(f, fs, false)) || collapseEdge(shortEdge) || (!cap && fixCap(f, fs, false));
  }

  // a step that opens edges of the closed solid, moves the volume by more than the move bound times the area, or
  // changes the genus of the shells, is undone; pair_all: pairs the whole mesh before that test, where a new edge
  // can meet one the local pairing kept
  template <typename Collect, typename Fix>
  uint32_t runStep(const char *name, uint32_t bit, Collect collect, Fix fix, int max_sweeps = MAX_SWEEPS, bool keep_genus = true,
    bool pair_all = false)
  {
    uint32_t fixes = 0;
    double volume = 0, bound = 0;
    int genus = 0;
    for (int sweep = 0; sweep < max_sweeps; ++sweep)
    {
      ++sweeps;
      candidates.clear();
      collect();
      if (candidates.empty())
        break;
      if (sweep == 0)
      {
        backup = s;
        volume = volumeKnown ? knownVolume : signed_volume(s);
        for (uint32_t f = 0; f < s.faceCount(); ++f)
          bound += length(face_cross(s, f));
        bound = params.maxMoveFactor * tol * (bound * 0.5);
        genus = !keep_genus ? 0 : genusKnown ? knownGenus : genusTwice();
      }
      uint32_t applied = 0;
      for (uint32_t c : candidates)
        applied += fix(c);
      if (!applied)
        break;
      fixes += applied;
      rebuildLocal();
    }
    if (!fixes) // no edit: the mesh and its measures stay
      return 0;
    if (pair_all || eastl::find(state.begin(), state.end(), DEAD) != state.end())
      rebuild();
    const uint32_t endOpen = open.size();
    const double endVolume = signed_volume(s);
    const int endGenus = keep_genus ? genusTwice() : 0;
    const bool opened = endOpen > 0;
    const double moved = fabs(endVolume - volume);
    const bool genusChanged = keep_genus && endGenus != genus;
    volumeKnown = true;
    genusKnown = false;
    if (!opened && moved <= bound && !genusChanged)
    {
      knownVolume = endVolume;
      knownGenus = endGenus;
      genusKnown = keep_genus;
      return fixes;
    }
    // a rollback pairs anew: only the volume stays
    knownVolume = volume;
    // an open shell's Euler sum is no genus
    eastl::string genusChange;
    if (keep_genus && !opened)
      genusChange.sprintf(", genus change %g", (endGenus - genus) * 0.5);
    logwarn("daCSG cleanup: the %s step is undone: %u open halfedges, volume moved %g of %g allowed%s", name, endOpen, moved, bound,
      genusChange.c_str());
    if (!opened)
    {
      stats.rolledBack |= bit;
      stats.genusRolledBack |= genusChanged ? bit : 0;
      ++stats.unfixed;
    }
    s = backup;
    state.assign(s.faceCount(), LIVE);
    shapeStale = true;
    bvhSweep = NO_INDEX;
    rebuild();
    return 0;
  }

  void collectGroups()
  {
    order.clear();
    groups.clear();
    for (uint32_t f = 0; f < s.faceCount(); ++f)
      if (state[f] == LIVE && s.leafRef[f].leaf != NO_INDEX)
        order.push_back(f);
    const auto key = [&](uint32_t f) { return (uint64_t(s.leafRef[f].leaf) << 32) | s.leafRef[f].leafFace; };
    eastl::sort(order.begin(), order.end(), [&](uint32_t a, uint32_t b) { return key(a) != key(b) ? key(a) < key(b) : a < b; });
    for (uint32_t i = 0, j = 0; i < order.size(); i = j)
    {
      for (j = i + 1; j < order.size() && key(order[j]) == key(order[i]);)
        ++j;
      if (j - i >= 2)
      {
        candidates.push_back(groups.size());
        groups.push_back(TriPair(i, j));
      }
    }
    faceStamp.assign(s.faceCount(), 0);
    vertStamp.assign(s.pos.size(), 0);
    visitStamp.assign(s.pos.size(), 0);
    boundaryOut.resize(s.pos.size());
  }

  // false at pinch vertices and open chains
  bool groupLoops(dag::ConstSpan<uint32_t> faces, const DPoint3 &n)
  {
    const DPoint3 &origin = s.pos[s.faceVert(faces[0], 0)];
    uint32_t boundary = 0;
    for (uint32_t f : faces)
      for (int k = 0; k < 3; ++k)
      {
        const uint32_t h = 3 * f + k, twin = s.halfedge[h].pairedHalfedge, v = s.halfedge[h].startVert;
        if (!(fabs((s.pos[v] - origin) * n) <= tol))
          return false;
        if (twin != NO_INDEX && faceStamp[twin / 3] == stamp)
          continue;
        if (vertStamp[v] == stamp)
          return false;
        vertStamp[v] = stamp;
        boundaryOut[v] = h;
        ++boundary;
      }
    loopVerts.clear();
    loopStart.clear();
    for (uint32_t f : faces)
      for (int k = 0; k < 3; ++k)
      {
        uint32_t v = s.halfedge[3 * f + k].startVert;
        if (vertStamp[v] != stamp || visitStamp[v] == stamp || boundaryOut[v] != 3 * f + k)
          continue;
        const uint32_t first = v;
        loopStart.push_back(loopVerts.size());
        for (; visitStamp[v] != stamp; v = s.halfedge[boundaryOut[v]].endVert)
        {
          if (vertStamp[v] != stamp)
            return false;
          visitStamp[v] = stamp;
          loopVerts.push_back(v);
        }
        if (v != first)
          return false;
      }
    loopStart.push_back(loopVerts.size());
    return loopVerts.size() == boundary;
  }

  // No vertex moves: the volume guard holds
  bool mergeGroup(uint32_t first, uint32_t end)
  {
    const dag::ConstSpan<uint32_t> faces(order.data() + first, end - first);
    ++stamp;
    DPoint3 n(0, 0, 0);
    double oldMin = 180;
    for (uint32_t f : faces)
    {
      faceStamp[f] = stamp;
      n += face_cross(s, f);
      oldMin = min(oldMin, shape[f].minAngleDeg);
    }
    const double len = length(n);
    if (!(len > 0) || !groupLoops(faces, n * (1 / len)))
      return false;
    const int u = plane_axis(n, 1), v = plane_axis(n, 2);
    const uint32_t loops = loopStart.size() - 1;
    dag::Vector<double> &area = loopArea;
    area.assign(loops, 0);
    loopPts.clear();
    for (uint32_t l = 0; l < loops; ++l)
      for (uint32_t i = loopStart[l]; i < loopStart[l + 1]; ++i)
      {
        const DPoint3 &o = s.pos[loopVerts[loopStart[l]]], &p = s.pos[loopVerts[i]];
        const DPoint3 &q = s.pos[loopVerts[i + 1 < loopStart[l + 1] ? i + 1 : loopStart[l]]];
        loopPts.push_back(DPoint2(p[u], p[v]));
        area[l] += (p[u] - o[u]) * (q[v] - o[v]) - (q[u] - o[u]) * (p[v] - o[v]);
      }
    const auto loop = [&](uint32_t l) { return make_span_const(loopPts.data() + loopStart[l], loopStart[l + 1] - loopStart[l]); };
    uint32_t outers = 0, holeLoops = 0;
    for (uint32_t l = 0; l < loops; ++l)
    {
      outers += area[l] > 0;
      holeLoops += area[l] < 0;
    }
    // several outers would need hole containment tests
    if (outers + holeLoops != loops || (holeLoops && outers != 1))
      return false;
    newTris.clear();
    dag::Vector<dag::ConstSpan<DPoint2>> &holes = holeSpans;
    dag::Vector<uint32_t> &ids = loopIds;
    for (uint32_t o = 0; o < loops; ++o)
    {
      if (!(area[o] > 0))
        continue;
      holes.clear();
      ids.assign(loopVerts.begin() + loopStart[o], loopVerts.begin() + loopStart[o + 1]);
      for (uint32_t l = 0; l < loops; ++l)
        if (area[l] < 0)
        {
          holes.push_back(loop(l));
          ids.insert(ids.end(), loopVerts.begin() + loopStart[l], loopVerts.begin() + loopStart[l + 1]);
        }
      eastl::string err;
      if (!triangulate_polygon(loop(o), make_span_const(holes), tris, err))
        return false;
      for (uint32_t t : tris)
        newTris.push_back(ids[t]);
    }
    const uint32_t count = newTris.size() / 3;
    if (!count || count > faces.size())
      return false;
    double newMin = 180;
    for (uint32_t i = 0; i < newTris.size(); i += 3)
      newMin = min(newMin, triangle_shape(s.pos[newTris[i]], s.pos[newTris[i + 1]], s.pos[newTris[i + 2]]).minAngleDeg);
    if (!(newMin > oldMin || (count < faces.size() && newMin >= oldMin)))
      return false;
    for (uint32_t i = 0; i < faces.size(); ++i)
    {
      const uint32_t f = faces[i];
      state[f] = i < count ? TOUCHED : DEAD;
      for (int k = 0; k < 3 && i < count; ++k)
      {
        loose.push_back(s.halfedge[3 * f + k].pairedHalfedge);
        s.halfedge[3 * f + k] = Halfedge{newTris[3 * i + k], newTris[3 * i + (k + 1) % 3], NO_INDEX};
      }
    }
    stats.mergeRemoved += faces.size() - count;
    return true;
  }

  void nameVertex(uint32_t v, uint32_t f)
  {
    nameFace.push_back(f);
    nameNext.push_back(vertFirst[v]);
    vertFirst[v] = nameFace.size() - 1;
  }

  // a face of an overlap edit, named in the sweep's vertex lists
  void addFace(const uint32_t v[3], uint32_t like)
  {
    for (int k = 0; k < 3; ++k)
    {
      s.halfedge.push_back(Halfedge{v[k], v[(k + 1) % 3], NO_INDEX});
      nameVertex(v[k], s.faceCount());
    }
    s.face.push_back(FaceInfo(s.face[like]));
    s.leafRef.push_back(LeafRef(s.leafRef[like]));
    if (!s.facePlaneKey.empty())
      s.facePlaneKey.push_back(uint64_t(s.facePlaneKey[like]));
    state.push_back(TOUCHED);
  }

  bool removeOverlap(uint32_t gi, uint32_t &refused)
  {
    const OverlapGroup &g = overlapGroups[gi];
    OverlapEdit &e = overlapEdit;
    const bool resolved = remove_overlap(s, g, tol, charts, e) &&
                          eastl::all_of(e.dead.begin(), e.dead.end(), [&](uint32_t f) { return state[f] == LIVE; });
    refused += !resolved;
    if (!resolved)
      return false;
    for (uint32_t i = 0; i < e.tris.size(); i += 3)
      addFace(e.tris.data() + i, e.triSource[i / 3]);
    // a vertex takes part in one weld at most, so the welds apply one at a time
    for (uint32_t i = 0; i < e.rename.size(); i += 2)
      for (uint32_t n = vertFirst[e.rename[i]]; n != NO_INDEX; n = nameNext[n])
      {
        const uint32_t f = nameFace[n], from = e.rename[i], to = e.rename[i + 1];
        for (Halfedge *he = &s.halfedge[3 * f]; he < &s.halfedge[3 * f] + 3; ++he)
          *he = Halfedge{he->startVert == from ? to : he->startVert, he->endVert == from ? to : he->endVert, he->pairedHalfedge};
        nameVertex(to, f);
      }
    for (uint32_t f : e.dead)
      state[f] = DEAD;
    stats.overlapRemoved += g.faces.size();
    shapeStale = true;
    bvhSweep = NO_INDEX;
    return true;
  }

  void addFaces(bool slivers)
  {
    for (uint32_t f = 0; f < s.faceCount(); ++f)
      if (state[f] == LIVE && (slivers ? is_sliver(shape[f], params) : shape[f].height <= tol))
        candidates.push_back(f);
  }

  void run()
  {
    state.assign(s.faceCount(), LIVE);
    rebuild();
    for (uint32_t f = 0; f < s.faceCount(); ++f)
      stats.minAngleBefore.add(shape[f].minAngleDeg);

    stats.weldRemoved = runStep(
      "weld", 1,
      [&] {
        for (uint32_t h = 0; h < s.halfedge.size(); ++h)
          if (h < s.halfedge[h].pairedHalfedge && s.halfedge[h].pairedHalfedge != NO_INDEX && edgeLengthSq(h) < tol * tol)
            candidates.push_back(h);
        eastl::sort(candidates.begin(), candidates.end(), [&](uint32_t a, uint32_t b) { return edgeLengthSq(a) < edgeLengthSq(b); });
      },
      [&](uint32_t h) { return collapseEdge(h); });
    stats.coincidentRemoved = runStep(
      "coincident", 2,
      [&] {
        skin_pairs(s, skins);
        for (uint32_t i = 0; i < skins.size(); ++i)
          if (state[skins[i].first] == LIVE && state[skins[i].second] == LIVE)
            candidates.push_back(i);
      },
      [&](uint32_t i) {
        state[skins[i].first] = state[skins[i].second] = DEAD;
        return true;
      },
      MAX_SWEEPS, false); // a skin on the surface has no genus to keep
    uint32_t overlapsRefused = 0;
    const uint32_t overlaps = runStep(
      "overlap", 32,
      [&] {
        overlapsRefused = 0;
        liveMask.resize(s.faceCount());
        for (uint32_t f = 0; f < s.faceCount(); ++f)
          liveMask[f] = state[f] == LIVE;
        find_overlap_groups(s, make_span_const(liveMask), tol, overlapGroups);
        for (uint32_t i = 0; i < overlapGroups.size(); ++i)
          candidates.push_back(i);
        if (overlapGroups.empty())
          return;
        vertFirst.assign(s.pos.size(), NO_INDEX);
        nameNext.clear();
        nameFace.clear();
        for (uint32_t h = 0; h < s.halfedge.size(); ++h)
          nameVertex(s.halfedge[h].startVert, h / 3);
      },
      [&](uint32_t gi) { return removeOverlap(gi, overlapsRefused); }, 8); // each sweep searches the whole mesh
    if (!overlaps)
      stats.overlapRemoved = 0;
    if (overlapsRefused)
      logwarn("daCSG cleanup: the overlap step left %u groups it could not resolve", overlapsRefused);
    stats.degenerateRemoved = runStep("degenerate", 4, [&] { addFaces(false); }, [&](uint32_t f) { return fixFace(f, false); });
    stats.sliverRemoved = runStep("sliver", 8, [&] { addFaces(true); }, [&](uint32_t f) { return fixFace(f, true); });
    // one sweep: a second finds no gain
    const uint32_t merged = runStep(
      "merge", 16, [&] { collectGroups(); }, [&](uint32_t g) { return mergeGroup(groups[g].first, groups[g].second); }, 1, true, true);
    if (!merged)
      stats.mergeRemoved = 0;

    if (stats.weldRemoved || stats.coincidentRemoved || overlaps || stats.degenerateRemoved || stats.sliverRemoved || merged)
      finish();
    skin_pairs(s, skins);
    dag::Vector<uint8_t> skin(s.faceCount(), 0);
    for (const TriPair &p : skins)
      skin[p.first] = skin[p.second] = 1;
    for (uint32_t f = 0; f < s.faceCount(); ++f)
    {
      const FaceShape &fs = shape[f];
      stats.minAngleAfter.add(fs.minAngleDeg);
      stats.unfixed += skin[f] || fs.edgeLen[fs.shortest] < tol || fs.height <= tol || is_sliver(fs, params);
    }
  }

  double edgeLengthSq(uint32_t h) const { return lengthSq(s.pos[s.halfedge[h].endVert] - s.pos[s.halfedge[h].startVert]); }

  void finish()
  {
    dag::Vector<uint32_t> remap(s.pos.size(), NO_INDEX);
    for (const Halfedge &e : s.halfedge)
      remap[e.startVert] = 0;
    uint32_t used = 0;
    for (uint32_t v = 0; v < remap.size(); ++v)
      if (remap[v] == 0)
      {
        s.pos[used] = s.pos[v];
        if (!s.provenance.empty())
          s.provenance[used] = s.provenance[v];
        remap[v] = used++;
      }
    s.pos.resize(used);
    if (!s.provenance.empty())
      s.provenance.resize(used);
    s.bbox = points_bbox(s.pos);
    for (Halfedge &e : s.halfedge)
    {
      e.startVert = remap[e.startVert];
      e.endVert = remap[e.endVert];
    }
    eastl::string err;
    build_halfedges(s, err);
    compute_vertex_normals(s);
    s.convex = false;
  }
};

bool cleanup(Solid &s, const CleanupParams &p, double tolerance, CleanupStats &stats, eastl::string &err, const ChartTest *charts)
{
  stats = CleanupStats();
  if (!indices_valid(s) || s.leafRef.size() != s.faceCount() || (!s.provenance.empty() && s.provenance.size() != s.pos.size()) ||
      (!s.facePlaneKey.empty() && s.facePlaneKey.size() != s.faceCount()))
  {
    err = "cleanup: a face references a missing vertex or leaf, or an optional array has the wrong size";
    return false;
  }
  Cleaner(s, p, tolerance, stats, charts).run();
  if (!s.closed)
    return (err = "cleanup: the cleaned solid is not closed", false);
  return true;
}
} // namespace dacsg
