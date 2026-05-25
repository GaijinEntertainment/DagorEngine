// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <graphEditor/graph_data.h>

class DataBlock;

// Compiles a GraphData (nodes + edges + propertyValues + descriptor data from
// base_nodes.blk) into the two DataBlocks the texgen pipeline consumes. Both
// outputs are cleared then populated. Returns false on internal failure (e.g.
// the 400-iteration topo-sort guard fires); malformed input yields a partial
// graph with return value true.
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
bool compile_graph_to_blks(const GraphData &data, const DataBlock &base_nodes_blk, const char *shader_includes_dir,
  DataBlock &out_main_graph_blk, DataBlock &out_shader_list_blk);
