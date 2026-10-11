// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/hash_map.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>

// constraint: one pass advances one dependency level, so this also caps chain depth.
constexpr int MAX_GRAPH_CHAIN_PASSES = 400;

struct GraphData;
struct DeadPaths;
struct PinConnectivity;
class DataBlock;

// The flag leads on purpose, and the layout is already tight: the three ints fill the padding after
// it, so the strings start 8-aligned and the whole is 64 bytes. Moving the flag last costs 8. PVS
// models each string as one byte, which is the arithmetic behind its "20 to 16".
struct NodeResolution
{                        // -V802
  bool resolved = false; // every other field is meaningless when false
  int width = 0;
  int height = 0;
  int depth = 0;
  eastl::string type;
  eastl::string wrap;
};

// `resolved` is a validity flag, not part of the value, so it takes no part here.
inline bool same_texture_result(const NodeResolution &a, const NodeResolution &b)
{
  return a.width == b.width && a.height == b.height && a.depth == b.depth && a.type == b.type && a.wrap == b.wrap;
}

struct GraphResolutions
{
  eastl::vector<NodeResolution> nodes; // parallel to GraphData::nodes

  // Null unless the node resolved, so a caller never reads a zero-filled record by mistake.
  const NodeResolution *byIndex(int node_index) const
  {
    if (node_index < 0 || node_index >= static_cast<int>(nodes.size()) || !nodes[node_index].resolved)
    {
      return nullptr;
    }
    return &nodes[node_index];
  }
};

// Recomputes `out` from scratch. `node_descriptors` is parallel to `g.nodes` and may hold nulls.
// `seeds`, keyed by node id, pre-resolves subgraph instances the caller already knows; the rest
// propagate from them. Only instances take a seed, because a recycled id can hand an old entry to
// an unrelated node. A node with no upstream is left unclamped.
void compute_graph_resolutions(const GraphData &g, const eastl::vector<const DataBlock *> &node_descriptors,
  const eastl::hash_map<int, int> &id_to_index, const DeadPaths &dead, const PinConnectivity &conn,
  const eastl::hash_map<int, NodeResolution> *seeds, GraphResolutions &out);
