// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "kernelExact/arrangement.h"
#include <EASTL/sort.h>

namespace dacsg::exact
{
static bool constraint_less(const Constraint &x, const Constraint &y)
{
  return x.p[0] != y.p[0] ? x.p[0] < y.p[0] : x.p[1] != y.p[1] ? x.p[1] < y.p[1] : x.line.kind < y.line.kind;
}

static void add_constraint(const Arrangement &a, const Group &g, uint32_t p0, uint32_t p1, const Line &line,
  dag::Vector<Constraint> &out)
{
  if (p0 == p1)
    return;
  Constraint c;
  c.p[0] = p0 < p1 ? p0 : p1;
  c.p[1] = p0 < p1 ? p1 : p0;
  c.line = line;
  const int u = (g.axis + 1) % 3, v = (g.axis + 2) % 3;
  const ExactPoint &x = a.points[c.p[0]], &y = a.points[c.p[1]];
  c.lo[0] = min(x.approx[u] - x.err, y.approx[u] - y.err);
  c.hi[0] = max(x.approx[u] + x.err, y.approx[u] + y.err);
  c.lo[1] = min(x.approx[v] - x.err, y.approx[v] - y.err);
  c.hi[1] = max(x.approx[v] + x.err, y.approx[v] + y.err);
  out.push_back(c);
}

void group_constraints(const Arrangement &a, uint32_t group, dag::Vector<Constraint> &out)
{
  out.clear();
  const Group &g = a.groups[group];
  for (uint32_t i = g.firstTri; i < g.firstTri + g.triCount; ++i)
  {
    const uint32_t t = a.groupTris[i];
    const Tri &tri = a.tris[t];
    for (int k = 0; k < 3; ++k)
    {
      const uint32_t p0 = tri.v[k], p1 = tri.v[(k + 1) % 3];
      add_constraint(a, g, p0, p1, Line{LineKind::Edge, p0 < p1 ? p0 : p1, p0 < p1 ? p1 : p0}, out);
    }
    for (uint32_t s = a.triSegStart[t]; s < a.triSegStart[t + 1]; ++s)
      add_constraint(a, g, a.segments[s].p[0], a.segments[s].p[1], a.segments[s].line, out);
  }
  eastl::sort(out.begin(), out.end(), constraint_less);
  uint32_t n = 0;
  for (uint32_t i = 0; i < out.size(); ++i)
    if (i == 0 || out[i].p[0] != out[n - 1].p[0] || out[i].p[1] != out[n - 1].p[1])
      out[n++] = out[i];
  out.resize(n);
}

static bool proper_crossing(const Arrangement &a, const Group &g, const Constraint &x, const Constraint &y)
{
  const ExactPoint &x0 = a.points[x.p[0]], &x1 = a.points[x.p[1]], &y0 = a.points[y.p[0]], &y1 = a.points[y.p[1]];
  const uint32_t group = uint32_t(&g - a.groups.data());
  const int s0 = orient2d(x0, x1, y0, g.axis, group), s1 = orient2d(x0, x1, y1, g.axis, group);
  if (s0 * s1 >= 0)
    return false;
  const int t0 = orient2d(y0, y1, x0, g.axis, group), t1 = orient2d(y0, y1, x1, g.axis, group);
  return t0 * t1 < 0;
}

static uint32_t other_plane(const Line &line, uint32_t group) { return line.a == group ? line.b : line.a; }

static void crossing_point(Arrangement &a, uint32_t group, const Constraint &x, const Constraint &y)
{
  const Group &g = a.groups[group];
  ExactPoint p;
  const Constraint &e = x.line.kind == LineKind::Edge ? x : y, &s = x.line.kind == LineKind::Edge ? y : x;
  if (e.line.kind == LineKind::Edge && s.line.kind == LineKind::Edge)
  {
    if (!make_ssi(a.points[e.p[0]], a.points[e.p[1]], a.points[s.p[0]], a.points[s.p[1]], g.axis, a.arena, p))
      return;
    set_key(p, e.p[0], e.p[1], s.p[0], s.p[1]);
  }
  else if (e.line.kind == LineKind::Edge)
  {
    const uint32_t h = other_plane(s.line, group);
    if (!make_lpi(a.points[e.p[0]], a.points[e.p[1]], a.groups[h].plane, a.arena, p))
      return;
    set_key(p, e.p[0], e.p[1], h);
  }
  else
  {
    const uint32_t h1 = other_plane(e.line, group), h2 = other_plane(s.line, group);
    if (h1 == h2 || !make_tpi(g.plane, a.groups[h1].plane, a.groups[h2].plane, a.arena, p))
      return;
    set_key(p, group, h1, h2);
  }
  a.candidates.push_back(p);
}

void find_crossings(Arrangement &a)
{
  dag::Vector<Constraint> cs;
  dag::Vector<uint32_t> order;
  for (uint32_t group = 0; group < a.groups.size(); ++group)
  {
    group_constraints(a, group, cs);
    if (cs.size() <= 3)
      continue;
    order.resize(cs.size());
    for (uint32_t i = 0; i < order.size(); ++i)
      order[i] = i;
    eastl::sort(order.begin(), order.end(), [&](uint32_t x, uint32_t y) { return cs[x].lo[0] < cs[y].lo[0]; });
    for (uint32_t i = 0; i < order.size(); ++i)
    {
      const Constraint &x = cs[order[i]];
      for (uint32_t j = i + 1; j < order.size() && cs[order[j]].lo[0] <= x.hi[0]; ++j)
      {
        const Constraint &y = cs[order[j]];
        if (y.lo[1] > x.hi[1] || y.hi[1] < x.lo[1] || x.p[0] == y.p[0] || x.p[0] == y.p[1] || x.p[1] == y.p[0] || x.p[1] == y.p[1])
          continue;
        if (proper_crossing(a, a.groups[group], x, y))
          crossing_point(a, group, x, y);
      }
    }
  }
}
} // namespace dacsg::exact
