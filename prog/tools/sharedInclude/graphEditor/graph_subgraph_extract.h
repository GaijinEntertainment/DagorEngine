// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <graphEditor/graph_data.h>

#include <generic/dag_span.h>

#include <EASTL/string.h>
#include <EASTL/vector.h>

class DataBlock;

// One parent edge the instance takes over.
struct ExtractCrossing
{
  int parentEdgeId = -1; // the rewired edge keeps this id
  int outsideNode = -1;  // the end that stays in the parent
  int outsidePin = -1;
  int boundaryNodeId = -1; // in the child: synthesized, or copied from the parent
  eastl::string ifaceName;
  bool isInput = false; // outside -> inner, so an instance input
  bool muted = false;
};

struct ExtractPlan
{
  eastl::vector<ExtractCrossing> crossings; // inputs first, then outputs
  eastl::vector<eastl::string> warnings;
  eastl::string error;
  float centroidX = 0.0f; // mean top-left of the extracted nodes
  float centroidY = 0.0f;
};

// The pure half of Create subgraph: `out_child` gets the nodes (parent ids kept) plus a boundary per crossing
// input edge and per crossing output pin, `out_plan` how the parent edges map onto the instance. False with out_plan.error.
// `graphs_root_dir` holds the files of the instances that feed the set: their outputs declare every type,
// so an input boundary takes the one their body produces. Of `parent` only nodes, edges and graphTexture* are read.
bool extract_subgraph(const GraphData &parent, dag::ConstSpan<int> node_ids, const DataBlock &base_nodes_blk,
  const char *graphs_root_dir, GraphData &out_child, ExtractPlan &out_plan);
