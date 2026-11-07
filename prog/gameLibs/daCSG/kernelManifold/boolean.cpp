// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "kernelManifold/build.h"
#include "mesh/bvh.h"
#include "mesh/unionFind.h"
#include <gameLibs/daParallel/parallel_api.h>
#include <EASTL/sort.h>
#include <float.h>

namespace dacsg
{
static void build_edges(Side &side)
{
  const Solid &s = side.s;
  side.edgeOf.assign(s.halfedge.size(), NO_INDEX);
  side.edges.reserve(s.halfedge.size() / 2);
  for (uint32_t h = 0; h < s.halfedge.size(); ++h)
  {
    const Halfedge &e = s.halfedge[h];
    if (e.startVert > e.endVert)
      continue;
    side.edgeOf[h] = side.edgeOf[e.pairedHalfedge] = side.edges.size();
    side.edges.push_back(Edge{e.startVert, e.endVert, h});
  }
}

// queries come from other's xy bounds, and from a crossing's tolerance box, which can reach a tolerance past them
static void build_face_bvh(const Solid &s, const Solid &other, double tolerance, Bvh &bvh)
{
  const DPoint3 grow(tolerance, tolerance, 0);
  bbox3f reach = outward_box(other.bbox.lim[0] - grow, other.bbox.lim[1] + grow);
  reach.bmin = v_perm_xyab(reach.bmin, v_splats(-FLT_MAX));
  reach.bmax = v_perm_xyab(reach.bmax, v_splats(FLT_MAX));
  dag::Vector<bbox3f> boxes;
  dag::Vector<uint32_t> ids;
  for (uint32_t f = 0; f < s.faceCount(); ++f)
  {
    const bbox3f box = face_box(s, f);
    if (v_bbox3_test_box_intersect(box, reach))
    {
      boxes.push_back(box);
      ids.push_back(f);
    }
  }
  bvh.build(make_span_const(boxes), make_span_const(ids));
}

// one winding read per run of edges (spec 6)
static void compute_windings(Side &side, const Bvh &other_tris, const Shadow &shadow)
{
  const Solid &s = side.s;
  UnionFind runs;
  runs.init(s.pos.size());
  for (uint32_t e = 0; e < side.edges.size(); ++e)
  {
    bool cut = false;
    for (const Crossing &c : side.edgeCrossings(e))
      cut |= c.s != 0;
    if (!cut)
      runs.joinLower(side.edges[e].v0, side.edges[e].v1);
  }
  side.winding.assign(s.pos.size(), 0);
  dag::Vector<uint32_t> tris;
  for (uint32_t v = 0; v < s.pos.size(); ++v)
    if (runs.find(v) == v)
    {
      // the faces over and under the vertex: a P vertex counts those over it, a Q vertex those under it
      bbox3f column = outward_box(s.pos[v], s.pos[v]);
      column.bmin = v_perm_xyab(column.bmin, v_splats(-FLT_MAX));
      column.bmax = v_perm_xyab(column.bmax, v_splats(FLT_MAX));
      other_tris.overlapping(column, tris);
      side.winding[v] = shadow.winding(side.isP, v, make_span_const(tris));
    }
  for (uint32_t v = 0; v < s.pos.size(); ++v)
    side.winding[v] = side.winding[runs.find(v)];
}

// a crossing lies on the two planes of its edge and on the one it crossed
static void find_crossings(Side &side, const Side &other, const Bvh &other_tris, const Shadow &shadow, const KernelParams &params,
  Assembly &a, KernelStats &stats)
{
  struct Hit
  {
    Crossing c;
    DPoint3 pos;
  };
  struct alignas(64) Worker
  {
    dag::Vector<uint32_t> tris;
    dag::Vector<Hit> hits;
    int64_t tested = 0;
  };
  const WorkerLease lease(params.budget,
    min(daparallel::worker_count(params.threads), max(1u, (uint32_t(side.edges.size()) + 255) / 256)));
  const uint32_t workers = lease.workers();
  dag::Vector<Worker> work(workers);
  dag::Vector<daparallel::Chunk> chunks;
  daparallel::parallel_chunks(
    workers, side.edges.size(), 256, chunks,
    [&](uint32_t begin, uint32_t end, uint32_t worker) {
      Worker &w = work[worker];
      for (uint32_t ei = begin; ei < end; ++ei)
      {
        const Edge &e = side.edges[ei];
        const DPoint3 &p0 = side.s.pos[e.v0], &p1 = side.s.pos[e.v1];
        other_tris.overlapping(outward_box(min(p0, p1), max(p0, p1)), w.tris);
        w.tested += w.tris.size();
        for (const uint32_t f : w.tris)
        {
          Hit h{Crossing{ei, f, 0, 0, NO_INDEX}};
          h.c.s = shadow.intersect12(side.isP, e.he, f, h.pos, h.c.t);
          if (h.c.s)
            w.hits.push_back(h);
        }
      }
    },
    [&](uint32_t worker) { return uint32_t(work[worker].hits.size()); });
  uint32_t hits = 0;
  for (const Worker &w : work)
  {
    stats.add("pairs tested", w.tested);
    hits += w.hits.size();
  }
  side.crossings.reserve(side.crossings.size() + hits);
  a.pos.reserve(a.pos.size() + hits);
  a.provenance.reserve(a.provenance.size() + hits);
  for (const daparallel::Chunk &c : chunks)
    for (uint32_t i = c.from; i < c.to; ++i)
    {
      const Hit &h = work[c.worker].hits[i];
      const Edge &e = side.edges[h.c.edge];
      side.crossings.push_back(h.c);
      side.crossings.back().vert = a.pos.size();
      a.pos.push_back(h.pos);
      Provenance &p = a.provenance.push_back();
      const uint32_t twin = side.s.halfedge[e.he].pairedHalfedge;
      for (const uint64_t key :
        {plane_key(side.s, e.he / 3), twin == NO_INDEX ? Provenance::NONE : plane_key(side.s, twin / 3), plane_key(other.s, h.c.face)})
        if (key != Provenance::NONE)
          p.add(key);
    }
}

static void index_crossings(Side &side);

// A crossing of the other side within the tolerance of an edge of its face splits that edge for both faces with no
// winding change, so the face loop meets its boundary at a shared vertex, not at a vertex that lies on an edge.
// The crossings of side are indexed before and after
static void split_edges_at_crossings(Side &side, const Side &other, const Assembly &a)
{
  dag::Vector<Crossing> added;
  for (const Crossing &c : other.crossings)
  {
    if (!c.s)
      continue;
    const DPoint3 &x = a.pos[c.vert];
    for (int k = 0; k < 3; ++k)
    {
      const uint32_t e = side.edgeOf[3 * c.face + k];
      const DPoint3 &p0 = side.s.pos[side.edges[e].v0], d = side.s.pos[side.edges[e].v1] - p0;
      const double dd = d * d;
      if (!(dd > 0))
        continue;
      const double t = ((x - p0) * d) / dd, len = sqrt(dd);
      if (t * len <= a.tolerance || (1 - t) * len <= a.tolerance || lengthSq(x - (p0 + d * t)) > a.tolerance * a.tolerance)
        continue;
      // the edge's crossings are sorted by t: only those near t can be within the tolerance
      const dag::ConstSpan<Crossing> own = side.edgeCrossings(e);
      const double near = a.tolerance / len * 1.0000001;
      bool taken = false;
      for (
        const Crossing *o = eastl::lower_bound(own.begin(), own.end(), t - near, [](const Crossing &c, double v) { return c.t < v; });
        o != own.end() && o->t - t <= near; ++o)
        taken |= fabs(o->t - t) * len <= a.tolerance;
      if (!taken)
        added.push_back(Crossing{e, NO_INDEX, t, 0, c.vert});
    }
  }
  if (added.empty())
    return;
  side.crossings.insert(side.crossings.end(), added.begin(), added.end());
  index_crossings(side);
}

// A crossing within the tolerance of another crossing or of a solid vertex is that vertex: an edge through an edge,
// or a vertex on an edge of the other solid, makes the same point once per side. Two solid vertices stay apart.
// box_vert: one box per crossing position left, for the new edge splits
static void merge_coincident_crossings(Side &p, Side &q, Assembly &a, uint32_t first, const Bvh &p_tris, const Bvh &q_tris,
  Bvh &points, dag::Vector<uint32_t> &box_vert)
{
  const uint32_t count = a.pos.size();
  if (first == count)
    return;
  UnionFind merged;
  merged.init(count);
  // two solid vertices join only through a crossing between them: the arrangement has a vertex there
  const double tol = a.tolerance;
  const auto join = [&](uint32_t c, uint32_t v) {
    if (lengthSq(a.pos[c] - a.pos[v]) <= tol * tol)
      merged.joinLower(c, v);
  };
  dag::Vector<bbox3f> boxes;
  boxes.reserve(count - first);
  if (count > 16 * (count - first))
  {
    // few crossings in a large mesh: a sort of every vertex costs more than an index of the crossings, queried by the
    // tolerance box for the crossings in it and for the solid vertices as corners of the faces in it
    for (uint32_t i = first; i < count; ++i)
      boxes.push_back(outward_box(a.pos[i], a.pos[i]));
    Bvh crossings;
    crossings.build(make_span_const(boxes));
    const DPoint3 spread(tol, tol, tol);
    dag::Vector<uint32_t> hits;
    for (uint32_t c = first; c < count; ++c)
    {
      const bbox3f box = outward_box(a.pos[c] - spread, a.pos[c] + spread);
      crossings.overlapping(box, hits);
      for (uint32_t h : hits)
        if (first + h > c)
          join(c, first + h);
      for (const Side *side : {&p, &q})
      {
        (side == &p ? p_tris : q_tris).overlapping(box, hits);
        for (uint32_t f : hits)
          for (int k = 0; k < 3; ++k)
            join(c, side->vertBase + side->s.faceVert(f, k));
      }
    }
    boxes.clear();
  }
  else
  {
    // each crossing scans its x window: forward for every point, backward for the solid vertices, so a pair comes up once
    dag::Vector<uint32_t> order(count);
    for (uint32_t i = 0; i < count; ++i)
      order[i] = i;
    eastl::sort(order.begin(), order.end(), [&](uint32_t x, uint32_t y) { return a.pos[x].x < a.pos[y].x; });
    for (uint32_t i = 0; i < count; ++i)
    {
      const uint32_t c = order[i];
      if (c < first)
        continue;
      for (uint32_t j = i + 1; j < count && a.pos[order[j]].x - a.pos[c].x <= tol; ++j)
        join(c, order[j]);
      for (uint32_t j = i; j-- > 0 && a.pos[c].x - a.pos[order[j]].x <= tol;)
        if (order[j] < first)
          join(c, order[j]);
    }
  }
  for (Side *side : {&p, &q})
    for (Crossing &c : side->crossings)
      c.vert = merged.find(c.vert);
  a.remap.resize(count);
  for (uint32_t v = 0; v < count; ++v)
  {
    a.remap[v] = merged.find(v);
    if (a.remap[v] != v)
      a.provenance[a.remap[v]].intersect(a.provenance[v]);
  }
  dag::Vector<uint8_t> boxed(count, 0);
  box_vert.reserve(count - first);
  for (uint32_t i = first; i < count; ++i)
  {
    const uint32_t r = merged.find(i);
    if (boxed[r])
      continue;
    boxed[r] = 1;
    boxes.push_back(outward_box(a.pos[r], a.pos[r]));
    box_vert.push_back(r);
  }
  points.build(make_span_const(boxes));
}

static void index_crossings(Side &side)
{
  eastl::sort(side.crossings.begin(), side.crossings.end(), [](const Crossing &x, const Crossing &y) {
    return x.edge != y.edge ? x.edge < y.edge : x.t != y.t ? x.t < y.t : x.face < y.face;
  });
  side.crossStart.assign(side.edges.size() + 1, 0);
  for (const Crossing &c : side.crossings)
    ++side.crossStart[c.edge + 1];
  for (uint32_t e = 0; e < side.edges.size(); ++e)
    side.crossStart[e + 1] += side.crossStart[e];
}

// Crossings within the tolerance of each other have no order from the geometry: take the one that keeps the winding in 0..1 first
static void order_coincident(Side &side, double tolerance)
{
  for (uint32_t e = 0; e < side.edges.size(); ++e)
  {
    Crossing *c = side.crossings.data() + side.crossStart[e];
    const uint32_t n = side.crossStart[e + 1] - side.crossStart[e];
    if (n < 2)
      continue;
    const double len = length(side.s.pos[side.edges[e].v1] - side.s.pos[side.edges[e].v0]);
    const double near = len > 0 ? tolerance / len : 0;
    int w = side.winding[side.edges[e].v0];
    for (uint32_t i = 0; i < n; ++i)
    {
      const int wanted = w == 0 ? 1 : -1;
      for (uint32_t m = i + 1; c[i].s != wanted && m < n && c[m].t - c[i].t <= near; ++m)
        if (c[m].s == wanted)
          eastl::swap(c[i], c[m]);
      w += c[i].s;
    }
  }
}

static bool check_windings(const Side &side, eastl::string &err)
{
  for (uint32_t e = 0; e < side.edges.size(); ++e)
  {
    int w = side.winding[side.edges[e].v0];
    for (const Crossing &c : side.edgeCrossings(e))
      w += c.s;
    if (w != side.winding[side.edges[e].v1])
    {
      err.sprintf("%s edge %u: the winding along the edge ends at %d, the vertex has %d",
        side.isP ? "first operand" : "second operand", e, w, side.winding[side.edges[e].v1]);
      return false;
    }
  }
  return true;
}

static void collect_face_pairs(const Side &side, dag::Vector<FacePairVert> &out)
{
  out.reserve(out.size() + 2 * side.crossings.size());
  for (uint32_t ci = 0; ci < side.crossings.size(); ++ci)
  {
    const Crossing &c = side.crossings[ci];
    if (!c.s)
      continue;
    const Edge &e = side.edges[c.edge];
    const uint32_t hes[2] = {e.he, side.s.halfedge[e.he].pairedHalfedge};
    for (uint32_t he : hes)
    {
      const uint32_t own = he / 3;
      out.push_back(FacePairVert{side.isP ? own : c.face, side.isP ? c.face : own, c.vert, side.isP, he, ci});
    }
  }
}

// Direction in the P face; a vertex on a Q edge gives it through the Q face
static int direction_in_p(const Side &p, const Side &q, const FacePairVert &v)
{
  if (v.onP)
    return new_edge_direction(p, v.he, v.crossing);
  const int dq = new_edge_direction(q, v.he, v.crossing);
  return q.flip ? dq : -dq;
}

// Crossing vertices on the segment within the tolerance, strictly inside, in order along it
static void points_on(const Bvh &points, dag::ConstSpan<uint32_t> box_vert, dag::ConstSpan<DPoint3> pos, uint32_t from, uint32_t to,
  double tolerance, dag::Vector<uint32_t> &scratch, dag::Vector<eastl::pair<double, uint32_t>> &out)
{
  out.clear();
  const DPoint3 &a = pos[from], d = pos[to] - a;
  const double dd = d * d, len = sqrt(dd);
  if (!(dd > 0))
    return;
  const DPoint3 spread(tolerance, tolerance, tolerance);
  scratch.clear();
  points.overlapping(outward_box(min(a, pos[to]) - spread, max(a, pos[to]) + spread), scratch);
  for (uint32_t i : scratch)
  {
    const uint32_t v = box_vert[i];
    if (v == from || v == to)
      continue;
    const DPoint3 &x = pos[v];
    const double t = ((x - a) * d) / dd;
    if (t * len <= tolerance || (1 - t) * len <= tolerance || lengthSq(x - (a + d * t)) > tolerance * tolerance)
      continue;
    out.push_back(eastl::make_pair(t, v));
  }
  eastl::sort(out.begin(), out.end());
}

// The windings along the edge around each crossing, the crossings merged into one vertex taken together, once per
// crossing after the order is final; a vertex that comes up again later on the edge reads its first run
static void record_winding_bounds(Side &side)
{
  side.boundsWant.resize(side.crossings.size());
  struct Run
  {
    uint32_t vert;
    int lo, hi;
  };
  dag::Vector<Run> runs;
  for (uint32_t e = 0; e < side.edges.size(); ++e)
  {
    runs.clear();
    int w = side.winding[side.edges[e].v0];
    for (uint32_t k = side.crossStart[e]; k < side.crossStart[e + 1];)
    {
      const uint32_t vert = side.crossings[k].vert;
      Run run{vert, w, w};
      uint32_t end = k;
      for (; end < side.crossStart[e + 1] && side.crossings[end].vert == vert; ++end)
      {
        w += side.crossings[end].s;
        run.lo = min(run.lo, w);
        run.hi = max(run.hi, w);
      }
      for (const Run &first : runs)
        if (first.vert == vert)
        {
          run = first;
          break;
        }
      runs.push_back(run);
      for (; k < end; ++k)
        side.boundsWant[k] = side.want >= run.lo && side.want <= run.hi;
    }
  }
}

// A new edge from a crossing bounds a kept region of the face only when one of the windings around it is the side's
// want (an operand that overlaps itself has regions of winding 2)
static bool crossing_bounds_want(const Side &side, uint32_t crossing) { return side.boundsWant[crossing]; }

static bool make_new_edges(const Side &p, const Side &q, dag::ConstSpan<DPoint3> pos, const Bvh &points,
  dag::ConstSpan<uint32_t> box_vert, double tolerance, dag::Vector<FacePairVert> &verts, dag::Vector<NewEdge> &p_edges,
  dag::Vector<NewEdge> &q_edges, eastl::string &err)
{
  eastl::sort(verts.begin(), verts.end(), [](const FacePairVert &a, const FacePairVert &b) {
    return a.p != b.p ? a.p < b.p : a.q != b.q ? a.q < b.q : a.vert < b.vert;
  });
  dag::Vector<uint32_t> scratch;
  dag::Vector<eastl::pair<double, uint32_t>> inside;
  for (uint32_t i = 0; i < verts.size();)
  {
    uint32_t j = i + 1;
    while (j < verts.size() && verts[j].p == verts[i].p && verts[j].q == verts[i].q)
      ++j;
    if ((j - i) % 2)
    {
      err.sprintf("faces %u and %u meet in %u points, not 2", verts[i].p, verts[i].q, j - i);
      return false;
    }
    // a coplanar contact can give two segments on one line: pair the points along it; crossings merged into one
    // vertex alternate their directions with the neighbours, so a pair of them is a zero-length edge and the rest
    // continue the line
    if (j - i > 2)
    {
      const DPoint3 origin = pos[verts[i].vert];
      DPoint3 dir(0, 0, 0);
      for (uint32_t k = i + 1; k < j; ++k)
        if (lengthSq(pos[verts[k].vert] - origin) > lengthSq(dir))
          dir = pos[verts[k].vert] - origin;
      eastl::sort(verts.begin() + i, verts.begin() + j, [&](const FacePairVert &x, const FacePairVert &y) {
        const double ax = (pos[x.vert] - origin) * dir, ay = (pos[y.vert] - origin) * dir;
        return ax != ay ? ax < ay : x.vert < y.vert;
      });
      for (uint32_t s = i; s < j;)
      {
        uint32_t e = s + 1;
        while (e < j && verts[e].vert == verts[s].vert)
          ++e;
        if (e - s > 1)
        {
          int opens = 0;
          for (uint32_t k = s; k < e; ++k)
            opens += direction_in_p(p, q, verts[k]) > 0;
          int want = s > i ? -direction_in_p(p, q, verts[s - 1]) : opens * 2 >= int(e - s) ? 1 : -1;
          for (uint32_t k = s; k < e; ++k, want = -want)
            for (uint32_t m = k; m < e; ++m) //-V791 selection sort: verts[k] itself may match
              if (direction_in_p(p, q, verts[m]) == want)
              {
                eastl::swap(verts[k], verts[m]);
                break;
              }
        }
        s = e;
      }
    }
    for (uint32_t k = i; k < j; k += 2)
    {
      const FacePairVert &a = verts[k], &b = verts[k + 1];
      const int da = direction_in_p(p, q, a), db = direction_in_p(p, q, b);
      if (da == db)
      {
        err.sprintf("faces %u and %u: the new edge has no consistent direction", a.p, a.q);
        return false;
      }
      const uint32_t from = da > 0 ? a.vert : b.vert, to = da > 0 ? b.vert : a.vert;
      if (from == to)
        continue;
      const bool onP = (!a.onP || crossing_bounds_want(p, a.crossing)) && (!b.onP || crossing_bounds_want(p, b.crossing));
      const bool onQ = (a.onP || crossing_bounds_want(q, a.crossing)) && (b.onP || crossing_bounds_want(q, b.crossing));
      if (!onP && !onQ)
        continue;
      // a crossing on the new edge splits it on both faces, so the pieces along a shared line match up; one off a face's
      // plane is near the edge only because a merge moved an end of it
      points_on(points, box_vert, pos, from, to, tolerance, scratch, inside);
      const DPoint3 np = unit_or_zero(face_cross(p.s, a.p)), nq = unit_or_zero(face_cross(q.s, a.q));
      const DPoint3 &pp = p.s.pos[p.s.faceVert(a.p, 0)], &pq = q.s.pos[q.s.faceVert(a.q, 0)];
      uint32_t kept = 0;
      for (const eastl::pair<double, uint32_t> &x : inside)
        if (fabs((pos[x.second] - pp) * np) <= tolerance && fabs((pos[x.second] - pq) * nq) <= tolerance)
          inside[kept++] = x;
      inside.resize(kept);
      uint32_t prev = from;
      for (uint32_t m = 0; m <= inside.size(); ++m)
      {
        const uint32_t next = m < inside.size() ? inside[m].second : to;
        if (onP)
          p_edges.push_back(NewEdge{a.p, prev, next});
        if (onQ)
          q_edges.push_back(q.flip ? NewEdge{a.q, prev, next} : NewEdge{a.q, next, prev});
        prev = next;
      }
    }
    i = j;
  }
  // by face for the assembly, then in one order on every thread count
  for (dag::Vector<NewEdge> *edges : {&p_edges, &q_edges})
    eastl::sort(edges->begin(), edges->end(), [](const NewEdge &x, const NewEdge &y) {
      return x.face != y.face ? x.face < y.face : x.from != y.from ? x.from < y.from : x.to < y.to;
    });
  return true;
}

// A hint for the output buffers: the whole faces of the wanted winding and a few triangles per crossing and new edge
static uint32_t output_hint(const Side &side, uint32_t new_edges)
{
  uint32_t faces = 2 * (side.crossings.size() + new_edges);
  for (uint32_t f = 0; f < side.s.faceCount(); ++f)
    faces += side.winding[side.s.faceVert(f, 0)] == side.want;
  return faces;
}

// edges: sorted by face; hint: output_hint of the side
static bool assemble_side(const Side &side, dag::ConstSpan<NewEdge> edges, uint32_t hint, const KernelParams &params, Assembly &a,
  eastl::string &err)
{
  struct alignas(64) Worker
  {
    Emitted out;
    FaceScratch scratch;
    uint32_t failed = NO_INDEX;
    eastl::string err;
  };
  // workers pay off per 512 new edges
  const WorkerLease lease(params.budget, min(daparallel::worker_count(params.threads), 1 + uint32_t(edges.size()) / 512));
  const uint32_t workers = lease.workers();
  dag::Vector<Worker> work(workers);
  for (Worker &w : work)
    w.out.reserve(hint / workers);
  dag::Vector<daparallel::Chunk> chunks;
  // few heavy faces still spread over workers
  daparallel::parallel_chunks(
    workers, side.s.faceCount(), max(side.s.faceCount() / (8 * workers), 1u), chunks,
    [&](uint32_t begin, uint32_t end, uint32_t worker) {
      Worker &w = work[worker];
      uint32_t next =
        eastl::lower_bound(edges.begin(), edges.end(), begin, [](const NewEdge &e, uint32_t f) { return e.face < f; }) - edges.begin();
      // the first failed face in face order gives the error, as on one thread
      for (uint32_t f = begin; f < end && f < w.failed; ++f)
      {
        const uint32_t first = next;
        while (next < edges.size() && edges[next].face == f)
          ++next;
        if (!assemble_face(side, f, dag::ConstSpan<NewEdge>(edges.data() + first, next - first), a, w.out, w.scratch, w.err))
          w.failed = f;
      }
    },
    [&](uint32_t worker) { return uint32_t(work[worker].out.face.size()); });
  uint32_t failed = NO_INDEX;
  for (const Worker &w : work)
    if (w.failed < failed)
    {
      failed = w.failed;
      err = w.err;
    }
  if (failed != NO_INDEX)
    return false;
  uint32_t total = a.face.size();
  for (const daparallel::Chunk &c : chunks)
    total += c.to - c.from;
  a.reserve(total);
  for (const daparallel::Chunk &c : chunks)
    a.append(work[c.worker].out, c.from, c.to);
  return true;
}

static void copy_solid(const Solid &src, Solid &out)
{
  out = src;
  out.leafIndex = NO_INDEX;
}

static bool same_solid(const Solid &a, const Solid &b)
{
  if (a.pos.size() != b.pos.size() || a.halfedge.size() != b.halfedge.size())
    return false;
  for (uint32_t v = 0; v < a.pos.size(); ++v)
    if (!(a.pos[v] == b.pos[v]))
      return false;
  for (uint32_t h = 0; h < a.halfedge.size(); ++h)
    if (a.halfedge[h].startVert != b.halfedge[h].startVert)
      return false;
  return true;
}

// spec 6.3: avoid shared z-parallel planes
static int projection_axis(const Solid &a, const Solid &b)
{
  const Solid &few = a.faceCount() <= b.faceCount() ? a : b, &many = &few == &a ? b : a;
  dag::Vector<uint64_t> keys(few.facePlaneKey.begin(), few.facePlaneKey.end());
  eastl::sort(keys.begin(), keys.end());
  bool shared = false;
  double least[2] = {1, 1};
  for (uint32_t f = 0; f < many.facePlaneKey.size(); ++f)
    if (many.facePlaneKey[f] != Provenance::NONE && eastl::binary_search(keys.begin(), keys.end(), many.facePlaneKey[f]))
    {
      const DPoint3 n = unit_or_zero(face_cross(many, f));
      shared |= n.z * n.z < 1e-12 * lengthSq(n);
      least[0] = min(least[0], fabs(n.x));
      least[1] = min(least[1], fabs(n.y));
    }
  return !shared || max(least[0], least[1]) < 1e-6 ? 2 : least[0] >= least[1] ? 0 : 1;
}

// cycled(cycled(v, a), 1 - a) == v
static DPoint3 cycled(const DPoint3 &v, int axis) { return axis == 0 ? DPoint3(v.y, v.z, v.x) : DPoint3(v.z, v.x, v.y); }

static void cycle_axes(Solid &s, int axis)
{
  for (DPoint3 &v : s.pos)
    v = cycled(v, axis);
  for (DPoint3 &v : s.vertNormal)
    v = cycled(v, axis);
  for (DPoint3 &v : s.bbox.lim)
    v = cycled(v, axis);
}

static bool boolean_along_z(BoolTree::Kind op, const Solid &a, const Solid &b, const KernelParams &params, Solid &out,
  KernelStats &stats, eastl::string &err)
{
  out = Solid();
  out.closed = true;
  const bool aOpen = a.faceCount() && !a.closed, bOpen = b.faceCount() && !b.closed;
  if (aOpen || bOpen)
  {
    err = aOpen ? "the first operand is not closed" : "the second operand is not closed";
    return false;
  }
  if (!a.faceCount() || !b.faceCount())
  {
    const Solid &kept = a.faceCount() ? a : b;
    if (kept.faceCount() && (op == BoolTree::Kind::Union || (op == BoolTree::Kind::Subtract && a.faceCount())))
      copy_solid(kept, out);
    return true;
  }
  // copies tie everywhere: concave ties stay undecided
  if (same_solid(a, b))
  {
    if (op != BoolTree::Kind::Subtract)
      copy_solid(a, out);
    return true;
  }

  const bool isUnion = op == BoolTree::Kind::Union, isSubtract = op == BoolTree::Kind::Subtract;
  Side p(a, true, isUnion || isSubtract ? 0 : 1, false);
  Side q(b, false, isUnion ? 0 : 1, isSubtract);
  build_edges(p);
  build_edges(q);
  Bvh pTris, qTris;
  build_face_bvh(a, b, params.tolerance, pTris);
  build_face_bvh(b, a, params.tolerance, qTris);
  const Shadow shadow(a, b, isUnion ? 1 : -1);

  Assembly built;
  built.tolerance = params.tolerance;
  built.pos = a.pos;
  built.pos.insert(built.pos.end(), b.pos.begin(), b.pos.end());
  for (const Solid *operand : {&a, &b})
    if (operand->provenance.empty())
      built.provenance.resize(built.provenance.size() + operand->pos.size());
    else
      built.provenance.insert(built.provenance.end(), operand->provenance.begin(), operand->provenance.end());
  p.vertBase = 0;
  q.vertBase = a.pos.size();
  find_crossings(p, q, qTris, shadow, params, built, stats);
  find_crossings(q, p, pTris, shadow, params, built, stats);
  Bvh points;
  dag::Vector<uint32_t> boxVert;
  merge_coincident_crossings(p, q, built, q.vertBase + b.pos.size(), pTris, qTris, points, boxVert);
  stats.add("intersection vertices", boxVert.size());
  index_crossings(p);
  index_crossings(q);
  split_edges_at_crossings(p, q, built);
  split_edges_at_crossings(q, p, built);
  compute_windings(p, qTris, shadow);
  compute_windings(q, pTris, shadow);
  if (!check_windings(p, err) || !check_windings(q, err))
    return false;
  order_coincident(p, params.tolerance);
  order_coincident(q, params.tolerance);
  record_winding_bounds(p);
  record_winding_bounds(q);

  dag::Vector<FacePairVert> pairVerts;
  collect_face_pairs(p, pairVerts);
  collect_face_pairs(q, pairVerts);
  dag::Vector<NewEdge> pNew, qNew;
  if (
    !make_new_edges(p, q, make_span_const(built.pos), points, make_span_const(boxVert), params.tolerance, pairVerts, pNew, qNew, err))
    return false;
  const uint32_t pHint = output_hint(p, pNew.size()), qHint = output_hint(q, qNew.size());
  built.reserve(pHint + qHint);
  if (!assemble_side(p, make_span_const(pNew), pHint, params, built, err) ||
      !assemble_side(q, make_span_const(qNew), qHint, params, built, err))
    return false;
  stats.add("faces before simplify", built.face.size());
  if (!finish_solid(built, out, err))
    return false;
  if (!simplify_solid(out, params.tolerance, err))
    return false;
  stats.add("faces after simplify", out.faceCount());
  return true;
}

bool manifold_boolean(BoolTree::Kind op, const Solid &a, const Solid &b, const KernelParams &params, Solid &out, KernelStats &stats,
  eastl::string &err)
{
  const int axis = projection_axis(a, b);
  if (axis == 2)
    return boolean_along_z(op, a, b, params, out, stats, err);
  Solid cycledA = a, cycledB = b;
  cycle_axes(cycledA, axis);
  cycle_axes(cycledB, axis);
  const bool ok = boolean_along_z(op, cycledA, cycledB, params, out, stats, err);
  cycle_axes(out, 1 - axis);
  return ok;
}

class ManifoldKernel final : public IPairwiseKernel
{
public:
  const char *name() const override { return "manifold"; }
  bool concurrentBooleans() const override { return true; }
  bool boolean(BoolTree::Kind op, const Solid &a, const Solid &b, const KernelParams &p, Solid &out, KernelStats &stats,
    eastl::string &err) override
  {
    return manifold_boolean(op, a, b, p, out, stats, err);
  }
};

IKernel *manifold_kernel_instance()
{
  static ManifoldKernel kernel;
  return &kernel;
}
} // namespace dacsg
