// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <graphEditor/graph_data.h>

#include <EASTL/hash_map.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>

class DataBlock;

// The first subgraph instance that failed to splice: why, and the stems of the subgraphs from the
// top-level instance down to the failing one, which is last.
struct SubgraphExpandError
{
  eastl::string message;
  eastl::vector<eastl::string> chain;
};

// Inline-expands every `plugin:t = "subgraph"` instance node in `gd` into a flat graph by
// loading the referenced child file (`<graphs_root_dir>/<descName>.subgraph.blk`), recursively
// expanding it, then splicing its post-boundary contents into `gd` and rewiring edges:
//   - parent edges into the instance's input pins -> sources of internal edges that previously
//     originated at the matching `subgraph in: TYPE` boundary;
//   - parent edges out of the instance's output pins -> upstream producer of the internal edge
//     that drove the matching `subgraph out` boundary;
//   - the boundary nodes themselves and all edges that touched them are removed.
//
// The user-facing GraphData (held by the plugin) is NOT this `gd` -- callers copy their authoring
// state into a local first and run this on the copy, so the editor still shows instance nodes
// while the compile pipeline sees a flat tree. `base_nodes_blk` is forwarded to the child loaders
// for descriptor resolution; `graphs_root_dir` is the directory the synthesis pass scanned (the
// plugin's subgraphsDir).
//
// Errors (missing child file, parse failure, cycle, duplicate or missing interface boundary,
// multi-source into a single-connect instance input) are logged via debug() and leave the offending
// instance in place; expansion continues with siblings. Returns false when an instance failed, and
// `out_error`, when given, then holds the first failure.
// `out_instance_output_source`, when given, maps each expanded instance output pin to the pin that
// drives it, both as make_pin_id keys. The instance is gone from the flat graph, so this is the only
// way to attribute a per-node compile result back to it.
// constraint: a value can name another instance's output pin, because an input boundary rewrites an
// interior edge to the parent's source pin and a pass-through body then hands that on. Follow the
// chain until a key stops matching; only the last hop is a real node.
// constraint: a missing key is a fact, not a gap to fill. An output pin gets no key when its
// boundary has no producer, when the producer is muted, or when an unwired pass-through leaves its
// own in-boundary as the producer - the same cases where the expansion drops the parent wire.
bool expand_subgraphs(GraphData &gd, const DataBlock &base_nodes_blk, const char *graphs_root_dir,
  eastl::hash_map<uint64_t, uint64_t> *out_instance_output_source = nullptr, SubgraphExpandError *out_error = nullptr);

struct ExplodeResult
{
  eastl::vector<GraphData::Node> addedNodes;         // ids above every id in the graph
  eastl::vector<GraphData::Edge> addedEdges;         // a rewired parent edge keeps its id
  eastl::vector<GraphData::Edge> droppedParentEdges; // instance edges with no wire behind them in the child, or a muted second source
  eastl::string error;
};

// One instance's body, one level deep, moved so its centroid lands on the instance, snapped to `snap` (0 or less: no snap).
// Reads only gd.nodes and gd.edges, which need the instance's live position; `gd` is not changed: remove the instance
// with every edge on it, then add addedNodes and addedEdges. False with out.error on failure.
bool explode_subgraph_instance(const GraphData &gd, int instance_id, const DataBlock &base_nodes_blk, const char *graphs_root_dir,
  float snap, ExplodeResult &out);
