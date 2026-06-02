// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <graphEditor/graph_data.h>

class DataBlock;

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
// multi-source into a single-connect instance input) are logged via debug() and the offending
// instance is left in place; expansion continues with siblings. The plugin layer can emit a
// summary DAEDITOR3.conError when the return value indicates trouble. Matches the tolerance
// of compile_graph_to_blks.
//
// Returns true if expansion ran to completion (with or without per-instance warnings). Returns
// false only on a hard internal failure (currently unused; kept for future extension).
bool expand_subgraphs(GraphData &gd, const DataBlock &base_nodes_blk, const char *graphs_root_dir);
