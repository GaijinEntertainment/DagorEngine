// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>
#include <EASTL/vector.h>

#include <graphEditor/graph_data.h>

// Value types shared by the canvas, the graph document and the undo entries.

// A node id with a canvas position.
struct NodePos
{
  int nodeId;
  float x;
  float y;
};

// A block (group) node id with its size.
struct BlockSize
{
  int nodeId;
  float width;
  float height;
};

// Node and link ids kept sorted, so two selections compare as sets. imgui-node-editor owns one
// mixed selection, so nodes and links travel together.
struct GraphSelection
{
  eastl::vector<int> nodes;
  eastl::vector<int> links;

  bool operator==(const GraphSelection &) const = default;
};

// One subgraph instance's pins and the edges touching it, each edge with its list index, ascending.
struct InstancePins
{
  eastl::vector<GraphData::Pin> pins;
  eastl::vector<GraphData::Edge> edges;
  eastl::vector<int> edgeIndices;
};

// Substituted for UNSET_HEIGHT rather than stored, so an unset graph and one saved with the default
// read the same. The values mirror what the standalone graphEditor app reads from landSettings.
constexpr float DEFAULT_HEIGHT_SCALE = 2000.0f;
constexpr float DEFAULT_HEIGHT_MIN = -0.05f;
constexpr float DEFAULT_CELL_SIZE = 4.0f;

inline float effective_height(float v, float fallback) { return v == UNSET_HEIGHT ? fallback : v; }

// The graph-level fields edited with no node selected. Compared whole for the changed-guard.
struct GraphSettings
{
  eastl::string renderDir;
  eastl::string entityDir;
  float heightmapScale = UNSET_HEIGHT;
  float heightmapMin = UNSET_HEIGHT;
  float heightmapCellSize = UNSET_HEIGHT;
  int graphTextureWidth = 0;
  int graphTextureHeight = 0;
  int graphTextureDepth = 0;
  eastl::string graphTextureType;
  eastl::string graphTextureWrap;

  bool operator==(const GraphSettings &) const = default;
};
