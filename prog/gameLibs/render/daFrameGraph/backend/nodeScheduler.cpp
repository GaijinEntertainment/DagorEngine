// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "nodeScheduler.h"

#include <EASTL/bitvector.h>
#include <EASTL/stack.h>
#include <EASTL/algorithm.h>
#include <EASTL/numeric.h>
#include <dag/dag_vectorSet.h>

#include <debug/dag_assert.h>
#include <debug/backendDebug.h>

#include <id/idRange.h>


namespace dafg
{

auto NodeScheduler::schedule(const intermediate::Graph &graph, const PassColoring &pass_coloring) -> NodePermutation
{
  static constexpr intermediate::NodeIndex NOT_VISITED = MINUS_ONE_SENTINEL_FOR<intermediate::NodeIndex>;

  NodePermutation result(graph.nodes.size(), NOT_VISITED);

  FRAMEMEM_VALIDATE;

  // This is called Kahn's algorithm, apparently. It generates a
  // topological sort of the graph and is strictly better than a DFS
  // or a BFS because depending on choice of next node at each iteration,
  // we are capable of generating ANY correct topological sort.

  // "how many edges point into me?"
  IdIndexedMapping<intermediate::NodeIndex, uint16_t, framemem_allocator> inDegree(graph.nodes.size());
  for (auto [nodeId, node] : graph.nodes.enumerate())
    for (auto pred : node.predecessors)
      ++inDegree[pred];

  // "how many edges point into this pass from different passes?"
  IdIndexedMapping<PassColor, uint16_t, framemem_allocator> passInDegree;
  passInDegree.reserve(graph.nodes.size());
  for (auto [nodeId, _] : graph.nodes.enumerate())
    passInDegree.expandMapping(pass_coloring[nodeId], 0);

  for (auto [nodeId, node] : graph.nodes.enumerate())
    for (auto pred : node.predecessors)
      if (pass_coloring[nodeId] != pass_coloring[pred])
        ++passInDegree[pass_coloring[pred]];

  // The frontier contains all nodes with in-degree 0
  dag::Vector<intermediate::NodeIndex, framemem_allocator> frontier;
  frontier.reserve(graph.nodes.size());
  frontier.push_back(static_cast<intermediate::NodeIndex>(graph.nodes.size() - 1));

  eastl::underlying_type_t<intermediate::NodeIndex> timer = graph.nodes.size() - 1;
  PassColor lastColor = pass_coloring.back();


  auto orderComp = [&graph, &lastColor, &pass_coloring, &passInDegree](intermediate::NodeIndex l, intermediate::NodeIndex r) {
    if (graph.nodes[l].multiplexingIndex != graph.nodes[r].multiplexingIndex)
      return graph.nodes[l].multiplexingIndex < graph.nodes[r].multiplexingIndex;

    if ((pass_coloring[l] == lastColor) != (pass_coloring[r] == lastColor))
      return (pass_coloring[l] == lastColor) < (pass_coloring[r] == lastColor);

    if (passInDegree[pass_coloring[l]] != passInDegree[pass_coloring[r]])
      return passInDegree[pass_coloring[l]] > passInDegree[pass_coloring[r]];

    if (graph.nodes[l].priority != graph.nodes[r].priority)
      return graph.nodes[l].priority < graph.nodes[r].priority;

    return eastl::to_underlying(l) > eastl::to_underlying(r);
  };

  // The core property of the pass coloring is that if we condense the
  // graph according to it, we should still get a DAG.
  // This property guarantees to us that if we schedule nodes using
  // passInDegree as priority, each pass will be scheduled as a
  // contiguous sequence of nodes.
  // Sadly, when multiplexing is enabled, we cannot do that due to
  // legacy global state modification in some projects, so we issue
  // a warning whenever this property is violated for informative purposes.
  bool passContiguityViolated = false;

  while (!frontier.empty())
  {
    // This can be a bit slow, but the way orderComp works changes on
    // every iteration, so we can't really cache anything :/
    const auto it = eastl::max_element(frontier.begin(), frontier.end(), orderComp);
    const auto curr = *it;

    frontier.erase_unsorted(it);
    G_FAST_ASSERT(inDegree[curr] == 0 && result[curr] == NOT_VISITED);

    if (passInDegree[pass_coloring[curr]] != 0)
      passContiguityViolated = true;

    result[curr] = static_cast<intermediate::NodeIndex>(timer--);
    lastColor = pass_coloring[curr];

    for (auto pred : graph.nodes[curr].predecessors)
    {
      if (pass_coloring[curr] != pass_coloring[pred])
        --passInDegree[pass_coloring[pred]];

      if (--inDegree[pred] == 0)
        frontier.push_back(pred);
    }
  }

  if (passContiguityViolated)
    logwarn("daFG: Node scheduler could not schedule some passes to be contiguous! This is fine, but sub-optimal.");

#if DAGOR_DBGLEVEL > 0
  for (auto i : result)
    G_ASSERTF(i != NOT_VISITED, "daFG: IR graph was not a DAG! This is an invariant that MUST hold!");
#endif

  return result;
}

} // namespace dafg
