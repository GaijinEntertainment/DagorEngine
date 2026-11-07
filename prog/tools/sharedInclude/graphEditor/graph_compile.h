// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <graphEditor/graph_data.h>

class DataBlock;
struct GraphResolutions;
struct DeadPaths;
struct SubgraphNodeSource;

struct CompiledNameSource
{
  int nodeId = -1;
  int pinIdx = -1; // output pin of a register; -1 for a block, or when no pin owns the register
};
using CompiledNameSources = eastl::hash_map<eastl::string, CompiledNameSource>;

// Compiles a GraphData (nodes + edges + propertyValues + descriptor data from
// base_nodes.blk) into the two DataBlocks the texgen pipeline consumes. Both
// outputs are cleared then populated. Returns false only when the topo-sort pass
// left nodes unemitted after its 400-pass guard fired; malformed input, including a
// dependency loop, yields a partial graph with return value true.
//
// `out_message` reports the outcome in the user's terms, empty when there is nothing
// to say. Severity follows the return value, so a message with true is a warning.
//
// `shader_includes_dir` is the directory containing `_<descName>.blk` shader
// files; included via BLK's text `include` directive when parsing the shader
// list. Pass nullptr to skip include resolution (shader_editor nodes will then
// fail to find their shaders -- the texgen call will log "shader <X> not found").
//
// Mirrors `generateHlslCode` in tools/graphEditor/mainNodes.js. The C++ port
// emits BLK directly (no /*MAIN_GRAPH_START*/ marker comments) since the
// pipeline never reads the marker form -- the JS only emits markers so the
// BLK can be embedded in a JSON file.
//
// `out_pin_custom_texture_names` is parallel to `data.nodes`:
// `out_pin_custom_texture_names[nodeIdx][pinIdx]` is the texgen register name
// assigned to that output pin (empty for input pins, non-texture outputs, and
// bypassed nodes). Compile writes here instead of mutating `data` so the
// preview panel's non-locked read of `Pin::customTextureName` on the main
// thread doesn't race with the worker-thread compile. Callers must apply the
// names to `GraphData::Pin::customTextureName` on the main thread.
//
// `out_node_resolutions`, when given, receives the per-node propagated texture size / format,
// parallel to `data.nodes`. The canvas footer reads it so it shows what the compile emits rather
// than a second guess at the same rules. `out_dead` receives the dead set this compile used, so a
// caller that needs to know which nodes were dropped does not recompute it.
//
// `out_name_sources`, when given, is cleared, then maps each node block name of `out_main_graph_blk`, and each
// output register by the name texgen prints for it (`<block>.<n>`), to the node that made it.
// `out_loop_node_ids` receives the ids of every node on the dependency loop `out_message` reports; only with `out_message`.
bool compile_graph_to_blks(const GraphData &data, const DataBlock &base_nodes_blk, const char *shader_includes_dir,
  DataBlock &out_main_graph_blk, DataBlock &out_shader_list_blk,
  eastl::vector<eastl::vector<eastl::string>> &out_pin_custom_texture_names, eastl::string *out_message = nullptr,
  GraphResolutions *out_node_resolutions = nullptr, DeadPaths *out_dead = nullptr, CompiledNameSources *out_name_sources = nullptr,
  eastl::vector<int> *out_loop_node_ids = nullptr);

// Clears `out_labels`, then labels each of `sources` with its node's canvas title: 'Title', 'Instance/Title' for a node
// spliced from a subgraph instance of `shown` (`node_instance` as from expand_subgraphs), 'Title.Pin' for a register
// of an output pin. `compiled` is the graph `sources` came from. A node with no title gets no label.
void make_compiled_name_labels(const GraphData &compiled, const GraphData &shown, const eastl::hash_map<int, int> &node_instance,
  const CompiledNameSources &sources, eastl::hash_map<eastl::string, eastl::string> &out_labels);

// Clears `out_node_ids`, then maps each node block of `main_graph_blk` to the id of the node drawn for it: its own, or
// that of the subgraph instance it was spliced from (`node_instance` as from expand_subgraphs). Clears `out_sources`,
// then maps the block of each spliced node to where it is authored (`node_source` as from expand_subgraphs).
void make_block_node_ids(const DataBlock &main_graph_blk, const CompiledNameSources &sources,
  const eastl::hash_map<int, int> &node_instance, const eastl::hash_map<int, SubgraphNodeSource> &node_source,
  eastl::hash_map<eastl::string, int> &out_node_ids, eastl::hash_map<eastl::string, SubgraphNodeSource> &out_sources);
