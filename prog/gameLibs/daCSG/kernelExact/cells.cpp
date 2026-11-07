// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "kernelExact/arrangement.h"
#include "mesh/unionFind.h"
#include <EASTL/sort.h>

namespace dacsg::exact
{
struct EdgeLine
{
  uint64_t key;
  Line line;
};

static void collect_faces(const Triangulations &t, Cells &c, dag::Vector<EdgeLine> &lines)
{
  for (uint32_t g = 0; g + 1 < t.subTriStart.size(); ++g)
  {
    for (uint32_t i = t.subTriStart[g]; i < t.subTriStart[g + 1]; ++i)
    {
      const SubTri &s = t.subTris[i];
      Face &f = c.faces.push_back();
      f.v[0] = s.v[0];
      f.v[1] = s.v[1];
      f.v[2] = s.v[2];
      f.group = g;
      f.firstMember = c.members.size();
      f.memberCount = s.memberCount;
      c.members.insert(c.members.end(), t.members.begin() + s.firstMember, t.members.begin() + s.firstMember + s.memberCount);
    }
    for (uint32_t i = t.edgeStart[g]; i < t.edgeStart[g + 1]; ++i)
      lines.push_back(EdgeLine{edge_key(t.edges[i].v[0], t.edges[i].v[1]), t.edges[i].line});
  }
  eastl::sort(lines.begin(), lines.end(), [](const EdgeLine &x, const EdgeLine &y) { return x.key < y.key; });
}

static const Line *find_line(const dag::Vector<EdgeLine> &lines, uint64_t key)
{
  auto it = eastl::lower_bound(lines.begin(), lines.end(), key, [](const EdgeLine &e, uint64_t k) { return e.key < k; });
  return it != lines.end() && it->key == key ? &it->line : nullptr;
}

// the line of the edge lo..hi, pointing from lo to hi
static bool edge_direction(const Arrangement &a, const Line &line, uint32_t lo, uint32_t hi, ExactDir &d)
{
  if (line.kind == LineKind::Edge)
    d = edge_dir(a.points[line.a], a.points[line.b]);
  else
    d = line_dir(a.groups[line.a].plane, a.groups[line.b].plane);
  const int axis = dir_axis(d);
  const int along = compare_coord(a.points[hi], a.points[lo], axis);
  if (along == 0 || dir_sign(d, axis) == 0)
    return false;
  if (along != dir_sign(d, axis))
    dir_negate(d);
  return true;
}

struct WedgeSort
{
  const Arrangement &a;
  const Cells &c;
  const ExactDir &d;

  int sign(uint32_t w) const { return w & 1 ? -1 : 1; }
  const Plane &plane(uint32_t w) const { return a.groups[c.faces[w >> 1].group].plane; }
  // i before j inside the half turn of one bucket
  bool ccw(uint32_t i, uint32_t j) const { return sign(i) * sign(j) * wedge_order(plane(i), plane(j), d) > 0; }
};

// around the edge: the first wedge, then those within a half turn, the opposite one, the rest
static void sort_wedges(const Arrangement &a, Cells &c, uint32_t *w, uint32_t count, const ExactDir &d)
{
  WedgeSort s{a, c, d};
  dag::Vector<eastl::pair<int, uint32_t>> items(count); // bucket, wedge
  items[0] = {0, w[0]};
  for (uint32_t i = 1; i < count; ++i)
  {
    const int rel = s.sign(w[0]) * s.sign(w[i]) * wedge_order(s.plane(w[0]), s.plane(w[i]), d);
    items[i] = {rel > 0 ? 1 : rel < 0 ? 3 : s.sign(w[0]) * s.sign(w[i]) * sign_dot(s.plane(w[0]), s.plane(w[i])) > 0 ? 0 : 2, w[i]};
  }
  eastl::sort(items.begin(), items.end(), [&](const eastl::pair<int, uint32_t> &x, const eastl::pair<int, uint32_t> &y) {
    if (x.first != y.first)
      return x.first < y.first;
    return (x.first == 1 || x.first == 3) && s.ccw(x.second, y.second);
  });
  for (uint32_t i = 0; i < count; ++i)
    w[i] = items[i].second;
}

static bool folded(const Arrangement &a, const Cells &c, uint32_t face)
{
  bool front = false, back = false;
  for (uint32_t m = c.faces[face].firstMember; m < c.faces[face].firstMember + c.faces[face].memberCount; ++m)
    (a.tris[c.members[m]].orient > 0 ? front : back) = true;
  return front && back;
}

static bool build_edges(const Arrangement &a, const dag::Vector<EdgeLine> &lines, Cells &c, UnionFind &sides, eastl::string &err)
{
  struct Incidence
  {
    uint64_t key;
    uint32_t wedge;
    uint8_t corner;
  };
  dag::Vector<Incidence> inc;
  inc.reserve(c.faces.size() * 3);
  for (uint32_t f = 0; f < c.faces.size(); ++f)
    for (int k = 0; k < 3; ++k)
    {
      const uint32_t p = c.faces[f].v[k], q = c.faces[f].v[(k + 1) % 3];
      inc.push_back(Incidence{edge_key(p, q), f * 2 + (p > q), uint8_t(k)});
    }
  eastl::sort(inc.begin(), inc.end(),
    [](const Incidence &x, const Incidence &y) { return x.key != y.key ? x.key < y.key : x.wedge < y.wedge; });
  c.faceEdge.resize(c.faces.size() * 3);
  c.edgeWedge.resize(inc.size());
  c.edgeStart.clear();
  for (uint32_t i = 0; i < inc.size();)
  {
    uint32_t j = i + 1;
    while (j < inc.size() && inc[j].key == inc[i].key)
      ++j;
    const uint32_t edge = c.edgeStart.size();
    c.edgeStart.push_back(i);
    const uint32_t lo = uint32_t(inc[i].key >> 32), hi = uint32_t(inc[i].key);
    uint32_t *w = &c.edgeWedge[i];
    for (uint32_t k = i; k < j; ++k)
      w[k - i] = inc[k].wedge;
    uint32_t groupsAtEdge = 1;
    for (uint32_t k = i + 1; k < j; ++k)
      groupsAtEdge += c.faces[w[k - i] >> 1].group != c.faces[w[0] >> 1].group;
    // a lone face is the fold of a leaf's sheet back onto itself, with members on both sides: its sides join around the edge
    if (j - i < 2 && !folded(a, c, w[0] >> 1))
    {
      err.sprintf("arrangement edge %u..%u has one face", lo, hi);
      return false;
    }
    if (groupsAtEdge > 1 && j - i > 2)
    {
      const Line *line = find_line(lines, inc[i].key);
      ExactDir d;
      if (!line || !edge_direction(a, *line, lo, hi, d))
      {
        err.sprintf("arrangement edge %u..%u between plane groups has no line", lo, hi);
        return false;
      }
      sort_wedges(a, c, w, j - i, d);
    }
    for (uint32_t k = 0; k < j - i; ++k)
    {
      const uint32_t wa = w[k], wb = w[(k + 1) % (j - i)];
      sides.join((wa >> 1) * 2 + (wa & 1), (wb >> 1) * 2 + 1 - (wb & 1));
    }
    for (uint32_t k = i; k < j; ++k)
      c.faceEdge[(inc[k].wedge >> 1) * 3 + inc[k].corner] = edge;
    i = j;
  }
  c.edgeStart.push_back(inc.size());
  return true;
}

static bool seed_component(const Arrangement &a, const Cells &c, uint32_t comp, dag::ConstSpan<uint32_t> face_comp,
  dag::Vector<int32_t> &w, uint32_t &shell, eastl::string &err)
{
  w.assign(c.leafCount, 0);
  for (uint32_t f = 0; f < c.faces.size(); ++f)
    if (face_comp[f] == comp && seed_winding(a, c.faces[f], w.data()))
    {
      shell = c.faceShell[f * 2];
      return true;
    }
  err.sprintf("no face of surface component %u admits a seed ray", comp);
  return false;
}

static bool walk_windings(const Arrangement &a, Cells &c, dag::ConstSpan<uint32_t> face_comp, eastl::string &err)
{
  const uint32_t n = c.leafCount;
  c.shellWinding.assign(size_t(c.shellCount) * n, 0);
  dag::Vector<uint8_t> assigned(c.shellCount, 0);
  dag::Vector<uint32_t> sideStart, sideList, queue;
  bucket_items(c.faceShell.size(), c.shellCount, [&](uint32_t i) { return c.faceShell[i]; }, sideStart, sideList);
  dag::Vector<int32_t> seed, delta(n);
  for (uint32_t f = 0; f < c.faces.size(); ++f)
  {
    if (assigned[c.faceShell[f * 2]] || assigned[c.faceShell[f * 2 + 1]])
      continue;
    uint32_t shell;
    if (!seed_component(a, c, face_comp[f], face_comp, seed, shell, err))
      return false;
    for (uint32_t l = 0; l < n; ++l)
      c.shellWinding[size_t(shell) * n + l] = seed[l];
    assigned[shell] = 1;
    queue.clear();
    queue.push_back(shell);
    for (uint32_t head = 0; head < queue.size(); ++head)
    {
      const uint32_t s = queue[head];
      for (uint32_t i = sideStart[s]; i < sideStart[s + 1]; ++i)
      {
        const uint32_t side = sideList[i], face = side >> 1, other = c.faceShell[side ^ 1];
        const Face &fc = c.faces[face];
        for (uint32_t l = 0; l < n; ++l)
          delta[l] = 0;
        for (uint32_t m = fc.firstMember; m < fc.firstMember + fc.memberCount; ++m)
          delta[a.tris[c.members[m]].leaf] += (side & 1 ? -1 : 1) * a.tris[c.members[m]].orient;
        if (!assigned[other])
        {
          for (uint32_t l = 0; l < n; ++l)
            c.shellWinding[size_t(other) * n + l] = c.shellWinding[size_t(s) * n + l] + delta[l];
          assigned[other] = 1;
          queue.push_back(other);
          continue;
        }
        for (uint32_t l = 0; l < n; ++l)
          if (c.shellWinding[size_t(other) * n + l] != c.shellWinding[size_t(s) * n + l] + delta[l])
          {
            err.sprintf("internal error: winding conflict across arrangement face %u for leaf %u", face, a.callerLeaf(l));
            return false;
          }
      }
    }
  }
  return true;
}

bool build_cells(const Arrangement &a, const Triangulations &t, Cells &c, eastl::string &err)
{
  c = Cells();
  c.leafCount = a.leafTriBase.size();
  dag::Vector<EdgeLine> lines;
  collect_faces(t, c, lines);
  if (c.faces.empty())
    return true;
  UnionFind sides;
  sides.init(c.faces.size() * 2);
  if (!build_edges(a, lines, c, sides, err))
    return false;
  c.shellCount = sides.compress(c.faceShell);

  UnionFind comps;
  comps.init(c.faces.size());
  for (uint32_t e = 0; e + 1 < c.edgeStart.size(); ++e)
    for (uint32_t i = c.edgeStart[e] + 1; i < c.edgeStart[e + 1]; ++i)
      comps.join(c.edgeWedge[i] >> 1, c.edgeWedge[c.edgeStart[e]] >> 1);
  dag::Vector<uint32_t> faceComp;
  c.componentCount = comps.compress(faceComp);
  return walk_windings(a, c, make_span_const(faceComp), err);
}
} // namespace dacsg::exact
