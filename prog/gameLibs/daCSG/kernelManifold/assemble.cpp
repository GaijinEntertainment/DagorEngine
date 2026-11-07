// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "kernelManifold/build.h"
#include "mesh/solidUtil.h"
#include "mesh/polygon.h"
#include "mesh/unionFind.h"
#include <EASTL/sort.h>
#include <math.h>

namespace dacsg
{
// visit(from, to, winding of the piece) along the halfedge
template <class Visit>
static void walk_halfedge(const Side &side, uint32_t he, const Assembly &out, Visit &&visit)
{
  const Halfedge &h = side.s.halfedge[he];
  const uint32_t e = side.edgeOf[he];
  const bool fwd = side.edges[e].he == he;
  const uint32_t count = side.crossStart[e + 1] - side.crossStart[e];
  uint32_t cur = out.id(side.vertBase + h.startVert);
  int w = side.winding[h.startVert];
  for (uint32_t i = 0; i < count; ++i)
  {
    const Crossing &c = side.crossings[side.crossStart[e] + (fwd ? i : count - 1 - i)];
    visit(cur, c.vert, w);
    cur = c.vert;
    w += fwd ? c.s : -c.s;
  }
  visit(cur, out.id(side.vertBase + h.endVert), w);
}

// The boundary enters the other solid at a +1 crossing: with want 0 the retained piece ends there and the new edge starts
int new_edge_direction(const Side &side, uint32_t he, uint32_t crossing)
{
  const Crossing &c = side.crossings[crossing];
  const int s = side.edges[c.edge].he == he ? c.s : -c.s;
  return (s > 0) == (side.want == 0) ? 1 : -1;
}

// a bridge of the ear clipper between two visits of one vertex gives a triangle with that vertex twice: no face
static void emit(const Side &side, uint32_t f, Emitted &out, uint32_t a, uint32_t b, uint32_t c)
{
  if (a == b || b == c || a == c)
    return;
  out.tri.push_back(a);
  out.tri.push_back(side.flip ? c : b);
  out.tri.push_back(side.flip ? b : c);
  out.face.push_back(side.s.face[f]);
  out.leafRef.push_back(side.s.leafRef[f]);
  out.facePlaneKey.push_back(plane_key(side.s, f));
}

void Emitted::append(const Emitted &src, uint32_t from, uint32_t to)
{
  tri.insert(tri.end(), src.tri.begin() + 3 * from, src.tri.begin() + 3 * to);
  face.insert(face.end(), src.face.begin() + from, src.face.begin() + to);
  leafRef.insert(leafRef.end(), src.leafRef.begin() + from, src.leafRef.begin() + to);
  facePlaneKey.insert(facePlaneKey.end(), src.facePlaneKey.begin() + from, src.facePlaneKey.begin() + to);
}

static bool fail_face(const Side &side, uint32_t f, const char *why, eastl::string &err)
{
  err.sprintf("face %u of the %s operand: %s", f, side.isP ? "first" : "second", why);
  return false;
}

// At a vertex the loop touches twice the sharpest left turn continues, and the part since the first visit becomes
// a loop of its own, so every loop is simple and the loops only touch
template <typename Flat>
static bool chain_loops(FaceScratch &scratch, const Flat &flat)
{
  dag::Vector<Seg> &segs = scratch.segs;
  dag::Vector<uint8_t> &used = scratch.used;
  dag::Vector<uint32_t> &split = scratch.split, &splitStart = scratch.splitStart, &loop_verts = scratch.loopVerts,
                        &loop_start = scratch.loopStart, &loop_slots = scratch.loopSlots, &visitMark = scratch.visitMark,
                        &visitAt = scratch.visitAt;
  eastl::sort(segs.begin(), segs.end(), [](const Seg &a, const Seg &b) { return a.from != b.from ? a.from < b.from : a.to < b.to; });
  used.assign(segs.size(), 0);
  // a vertex's slot is the first seg from it: visitMark[slot] == mark puts it in the loop at loop_verts[visitAt[slot]]; the
  // marks count on across faces, so a stale one never matches
  visitMark.resize(segs.size(), 0);
  visitAt.resize(segs.size());
  const auto slot_of = [&](uint32_t v) {
    return uint32_t(
      eastl::lower_bound(segs.begin(), segs.end(), v, [](const Seg &s, uint32_t w) { return s.from < w; }) - segs.begin());
  };
  split.clear();
  splitStart.clear();
  loop_verts.clear();
  loop_slots.clear();
  loop_start.clear();
  for (uint32_t i = 0; i < segs.size(); ++i)
  {
    if (used[i])
      continue;
    const uint32_t begin = loop_verts.size(), mark = ++scratch.loopMark;
    loop_start.push_back(begin);
    const uint32_t first = segs[i].from;
    for (uint32_t cur = i, slot = slot_of(first);;)
    {
      used[cur] = 1;
      visitMark[slot] = mark;
      visitAt[slot] = loop_verts.size();
      loop_verts.push_back(segs[cur].from);
      loop_slots.push_back(slot);
      const uint32_t to = segs[cur].to;
      if (to == first)
        break;
      uint32_t j = slot_of(to);
      if (j < segs.size() && segs[j].from == to && visitMark[j] == mark)
      {
        const uint32_t k = visitAt[j];
        splitStart.push_back(split.size());
        split.insert(split.end(), loop_verts.begin() + k, loop_verts.end());
        for (uint32_t m = k; m < loop_verts.size(); ++m)
          visitMark[loop_slots[m]] = 0;
        loop_verts.resize(k);
        loop_slots.resize(k);
      }
      slot = j;
      const DPoint2 in = flat(to) - flat(segs[cur].from);
      uint32_t best = NO_INDEX;
      double bestTurn = 0;
      for (; j < segs.size() && segs[j].from == to; ++j)
      {
        if (used[j])
          continue;
        const DPoint2 out = flat(segs[j].to) - flat(to);
        const double turn = atan2(in.x * out.y - in.y * out.x, in.x * out.x + in.y * out.y);
        if (best == NO_INDEX || turn > bestTurn)
        {
          best = j;
          bestTurn = turn;
        }
      }
      if (best == NO_INDEX)
        return false;
      cur = best;
    }
  }
  splitStart.push_back(split.size());
  for (uint32_t l = 0; l + 1 < splitStart.size(); ++l)
  {
    loop_start.push_back(loop_verts.size());
    loop_verts.insert(loop_verts.end(), split.begin() + splitStart[l], split.begin() + splitStart[l + 1]);
  }
  loop_start.push_back(loop_verts.size());
  return true;
}

// relative to the first point, so a loop within rounding of one point has an area of that size, not of the coordinates
static double loop_area(dag::ConstSpan<DPoint2> pts)
{
  double a = 0;
  for (uint32_t i = 0, j = pts.size() - 1; i < pts.size(); j = i++)
  {
    const DPoint2 u = pts[j] - pts[0], v = pts[i] - pts[0];
    a += u.x * v.y - v.x * u.y;
  }
  return a * 0.5;
}

// Crossings merged into one vertex can leave a new edge between two regions that are both dropped: it leaves a
// vertex with more segments out than in for one with more in than out, and no loop can hold it. A drop takes one
// out from its start and one in from its end, which makes no other edge an orphan, so one pass in order finds them
static void drop_orphan_new_edges(dag::Vector<Seg> &segs, uint32_t piece_count, dag::Vector<VertexFlow> &flow)
{
  flow.clear();
  for (const Seg &s : segs)
  {
    flow.push_back(VertexFlow{s.from, 1});
    flow.push_back(VertexFlow{s.to, -1});
  }
  eastl::sort(flow.begin(), flow.end(), [](const VertexFlow &a, const VertexFlow &b) { return a.vert < b.vert; });
  uint32_t distinct = 0;
  for (uint32_t i = 0; i < flow.size(); ++i)
    if (distinct && flow[distinct - 1].vert == flow[i].vert)
      flow[distinct - 1].outMinusIn += flow[i].outMinusIn;
    else
      flow[distinct++] = flow[i];
  flow.resize(distinct);
  const auto at = [&](uint32_t v) {
    return eastl::lower_bound(flow.begin(), flow.end(), v, [](const VertexFlow &f, uint32_t vert) { return f.vert < vert; });
  };
  uint32_t live = piece_count;
  for (uint32_t i = piece_count; i < segs.size(); ++i)
  {
    VertexFlow *from = at(segs[i].from), *to = at(segs[i].to);
    if (from->outMinusIn > 0 && to->outMinusIn < 0)
    {
      --from->outMinusIn;
      ++to->outMinusIn;
      continue;
    }
    segs[live++] = segs[i];
  }
  segs.resize(live);
}

// the smallest altitude: the doubled area over the longest side
static double altitude2(const DPoint2 &a, const DPoint2 &b, const DPoint2 &c)
{
  const double longest = sqrt(max(max(lengthSq(b - a), lengthSq(c - b)), lengthSq(a - c)));
  return longest > 0 ? fabs(orient2(a, b, c)) / longest : 0;
}

// thinner triangles fold within the validator's tolerance
static constexpr double THIN_TOLERANCES = 8;

// twin[h] is the corner of the other triangle on the edge that starts at corner h, NO_INDEX on the piece's border
static void find_twins(const dag::Vector<uint32_t> &tris, dag::Vector<EdgeKey> &keys, dag::Vector<uint32_t> &twin)
{
  keys.resize(tris.size());
  for (uint32_t h = 0; h < tris.size(); ++h)
    keys[h] = EdgeKey{(uint64_t(tris[h]) << 32) | tris[next_he(h)], h};
  eastl::sort(keys.begin(), keys.end());
  twin.assign(tris.size(), NO_INDEX);
  for (uint32_t h = 0; h < tris.size(); ++h)
  {
    const EdgeKey back{(uint64_t(tris[next_he(h)]) << 32) | tris[h], 0};
    const auto it = eastl::lower_bound(keys.begin(), keys.end(), back);
    if (it != keys.end() && it->key == back.key)
      twin[h] = it->he;
  }
}

// the triangle (a b c) at corner h and (b a d) across become (a d c) and (d b c)
static void flip_edge(dag::Vector<uint32_t> &tris, dag::Vector<uint32_t> &twin, uint32_t h)
{
  const uint32_t m = twin[h], b = tris[next_he(h)], c = tris[prev_he(h)], d = tris[prev_he(m)];
  const uint32_t ad = twin[next_he(m)], db = twin[prev_he(m)], bc = twin[next_he(h)];
  tris[next_he(h)] = d;
  tris[m] = d;
  tris[next_he(m)] = b;
  tris[prev_he(m)] = c;
  const auto link = [&](uint32_t x, uint32_t y) {
    twin[x] = y;
    if (y != NO_INDEX)
      twin[y] = x;
  };
  link(h, ad);
  link(next_he(h), prev_he(m));
  link(m, db);
  link(next_he(m), bc);
}

// A triangle thinner than thin takes the other diagonal of its quad with a neighbour of the same piece when that
// raises the smaller altitude of the two: a needle on a straight run of the cut joins a vertex off the run instead.
// Each flip raises the sorted altitudes of the piece, so the pass ends
static void fatten_triangles(dag::Vector<uint32_t> &tris, const dag::Vector<DPoint2> &flat, double thin, FaceScratch &scratch)
{
  const uint32_t count = tris.size() / 3;
  dag::Vector<uint32_t> &work = scratch.work;
  work.clear();
  if (count >= 2)
    for (uint32_t t = 0; t < count; ++t)
      if (altitude2(flat[tris[3 * t]], flat[tris[3 * t + 1]], flat[tris[3 * t + 2]]) < thin)
        work.push_back(t);
  if (work.empty())
    return;
  dag::Vector<uint32_t> &twin = scratch.twin;
  find_twins(tris, scratch.cornerKeys, twin);
  dag::Vector<uint8_t> &queued = scratch.queued;
  queued.assign(count, 0);
  for (const uint32_t t : work)
    queued[t] = 1;
  const auto push = [&](uint32_t h) {
    if (h != NO_INDEX && !queued[h / 3])
    {
      queued[h / 3] = 1;
      work.push_back(h / 3);
    }
  };
  for (uint32_t flips = 0; !work.empty() && flips < 8 * count;)
  {
    const uint32_t t = work.back();
    work.pop_back();
    queued[t] = 0;
    const uint32_t *v = tris.data() + 3 * t;
    const double own = altitude2(flat[v[0]], flat[v[1]], flat[v[2]]);
    if (own >= thin)
      continue;
    uint32_t bestCorner = NO_INDEX;
    double best = 0;
    for (uint32_t h = 3 * t; h < 3 * t + 3; ++h)
    {
      const uint32_t m = twin[h];
      if (m == NO_INDEX)
        continue;
      const DPoint2 &pa = flat[tris[h]], &pb = flat[tris[next_he(h)]], &pc = flat[tris[prev_he(h)]], &pd = flat[tris[prev_he(m)]];
      if (!(orient2(pa, pd, pc) > 0) || !(orient2(pd, pb, pc) > 0))
        continue;
      const double gain = min(altitude2(pa, pd, pc), altitude2(pd, pb, pc));
      if (gain > min(own, altitude2(pb, pa, pd)) && gain > best)
      {
        best = gain;
        bestCorner = h;
      }
    }
    if (bestCorner == NO_INDEX)
      continue;
    const uint32_t m = twin[bestCorner];
    flip_edge(tris, twin, bestCorner);
    ++flips;
    // the two triangles and the four around them have new quads
    const uint32_t flipped[2] = {bestCorner, m};
    for (const uint32_t h : flipped)
      for (uint32_t k = h - h % 3; k < h - h % 3 + 3; ++k)
      {
        push(k);
        push(twin[k]);
      }
  }
}

bool assemble_face(const Side &side, uint32_t f, dag::ConstSpan<NewEdge> new_edges, const Assembly &in, Emitted &out,
  FaceScratch &scratch, eastl::string &err)
{
  bool cut = !new_edges.empty();
  for (int k = 0; k < 3; ++k)
  {
    const uint32_t e = side.edgeOf[3 * f + k];
    cut |= side.crossStart[e + 1] > side.crossStart[e];
  }
  if (!cut)
  {
    if (side.winding[side.s.faceVert(f, 0)] == side.want)
      emit(side, f, out, in.id(side.vertBase + side.s.faceVert(f, 0)), in.id(side.vertBase + side.s.faceVert(f, 1)),
        in.id(side.vertBase + side.s.faceVert(f, 2)));
    return true;
  }

  // a piece between two crossings merged into one vertex has no length and no side
  dag::Vector<Seg> &segs = scratch.segs;
  segs.clear();
  for (int k = 0; k < 3; ++k)
    walk_halfedge(side, 3 * f + k, in, [&](uint32_t from, uint32_t to, int w) {
      if (w == side.want && from != to)
        segs.push_back(Seg{from, to});
    });
  const uint32_t pieceCount = segs.size();
  for (const NewEdge &e : new_edges)
    if (e.from != e.to)
      segs.push_back(Seg{e.from, e.to});
  drop_orphan_new_edges(segs, pieceCount, scratch.flow);
  if (segs.empty())
    return true;
  const DPoint3 n = face_cross(side.s, f);
  const int axis = dominant_axis(n);
  const int ua = (axis + 1) % 3, va = (axis + 2) % 3;
  const bool mirror = n[axis] < 0;
  const auto flat = [&](uint32_t id) {
    const DPoint3 &p = in.pos[id];
    return mirror ? DPoint2(p[va], p[ua]) : DPoint2(p[ua], p[va]);
  };
  if (!chain_loops(scratch, flat))
    return fail_face(side, f, "the boundary does not close", err);

  const dag::Vector<uint32_t> &loopVerts = scratch.loopVerts, &loopStart = scratch.loopStart;
  const uint32_t loopCount = loopStart.size() - 1;
  dag::Vector<DPoint2> &pts = scratch.pts;
  pts.clear();
  for (const uint32_t v : loopVerts)
    pts.push_back(flat(v));
  const auto loop = [&](uint32_t l) { return dag::ConstSpan<DPoint2>(pts.data() + loopStart[l], loopStart[l + 1] - loopStart[l]); };
  dag::Vector<double> &area = scratch.area;
  area.resize(loopCount);
  dag::Vector<uint32_t> &outerOf = scratch.outerOf;
  outerOf.assign(loopCount, NO_INDEX);
  for (uint32_t l = 0; l < loopCount; ++l)
    area[l] = loop(l).size() < 3 ? 0 : loop_area(loop(l));
  for (uint32_t l = 0; l < loopCount; ++l)
  {
    if (area[l] >= 0)
      continue;
    // a hole can touch its loop along an edge, so any vertex strictly inside counts
    uint32_t best = NO_INDEX;
    for (uint32_t o = 0; o < loopCount; ++o)
    {
      if (area[o] < 0 || loop(o).size() < 3 || (best != NO_INDEX && area[o] >= area[best]))
        continue;
      for (const DPoint2 &hp : loop(l))
        if (inside_loop(loop(o), hp))
        {
          best = o;
          break;
        }
    }
    if (best != NO_INDEX)
    {
      outerOf[l] = best;
      continue;
    }
    // a sliver loop of nearly zero area can come out negative and off its outer loop: it stands alone, flat
    double perimeter = 0;
    const dag::ConstSpan<DPoint2> hole = loop(l);
    for (uint32_t i = 0, j = hole.size() - 1; i < hole.size(); j = i++)
      perimeter += length(hole[i] - hole[j]);
    if (-area[l] > in.tolerance * perimeter)
      return fail_face(side, f, "a hole lies outside every loop", err);
    area[l] = 0;
  }

  dag::Vector<uint32_t> &ids = scratch.ids, &tris = scratch.tris;
  dag::Vector<DPoint2> &flatIds = scratch.flatIds;
  dag::Vector<dag::ConstSpan<DPoint2>> &holes = scratch.holes;
  for (uint32_t o = 0; o < loopCount; ++o)
  {
    if (area[o] < 0 || loop(o).size() < 3)
      continue;
    ids.assign(loopVerts.begin() + loopStart[o], loopVerts.begin() + loopStart[o + 1]);
    flatIds.assign(pts.begin() + loopStart[o], pts.begin() + loopStart[o + 1]);
    holes.clear();
    for (uint32_t l = 0; l < loopCount; ++l)
      if (outerOf[l] == o)
      {
        holes.push_back(loop(l));
        ids.insert(ids.end(), loopVerts.begin() + loopStart[l], loopVerts.begin() + loopStart[l + 1]);
        flatIds.insert(flatIds.end(), pts.begin() + loopStart[l], pts.begin() + loopStart[l + 1]);
      }
    eastl::string why;
    if (!triangulate_polygon(loop(o), make_span_const(holes), tris, why, true))
      return fail_face(side, f, why.c_str(), err);
    fatten_triangles(tris, flatIds, THIN_TOLERANCES * in.tolerance, scratch);
    for (uint32_t i = 0; i < tris.size(); i += 3)
      emit(side, f, out, ids[tris[i]], ids[tris[i + 1]], ids[tris[i + 2]]);
  }
  return true;
}

static uint32_t third_vert(const Solid &s, uint32_t he) { return s.halfedge[prev_he(he)].startVert; }

// Faces around an edge with 2k halfedges: sorted by angle, a face whose halfedge runs v1->v0 opens a wedge, the next closes it
static bool pair_wedges(Solid &s, dag::ConstSpan<EdgeKey> group, double width)
{
  const uint32_t h0 = group[0].he;
  const uint32_t v0 = s.halfedge[h0].startVert < s.halfedge[h0].endVert ? s.halfedge[h0].startVert : s.halfedge[h0].endVert;
  const uint32_t v1 = s.halfedge[h0].startVert ^ s.halfedge[h0].endVert ^ v0;
  const DPoint3 origin = s.pos[v0];
  const DPoint3 d = unit_or_zero(s.pos[v1] - origin);
  const DPoint3 w0 = s.pos[third_vert(s, h0)] - origin;
  const DPoint3 r0 = unit_or_zero(w0 - d * (d * w0)), r1 = d % r0;
  struct Angled
  {
    double angle, reach;
    uint32_t he;
  };
  dag::Vector<Angled> order;
  for (const EdgeKey &k : group)
  {
    const DPoint3 w = s.pos[third_vert(s, k.he)] - origin;
    order.push_back(Angled{atan2(w * r1, w * r0), length(w - d * (d * w)), k.he});
  }
  eastl::sort(order.begin(), order.end(),
    [](const Angled &a, const Angled &b) { return a.angle != b.angle ? a.angle < b.angle : a.he < b.he; });
  const uint32_t n = order.size();
  const auto opens = [&](uint32_t i) { return s.halfedge[order[i].he].startVert == v1; };
  // coplanar faces have no order from their angle: in a cluster of equal angles an opener goes first when the
  // wedge before it is empty, a closer when it is solid, so every cluster is a sliver, not a flipped pairing
  const auto sameAngle = [&](uint32_t i, uint32_t j) {
    const double gap = fabs(order[i].angle - order[j].angle), arc = eastl::min(gap, 6.28318530717958647692 - gap);
    return arc <= 1e-9 || arc * eastl::min(order[i].reach, order[j].reach) < width; // circular: -pi and pi meet
  };
  uint32_t start = 0;
  while (start < n && sameAngle(start, (start + n - 1) % n))
    ++start;
  if (start == n)
    start = 0;
  for (int pass = 0; pass < 2; ++pass)
  {
    bool solid = pass == 1;
    for (uint32_t i = 0; i < n;)
    {
      uint32_t end = i + 1;
      while (end < n && sameAngle((start + end) % n, (start + end - 1) % n))
        ++end;
      for (uint32_t k = i; k < end; ++k)
      {
        uint32_t pick = k;
        while (pick < end && opens((start + pick) % n) == solid)
          ++pick;
        if (pick < end)
          eastl::swap(order[(start + k) % n], order[(start + pick) % n]);
        solid = opens((start + k) % n);
      }
      i = end;
    }
    bool ok = true;
    for (uint32_t i = 0; i < n && ok; ++i)
      ok = opens(i) != opens((i + 1) % n);
    if (!ok)
      continue;
    for (uint32_t i = 0; i < n; ++i)
    {
      const uint32_t a = order[i].he, b = order[(i + 1) % n].he;
      if (!opens(i))
        continue;
      s.halfedge[a].pairedHalfedge = b;
      s.halfedge[b].pairedHalfedge = a;
    }
    return true;
  }
  return false;
}

static double line_dist(const Solid &s, uint32_t he)
{
  const DPoint3 &a = s.pos[s.halfedge[he].startVert], d = s.pos[s.halfedge[he].endVert] - a;
  const DPoint3 w = s.pos[third_vert(s, he)] - a;
  const double dd = d * d;
  return dd > 0 ? length(w - d * ((w * d) / dd)) : length(w);
}

// Two faces on the same three points are a skin, two flat faces along the edge (a zero-width strip) a pair without
// an angle: each kind pairs with its own first, the rest by wedge order
static bool pair_group(Solid &s, dag::Vector<EdgeKey> &keys, uint32_t first, uint32_t end, dag::Vector<EdgeKey> &rest,
  double tolerance)
{
  rest.clear();
  for (int kind = 0; kind < 2; ++kind)
    for (uint32_t k = first; k < end; ++k)
    {
      const uint32_t a = keys[k].he;
      if (s.halfedge[a].pairedHalfedge != NO_INDEX || (kind == 1 && line_dist(s, a) > tolerance))
        continue;
      for (uint32_t m = k + 1; m < end; ++m)
      {
        const uint32_t b = keys[m].he;
        if (s.halfedge[b].pairedHalfedge != NO_INDEX || s.halfedge[a].startVert == s.halfedge[b].startVert)
          continue;
        if (kind == 0 ? s.pos[third_vert(s, a)] == s.pos[third_vert(s, b)] : line_dist(s, b) <= tolerance)
        {
          s.halfedge[a].pairedHalfedge = b;
          s.halfedge[b].pairedHalfedge = a;
          break;
        }
      }
    }
  for (uint32_t k = first; k < end; ++k)
    if (s.halfedge[keys[k].he].pairedHalfedge == NO_INDEX)
      rest.push_back(keys[k]);
  // merged walls of a sub-tolerance fin can cross
  const dag::ConstSpan<EdgeKey> left = make_span_const(rest);
  return rest.empty() || (rest.size() % 2 == 0 && (pair_wedges(s, left, 0) || pair_wedges(s, left, tolerance)));
}

// crowded: the edges of more than two halfedges it paired
static bool pair_output(Solid &s, double tolerance, uint32_t &crowded, eastl::string &err)
{
  const uint32_t heCount = s.halfedge.size();
  dag::Vector<EdgeKey> keys(heCount), scratch;
  // the sort of all keys as a count by the lower vertex, then each vertex's few keys
  dag::Vector<uint32_t> start(s.pos.size() + 1, 0);
  for (const Halfedge &e : s.halfedge)
    ++start[min(e.startVert, e.endVert) + 1];
  for (uint32_t v = 0; v < s.pos.size(); ++v)
    start[v + 1] += start[v];
  for (uint32_t h = 0; h < heCount; ++h)
  {
    const Halfedge &e = s.halfedge[h];
    const uint32_t lo = e.startVert < e.endVert ? e.startVert : e.endVert, hi = e.startVert ^ e.endVert ^ lo;
    keys[start[lo]++] = EdgeKey{(uint64_t(lo) << 32) | hi, h};
    s.halfedge[h].pairedHalfedge = NO_INDEX;
  }
  for (uint32_t v = 0, first = 0; v < s.pos.size(); first = start[v++])
    eastl::insertion_sort(keys.begin() + first, keys.begin() + start[v]);
  uint32_t bad = 0;
  for (uint32_t i = 0; i < heCount;)
  {
    uint32_t j = i + 1;
    while (j < heCount && keys[j].key == keys[i].key)
      ++j;
    if (j - i == 2 && s.halfedge[keys[i].he].startVert != s.halfedge[keys[i + 1].he].startVert)
    {
      const uint32_t a = keys[i].he, b = keys[i + 1].he;
      s.halfedge[a].pairedHalfedge = b;
      s.halfedge[b].pairedHalfedge = a;
    }
    else if ((j - i) % 2 || j - i == 2 || !pair_group(s, keys, i, j, scratch, tolerance))
      ++bad;
    else
      ++crowded;
    i = j;
  }
  if (bad)
    err.sprintf("the result is not closed: %u edges have no pairing", bad);
  return bad == 0;
}

// A vertex with several fans is duplicated so each shell is a manifold
static void split_fans(Solid &s)
{
  const uint32_t heCount = s.halfedge.size();
  dag::Vector<uint8_t> seen(heCount, 0);
  dag::Vector<uint8_t> vertSeen(s.pos.size(), 0);
  for (uint32_t h0 = 0; h0 < heCount; ++h0)
  {
    if (seen[h0])
      continue;
    const uint32_t v = s.halfedge[h0].startVert;
    uint32_t id = v;
    if (vertSeen[v])
    {
      id = s.pos.size();
      const DPoint3 copy = s.pos[v];
      s.pos.push_back(copy);
      if (!s.provenance.empty())
      {
        const Provenance p = s.provenance[v];
        s.provenance.push_back(p);
      }
    }
    vertSeen[v] = 1;
    for (uint32_t h = h0;;)
    {
      seen[h] = 1;
      const uint32_t prev = prev_he(h);
      s.halfedge[h].startVert = id;
      s.halfedge[prev].endVert = id;
      h = s.halfedge[prev].pairedHalfedge;
      if (h == h0 || seen[h])
        break;
    }
  }
}

void compact_vertices(Solid &s)
{
  dag::Vector<uint32_t> remap(s.pos.size(), NO_INDEX);
  dag::Vector<DPoint3> pos;
  dag::Vector<Provenance> provenance;
  // every vertex in use starts a halfedge
  const uint32_t used = min(uint32_t(s.pos.size()), uint32_t(s.halfedge.size()));
  pos.reserve(used);
  provenance.reserve(s.provenance.empty() ? 0 : used);
  for (Halfedge &e : s.halfedge)
  {
    if (remap[e.startVert] == NO_INDEX)
    {
      remap[e.startVert] = pos.size();
      pos.push_back(s.pos[e.startVert]);
      if (!s.provenance.empty())
        provenance.push_back(s.provenance[e.startVert]);
    }
  }
  for (Halfedge &e : s.halfedge)
  {
    e.startVert = remap[e.startVert];
    e.endVert = remap[e.endVert];
  }
  s.pos = eastl::move(pos);
  s.provenance = eastl::move(provenance);
  s.bbox = points_bbox(s.pos);
}

bool pair_and_split(Solid &s, double tolerance, eastl::string &err)
{
  uint32_t crowded = 0;
  if (!pair_output(s, tolerance, crowded, err))
    return false;
  split_fans(s);
  compact_vertices(s);
  // a crowded edge whose two ends are one fan each keeps its faces
  dag::Vector<uint32_t> pair;
  const EdgeCounts c = crowded ? pair_halfedges(s.halfedge, s.pos.size(), pair) : EdgeCounts();
  s.closed = c.closed();
  if (!s.closed)
    err.sprintf("the result is not closed: %u edges keep more than two faces", c.overPairedEdges);
  return s.closed;
}

// Two crossings at one point give an edge of zero length: its vertices are one, its faces are flat and go
static void weld_zero_edges(Assembly &a)
{
  UnionFind weld;
  weld.init(a.pos.size());
  bool any = false;
  for (uint32_t h = 0; h < a.tri.size(); ++h)
  {
    const uint32_t s = a.tri[h], e = a.tri[next_he(h)];
    if (s != e && a.pos[s] == a.pos[e])
    {
      weld.joinLower(s, e);
      any = true;
    }
  }
  if (!any)
    return;
  // the welded vertex lies on the planes both lay on
  for (uint32_t v = 0; v < a.pos.size(); ++v)
    a.provenance[weld.find(v)].intersect(a.provenance[v]);
  uint32_t live = 0;
  for (uint32_t f = 0; f < a.face.size(); ++f)
  {
    const uint32_t v0 = weld.find(a.tri[3 * f]), v1 = weld.find(a.tri[3 * f + 1]), v2 = weld.find(a.tri[3 * f + 2]);
    if (v0 == v1 || v1 == v2 || v0 == v2)
      continue;
    a.tri[3 * live] = v0;
    a.tri[3 * live + 1] = v1;
    a.tri[3 * live + 2] = v2;
    a.face[live] = a.face[f];
    a.leafRef[live] = a.leafRef[f];
    a.facePlaneKey[live] = a.facePlaneKey[f];
    ++live;
  }
  a.tri.resize(3 * live);
  a.face.resize(live);
  a.leafRef.resize(live);
  a.facePlaneKey.resize(live);
}

bool finish_solid(Assembly &a, Solid &out, eastl::string &err)
{
  weld_zero_edges(a);
  out = Solid();
  out.pos = eastl::move(a.pos);
  out.provenance = eastl::move(a.provenance);
  out.face = eastl::move(a.face);
  out.leafRef = eastl::move(a.leafRef);
  out.facePlaneKey = eastl::move(a.facePlaneKey);
  out.halfedge.resize(a.tri.size());
  for (uint32_t h = 0; h < a.tri.size(); ++h)
    out.halfedge[h] = Halfedge{a.tri[h], a.tri[next_he(h)], NO_INDEX};
  return pair_and_split(out, a.tolerance, err);
}
} // namespace dacsg
