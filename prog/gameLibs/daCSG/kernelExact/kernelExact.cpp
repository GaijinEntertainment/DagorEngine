// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "mesh/solidUtil.h"
#include "kernelExact/arrangement.h"
#include "kernelExact/exact.h"
#include <gameLibs/daParallel/parallel_api.h>
#include <perfMon/dag_cpuFreq.h>
#include <EASTL/sort.h>

namespace dacsg
{
namespace
{
using namespace exact;

// the time of each phase
struct PhaseTimer
{
  KernelStats &stats;
  int64_t t = ref_time_ticks();

  void lap(const char *name)
  {
    stats.add(name, get_time_nsec(t) / 1000);
    t = ref_time_ticks();
  }
};

// one arrangement of the whole tree; leafRef names its leaves
bool arrange(const BoolTree &tree, dag::ConstSpan<uint32_t> leaf_of, const KernelParams &p, Solid &out, KernelStats &stats,
  eastl::string &err)
{
  out = Solid();
  uint32_t trisIn = 0;
  for (const Solid &leaf : tree.leaves)
    trisIn += leaf.faceCount();
  if (trisIn > p.maxArrangementTris)
  {
    err.sprintf("%u triangles exceed maxArrangementTris %u", trisIn, p.maxArrangementTris);
    return false;
  }
  if (trisIn == 0)
  {
    out.closed = true;
    return true;
  }
  take_exact_fallbacks();
  // an early return of an earlier arrange can leave the flag set
  take_ex_overflow();
  PhaseTimer timer{stats};
  Arrangement a;
  a.leafOf = leaf_of;
  if (!build_leaves(tree, p, a, err))
    return false;
  timer.lap("us in leaves");
  // a lease lasts its call
  if (!find_pairs(a, WorkerLease(p.budget, daparallel::worker_count(p.threads)).workers()))
  {
    err.sprintf("over %u pairs of leaves overlap: split the model or raise maxArrangementTris", p.maxArrangementTris);
    return false;
  }
  assign_ids(a);
  index_segments(a);
  timer.lap("us in pairs");
  find_crossings(a);
  assign_ids(a);
  a.candidates = dag::Vector<ExactPoint>();
  timer.lap("us in crossings");
  if (a.fp && take_ex_overflow())
  {
    err = "the shewchuk backend ran out of expansion terms: the coordinates spread too wide";
    return false;
  }
  dag::Vector<uint64_t> pairs;
  insert_points(a, pairs);
  finish_tri_points(a, pairs);
  timer.lap("us in insert");
  Triangulations groups;
  if (!triangulate_groups(a, WorkerLease(p.budget, daparallel::worker_count(p.threads)).workers(), groups, err))
    return false;
  timer.lap("us in triangulate");
  Cells cells;
  if (!build_cells(a, groups, cells, err))
    return false;
  timer.lap("us in cells");
  if (!extract_solid(a, tree, cells, p.tolerance, out, err))
    return false;
  timer.lap("us in extract");
  if (a.fp && take_ex_overflow())
  {
    err = "the shewchuk backend ran out of expansion terms in a predicate";
    return false;
  }
  stats.add("lpi points", a.pointsByKind[int(PointKind::Lpi)]);
  stats.add("ssi points", a.pointsByKind[int(PointKind::Ssi)]);
  stats.add("tpi points", a.pointsByKind[int(PointKind::Tpi)]);
  stats.add("exact fallbacks", take_exact_fallbacks());
  stats.add("arrangement faces", cells.faces.size());
  stats.add("shells", cells.shellCount);
  stats.add("cells", cells.shellCount + 1 - cells.componentCount);
  stats.add("output faces", out.faceCount());
  return true;
}

// the subtree under root as its own tree, children first and the leaves left first, so the first-leaf rule
// (spec 5.4) holds; leaf_of maps its leaves back
void cut_subtree(const BoolTree &tree, uint32_t root, BoolTree &sub, dag::Vector<uint32_t> &leaf_of)
{
  dag::Vector<uint32_t> nodes, stack, newIndex(tree.nodes.size(), NO_INDEX);
  sub = BoolTree();
  leaf_of.clear();
  stack.push_back(root);
  while (!stack.empty())
  {
    const uint32_t n = stack.back();
    stack.pop_back();
    if (newIndex[n] != NO_INDEX)
      continue;
    nodes.push_back(n);
    const BoolTree::Node &node = tree.nodes[n];
    if (node.kind == BoolTree::Kind::Leaf)
    {
      newIndex[n] = leaf_of.size();
      leaf_of.push_back(node.leaf);
      sub.leaves.push_back(tree.leaves[node.leaf]);
      continue;
    }
    newIndex[n] = 0; // visited; the copy loop sets it
    stack.push_back(node.child[1]);
    stack.push_back(node.child[0]);
  }
  eastl::sort(nodes.begin(), nodes.end());
  for (uint32_t n : nodes)
  {
    BoolTree::Node copy = tree.nodes[n];
    if (copy.kind == BoolTree::Kind::Leaf)
      copy.leaf = newIndex[n];
    else
      for (uint32_t &c : copy.child)
        c = newIndex[c];
    newIndex[n] = sub.nodes.size();
    sub.nodes.push_back(copy);
  }
}

class ExactKernel final : public IKernel
{
public:
  const char *name() const override { return "exact"; }

  // Concat operands never touch (spec 5.2), so each boolean subtree is its own arrangement
  bool evaluate(const BoolTree &tree, const KernelParams &p, Solid &out, KernelStats &stats, eastl::string &err) override
  {
    out = Solid();
    out.closed = true;
    if (tree.nodes.empty())
      return true;
    if (!(p.tolerance > 0))
    {
      err.sprintf("the tolerance %g is not positive", p.tolerance);
      return false;
    }
    // the output weld's cell indices are int64; output vertices stay in the leaf boxes
    for (const Solid &leaf : tree.leaves)
      if (!leaf.bbox.isempty() && !(max_abs(leaf.bbox) / p.tolerance < 0x1p61))
      {
        err.sprintf("the tolerance %g is too fine for coordinates up to %g", p.tolerance, max_abs(leaf.bbox));
        return false;
      }
    if (!bool_tree_valid(tree, err))
      return false;
    dag::Vector<uint32_t> stack, leafOf;
    stack.push_back(tree.nodes.size() - 1);
    while (!stack.empty())
    {
      const uint32_t n = stack.back();
      stack.pop_back();
      const BoolTree::Node &node = tree.nodes[n];
      if (node.kind == BoolTree::Kind::Concat)
      {
        stack.push_back(node.child[1]);
        stack.push_back(node.child[0]);
        continue;
      }
      if (node.kind == BoolTree::Kind::Leaf)
      {
        const uint32_t first = out.faceCount();
        append_solid(out, tree.leaves[node.leaf]);
        for (uint32_t f = first; f < out.faceCount(); ++f)
          out.leafRef[f] = LeafRef{node.leaf, f - first};
        continue;
      }
      BoolTree sub;
      Solid part;
      cut_subtree(tree, n, sub, leafOf);
      if (!arrange(sub, make_span_const(leafOf), p, part, stats, err))
        return false;
      for (LeafRef &r : part.leafRef)
        r.leaf = leafOf[r.leaf];
      append_solid(out, part);
    }
    return true;
  }
};
} // namespace

IKernel *exact_kernel_instance()
{
  static ExactKernel kernel;
  return &kernel;
}
} // namespace dacsg
