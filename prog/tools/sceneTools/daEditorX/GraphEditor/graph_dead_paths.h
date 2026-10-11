// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <graphEditor/graph_data.h>

#include <EASTL/hash_map.h>
#include <EASTL/vector.h>
#include <stdint.h>

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
void compute_dead_paths(const GraphData &g, const eastl::hash_map<int, int> &id_to_index, DeadPaths &out);

struct LiveEdge
{
  int nodeA = 0; // into GraphData::nodes
  int pinA = 0;
  int nodeB = 0;
  int pinB = 0;
};

// False when either end dangles or a pin index is out of range.
// constraint: every pass over the edges shares this, or they disagree about which edges exist.
inline bool resolve_edge_endpoints(const GraphData &g, const eastl::hash_map<int, int> &id_to_index, const GraphData::Edge &edge,
  LiveEdge &out)
{
  const auto ia = id_to_index.find(edge.elemA);
  const auto ib = id_to_index.find(edge.elemB);
  if (ia == id_to_index.end() || ib == id_to_index.end())
  {
    return false;
  }
  out.nodeA = ia->second;
  out.nodeB = ib->second;
  out.pinA = edge.pinA;
  out.pinB = edge.pinB;
  return out.pinA >= 0 && out.pinA < static_cast<int>(g.nodes[out.nodeA].pins.size()) && out.pinB >= 0 &&
         out.pinB < static_cast<int>(g.nodes[out.nodeB].pins.size());
}

template <class Fn>
void for_each_live_edge(const GraphData &g, const DeadPaths &dead, const eastl::hash_map<int, int> &id_to_index, Fn &&fn)
{
  LiveEdge e;
  for (int i = 0; i < static_cast<int>(g.edges.size()); ++i)
  {
    if (!dead.isDeadEdge(i) && resolve_edge_endpoints(g, id_to_index, g.edges[i], e))
    {
      fn(e);
    }
  }
}

// Which pins an edge lands on, flat: pinBase[node] + pin.
struct PinConnectivity
{
  eastl::vector<int> pinBase;
  eastl::vector<uint8_t> connected;
};

void build_pin_connectivity(const GraphData &g, const DeadPaths &dead, const eastl::hash_map<int, int> &id_to_index,
  PinConnectivity &conn);
