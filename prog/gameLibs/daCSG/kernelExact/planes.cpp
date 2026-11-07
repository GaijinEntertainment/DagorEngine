// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "kernelExact/arrangement.h"
#include <gameLibs/daParallel/parallel_api.h>
#include <EASTL/sort.h>

namespace dacsg::exact
{
static bool approx_less(const DPoint3 &a, const DPoint3 &b)
{
  for (int i = 0; i < 3; ++i)
    if (a[i] != b[i])
      return a[i] < b[i];
  return false;
}

// the leaf vertices on the int64grid grid, or the doubles as given for the shewchuk backend; both approx are exact
struct LeafVertices
{
  const BoolTree &tree;
  dag::Vector<dag::Vector<GridPoint>> grid; // empty for fp
  bool fp;

  DPoint3 approx(uint32_t l, uint32_t v) const
  {
    if (fp)
      return tree.leaves[l].pos[v];
    const GridPoint &g = grid[l][v];
    return DPoint3(double(g.c[0]), double(g.c[1]), double(g.c[2]));
  }
  ExactPoint make(uint32_t l, uint32_t v) const { return fp ? make_fp_point(tree.leaves[l].pos[v]) : make_grid_point(grid[l][v]); }
};

// one id per distinct vertex position
static void register_leaf_points(const LeafVertices &lv, Arrangement &a, dag::Vector<dag::Vector<uint32_t>> &ids)
{
  dag::Vector<uint64_t> refs; // leaf << 32 | vertex
  for (uint32_t l = 0; l < lv.tree.leaves.size(); ++l)
    for (uint32_t v = 0; v < lv.tree.leaves[l].vertCount(); ++v)
      refs.push_back(uint64_t(l) << 32 | v);
  auto at = [&](uint64_t r) { return lv.approx(r >> 32, uint32_t(r)); };
  eastl::sort(refs.begin(), refs.end(), [&](uint64_t x, uint64_t y) {
    const DPoint3 p = at(x), q = at(y);
    return approx_less(p, q) || (p == q && x < y);
  });
  ids.resize(lv.tree.leaves.size());
  for (uint32_t l = 0; l < ids.size(); ++l)
    ids[l].resize(lv.tree.leaves[l].vertCount());
  // sized once: a vector that doubles through the size classes commits a page per class
  uint32_t distinct = 0;
  for (uint32_t i = 0; i < refs.size(); ++i)
    distinct += i == 0 || !(at(refs[i]) == at(refs[i - 1]));
  reserve_octave(a.points, distinct);
  reserve_octave(a.pointLeaf, distinct);
  for (uint32_t i = 0; i < refs.size(); ++i)
  {
    if (i == 0 || !(at(refs[i]) == at(refs[i - 1])))
    {
      ExactPoint p = lv.make(refs[i] >> 32, uint32_t(refs[i]));
      set_key(p, a.points.size());
      a.points.push_back(p);
      a.pointLeaf.push_back(uint32_t(refs[i] >> 32));
    }
    else if (a.pointLeaf.back() != uint32_t(refs[i] >> 32))
      a.pointLeaf.back() = NO_INDEX;
    ids[refs[i] >> 32][uint32_t(refs[i])] = a.points.size() - 1;
  }
}

// a BVH per leaf over its triangle boxes and one over the leaf boxes, so a leaf meets only the leaves it overlaps;
// shewchuk: the registry unit from their extent
static void build_leaf_bvhs(Arrangement &a)
{
  const uint32_t leaves = a.leafTriBase.size();
  dag::Vector<bbox3f> leafBox(leaves);
  bbox3f all;
  v_bbox3_init_empty(all);
  a.leafBvh.resize(leaves);
  a.triBox.resize(a.tris.size());
  for (uint32_t l = 0; l < leaves; ++l)
  {
    const uint32_t first = a.leafTriBase[l], end = l + 1 < leaves ? a.leafTriBase[l + 1] : a.tris.size();
    v_bbox3_init_empty(leafBox[l]);
    for (uint32_t t = first; t < end; ++t)
    {
      const DPoint3 &p0 = a.points[a.tris[t].v[0]].approx, &p1 = a.points[a.tris[t].v[1]].approx,
                    &p2 = a.points[a.tris[t].v[2]].approx;
      a.triBox[t] = outward_box(min(min(p0, p1), p2), max(max(p0, p1), p2));
      v_bbox3_add_box(leafBox[l], a.triBox[t]);
    }
    a.leafBvh[l].build(make_span_const(a.triBox).subspan(first, end - first));
    v_bbox3_add_box(all, leafBox[l]);
  }
  a.leafTree.build(make_span_const(leafBox));
  if (!a.fp)
    return;
  alignas(16) float lo[4], hi[4];
  v_st(lo, all.bmin);
  v_st(hi, all.bmax);
  double extent = 0;
  for (int i = 0; i < 3; ++i)
    extent = eastl::max(extent, double(hi[i] - lo[i]));
  // the bucket resolution of the grid, as a power of two of the model's extent
  int e = 0;
  frexp(extent, &e);
  a.unit = ldexp(1.0, e - 40);
}

// int64grid snaps the vertices to the grid, shewchuk keeps the doubles with an identity frame
bool build_leaves(const BoolTree &tree, const KernelParams &p, Arrangement &a, eastl::string &err)
{
  a.fp = p.backend == Backend::Shewchuk;
  a.pairCap = p.maxArrangementTris;
  // past it the seed ray's bounds and the plane products of three-plane points leave the double range; an empty leaf
  // has the default box
  for (const Solid &leaf : tree.leaves)
    if (a.fp && leaf.faceCount() && !(max_abs(leaf.bbox) <= 1e18))
    {
      err = "the shewchuk backend takes coordinates up to 1e18";
      return false;
    }
  LeafVertices lv{tree, {}, a.fp};
  if (a.fp)
    a.frame = GridFrame();
  else if (!snap_leaves(make_span_const(tree.leaves), p.gridBits, a.frame, lv.grid, err))
    return false;
  dag::Vector<dag::Vector<uint32_t>> ids;
  register_leaf_points(lv, a, ids);

  uint32_t trisIn = 0;
  for (const Solid &leaf : tree.leaves)
    trisIn += leaf.faceCount();
  dag::Vector<Plane> planes;
  reserve_octave(planes, trisIn);
  reserve_octave(a.tris, trisIn);
  for (uint32_t l = 0; l < tree.leaves.size(); ++l)
  {
    const Solid &leaf = tree.leaves[l];
    a.leafTriBase.push_back(a.tris.size());
    a.leafConvex.push_back(leaf.convex);
    for (uint32_t f = 0; f < leaf.faceCount(); ++f)
    {
      Tri t;
      t.leaf = l;
      t.face = f;
      for (int k = 0; k < 3; ++k)
        t.v[k] = ids[l][leaf.faceVert(f, k)];
      Plane &pl = planes.push_back();
      t.orient = int8_t(make_plane(a.points[t.v[0]], a.points[t.v[1]], a.points[t.v[2]], a.arena, pl));
      // the snap can turn a sliver over: its grid normal must still point along the float face
      const bool turned = !a.fp && t.orient && t.orient * (DPoint3(pl.nd[0], pl.nd[1], pl.nd[2]) * face_cross(leaf, f)) <= 0;
      if (!t.orient || turned)
      {
        err.sprintf("%s: leaf %u face %u is %s", a.fp ? "shewchuk backend" : "model too small for the grid", a.callerLeaf(l), f,
          turned                       ? "turned after the snap"
          : a.fp && take_ex_overflow() ? "past the expansion terms: the coordinates spread too wide"
                                       : "degenerate");
        return false;
      }
      t.group = NO_INDEX;
      a.tris.push_back(t);
    }
  }

  a.groupTris.resize(a.tris.size());
  for (uint32_t i = 0; i < a.tris.size(); ++i)
    a.groupTris[i] = i;
  eastl::sort(a.groupTris.begin(), a.groupTris.end(), [&](uint32_t x, uint32_t y) {
    const int c = plane_cmp(planes[x], planes[y]);
    return c != 0 ? c < 0 : x < y;
  });
  dag::Vector<uint32_t> firstTri;
  for (uint32_t i = 0; i < a.groupTris.size(); ++i)
  {
    if (i == 0 || plane_cmp(planes[a.groupTris[i]], planes[a.groupTris[i - 1]]) != 0)
      firstTri.push_back(i);
    a.tris[a.groupTris[i]].group = firstTri.size() - 1;
  }
  reserve_octave(a.groups, firstTri.size());
  a.groups.resize(firstTri.size());
  for (uint32_t i = 0; i < a.groups.size(); ++i)
  {
    Group &g = a.groups[i];
    g.plane = planes[a.groupTris[firstTri[i]]];
    g.firstTri = firstTri[i];
    g.triCount = (i + 1 < firstTri.size() ? firstTri[i + 1] : uint32_t(a.groupTris.size())) - firstTri[i];
    g.axis = plane_axis(g.plane);
    g.sign = int8_t(plane_sign(g.plane, g.axis));
  }
  // its views die with the old arena
  planes = dag::Vector<Plane>();
  if (a.fp)
  {
    // only the group representatives keep their plane expansions
    ExArena kept;
    for (Group &g : a.groups)
    {
      for (ExView &v : g.plane.fn)
        v.e = kept.put(v.e, ExView::PREFIX + v.n);
      g.plane.fd.e = kept.put(g.plane.fd.e, ExView::PREFIX + g.plane.fd.n);
    }
    a.arena = eastl::move(kept);
  }
  build_leaf_bvhs(a);
  return true;
}

// a grid vertex (ref set) or an LPI that becomes a candidate only when it ends the overlap
struct CutPoint
{
  ExactPoint pt;
  uint32_t ref = NO_INDEX;
};

struct Cut
{
  CutPoint p[2];
  int count = 0;
};

// the side of each vertex of t against the other triangle's plane, a vertex both share on it; true when all are on one side
static bool plane_sides(const Arrangement &a, const Tri &t, const Tri &other, int s[3])
{
  for (int k = 0; k < 3; ++k)
  {
    const bool shared = t.v[k] == other.v[0] || t.v[k] == other.v[1] || t.v[k] == other.v[2];
    s[k] = shared ? 0 : sign_point_plane(a.points[t.v[k]], a.groups[other.group].plane);
  }
  return (s[0] > 0 && s[1] > 0 && s[2] > 0) || (s[0] < 0 && s[1] < 0 && s[2] < 0);
}

// a vertex off the plane with the other two on it or across it, or -1
static int apex(const int s[3])
{
  for (int k = 0; k < 3; ++k)
    if (s[k] != 0 && s[(k + 1) % 3] != s[k] && s[(k + 2) % 3] != s[k])
      return k;
  return -1;
}

// The segments of x and y on the line of their planes lie apart when each end of y's is on the same side of both ends
// of x's: an end is where an edge from the apex meets the other plane (the far vertex itself when it lies on it), and
// the tetrahedron of two such edges has the sign of the order of their ends, a common factor aside, so four equal
// signs mean no contact.
static bool segments_apart(const Arrangement &a, const Tri &x, const Tri &y, int jx, int jy)
{
  int first = 0;
  for (int j = 1; j < 3; ++j)
    for (int k = 1; k < 3; ++k)
    {
      const int o = orient3d(a.points[x.v[jx]], a.points[x.v[(jx + j) % 3]], a.points[y.v[jy]], a.points[y.v[(jy + k) % 3]]);
      if (o == 0 || (first != 0 && o != first))
        return false;
      first = o;
    }
  return true;
}

// t against the plane of other_group: the vertices on it and the LPIs of the crossing edges
static Cut cut_by_plane(const Arrangement &a, ExArena &arena, const Tri &t, uint32_t other_group, const int s[3])
{
  Cut c;
  for (int k = 0; k < 3; ++k)
    if (s[k] == 0 && c.count < 2)
    {
      c.p[c.count].pt = a.points[t.v[k]];
      c.p[c.count++].ref = t.v[k];
    }
  for (int k = 0; k < 3; ++k)
  {
    const int k1 = (k + 1) % 3;
    if (s[k] * s[k1] < 0 && c.count < 2 &&
        make_lpi(a.points[t.v[k]], a.points[t.v[k1]], a.groups[other_group].plane, arena, c.p[c.count].pt))
      set_key(c.p[c.count++].pt, t.v[k], t.v[k1], other_group);
  }
  return c;
}

static void sort_cut(Cut &c, int axis)
{
  if (c.count == 1)
    c.p[1] = c.p[0];
  else if (compare_coord(c.p[0].pt, c.p[1].pt, axis) > 0)
    eastl::swap(c.p[0], c.p[1]);
}

// one worker's output: its segments name its candidates by CAND index until the join
struct alignas(64) PairWorker
{
  dag::Vector<ExactPoint> candidates;
  dag::Vector<Segment> segments;
  ExArena arena;
  uint64_t fallbacks = 0;
  bool overflow = false;
};

static uint32_t register_cut_point(PairWorker &w, const CutPoint &p)
{
  if (p.ref != NO_INDEX)
    return p.ref;
  w.candidates.push_back(p.pt);
  return CAND | (w.candidates.size() - 1);
}

static void pair_intersect(const Arrangement &a, PairWorker &w, uint32_t ti, uint32_t tj)
{
  const Tri &x = a.tris[ti], &y = a.tris[tj];
  int sx[3], sy[3];
  if (plane_sides(a, x, y, sx) || plane_sides(a, y, x, sy))
    return;
  const int jx = apex(sx), jy = apex(sy);
  if (jx >= 0 && jy >= 0 && segments_apart(a, x, y, jx, jy))
    return;
  Cut cx = cut_by_plane(a, w.arena, x, y.group, sx);
  if (cx.count == 0)
    return;
  Cut cy = cut_by_plane(a, w.arena, y, x.group, sy);
  if (cy.count == 0)
    return;
  if (cx.count == 1 && cy.count == 1 && cx.p[0].ref != NO_INDEX && cx.p[0].ref == cy.p[0].ref)
    return;
  const ExactDir d = line_dir(a.groups[x.group].plane, a.groups[y.group].plane);
  const int axis = dir_axis(d);
  sort_cut(cx, axis);
  sort_cut(cy, axis);
  const CutPoint &lo = compare_coord(cx.p[0].pt, cy.p[0].pt, axis) >= 0 ? cx.p[0] : cy.p[0];
  const CutPoint &hi = compare_coord(cx.p[1].pt, cy.p[1].pt, axis) <= 0 ? cx.p[1] : cy.p[1];
  const int order = compare_coord(lo.pt, hi.pt, axis);
  if (order > 0)
    return;
  const uint32_t loRef = register_cut_point(w, lo);
  if (order == 0)
    return;
  const uint32_t hiRef = register_cut_point(w, hi);
  const Line line{LineKind::Planes, x.group, y.group};
  w.segments.push_back(Segment{{loRef, hiRef}, ti, line});
  w.segments.push_back(Segment{{loRef, hiRef}, tj, line});
}

static bool share_an_edge(const Tri &x, const Tri &y)
{
  int shared = 0;
  for (uint32_t u : x.v)
    for (uint32_t v : y.v)
      shared += u == v;
  return shared >= 2;
}

// Two faces of one leaf meet along their shared edge only, but a crossing within the tolerance passes the leaf's
// self-intersection test and can run for centimetres between nearly parallel faces, so the other pairs of a leaf
// that is not convex are cut like the faces of two leaves.
bool find_pairs(Arrangement &a, uint32_t workers)
{
  dag::Vector<TriPair> pairs, leafPairs;
  a.leafTree.selfOverlapsUnsorted(leafPairs, a.pairCap);
  if (leafPairs.size() > a.pairCap)
    return false;
  for (uint32_t l = 0; l < a.leafBvh.size(); ++l)
    if (!a.leafConvex[l])
      leafPairs.push_back(TriPair(l, l));
  eastl::sort(leafPairs.begin(), leafPairs.end());
  // BATCH bounds the pairs held: identical leaves pair quadratically
  constexpr uint32_t QUANT = 1024, BATCH = 1 << 22;
  dag::Vector<PairWorker> work(workers);
  dag::Vector<daparallel::Chunk> chunks, candChunks;
  const auto run_batch = [&]() {
    a.pairsHeld = eastl::max(a.pairsHeld, uint32_t(pairs.size()));
    candChunks.resize((pairs.size() + QUANT - 1) / QUANT);
    // a worker pays off per 4096 pairs
    daparallel::parallel_chunks(
      eastl::min(workers, uint32_t(1 + pairs.size() / (4 * QUANT))), pairs.size(), QUANT, chunks,
      [&](uint32_t begin, uint32_t end, uint32_t worker) {
        PairWorker &w = work[worker];
        const uint32_t candFrom = w.candidates.size();
        for (uint32_t i = begin; i < end; ++i)
        {
          const Tri &x = a.tris[pairs[i].first], &y = a.tris[pairs[i].second];
          if (x.group != y.group && (x.leaf != y.leaf || !share_an_edge(x, y)))
            pair_intersect(a, w, pairs[i].first, pairs[i].second);
        }
        candChunks[begin / QUANT] = daparallel::Chunk{worker, candFrom, uint32_t(w.candidates.size())};
        w.fallbacks += take_exact_fallbacks();
        w.overflow |= take_ex_overflow();
      },
      [&](uint32_t worker) { return uint32_t(work[worker].segments.size()); });
    for (uint32_t c = 0; c < chunks.size(); ++c)
    {
      const PairWorker &w = work[chunks[c].worker];
      const daparallel::Chunk &cand = candChunks[c];
      const uint32_t base = a.candidates.size();
      a.candidates.insert(a.candidates.end(), w.candidates.begin() + cand.from, w.candidates.begin() + cand.to);
      for (uint32_t s = chunks[c].from; s < chunks[c].to; ++s)
      {
        Segment seg = w.segments[s];
        for (uint32_t &p : seg.p)
          if (p & CAND)
            p = CAND | (base + (p & ~CAND) - cand.from);
        a.segments.push_back(seg);
      }
    }
    pairs.clear();
  };
  dag::Vector<uint32_t> hits;
  for (const TriPair &lp : leafPairs)
    for (uint32_t t = a.leafTriBase[lp.first]; t < a.tris.size() && a.tris[t].leaf == lp.first; ++t)
    {
      a.leafBvh[lp.second].overlapping(a.triBox[t], hits);
      for (uint32_t j : hits)
        if (lp.first != lp.second || j + a.leafTriBase[lp.second] > t)
          pairs.push_back(TriPair(t, j + a.leafTriBase[lp.second]));
      if (pairs.size() >= BATCH)
        run_batch();
    }
  run_batch();
  a.triBox = dag::Vector<bbox3f>();
  for (PairWorker &w : work)
  {
    a.arena.adopt(w.arena);
    add_exact_fallbacks(w.fallbacks);
    ex_overflow |= w.overflow;
  }
  return true;
}
} // namespace dacsg::exact
