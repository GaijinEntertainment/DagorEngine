// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <graphEditor/graph_data.h>
#include <osApiWrappers/dag_mutex.h>

#include <EASTL/hash_map.h>
#include <EASTL/shared_ptr.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>

class DataBlock;

// The first subgraph instance that failed to splice: why, and the stems of the subgraphs from the
// top-level instance down to the failing one, which is last.
struct SubgraphExpandError
{
  eastl::string message;
  eastl::vector<eastl::string> chain;
  // Every top-level instance that failed, or holds one that failed, with the first message about it.
  eastl::vector<eastl::pair<int, eastl::string>> instances;
};

// Where a node that expand_subgraphs spliced in is authored.
struct SubgraphNodeSource
{
  eastl::vector<eastl::string> stems; // the outermost instance's subgraph first, down to the one that holds the node
  int nodeId = -1;                    // in that last subgraph
};

// Subgraph bodies by path (`<root>/<stem>.subgraph.blk`), shared by threads under one lock. An open body is a document
// open in the editor, maybe unsaved: it wins over the file and is never kept as the file.
class SubgraphCache
{
public:
  struct OpenBody
  {
    eastl::shared_ptr<const GraphData> graph;
    eastl::vector<eastl::string> schemaErrors; // a body with any is not inlined
  };

  // Validated once and read without the lock, so the body must not change after the call.
  void publishOpen(const char *path, eastl::shared_ptr<const GraphData> body);
  void removeOpen(const char *path);
  // The caller's part when base_nodes_blk changes.
  void clearFiles();

  bool findOpen(const eastl::string &path, OpenBody &out) const;
  // Null unless the kept file has this size and modification time.
  eastl::shared_ptr<const GraphData> findFile(const eastl::string &path, int64_t size, int64_t mtime) const;
  void storeFile(const eastl::string &path, eastl::shared_ptr<const GraphData> graph, int64_t size, int64_t mtime);

private:
  struct Entry
  {
    eastl::shared_ptr<const GraphData> file;
    int64_t size = 0;
    int64_t mtime = 0;
    OpenBody open;
  };
  mutable dag::mutex lock;
  eastl::hash_map<eastl::string, Entry> entries;
};

// Inline-expands every `plugin:t = "subgraph"` instance node in `gd` into a flat graph by
// taking the referenced child (`<graphs_root_dir>/<descName>.subgraph.blk`: its open body in
// `cache`, else the file), recursively expanding it, then splicing its post-boundary contents
// into `gd` and rewiring edges:
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
// a value can name another instance's output pin, because an input boundary rewrites an
// interior edge to the parent's source pin and a pass-through body then hands that on. Follow the
// chain until a key stops matching; only the last hop is a real node.
// a missing key is a fact, not a gap to fill. An output pin gets no key when its
// boundary has no producer, when the producer is muted, or when an unwired pass-through leaves its
// own in-boundary as the producer - the same cases where the expansion drops the parent wire.
// `cache`, when given, supplies the open bodies and keeps the loaded files for later calls.
// `out_node_instance`, when given, is cleared, then maps each spliced node id to the id of the instance node it came
// from, which the expansion removes from `gd`; a node of a nested subgraph maps to its outermost instance.
// `out_node_source`, when given, is cleared, then maps each spliced node id to where the node is authored.
bool expand_subgraphs(GraphData &gd, const DataBlock &base_nodes_blk, const char *graphs_root_dir,
  eastl::hash_map<uint64_t, uint64_t> *out_instance_output_source = nullptr, SubgraphExpandError *out_error = nullptr,
  SubgraphCache *cache = nullptr, eastl::hash_map<int, int> *out_node_instance = nullptr,
  eastl::hash_map<int, SubgraphNodeSource> *out_node_source = nullptr);

struct ExplodeResult
{
  eastl::vector<GraphData::Node> addedNodes;         // ids above every id in the graph
  eastl::vector<GraphData::Edge> addedEdges;         // a rewired parent edge keeps its id
  eastl::vector<GraphData::Edge> droppedParentEdges; // instance edges with no wire behind them in the child, or a muted second source
  eastl::string error;
};

// One instance's body, one level deep, moved so its centroid lands on the instance, snapped to `snap` (0 or less: no snap).
// Reads only gd.nodes and gd.edges, which need the instance's live position; `gd` is not changed: remove the instance
// with every edge on it, then add addedNodes and addedEdges. False with out.error on failure. `cache` as for expand_subgraphs.
bool explode_subgraph_instance(const GraphData &gd, int instance_id, const DataBlock &base_nodes_blk, const char *graphs_root_dir,
  float snap, ExplodeResult &out, SubgraphCache *cache = nullptr);
