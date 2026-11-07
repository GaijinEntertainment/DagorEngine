// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "kernelExact/arrangement.h"
#include "mesh/solidUtil.h"
#include "mesh/unionFind.h"
#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
#include <math.h>

namespace dacsg::exact
{
namespace
{
uint64_t cell_key(const DPoint3 &p, double size)
{
  return mix_cell(int64_t(floor(p.x / size)), int64_t(floor(p.y / size)), int64_t(floor(p.z / size)));
}

struct Pass
{
  const Arrangement &a;
  const Cells &c;
  const double tolerance;
  dag::Vector<OutFace> &faces;
  UnionFind weld, corners;
  dag::Vector<DPoint3> world;   // by registry id
  dag::Vector<uint8_t> known;   // world[id] is computed
  dag::Vector<uint32_t> heEdge; // by halfedge: arrangement edge * 2 + (1 when the weld turned the edge around)
  dag::Vector<uint32_t> mate;   // by halfedge
  dag::Vector<EdgeKey> keys;    // every halfedge by its welded edge
  dag::Vector<uint32_t> order;
  dag::Vector<double> angle; // by order index

  DPoint3 pos(uint32_t id)
  {
    if (!known[id])
    {
      world[id] = grid_to_world(a.frame, a.points[id].approx);
      known[id] = 1;
    }
    return world[id];
  }
  uint32_t corner_of(uint32_t face, uint32_t id) const
  {
    for (uint32_t k = 0; k < 3; ++k)
      if (faces[face].v[k] == id)
        return face * 3 + k;
    return face * 3;
  }
  bool forward(uint32_t he, uint32_t lo) const { return faces[he / 3].v[he % 3] == lo; }
  // the position of the halfedge's face in the sorted wedge list of its arrangement edge, counterclockwise about lo -> hi
  int32_t wedge_rank(uint32_t he) const
  {
    const uint32_t e = heEdge[he] >> 1, af = faces[he / 3].face;
    for (uint32_t w = c.edgeStart[e]; w < c.edgeStart[e + 1]; ++w)
      if ((c.edgeWedge[w] >> 1) == af)
        return heEdge[he] & 1 ? -int32_t(w) : int32_t(w);
    return 0;
  }

  void weld_vertices();
  void drop_faces();
  void pair_group(uint32_t first, uint32_t count);
  void pair_edges();
  bool build(Solid &out, eastl::string &err);
};

// every vertex joins the lowest id within the tolerance of its cluster root, so a cluster stays within one tolerance of it
void Pass::weld_vertices()
{
  weld.init(a.points.size());
  world.resize(a.points.size(), DPoint3(0, 0, 0));
  known.resize(a.points.size(), 0);
  dag::Vector<uint32_t> ids;
  for (const OutFace &f : faces)
    for (uint32_t id : f.v)
      ids.push_back(id);
  eastl::sort(ids.begin(), ids.end());
  ids.erase(eastl::unique(ids.begin(), ids.end()), ids.end());
  KeyChains chains;
  chains.init(a.points.size(), ids.size());
  for (uint32_t id : ids)
  {
    const DPoint3 p = pos(id);
    const int64_t cx = int64_t(floor(p.x / tolerance)), cy = int64_t(floor(p.y / tolerance)), cz = int64_t(floor(p.z / tolerance));
    uint32_t root = NO_INDEX;
    for (int dx = -1; dx <= 1; ++dx)
      for (int dy = -1; dy <= 1; ++dy)
        for (int dz = -1; dz <= 1; ++dz)
        {
          const uint64_t key = mix_cell(cx + dx, cy + dy, cz + dz);
          for (uint32_t q = chains.first(key); q != NO_INDEX; q = chains.next[q])
            if (cell_key(pos(q), tolerance) == key && length(pos(weld.find(q)) - p) <= tolerance)
              root = eastl::min(root, weld.find(q));
        }
    if (root != NO_INDEX)
      weld.joinLower(id, root);
    chains.add(mix_cell(cx, cy, cz), id);
  }
}

// a face with a repeated vertex goes, and so does each pair with the same three ids in opposite order; two slivers
// with the same ids in the same order stay, as they can lie in different planes and the edge balance needs both
void Pass::drop_faces()
{
  dag::Vector<FaceKey> triples;
  dag::Vector<uint8_t> drop(faces.size(), 0);
  heEdge.assign(faces.size() * 3, 0);
  for (uint32_t i = 0; i < faces.size(); ++i)
  {
    OutFace &f = faces[i];
    const Face &cf = c.faces[f.face];
    for (uint32_t k = 0; k < 3; ++k)
    {
      const uint32_t x = f.v[k], y = f.v[(k + 1) % 3];
      for (uint32_t m = 0; m < 3; ++m)
        if (edge_key(cf.v[m], cf.v[(m + 1) % 3]) == edge_key(x, y))
          heEdge[i * 3 + k] = c.faceEdge[f.face * 3 + m] * 2 + (weld.find(eastl::min(x, y)) > weld.find(eastl::max(x, y)));
    }
    for (uint32_t &id : f.v)
      id = weld.find(id);
    if (f.v[0] == f.v[1] || f.v[1] == f.v[2] || f.v[2] == f.v[0])
    {
      drop[i] = 1;
      continue;
    }
    const int lo = f.v[0] < f.v[1] ? (f.v[0] < f.v[2] ? 0 : 2) : (f.v[1] < f.v[2] ? 1 : 2);
    const uint32_t b = f.v[(lo + 1) % 3], c2 = f.v[(lo + 2) % 3];
    triples.push_back(FaceKey{{f.v[lo], b < c2 ? b : c2, b < c2 ? c2 : b}, b > c2, i});
  }
  eastl::sort(triples.begin(), triples.end());
  for (uint32_t i = 0; i < triples.size();)
  {
    uint32_t even = i;
    while (even < triples.size() && triples[even].sameVerts(triples[i]) && !triples[even].odd)
      ++even;
    uint32_t j = even;
    while (j < triples.size() && triples[j].sameVerts(triples[i]))
      ++j;
    const uint32_t pairs = eastl::min(even - i, j - even);
    for (uint32_t k = 0; k < pairs; ++k)
      drop[triples[i + k].face] = drop[triples[even + k].face] = 1;
    i = j;
  }
  uint32_t kept = 0;
  for (uint32_t i = 0; i < faces.size(); ++i)
    if (!drop[i])
    {
      faces[kept] = faces[i];
      for (uint32_t k = 0; k < 3; ++k)
        heEdge[kept * 3 + k] = heEdge[i * 3 + k];
      ++kept;
    }
  faces.resize(kept);
  heEdge.resize(kept * 3);
}

// Each arrangement edge of the group keeps its exact wedge order: a face runs lo to hi with the out cell on its
// counterclockwise side, so the in sector runs from a backward face to the next forward one. A face whose in-sector
// neighbour dropped pairs with the nearest leftover of the other direction by the angles in doubles.
void Pass::pair_group(uint32_t first, uint32_t count)
{
  const uint32_t lo = uint32_t(keys[first].key >> 32), hi = uint32_t(keys[first].key);
  order.resize(count);
  for (uint32_t n = 0; n < count; ++n)
    order[n] = keys[first + n].he;
  eastl::sort(order.begin(), order.end(),
    [&](uint32_t x, uint32_t y) { return heEdge[x] != heEdge[y] ? heEdge[x] < heEdge[y] : wedge_rank(x) < wedge_rank(y); });
  uint32_t leftovers = 0;
  for (uint32_t i = 0; i < count;)
  {
    uint32_t j = i + 1;
    while (j < count && heEdge[order[j]] == heEdge[order[i]])
      ++j;
    for (uint32_t n = i; n < j; ++n)
    {
      const uint32_t h = order[n], h2 = order[n + 1 < j ? n + 1 : i];
      if (!forward(h, lo) && forward(h2, lo) && mate[h] == NO_INDEX && mate[h2] == NO_INDEX)
      {
        mate[h] = h2;
        mate[h2] = h;
      }
    }
    for (uint32_t n = i; n < j; ++n)
      leftovers += mate[order[n]] == NO_INDEX;
    i = j;
  }
  if (!leftovers)
    return;
  const DPoint3 d = unit_or_zero(pos(hi) - pos(lo));
  const DPoint3 seed = fabs(d.x) < fabs(d.y) ? (fabs(d.x) < fabs(d.z) ? DPoint3(1, 0, 0) : DPoint3(0, 0, 1))
                                             : (fabs(d.y) < fabs(d.z) ? DPoint3(0, 1, 0) : DPoint3(0, 0, 1));
  const DPoint3 u = unit_or_zero(seed - d * (seed * d)), v = d % u;
  angle.resize(count);
  for (uint32_t n = 0; n < count; ++n)
  {
    const uint32_t he = order[n];
    const DPoint3 r = pos(faces[he / 3].v[(he % 3 + 2) % 3]) - pos(lo);
    angle[n] = atan2(r * v, r * u);
  }
  constexpr double halfTurn = 3.14159265358979323846;
  for (uint32_t n = 0; n < count; ++n)
  {
    const uint32_t h = order[n];
    if (mate[h] != NO_INDEX || !forward(h, lo))
      continue;
    uint32_t best = NO_INDEX;
    double bestGap = 0;
    for (uint32_t m = 0; m < count; ++m)
    {
      const uint32_t h2 = order[m];
      if (mate[h2] != NO_INDEX || forward(h2, lo))
        continue;
      const double gap = fabs(fmod(fabs(angle[m] - angle[n]) + 3 * halfTurn, 2 * halfTurn) - halfTurn);
      if (best == NO_INDEX || gap < bestGap)
      {
        best = h2;
        bestGap = gap;
      }
    }
    if (best != NO_INDEX)
    {
      mate[h] = best;
      mate[best] = h;
    }
  }
}

void Pass::pair_edges()
{
  mate.assign(faces.size() * 3, NO_INDEX);
  keys.clear();
  for (uint32_t i = 0; i < faces.size(); ++i)
    for (uint32_t k = 0; k < 3; ++k)
      keys.push_back(EdgeKey{edge_key(faces[i].v[k], faces[i].v[(k + 1) % 3]), i * 3 + k});
  eastl::sort(keys.begin(), keys.end());
  for (uint32_t i = 0; i < keys.size();)
  {
    uint32_t j = i + 1;
    while (j < keys.size() && keys[j].key == keys[i].key)
      ++j;
    const uint32_t lo = uint32_t(keys[i].key >> 32);
    if (j - i == 2)
    {
      if (forward(keys[i].he, lo) != forward(keys[i + 1].he, lo))
      {
        mate[keys[i].he] = keys[i + 1].he;
        mate[keys[i + 1].he] = keys[i].he;
      }
    }
    else if (j - i > 2)
      pair_group(i, j - i);
    i = j;
  }
}

// The vertices duplicate per fan of mated faces. Two pairs of one edge that share both fans pinch it: the fans are
// oriented, so swapping the two pairs' mates splits the fan at each end and never joins fans, and the rounds end.
bool Pass::build(Solid &out, eastl::string &err)
{
  weld_vertices();
  drop_faces();
  pair_edges();
  dag::Vector<uint32_t> vert, vertId;
  dag::Vector<EdgeKey> copies;
  for (uint32_t round = 0;; ++round)
  {
    corners.init(faces.size() * 3);
    uint32_t unpaired = 0;
    for (uint32_t h = 0; h < mate.size(); ++h)
      if (mate[h] == NO_INDEX)
        ++unpaired;
      else if (h < mate[h])
        for (uint32_t end : {faces[h / 3].v[h % 3], faces[h / 3].v[(h % 3 + 1) % 3]})
          corners.join(corner_of(h / 3, end), corner_of(mate[h] / 3, end));
    if (unpaired)
    {
      err.sprintf("the arrangement output has %u halfedges with no pair", unpaired);
      return false;
    }
    vert.assign(faces.size() * 3, NO_INDEX);
    vertId.clear();
    out.halfedge.resize(faces.size() * 3);
    for (uint32_t i = 0; i < faces.size(); ++i)
    {
      uint32_t v[3];
      for (uint32_t k = 0; k < 3; ++k)
      {
        const uint32_t root = corners.find(i * 3 + k);
        if (vert[root] == NO_INDEX)
        {
          vert[root] = vertId.size();
          vertId.push_back(faces[i].v[k]);
        }
        v[k] = vert[root];
      }
      for (uint32_t k = 0; k < 3; ++k)
        out.halfedge[i * 3 + k] = Halfedge{v[k], v[(k + 1) % 3], NO_INDEX};
    }
    copies.clear();
    for (uint32_t h = 0; h < out.halfedge.size(); ++h)
      copies.push_back(EdgeKey{edge_key(out.halfedge[h].startVert, out.halfedge[h].endVert), h});
    eastl::sort(copies.begin(), copies.end());
    uint32_t swaps = 0;
    for (uint32_t i = 0; i < copies.size();)
    {
      uint32_t j = i + 1;
      while (j < copies.size() && copies[j].key == copies[i].key)
        ++j;
      if (j - i > 2)
      {
        uint32_t h[2] = {NO_INDEX, NO_INDEX}, found = 0;
        for (uint32_t n = i; n < j && found < 2; ++n)
          if (out.halfedge[copies[n].he].startVert == uint32_t(copies[i].key >> 32))
            h[found++] = copies[n].he;
        const uint32_t m1 = mate[h[0]], m2 = mate[h[1]];
        mate[h[0]] = m2;
        mate[m2] = h[0];
        mate[h[1]] = m1;
        mate[m1] = h[1];
        ++swaps;
      }
      i = j;
    }
    if (!swaps)
      break;
    if (round >= 32)
    {
      err.sprintf("the arrangement output is pinched along %u edges after welding", swaps);
      return false;
    }
  }
  out.pos.reserve(vertId.size());
  for (uint32_t id : vertId)
    out.pos.push_back(pos(id));
  out.bbox = points_bbox(out.pos);
  return true;
}
} // namespace

bool output_pass(const Arrangement &a, const Cells &c, double tolerance, dag::Vector<OutFace> &faces, Solid &out, eastl::string &err)
{
  Pass pass{a, c, tolerance, faces, {}, {}, {}, {}, {}, {}, {}, {}, {}};
  return faces.empty() || pass.build(out, err);
}
} // namespace dacsg::exact
