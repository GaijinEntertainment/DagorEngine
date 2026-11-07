// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "cleanup/cleanup.h"
#include "exec/pipeline.h"
#include "graph/optimize.h"
#include "mesh/solidUtil.h"
#include <gameLibs/daCSG/execute_api.h>
#include <gameLibs/daCSG/validate_api.h>
#include <math/dag_check_nan.h>
#include <perfMon/dag_cpuFreq.h>
#include <EASTL/sort.h>

namespace dacsg
{
struct TreeResult
{
  Solid solid;
  Mesh mesh;
  dag::Vector<uint32_t> faceGroup;
  dag::Vector<uint8_t> ownNormal;
  IKernel *kernel = nullptr;
  eastl::string refusal; // the kernel's, when the other kernel evaluated the tree
};

struct LeafCharts final : ChartTest
{
  const Lowered &lw;
  explicit LeafCharts(const Lowered &lowered) : lw(lowered) {}
  bool sameChart(const LeafRef &a, const LeafRef &b) const override { return same_chart(lw, a, b); }
};

struct Executor
{
  IKernel *kernel = nullptr;
  KernelParams kernelParams;
  CleanupParams cleanupParams;
  const ExecuteParams &params;
  ExecuteStats &stats;
  LowerContext ctx;

  Executor(const Graph &g, const ExecuteParams &p, ExecuteStats &s) : params(p), stats(s), ctx(g, kernelParams, s) {}
  bool lowerTree(OpResult root, Lowered &lw, eastl::string &err);
  bool evaluate(OpResult root, const Lowered &lw, IKernel *k, bool can_fall_back, TreeResult &res, eastl::string &err);
  // after one more run with the wide crossing check when the cleanup let a crossing through (spec 5.2 step 3); report:
  // the test of the result held
  bool validates(OpResult root, const Lowered &lw, TreeResult &res, ValidationReport &report);
  // the last step, after the validation (spec 5.2 step 7)
  bool resolve(OpResult root, const Lowered &lw, TreeResult &res, eastl::string &err);
  IKernel *fallback(OpResult root, const BoolTree &tree, IKernel *refused, Solid &out, eastl::string &err, bool batch = false);
  bool batch(Lowered &lw, eastl::string &err);
  bool evaluateAlone(Lowered &lw, uint32_t node, eastl::string &err);
  // a kernel sees the tree, not the graph: the node names the subtree
  bool failed(OpResult node, eastl::string &err) const { return err = ctx.g.getName(node) + ": " + err, false; }
};

// err: the refusal; the other kernel retries; batch: the tree is a batch of the root's tree
IKernel *Executor::fallback(OpResult root, const BoolTree &tree, IKernel *refused, Solid &out, eastl::string &err, bool batch)
{
  IKernel *other = other_kernel(refused);
  if (!other)
    return nullptr;
  eastl::string otherErr;
  out = Solid();
  if (!other->evaluate(tree, kernelParams, out, stats.kernel, otherErr))
  {
    err.append_sprintf("; %s refused too: %s", other->name(), otherErr.c_str());
    return nullptr;
  }
  stats.warnings.push_back(eastl::string(eastl::string::CtorSprintf(), "%snode %s: %s refused: %s; evaluated by %s",
    batch ? "a batch of " : "", ctx.g.getName(root).c_str(), refused->name(), err.c_str(), other->name()));
  err.clear();
  return other;
}

// the leaves the root reaches: a kept batch stands for the nodes it replaced, which stay in the list
static uint32_t reachable_tris(const BoolTree &tree, uint32_t *leaves)
{
  uint32_t tris = 0;
  dag::Vector<uint32_t> stack;
  if (!tree.nodes.empty())
    stack.push_back(tree.nodes.size() - 1);
  while (!stack.empty())
  {
    const BoolTree::Node &n = tree.nodes[stack.back()];
    stack.pop_back();
    if (n.kind != BoolTree::Kind::Leaf)
    {
      stack.push_back(n.child[0]);
      stack.push_back(n.child[1]);
      continue;
    }
    tris += tree.leaves[n.leaf].faceCount();
    if (leaves)
      ++*leaves;
  }
  return tris;
}

static void add_cleanup_stats(CleanupStats &sum, const CleanupStats &s)
{
  sum.weldRemoved += s.weldRemoved;
  sum.coincidentRemoved += s.coincidentRemoved;
  sum.degenerateRemoved += s.degenerateRemoved;
  sum.sliverRemoved += s.sliverRemoved;
  sum.mergeRemoved += s.mergeRemoved;
  sum.overlapRemoved += s.overlapRemoved;
  sum.unfixed += s.unfixed;
  sum.rolledBack |= s.rolledBack;
  sum.genusRolledBack |= s.genusRolledBack;
  for (int i = 0; i < AngleHistogram::BINS; ++i)
  {
    sum.minAngleBefore.bin[i] += s.minAngleBefore.bin[i];
    sum.minAngleAfter.bin[i] += s.minAngleAfter.bin[i];
  }
}

bool Executor::lowerTree(OpResult root, Lowered &lw, eastl::string &err)
{
  const int64_t t = ref_time_ticks();
  const bool lowered = lower(ctx, root, lw, err);
  if (lowered && params.clusterVertices)
    cluster_vertices(lw.tree, kernelParams.tolerance);
  stats.lowerUs += usec_since(t);
  if (!lowered)
    return false;
  stats.trisIn += reachable_tris(lw.tree, &stats.leaves);
  const int64_t batchStart = ref_time_ticks();
  const bool batched = batch(lw, err);
  stats.kernelUs += usec_since(batchStart);
  return batched;
}

bool Executor::evaluate(OpResult root, const Lowered &lw, IKernel *k, bool can_fall_back, TreeResult &res, eastl::string &err)
{
  const uint32_t trisIn = reachable_tris(lw.tree, nullptr);
  int64_t t = ref_time_ticks();
  bool evaluated = true;
  res.kernel = k;
  if (lw.tree.nodes.empty())
  {
    res.solid = Solid();
    res.solid.closed = true;
  }
  else
  {
    evaluated = k->evaluate(lw.tree, kernelParams, res.solid, stats.kernel, err);
    if (!evaluated && can_fall_back)
    {
      res.refusal.sprintf("%s refused: %s", k->name(), err.c_str());
      res.kernel = fallback(root, lw.tree, k, res.solid, err);
      evaluated = res.kernel != nullptr;
    }
  }
  const uint64_t kernelUs = usec_since(t);
  stats.kernelUs += kernelUs;
  if (!evaluated)
    return failed(root, err);
  round_to_float(res.solid);

  NodeStats &ns = stats.nodes.push_back();
  ns.node = root;
  ns.name = ctx.g.getName(root);
  ns.timeUs = kernelUs;
  ns.trisIn = trisIn;
  if (params.cleanup && res.solid.faceCount())
  {
    t = ref_time_ticks();
    const LeafCharts charts(lw);
    const bool cleaned = cleanup(res.solid, cleanupParams, kernelParams.tolerance, ns.cleanup, err, &charts);
    stats.cleanupUs += usec_since(t);
    if (!cleaned)
      return failed(root, err);
  }
  ns.trisOut = res.solid.faceCount();
  return true;
}

bool Executor::resolve(OpResult root, const Lowered &lw, TreeResult &res, eastl::string &err)
{
  const int64_t t = ref_time_ticks();
  const bool resolved = resolve_attributes(lw, res.solid, kernelParams.tolerance, res.mesh, res.faceGroup, res.ownNormal, err);
  stats.resolveUs += usec_since(t);
  return resolved || failed(root, err);
}

bool Executor::validates(OpResult root, const Lowered &lw, TreeResult &res, ValidationReport &report)
{
  int64_t t = ref_time_ticks();
  report = validate(res.solid, VALIDATE_SELF_INTERSECTION);
  stats.validateUs += usec_since(t);
  if (report.valid || !report.selfIntersectingPairs || !params.cleanup || cleanupParams.wideCrossingCheck)
    return report.valid;
  stats.warnings.push_back(eastl::string(eastl::string::CtorSprintf(),
    "node %s: %u self-intersecting face pairs after the cleanup; evaluated again with the wide crossing check",
    ctx.g.getName(root).c_str(), report.selfIntersectingPairs));
  // the stats keep the row of the run whose result is kept
  const uint32_t row = stats.nodes.size() - 1;
  TreeResult again;
  eastl::string err;
  cleanupParams.wideCrossingCheck = true;
  const bool evaluated = evaluate(root, lw, res.kernel, false, again, err);
  cleanupParams.wideCrossingCheck = false;
  t = ref_time_ticks();
  const ValidationReport wide = evaluated ? validate(again.solid, VALIDATE_SELF_INTERSECTION) : ValidationReport();
  stats.validateUs += usec_since(t);
  if (wide.valid)
  {
    stats.nodes[row] = stats.nodes.back();
    res = eastl::move(again);
    report = wide;
  }
  stats.nodes.resize(row + 1);
  return wide.valid;
}

// spec.md 7.2: the exact kernel arranges a boolean region whose leaves exceed maxArrangementTris in batches
bool Executor::batch(Lowered &lw, eastl::string &err)
{
  BoolTree &tree = lw.tree;
  const bool split = is_exact_kernel(kernel);
  const uint32_t budget = kernelParams.maxArrangementTris;
  dag::Vector<uint32_t> size(tree.nodes.size(), 0), alone(tree.nodes.size(), 0); // alone: its biggest region as a kernel call
  for (uint32_t i = 0; i < tree.nodes.size(); ++i)
  {
    const BoolTree::Node n = tree.nodes[i];
    if (n.kind == BoolTree::Kind::Leaf)
    {
      size[i] = tree.leaves[n.leaf].faceCount();
      continue;
    }
    const uint32_t big = size[n.child[1]] > size[n.child[0]] ? 1 : 0;
    for (uint32_t c : {n.child[big], n.child[1 - big]})
      if (split && n.kind != BoolTree::Kind::Concat && size[n.child[0]] + size[n.child[1]] > budget &&
          tree.nodes[c].kind != BoolTree::Kind::Leaf && alone[c] <= budget)
      {
        if (!evaluateAlone(lw, c, err))
          return false;
        if (tree.nodes[c].kind == BoolTree::Kind::Leaf)
        {
          size[c] = tree.leaves.back().faceCount();
          stats.kernel.add("batches", 1);
        }
      }
    size[i] = size[n.child[0]] + size[n.child[1]];
    alone[i] = n.kind == BoolTree::Kind::Concat ? max(alone[n.child[0]], alone[n.child[1]]) : size[i];
    stats.largestRegion = max(stats.largestRegion, alone[i]);
  }
  return true;
}

// The subtree under node rounded, cleaned and tested like an own subtree's result, then one leaf of the tree when
// it has fewer triangles than the subtree's leaves
bool Executor::evaluateAlone(Lowered &lw, uint32_t node, eastl::string &err)
{
  const OpResult region = lw.treeNode[node];
  BoolTree &tree = lw.tree, sub;
  dag::Vector<uint32_t> nodes, stack, index(node + 1, NO_INDEX);
  for (stack.push_back(node); !stack.empty();)
  {
    nodes.push_back(stack.back());
    stack.pop_back();
    if (tree.nodes[nodes.back()].kind != BoolTree::Kind::Leaf)
      stack.insert(stack.end(), tree.nodes[nodes.back()].child, tree.nodes[nodes.back()].child + 2);
  }
  eastl::sort(nodes.begin(), nodes.end());
  uint32_t firstLeaf = NO_INDEX, trisIn = 0;
  for (uint32_t n : nodes)
  {
    BoolTree::Node copy = tree.nodes[n];
    if (copy.kind == BoolTree::Kind::Leaf)
    {
      firstLeaf = min(firstLeaf, copy.leaf);
      trisIn += tree.leaves[copy.leaf].faceCount();
    }
    else
      for (uint32_t &c : copy.child)
        c = index[c];
    index[n] = sub.nodes.size();
    sub.nodes.push_back(copy);
  }
  // the batch keeps the tree's leaf numbers
  sub.leaves = eastl::move(tree.leaves);
  Solid out;
  const bool evaluated = kernel->evaluate(sub, kernelParams, out, stats.kernel, err) ||
                         (params.kernelFallback && fallback(region, sub, kernel, out, err, true));
  tree.leaves = eastl::move(sub.leaves);
  if (!evaluated)
    return failed(region, err);
  round_to_float(out);
  compute_vertex_normals(out);
  CleanupStats cs;
  const LeafCharts charts(lw);
  if (params.cleanup && !cleanup(out, cleanupParams, kernelParams.tolerance, cs, err, &charts))
    return failed(region, err);
  // an arrangement output can have more triangles than its leaves; such a batch saves the region nothing
  if (out.faceCount() >= trisIn)
    return true;
  // a leaf of the next level: valid, the self-intersection test included
  const ValidationReport report = validate(out, VALIDATE_SELF_INTERSECTION);
  if (!report.valid && report.selfIntersectingPairs && params.cleanup && !cleanupParams.wideCrossingCheck)
  {
    cleanupParams.wideCrossingCheck = true;
    const bool again = evaluateAlone(lw, node, err);
    cleanupParams.wideCrossingCheck = false;
    return again;
  }
  // the region stays the kernel's, and the fallback's
  if (!report.valid || !report.selfIntersectionTested)
  {
    stats.warnings.push_back(eastl::string(eastl::string::CtorSprintf(),
      "node %s: a batch of %u triangles does not validate: %s; its region is evaluated unbatched", ctx.g.getName(region).c_str(),
      out.faceCount(), report.error.c_str()));
    return true;
  }
  // the counts of a batch the tree keeps
  add_cleanup_stats(stats.cleanup, cs);
  for (LeafRef &ref : out.leafRef)
    ref = tree.leaves[ref.leaf].leafRef[ref.leafFace];
  out.leafIndex = tree.leaves.size();
  // inner nodes too, so a fallback walk of the tree evaluates no boolean the batch replaced
  for (uint32_t n : nodes)
    if (tree.nodes[n].kind != BoolTree::Kind::Leaf)
      tree.nodes[n] = BoolTree::Node{BoolTree::Kind::Leaf, {0, 0}, out.leafIndex};
  lw.add(eastl::move(out), lw.sources[firstLeaf]);
  return true;
}

static bool evaluate_graph(const Graph &g, OpResult root, dag::ConstSpan<uint32_t> clip, IKernel *kernel, float quality,
  const ExecuteParams &p, double pruned_tolerance, Mesh &out, ExecuteStats &stats, eastl::string &err)
{
  Executor ex(g, p, stats);
  ex.kernel = kernel;
  ex.ctx.clip.assign(clip.empty() ? 0 : g.nodeCount(), 0);
  for (uint32_t node : clip)
    ex.ctx.clip[node] = 1;
  int64_t t = ref_time_ticks();
  ex.ctx.quality = quality;
  ex.ctx.validateLeaves = p.validateLeaves;
  ex.ctx.meshLoader = p.meshLoader;
  ex.ctx.workers = daparallel::worker_count(p.kernelParams.threads);
  ThreadBudget budget(ex.ctx.workers);
  ex.kernelParams = p.kernelParams;
  ex.kernelParams.budget = &budget;
  dag::Vector<OpResult> own;
  BBoxD box;
  double tolerance = 0;
  // the float range holds with a caller's tolerance too: the output is floats
  bool prepared =
    prepare_graph(ex.ctx, root, !p.optimize, own, err) && graph_bbox(ex.ctx, root, box, err) && auto_tolerance(box, tolerance, err);
  ex.cleanupParams = p.cleanupParams;
  // the cleanup moves maxMoveFactor automatic tolerances, at least 2 of a caller's larger one (a vertex cluster's width)
  if (p.kernelParams.tolerance > tolerance)
    ex.cleanupParams.maxMoveFactor *= min(1.0, max(2 / p.cleanupParams.maxMoveFactor, tolerance / p.kernelParams.tolerance));
  if (p.kernelParams.tolerance != 0)
    tolerance = p.kernelParams.tolerance;
  // a root the optimizer pruned to Empty keeps the tolerance of the box it had, spec.md 5.1 and 10 item 5
  else if (tolerance == 0 && pruned_tolerance > 0)
    tolerance = pruned_tolerance;
  stats.lowerUs += usec_since(t);
  if (!prepared)
    return false;
  stats.tolerance = tolerance;
  ex.kernelParams.tolerance = tolerance;

  for (OpResult node : own)
  {
    Lowered nodeLw;
    TreeResult res;
    ValidationReport report;
    if (!ex.lowerTree(node, nodeLw, err) || !ex.evaluate(node, nodeLw, kernel, p.kernelFallback, res, err))
      return false;
    if (!ex.validates(node, nodeLw, res, report))
    {
      stats.warnings.push_back(eastl::string(eastl::string::CtorSprintf(),
        "node %s: a shared subtree does not validate: %s; it is evaluated in the root's tree", g.getName(node).c_str(),
        report.error.c_str()));
      continue;
    }
    if (!ex.resolve(node, nodeLw, res, err))
      return false;
    SourceMesh &src = ex.ctx.newSource();
    res.solid.face = eastl::move(res.mesh.face);
    src.solid = eastl::move(res.solid);
    src.mesh = eastl::move(res.mesh);
    src.faceGroup = eastl::move(res.faceGroup);
    if (eastl::find(res.ownNormal.begin(), res.ownNormal.end(), 1) != res.ownNormal.end())
      src.ownNormal = eastl::move(res.ownNormal);
    src.groupCount = nodeLw.groupCount;
    ex.ctx.instance[node.id] = &src;
  }

  Lowered lw;
  TreeResult res;
  if (!ex.lowerTree(root, lw, err) || !ex.evaluate(root, lw, kernel, p.kernelFallback, res, err))
    return false;
  auto failure = [&](const TreeResult &tr) {
    t = ref_time_ticks();
    const ValidationReport r = validate(tr.solid, VALIDATE_SELF_INTERSECTION);
    stats.validateUs += usec_since(t);
    return r.valid ? eastl::string() : r.error;
  };
  ValidationReport report;
  eastl::string why = !p.validate || ex.validates(root, lw, res, report) ? eastl::string() : report.error;
  // reruns the lowered root tree, instances included
  IKernel *other = !why.empty() && p.kernelFallback && res.kernel == kernel ? other_kernel(kernel) : nullptr;
  if (other)
  {
    TreeResult again;
    eastl::string otherWhy;
    const uint32_t rootRow = stats.nodes.size() - 1;
    const bool evaluated = ex.evaluate(root, lw, other, false, again, otherWhy);
    if (evaluated)
      otherWhy = failure(again);
    if (otherWhy.empty())
    {
      stats.warnings.push_back(eastl::string(eastl::string::CtorSprintf(), "root: %s result failed validation: %s; evaluated by %s",
        kernel->name(), why.c_str(), other->name()));
      why.clear();
      res = eastl::move(again);
      stats.nodes[rootRow] = stats.nodes.back();
    }
    else
      why.append_sprintf("; %s %s: %s", other->name(), evaluated ? "result failed validation" : "failed", otherWhy.c_str());
    // the stats keep the root run whose mesh is returned
    stats.nodes.resize(rootRow + 1);
  }
  // a root that failed validation keeps its mesh as far as the resolve got, and both texts
  eastl::string unresolved;
  if (!ex.resolve(root, lw, res, unresolved) && why.empty())
    return (err = eastl::move(unresolved), false);
  const bool valid = why.empty();
  if (!valid)
    err.sprintf("validation failed: %s%s%s%s%s", why.c_str(), res.refusal.empty() ? "" : "; ", res.refusal.c_str(),
      unresolved.empty() ? "" : "; ", unresolved.c_str());
  if (unresolved.empty())
    compute_output_normals(res.mesh, res.faceGroup, res.ownNormal, res.mesh.addChannel(CH_NORMAL, 3).data);
  out = eastl::move(res.mesh);
  stats.trisOut = out.faceCount();
  return valid;
}

static void to_source_nodes(dag::ConstSpan<uint32_t> origin, Mesh &out, ExecuteStats &stats)
{
  for (FaceInfo &f : out.face)
    if (f.prim < origin.size())
      f.prim = origin[f.prim];
  for (NodeStats &ns : stats.nodes)
    if (ns.node.id < origin.size())
      ns.node.id = origin[ns.node.id];
}

static bool run(const Graph &g, OpResult root, const ExecuteParams &p, Mesh &out, ExecuteStats &stats, eastl::string &err)
{
  if (root.id >= g.nodeCount())
  {
    err.sprintf("root node %u does not exist", root.id);
    return false;
  }
  // a product of finite floats is exact in double, so it is range-tested before it narrows
  float gq = g.getQuality(), pq = p.quality, quality = 0;
  if (!narrow_float(double(gq) * pq, quality) || !(quality > 0))
  {
    err.sprintf("the quality product %g * %g must round to a finite float above zero", gq, pq);
    return false;
  }
  const double tolerance = p.kernelParams.tolerance;
  if (!check_finite(tolerance) || tolerance < 0)
  {
    err.sprintf("tolerance must be finite and non-negative, not %g", tolerance);
    return false;
  }
  IKernel *kernel = find_kernel(p.kernel);
  if (!kernel)
  {
    err.sprintf("%s: %s", is_library_kernel(p.kernel) ? "kernel not built" : "unknown kernel", p.kernel ? p.kernel : "");
    return false;
  }
  if (!p.optimize)
    return evaluate_graph(g, root, {}, kernel, quality, p, 0, out, stats, err);

  Optimized opt;
  OptimizeParams op;
  op.tolerance = tolerance;
  op.bboxPrune = true;
  op.checkGraph = check_graph;
  op.meshLoader = p.meshLoader;
  const int64_t t = ref_time_ticks();
  const bool optimized = optimize(g, root, op, opt, stats.optimizer, err);
  stats.optimizeUs += usec_since(t);
  if (!optimized)
    return false;
  const bool ok =
    evaluate_graph(opt.graph, opt.root, make_span_const(opt.convexChains), kernel, quality, p, opt.tolerance, out, stats, err);
  to_source_nodes(make_span_const(opt.origin), out, stats);
  return ok;
}

bool execute(Graph &g, OpResult root, const ExecuteParams &p, Mesh &out, ExecuteStats *stats, eastl::string *err)
{
  ExecuteStats localStats;
  eastl::string localErr;
  ExecuteStats &st = stats ? *stats : localStats;
  eastl::string &e = err ? *err : localErr;
  st = ExecuteStats();
  e.clear();
  out = Mesh();
  const int64_t start = ref_time_ticks();
  const bool ok = run(g, root, p, out, st, e);
  // a batch has no row and adds its own counts
  for (const NodeStats &ns : st.nodes)
    add_cleanup_stats(st.cleanup, ns.cleanup);
  st.totalUs = usec_since(start);
  return ok;
}
} // namespace dacsg
