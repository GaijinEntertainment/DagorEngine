// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "mesh/solidUtil.h"
#include <gameLibs/daCSG/kernel_api.h>
#include <gameLibs/daParallel/parallel_api.h>
#include <EASTL/sort.h>
#include <EASTL/utility.h>

namespace dacsg
{
bool bool_tree_valid(const BoolTree &tree, eastl::string &err)
{
  dag::Vector<uint8_t> used(tree.nodes.size(), 0);
  for (uint32_t i = 0; i < tree.nodes.size(); ++i)
  {
    const BoolTree::Node &n = tree.nodes[i];
    const uint32_t a = n.child[0], b = n.child[1];
    if (n.kind == BoolTree::Kind::Leaf && n.leaf >= tree.leaves.size())
      return (err.sprintf("bool tree node %u: leaf %u does not exist", i, n.leaf), false);
    if (n.kind != BoolTree::Kind::Leaf && (a >= i || b >= i || used[a] || used[b]))
      return (err.sprintf("bool tree node %u: children must come first, once each", i), false);
    if (n.kind != BoolTree::Kind::Leaf)
      used[a] = used[b] = 1;
  }
  return true;
}

// spec.md 12: the counters and error of node order
bool IPairwiseKernel::evaluate(const BoolTree &tree, const KernelParams &p, Solid &out, KernelStats &stats, eastl::string &err)
{
  out = Solid();
  const uint32_t count = tree.nodes.size();
  if (!count)
  {
    out.closed = true;
    return true;
  }
  if (!bool_tree_valid(tree, err))
    return false;
  dag::Vector<Solid> owned(count);
  dag::Vector<const Solid *> result(count, nullptr);
  dag::Vector<uint32_t> height(count, 0);
  uint32_t top = 0;
  for (uint32_t i = 0; i < count; ++i)
  {
    const BoolTree::Node &n = tree.nodes[i];
    if (n.kind == BoolTree::Kind::Leaf)
      result[i] = &tree.leaves[n.leaf];
    else
    {
      height[i] = 1 + max(height[n.child[0]], height[n.child[1]]);
      top = max(top, height[i]);
    }
  }

  dag::Vector<KernelStats> nodeStats(count);
  dag::Vector<eastl::string> nodeErr(count);
  dag::Vector<uint8_t> failed(count, 0);
  // the booleans of a height and their loops share one budget
  ThreadBudget ownBudget(daparallel::worker_count(p.threads));
  KernelParams kp = p;
  kp.budget = p.budget ? p.budget : &ownBudget;
  auto run = [&](uint32_t i) {
    const BoolTree::Node &n = tree.nodes[i];
    const uint32_t a = n.child[0], b = n.child[1];
    if (n.kind == BoolTree::Kind::Concat)
    {
      nodeStats[i].add("concat", 1);
      if (result[a] == &owned[a] && a != b)
        owned[i] = eastl::move(owned[a]);
      else
        owned[i] = *result[a];
      append_solid(owned[i], *result[b]);
    }
    else
    {
      nodeStats[i].add("boolean", 1);
      if (!boolean(n.kind, *result[a], *result[b], kp, owned[i], nodeStats[i], nodeErr[i]))
      {
        failed[i] = 1;
        return;
      }
    }
    owned[a] = Solid();
    owned[b] = Solid();
    result[a] = result[b] = nullptr;
    result[i] = &owned[i];
  };
  const uint32_t workers = concurrentBooleans() ? daparallel::worker_count(p.threads) : 1;
  uint32_t firstFailed = count;
  dag::Vector<uint32_t> wave, byHeightStart, byHeight;
  if (workers > 1)
    bucket_items(count, top + 1, [&](uint32_t i) { return height[i]; }, byHeightStart, byHeight);
  // one worker walks in node order: a subtree's results stay in cache
  for (uint32_t h = 1; h <= (workers > 1 ? top : min(top, 1u)); ++h)
  {
    wave.clear();
    if (workers > 1)
    {
      for (uint32_t k = byHeightStart[h]; k < byHeightStart[h + 1]; ++k)
        if (byHeight[k] < firstFailed)
          wave.push_back(byHeight[k]);
    }
    else
      for (uint32_t i = 0; i < firstFailed; ++i)
        if (height[i] > 0)
          wave.push_back(i);
    if (workers > 1 && wave.size() > 1)
    {
      auto work = [&](uint32_t i) {
        const BoolTree::Node &n = tree.nodes[i];
        return result[n.child[0]]->faceCount() + result[n.child[1]]->faceCount();
      };
      eastl::sort(wave.begin(), wave.end(), [&](uint32_t x, uint32_t y) { return work(x) > work(y); });
      const WorkerLease lease(kp.budget, min(workers, uint32_t(wave.size())));
      daparallel::parallel_run(lease.workers(), wave.size(), 1, [&](uint32_t k, uint32_t, uint32_t) { run(wave[k]); });
    }
    else
      for (uint32_t k = 0; k < wave.size() && (k == 0 || !failed[wave[k - 1]]); ++k)
        run(wave[k]);
    for (uint32_t i : wave)
      firstFailed = failed[i] ? min(firstFailed, i) : firstFailed;
  }
  for (uint32_t i = 0; i < min(count, firstFailed + 1); ++i)
    for (const KernelCounter &c : nodeStats[i].counters)
      stats.add(c.name, c.value);
  if (firstFailed < count)
    return (err = nodeErr[firstFailed], false);
  if (result[count - 1] == &owned[count - 1])
    out = eastl::move(owned[count - 1]);
  else
    out = *result[count - 1];
  return true;
}
} // namespace dacsg
