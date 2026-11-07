// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "exec/pipeline.h"
#include "mesh/solidUtil.h"
#include "mesh/unionFind.h"
#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
#include <EASTL/tuple.h>
#include <math.h>

namespace dacsg
{
namespace
{
using Candidate = eastl::tuple<double, uint32_t, uint32_t>; // squared distance, lower index, higher index

struct VertexClusters
{
  dag::Vector<DPoint3> pos;
  dag::Vector<uint32_t> leafOf, next; // next: a ring of each cluster's members
  UnionFind sets;

  void join(uint32_t a, uint32_t b, double max_diam_sq)
  {
    a = sets.find(a);
    b = sets.find(b);
    if (a == b)
      return;
    // merging one leaf's points flattens its faces
    for (uint32_t x = a, xDone = 0; !xDone; x = next[x], xDone = x == a)
      for (uint32_t y = b, yDone = 0; !yDone; y = next[y], yDone = y == b)
        if (lengthSq(pos[x] - pos[y]) > max_diam_sq || (leafOf[x] == leafOf[y] && pos[x] != pos[y]))
          return;
    // no centroid: it would move every copy
    sets.joinLower(a, b);
    eastl::swap(next[a], next[b]);
  }

  void build(double tol)
  {
    const uint32_t n = pos.size();
    const double maxAbs = n ? max_abs(points_bbox(pos)) : 0;
    sets.init(n);
    for (uint32_t v = 0; v < n; ++v)
      next.push_back(v);
    if (!(maxAbs < tol * MAX_WELD_CELLS))
      return;
    HashChain cells;
    dag::Vector<Candidate> candidates;
    for (uint32_t i = 0; i < n; ++i)
    {
      // 2-tol cells: the nearest 8 suffice
      int64_t lo[3], own[3];
      for (int k = 0; k < 3; ++k)
      {
        lo[k] = int64_t(floor(pos[i][k] / (2 * tol) - 0.5));
        own[k] = int64_t(floor(pos[i][k] / (2 * tol)));
      }
      for (int d = 0; d < 8; ++d)
        for (uint32_t j = cells.first(cell_key(lo[0] + (d & 1), lo[1] + (d >> 1 & 1), lo[2] + (d >> 2))); j != NO_INDEX;
             j = cells.next[j])
          if (const double d2 = lengthSq(pos[i] - pos[j]); d2 <= tol * tol)
            candidates.push_back(Candidate(d2, j, i));
      cells.add(cell_key(own[0], own[1], own[2]), i);
    }
    eastl::sort(candidates.begin(), candidates.end());
    for (const Candidate &c : candidates)
      join(eastl::get<1>(c), eastl::get<2>(c), 4 * tol * tol);
  }
};

void cluster_set(dag::Vector<Solid> &leaves, dag::ConstSpan<uint32_t> set, double tol)
{
  VertexClusters vc;
  for (uint32_t l : set)
  {
    vc.pos.insert(vc.pos.end(), leaves[l].pos.begin(), leaves[l].pos.end());
    vc.leafOf.resize(vc.pos.size(), l);
  }
  vc.build(tol);
  uint32_t v = 0;
  for (uint32_t l : set)
  {
    Solid &s = leaves[l];
    dag::Vector<DPoint3> to(s.vertCount());
    for (DPoint3 &p : to)
      p = vc.pos[vc.sets.find(v++)];
    bool apply = !eastl::equal(to.begin(), to.end(), s.pos.begin());
    // exact kernel rejects flat or turned faces
    for (uint32_t f = 0; f < s.faceCount() && apply; ++f)
      apply = face_cross(s, f) * ((to[s.faceVert(f, 1)] - to[s.faceVert(f, 0)]) % (to[s.faceVert(f, 2)] - to[s.faceVert(f, 0)])) > 0;
    if (!apply)
      continue;
    // a moved vertex lies on its planes no more
    for (uint32_t i = 0; i < s.provenance.size(); ++i)
      if (!(to[i] == s.pos[i]))
        s.provenance[i] = Provenance();
    s.pos = eastl::move(to);
    s.bbox = points_bbox(s.pos);
    compute_vertex_normals(s);
  }
}
} // namespace

void cluster_vertices(BoolTree &tree, double tolerance)
{
  const uint32_t count = tree.nodes.size();
  dag::Vector<uint32_t> setOf(count, count - 1);
  dag::Vector<dag::Vector<uint32_t>> bySet(count);
  for (uint32_t i = count; i-- > 0;)
  {
    const BoolTree::Node &n = tree.nodes[i];
    if (n.kind == BoolTree::Kind::Leaf)
      bySet[setOf[i]].push_back(n.leaf);
    else
      for (uint32_t c : n.child)
        setOf[c] = n.kind == BoolTree::Kind::Concat && setOf[i] == i ? c : setOf[i];
  }
  for (dag::Vector<uint32_t> &set : bySet)
    if (set.size() > 1)
    {
      eastl::sort(set.begin(), set.end());
      cluster_set(tree.leaves, make_span_const(set), tolerance);
    }
}
} // namespace dacsg
