#pragma once

#include <detail/autoGrowVector.h>
#include <detail/ids.h>

namespace resource_slot::detail
{

template <typename Allocator>
struct Graph
{
  explicit Graph(intptr_t node_count) : visited(node_count), edges(node_count) {}

  void addEdge(NodeId from, NodeId to) { edges[from].push_back(to); }

  [[nodiscard]] bool topologicalSort(NodeId node, NodeList &visit_list, NodeList &cycle)
  {
    if (visited[node] == VisitedNode::FINISHED)
      return false;

    if (visited[node] == VisitedNode::IN_PROGRESS)
    {
      cycle.push_back(node);
      return true;
    }

    visited[node] = VisitedNode::IN_PROGRESS;
    for (const auto &nextNode : edges[node])
    {
      if (topologicalSort(nextNode, visit_list, cycle))
      {
        cycle.push_back(node);
        return true;
      }
    }

    visited[node] = VisitedNode::FINISHED;
    visit_list.push_back(node);
    return false;
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
