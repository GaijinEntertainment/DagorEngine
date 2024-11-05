// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <detail/autoGrowVector.h>
#include <render/resourceSlot/detail/ids.h>

namespace resource_slot::detail
{

template <typename Allocator>
struct Graph
{
  explicit Graph(intptr_t node_count) : visited(node_count), edges(node_count) {}

  void addEdge(NodeId from, NodeId to) { edges[from].push_back(to); }

  enum class TopologicalSortResult
  {
    SUCCESS,
    IN_CYCLE,
    FOUND_CYCLE
  };

  template <typename NodeList>
  [[nodiscard]] TopologicalSortResult topologicalSort(NodeId node, NodeList &visit_list, NodeList &cycle)
  {
    if (visited[node] == VisitedNode::FINISHED)
      return TopologicalSortResult::SUCCESS;

    if (visited[node] == VisitedNode::IN_PROGRESS)
    {
      cycle.push_back(node);
      visited[node] = VisitedNode::NOT_VISITED;
      return TopologicalSortResult::IN_CYCLE;
    }

    visited[node] = VisitedNode::IN_PROGRESS;
    for (const auto &nextNode : edges[node])
    {
      TopologicalSortResult recursiveRes = topologicalSort(nextNode, visit_list, cycle);
      if (recursiveRes == TopologicalSortResult::SUCCESS)
        continue;

      if (recursiveRes == TopologicalSortResult::IN_CYCLE)
      {
        cycle.push_back(node);
        visited[node] = VisitedNode::NOT_VISITED;
        return node == cycle.front() ? TopologicalSortResult::FOUND_CYCLE : TopologicalSortResult::IN_CYCLE;
      }

      return TopologicalSortResult::FOUND_CYCLE;
    }

    visited[node] = VisitedNode::FINISHED;
    visit_list.push_back(node);
    return TopologicalSortResult::SUCCESS;
  }

private:
  enum class VisitedNode
  {
    NOT_VISITED,
    IN_PROGRESS,
    FINISHED
  };
  AutoGrowVector<NodeId, VisitedNode, EXPECTED_MAX_NODE_COUNT, false, Allocator> visited;
  typedef dag::RelocatableFixedVector<NodeId, EXPECTED_MAX_SLOT_USAGE, true, Allocator> EdgeList;
  AutoGrowVector<NodeId, EdgeList, EXPECTED_MAX_NODE_COUNT, false, Allocator> edges;
};

} // namespace resource_slot::detail
