// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "cleanup/overlap.h"
#include "mesh/bvh.h"
#include "mesh/polygon.h"
#include "mesh/unionFind.h"
#include "validate/intersect.h"
#include <util/dag_hashedKeyMap.h>
#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
#include <math.h>

namespace dacsg
{
namespace
{
double cross2(const DPoint2 &a, const DPoint2 &b) { return a.x * b.y - a.y * b.x; }

bool within_plane(const Solid &s, uint32_t f, const DPoint3 &n, double d, double tol)
{
  for (int k = 0; k < 3; ++k)
    if (fabs(n * s.pos[s.faceVert(f, k)] - d) > tol)
      return false;
  return true;
}

using Insert = eastl::pair<double, uint32_t>; // position along the edge, overlay vertex

struct RimEdge
{
  uint32_t he, a, b; // mesh vertices until indexed, then overlay vertices
  uint8_t side;
  dag::Vector<Insert> inserted;
};

struct Cycle
{
  dag::Vector<uint32_t> hes;
  uint32_t ptFrom = 0; // its tail points in Overlay::cyclePt
  double area = 0;
  uint32_t comp = NO_INDEX, parent = NO_INDEX; // parent: the positive cycle around a component's outer cycle
  int w[2] = {0, 0};                           // windings of the two sides' regions on the cycle's left
  bool known = false;
};

// The planar graph of the two sides' rims in the plane: a cell inside both sides is the overlap, a cell inside one
// side becomes new faces of that side. The windings follow from the rim directions, so no point sampling is needed.
struct Overlay
{
  const Solid &s;
  const OverlapGroup &g;
  const double tol;
  const ChartTest *charts;
  OverlapEdit &edit;
  const int u, v;
  dag::Vector<eastl::pair<uint32_t, uint8_t>> sides; // the group's faces in order, each with its side
  dag::Vector<uint32_t> vertId, alias;
  dag::Vector<uint8_t> vertSide; // bit 1 side A, bit 2 side B
  dag::Vector<DPoint2> pt;
  dag::Vector<RimEdge> rim;
  dag::Vector<uint32_t> ea, eb; // graph edges, ea < eb
  dag::Vector<int> da, db;      // each side's rim direction along ea -> eb
  dag::Vector<uint32_t> outFirst, outHe;
  dag::Vector<Cycle> cycles;
  dag::Vector<uint32_t> cycleOf, tris;
  dag::Vector<DPoint2> cyclePt;

  Overlay(const Solid &solid, const OverlapGroup &group, double tolerance, const ChartTest *ct, OverlapEdit &e) :
    s(solid), g(group), tol(tolerance), charts(ct), edit(e), u(plane_axis(group.n, 1)), v(plane_axis(group.n, 2))
  {
    for (uint32_t f : g.faces)
      sides.push_back(eastl::make_pair(f, uint8_t(face_cross(s, f) * g.n > 0 ? 1 : 2)));
    eastl::sort(sides.begin(), sides.end());
  }

  // 0 for a face outside the group
  uint8_t side(uint32_t f) const
  {
    const auto at = eastl::lower_bound(sides.begin(), sides.end(), eastl::make_pair(f, uint8_t(0)));
    return at != sides.end() && at->first == f ? at->second : 0;
  }

  DPoint2 flat(const DPoint3 &p) const { return DPoint2(p[u], p[v]); }

  // the boundary halfedges of a side: closed loops without a pinch vertex
  bool collectRim(uint8_t sd)
  {
    dag::Vector<eastl::pair<uint32_t, uint32_t>> outs; // start vertex, halfedge
    for (uint32_t f : g.faces)
      for (int k = 0; k < 3 && side(f) == sd; ++k)
      {
        const uint32_t h = 3 * f + k, twin = s.halfedge[h].pairedHalfedge;
        if (twin == NO_INDEX || side(twin / 3) != sd)
          outs.push_back(eastl::make_pair(s.halfedge[h].startVert, h));
        else if (!same_look(s, f, twin / 3, charts))
          return false;
      }
    eastl::sort(outs.begin(), outs.end());
    for (uint32_t i = 1; i < outs.size(); ++i)
      if (outs[i].first == outs[i - 1].first)
        return false;
    for (const auto &o : outs)
    {
      const uint32_t end = s.halfedge[o.second].endVert;
      const auto at = eastl::lower_bound(outs.begin(), outs.end(), eastl::make_pair(end, 0u));
      if (at == outs.end() || at->first != end)
        return false;
      rim.push_back(RimEdge{o.second, o.first, end, sd});
    }
    return !outs.empty();
  }

  void indexVertices()
  {
    for (const RimEdge &e : rim)
    {
      vertId.push_back(e.a);
      vertId.push_back(e.b);
    }
    eastl::sort(vertId.begin(), vertId.end());
    vertId.erase(eastl::unique(vertId.begin(), vertId.end()), vertId.end());
    alias.resize(vertId.size());
    vertSide.assign(vertId.size(), 0);
    for (uint32_t i = 0; i < vertId.size(); ++i)
    {
      alias[i] = i;
      pt.push_back(flat(s.pos[vertId[i]]));
    }
    for (RimEdge &e : rim)
    {
      e.a = eastl::lower_bound(vertId.begin(), vertId.end(), e.a) - vertId.begin();
      e.b = eastl::lower_bound(vertId.begin(), vertId.end(), e.b) - vertId.begin();
      vertSide[e.a] |= e.side;
      vertSide[e.b] |= e.side;
    }
  }

  // a vertex within the tolerance of one on the other side is that vertex; two of one side that close are a defect
  bool weld()
  {
    for (uint32_t i = 0; i < pt.size(); ++i)
      for (uint32_t j = i + 1; j < pt.size(); ++j)
      {
        if (!(lengthSq(s.pos[vertId[i]] - s.pos[vertId[j]]) <= tol * tol))
          continue;
        if ((vertSide[i] & vertSide[j]) != 0 || alias[i] != i || alias[j] != j)
          return false;
        const uint32_t keep = (vertSide[i] & 1) ? i : j, gone = keep == i ? j : i;
        alias[gone] = keep;
        vertSide[keep] |= vertSide[gone];
        edit.rename.push_back(vertId[gone]);
        edit.rename.push_back(vertId[keep]);
      }
    for (RimEdge &e : rim)
    {
      e.a = alias[e.a];
      e.b = alias[e.b];
    }
    return true;
  }

  // the projection loosens these tests up to sqrt(3)
  bool nearEnd(const DPoint2 &p, const RimEdge &e) const { return min(lengthSq(p - pt[e.a]), lengthSq(p - pt[e.b])) <= tol * tol; }

  // a vertex on a rim edge splits it: a T junction of the two rims, or a vertex of one rim on an edge of the other
  void insertVertices()
  {
    for (RimEdge &e : rim)
    {
      const DPoint2 &a = pt[e.a], d = pt[e.b] - a;
      const double dd = d * d;
      for (uint32_t x = 0; x < pt.size(); ++x)
      {
        if (alias[x] != x || x == e.a || x == e.b || !(dd > 0))
          continue;
        const double t = (pt[x] - a) * d / dd;
        if (t > 0 && t < 1 && fabs(cross2(d, pt[x] - a)) <= tol * sqrt(dd))
          e.inserted.push_back(Insert(t, x));
      }
    }
  }

  // a proper crossing of the two rims has no vertex of either: the kernels put one there, so the group is not sound
  bool rimsCross() const
  {
    for (const RimEdge &e : rim)
      for (const RimEdge &f : rim)
      {
        if (e.side != 1 || f.side != 2 || e.a == f.a || e.a == f.b || e.b == f.a || e.b == f.b)
          continue;
        const DPoint2 a = pt[e.a], c = pt[f.a], r = pt[e.b] - a, q = pt[f.b] - c;
        const double den = cross2(r, q);
        if (fabs(den) <= 1e-12 * sqrt((r * r) * (q * q)))
          continue;
        const double t = cross2(c - a, q) / den, w = cross2(c - a, r) / den;
        if (t > 0 && t < 1 && w > 0 && w < 1 && !nearEnd(a + r * t, e) && !nearEnd(a + r * t, f))
          return true;
      }
    return false;
  }

  // the rims split at their inserted vertices, as undirected edges with each side's direction; a face across the rim
  // outside the group splits at the same vertices
  bool buildEdges()
  {
    dag::Vector<eastl::pair<uint64_t, eastl::pair<int, int>>> keys; // the vertex pair, the directions of the sides
    const uint32_t groupFaces = edit.dead.size();
    for (RimEdge &e : rim)
    {
      eastl::sort(e.inserted.begin(), e.inserted.end());
      const uint32_t twin = s.halfedge[e.he].pairedHalfedge;
      if (twin != NO_INDEX && side(twin / 3) == 0 && !e.inserted.empty())
      {
        // the face across the rim takes the seam vertices as a fan; a second rim edge on it is too much
        for (uint32_t i = groupFaces; i < edit.dead.size(); ++i)
          if (edit.dead[i] == twin / 3)
            return false;
        edit.dead.push_back(twin / 3);
        const uint32_t end = s.halfedge[twin].endVert, third = s.halfedge[next_he(twin)].endVert;
        uint32_t prev = s.halfedge[twin].startVert;
        for (uint32_t i = e.inserted.size() + 1; i-- > 0;)
        {
          const uint32_t next = i ? vertId[e.inserted[i - 1].second] : end;
          edit.tris.insert(edit.tris.end(), {prev, next, third});
          edit.triSource.push_back(twin / 3);
          prev = next;
        }
      }
      uint32_t prev = e.a;
      for (uint32_t i = 0; i <= e.inserted.size(); ++i)
      {
        const uint32_t next = i < e.inserted.size() ? e.inserted[i].second : e.b;
        const int dir = prev < next ? 1 : -1;
        if (prev != next)
          keys.push_back({edge_key(prev, next), {e.side == 1 ? dir : 0, e.side == 2 ? -dir : 0}});
        prev = next;
      }
    }
    eastl::sort(keys.begin(), keys.end());
    for (uint32_t i = 0, j = 0; i < keys.size(); i = j)
    {
      int sumA = 0, sumB = 0;
      for (j = i; j < keys.size() && keys[j].first == keys[i].first; ++j)
      {
        sumA += keys[j].second.first;
        sumB += keys[j].second.second;
      }
      if (sumA || sumB)
      {
        ea.push_back(keys[i].first >> 32);
        eb.push_back(uint32_t(keys[i].first));
        da.push_back(sumA);
        db.push_back(sumB);
      }
    }
    return !ea.empty();
  }

  uint32_t tail(uint32_t h) const { return h & 1 ? eb[h >> 1] : ea[h >> 1]; }
  uint32_t head(uint32_t h) const { return h & 1 ? ea[h >> 1] : eb[h >> 1]; }

  void buildSpokes()
  {
    dag::Vector<eastl::pair<eastl::pair<uint32_t, double>, uint32_t>> spokes; // vertex, angle, outgoing halfedge
    for (uint32_t e = 0; e < ea.size(); ++e)
    {
      const DPoint2 d = pt[eb[e]] - pt[ea[e]];
      spokes.push_back({{ea[e], atan2(d.y, d.x)}, 2 * e});
      spokes.push_back({{eb[e], atan2(-d.y, -d.x)}, 2 * e + 1});
    }
    eastl::sort(spokes.begin(), spokes.end());
    outFirst.assign(pt.size() + 1, 0);
    for (const auto &sp : spokes)
    {
      ++outFirst[sp.first.first + 1];
      outHe.push_back(sp.second);
    }
    for (uint32_t i = 1; i <= pt.size(); ++i)
      outFirst[i] += outFirst[i - 1];
  }

  // the cell on the left of h goes on along the spoke clockwise from the reverse of h
  uint32_t nextHe(uint32_t h) const
  {
    const uint32_t w = head(h), first = outFirst[w], count = outFirst[w + 1] - first;
    for (uint32_t i = 0; i < count; ++i)
      if (outHe[first + i] == (h ^ 1))
        return outHe[first + (i + count - 1) % count];
    return NO_INDEX;
  }

  bool walkCycles()
  {
    cycleOf.assign(2 * ea.size(), NO_INDEX);
    UnionFind comps;
    comps.init(pt.size());
    for (uint32_t e = 0; e < ea.size(); ++e)
      comps.join(ea[e], eb[e]);
    for (uint32_t h0 = 0; h0 < cycleOf.size(); ++h0)
    {
      if (cycleOf[h0] != NO_INDEX)
        continue;
      const uint32_t ci = cycles.size();
      Cycle &c = cycles.push_back();
      c.ptFrom = cyclePt.size();
      uint32_t h = h0;
      for (; h != NO_INDEX && cycleOf[h] == NO_INDEX; h = nextHe(h))
      {
        cycleOf[h] = ci;
        c.hes.push_back(h);
        cyclePt.push_back(pt[tail(h)]);
      }
      if (h != h0)
        return false;
      const DPoint2 &o = pt[tail(h0)];
      for (uint32_t k = 0; k < c.hes.size(); ++k)
        c.area += 0.5 * cross2(pt[tail(c.hes[k])] - o, pt[head(c.hes[k])] - o);
      c.comp = comps.find(tail(h0));
    }
    return true;
  }

  dag::ConstSpan<DPoint2> points(const Cycle &c) const { return dag::ConstSpan<DPoint2>(cyclePt.data() + c.ptFrom, c.hes.size()); }

  // each component's outer cycle takes the windings of the cell around it; the rest follow across the edges
  bool classify()
  {
    dag::Vector<uint32_t> outer(pt.size(), NO_INDEX), comps;
    for (uint32_t ci = 0; ci < cycles.size(); ++ci)
    {
      uint32_t &o = outer[cycles[ci].comp];
      if (o == NO_INDEX || cycles[ci].area < cycles[o].area)
        o = ci;
    }
    for (uint32_t r = 0; r < outer.size(); ++r)
      if (outer[r] != NO_INDEX)
        comps.push_back(outer[r]);
    eastl::sort(comps.begin(), comps.end(), [&](uint32_t a, uint32_t b) { return cycles[a].area < cycles[b].area; });
    dag::Vector<uint32_t> stack;
    for (uint32_t k = 0; k < comps.size(); ++k)
    {
      Cycle &o = cycles[comps[k]];
      if (!(o.area < 0))
        return false;
      const DPoint2 &probe = pt[tail(o.hes[0])];
      for (uint32_t ci = 0; ci < cycles.size(); ++ci)
      {
        const Cycle &c = cycles[ci];
        if (!c.known || !(c.area > 0) || (o.parent != NO_INDEX && !(c.area < cycles[o.parent].area)))
          continue;
        if (inside_loop(points(c), probe))
          o.parent = ci;
      }
      for (int k = 0; k < 2; ++k)
        o.w[k] = o.parent == NO_INDEX ? 0 : cycles[o.parent].w[k];
      o.known = true;
      stack.assign(1, comps[k]);
      while (!stack.empty())
      {
        const uint32_t ci = stack.back();
        stack.pop_back();
        for (uint32_t h : cycles[ci].hes)
        {
          const int sign = h & 1 ? -1 : 1;
          const int wa = cycles[ci].w[0] - sign * da[h >> 1], wb = cycles[ci].w[1] - sign * db[h >> 1];
          Cycle &t = cycles[cycleOf[h ^ 1]];
          if (t.known && (t.w[0] != wa || t.w[1] != wb))
            return false;
          if (!t.known)
          {
            t.w[0] = wa;
            t.w[1] = wb;
            t.known = true;
            stack.push_back(cycleOf[h ^ 1]);
          }
        }
      }
    }
    return true;
  }

  // the group face of a side under a point, else the side's first face
  uint32_t sourceFace(uint8_t sd, const DPoint2 &p) const
  {
    uint32_t first = NO_INDEX;
    for (uint32_t i = 0; i < g.faces.size(); ++i)
    {
      int inside = side(g.faces[i]) == sd ? 0 : -3;
      first = first == NO_INDEX && inside == 0 ? i : first;
      for (int k = 0; k < 3; ++k)
      {
        const DPoint2 a = flat(s.pos[s.faceVert(g.faces[i], k)]), b = flat(s.pos[s.faceVert(g.faces[i], (k + 1) % 3)]);
        inside += (sd == 1 ? 1 : -1) * cross2(b - a, p - a) >= 0;
      }
      if (inside == 3)
        return i;
    }
    return first;
  }

  // false when no cell lies inside both sides: nothing to remove
  bool triangulate()
  {
    bool overlap = false;
    dag::Vector<dag::ConstSpan<DPoint2>> holeSpans;
    dag::Vector<uint32_t> ids;
    for (uint32_t ci = 0; ci < cycles.size(); ++ci)
    {
      const Cycle &c = cycles[ci];
      if (!(c.area > 0))
        continue;
      if (c.w[0] < 0 || c.w[0] > 1 || c.w[1] < 0 || c.w[1] > 1)
        return false;
      overlap |= c.w[0] && c.w[1];
      if (c.w[0] == c.w[1])
        continue;
      const uint8_t sd = c.w[0] ? 1 : 2;
      ids.clear();
      for (uint32_t h : c.hes)
        ids.push_back(tail(h));
      holeSpans.clear();
      for (const Cycle &o : cycles)
        if (o.parent == ci)
        {
          holeSpans.push_back(points(o));
          for (uint32_t h : o.hes)
            ids.push_back(tail(h));
        }
      eastl::string err;
      if (!triangulate_polygon(points(c), make_span_const(holeSpans), tris, err) &&
          !triangulate_polygon(points(c), make_span_const(holeSpans), tris, err, true))
        return false;
      for (uint32_t i = 0; i < tris.size(); i += 3)
      {
        const uint32_t a = ids[tris[i]], b = ids[tris[i + 1]], d = ids[tris[i + 2]];
        edit.tris.push_back(vertId[a]);
        edit.tris.push_back(vertId[sd == 1 ? b : d]);
        edit.tris.push_back(vertId[sd == 1 ? d : b]);
        edit.triSource.push_back(g.faces[sourceFace(sd, (pt[a] + pt[b] + pt[d]) * (1.0 / 3))]);
      }
    }
    return overlap;
  }
};
} // namespace

// the cells of x in a unit grid: its own, and the neighbour when x lies within the margin of the wall between them
static void grid_cells(double x, double margin, int64_t &lo, int64_t &hi)
{
  const double c = floor(x);
  lo = hi = int64_t(c);
  lo -= x - c < margin;
  hi += c + 1 - x < margin;
}

// faces whose plane cell, in a grid of normals and offsets, holds both orientations, by orientation; a face near a
// cell wall is in both cells, so two faces closer than the margins always share a cell
static void overlap_candidates(const Solid &s, dag::ConstSpan<uint8_t> live, dag::ConstSpan<DPoint3> normal, double tol,
  dag::Vector<uint32_t> out[2])
{
  // normals 1e-5 apart shift offsets 2e-5 per unit radius
  const BBoxD box = points_bbox(s.pos);
  const DPoint3 centre = (box.lim[0] + box.lim[1]) * 0.5;
  const double reach = 2 * tol + 2e-5 * length(box.lim[1] - centre);
  const double cell = 1.0 / 64, margin = 1e-5 / cell, offCell = 8 * reach, offMargin = 0.125;
  // 0 is safe: canonical axis index >= 99
  HashedKeyMap<uint64_t, uint8_t, 0, oa_hashmap_util::MumStepHash<uint64_t>> cells;
  cells.reserve(2 * s.faceCount());
  for (int pass = 0; pass < 2 && tol > 0; ++pass)
    for (uint32_t f = 0; f < s.faceCount(); ++f)
    {
      const DPoint3 &unit = normal[f];
      const double top = max(max(fabs(unit.x), fabs(unit.y)), fabs(unit.z));
      int both = 0; // a bit per orientation
      // near a tie its opposite may take another axis
      for (int axis = 0; axis < 3 && live[f] && top > 0; ++axis)
        if (fabs(unit[axis]) >= top - 2e-5)
        {
          const bool flipped = unit[axis] < 0;
          const DPoint3 n = flipped ? -unit : unit;
          int64_t lo[4], hi[4];
          for (int i = 0; i < 3; ++i)
            grid_cells((n[i] + 1) / cell, margin, lo[i], hi[i]);
          grid_cells(n * (s.pos[s.faceVert(f, 0)] - centre) / offCell, offMargin, lo[3], hi[3]);
          for (int64_t a = lo[0]; a <= hi[0]; ++a)
            for (int64_t b = lo[1]; b <= hi[1]; ++b)
              for (int64_t c = lo[2]; c <= hi[2]; ++c)
                for (int64_t d = lo[3]; d <= hi[3]; ++d)
                {
                  uint8_t &bits =
                    *cells.emplace_if_missing((uint64_t(a) << 56) | (uint64_t(b) << 48) | (uint64_t(c) << 40) | uint32_t(d)).first;
                  bits |= pass == 0 ? uint8_t(flipped ? 2 : 1) : 0;
                  both |= int(bits == 3) << int(flipped);
                }
        }
      for (int sd = 0; sd < 2 && pass == 1; ++sd)
        if (both >> sd & 1)
          out[sd].push_back(f);
    }
}

void find_overlap_groups(const Solid &s, dag::ConstSpan<uint8_t> live, double tol, dag::Vector<OverlapGroup> &groups)
{
  groups.clear();
  const uint32_t count = s.faceCount();
  dag::Vector<DPoint3> normal(count);
  for (uint32_t f = 0; f < count; ++f)
    normal[f] = unit_or_zero(face_cross(s, f));
  dag::Vector<uint32_t> candidates[2];
  overlap_candidates(s, live, make_span_const(normal), tol, candidates);
  Bvh bvh[2];
  dag::Vector<bbox3f> boxes;
  for (int sd = 0; sd < 2; ++sd)
  {
    boxes.clear();
    for (uint32_t f : candidates[sd])
      boxes.push_back(face_box(s, f, tol));
    bvh[sd].build(make_span_const(boxes));
  }
  dag::Vector<TriPair> pairs;
  bvh[0].overlaps(bvh[1], pairs);
  UnionFind sets;
  sets.init(count);
  for (const TriPair &p : pairs)
  {
    const uint32_t a = candidates[0][p.first], b = candidates[1][p.second];
    if (!(normal[a] * normal[b] < 0))
      continue;
    DPoint3 pa[3], pb[3], from, to;
    for (int k = 0; k < 3; ++k)
    {
      pa[k] = s.pos[s.faceVert(a, k)];
      pb[k] = s.pos[s.faceVert(b, k)];
    }
    // the larger face has the sounder plane
    const uint32_t flat = lengthSq(face_cross(s, a)) >= lengthSq(face_cross(s, b)) ? a : b;
    const double d = normal[flat] * s.pos[s.faceVert(flat, 0)];
    if (!within_plane(s, a, normal[flat], d, tol) || !within_plane(s, b, normal[flat], d, tol) ||
        !triangles_intersect(pa, pb, tol, from, to))
      continue;
    sets.join(a, b);
  }
  // a face that joined no pair is a set of its own
  dag::Vector<uint32_t> setOf, start, faces;
  bucket_items(count, sets.compress(setOf), [&](uint32_t f) { return setOf[f]; }, start, faces);
  for (uint32_t k = 0; k + 1 < start.size(); ++k)
    if (start[k + 1] - start[k] > 1)
    {
      OverlapGroup &g = groups.push_back();
      g.faces.assign(faces.begin() + start[k], faces.begin() + start[k + 1]);
      g.n = normal[g.faces[0]];
    }
}

bool remove_overlap(const Solid &s, const OverlapGroup &g, double tol, const ChartTest *charts, OverlapEdit &edit)
{
  edit = OverlapEdit{{}, {}, {}, g.faces};
  Overlay o(s, g, tol, charts, edit);
  if (!o.collectRim(1) || !o.collectRim(2))
    return false;
  o.indexVertices();
  if (!o.weld())
    return false;
  o.insertVertices();
  if (o.rimsCross() || !o.buildEdges())
    return false;
  o.buildSpokes();
  return o.walkCycles() && o.classify() && o.triangulate();
}
} // namespace dacsg
