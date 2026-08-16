// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/vector.h>
#include <stdint.h>

struct GraphData;

// Derived dataflow state behind GraphData::Edge::muted.
//
//   An input pin is FED  if at least one edge lands on it.
//   An edge is DEAD      if it is muted, or the node producing its data is dead.
//   A node is DEAD       if it has at least one fed input pin and none of its fed input
//                        pins still has a live edge landing on it.
//
// Both corollaries are load-bearing:
//  - a node with no fed input pin is never dead, which is what keeps source nodes alive;
//  - a never-connected pin makes nothing dead. It resolves to def_val at compile time
//    (substitute() in graph_compile.cpp), so only an authored mute propagates.
//
// The compiler skips dead edges when building connectivity and drops dead nodes from the
// emitted graph entirely; the canvas draws a muted edge dashed and a dead-but-not-muted one
// solid and dimmed ("disabled").
struct DeadPaths
{
  eastl::vector<uint8_t> deadNode; // parallel to GraphData::nodes
  eastl::vector<uint8_t> deadEdge; // parallel to GraphData::edges

  bool isDeadNode(int node_index) const
  {
    return node_index >= 0 && node_index < static_cast<int>(deadNode.size()) && deadNode[node_index] != 0;
  }
  bool isDeadEdge(int edge_index) const
  {
    return edge_index >= 0 && edge_index < static_cast<int>(deadEdge.size()) && deadEdge[edge_index] != 0;
  }
};

// Recomputes `out` from scratch in O(nodes + edges). Tolerates malformed graphs the same way
// build_connectivity does: dangling endpoints and out-of-range pin indices are ignored, so a
// mute on such an edge simply has no effect.
void compute_dead_paths(const GraphData &g, DeadPaths &out);
