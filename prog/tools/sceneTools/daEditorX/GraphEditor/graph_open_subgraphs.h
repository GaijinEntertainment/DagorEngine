// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_span.h>
#include <graphEditor/graph_data.h>

#include <EASTL/hash_map.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>

class GraphDocument;
class SubgraphCache;

// A subgraph window's document and the path its instances resolve to.
struct OpenSubgraphDoc
{
  int tabId = -1;
  eastl::string stem;
  eastl::string path;
  const GraphDocument *doc = nullptr;
};

// Keeps the open bodies of the subgraph cache in step with the subgraph windows. Main thread only.
class OpenSubgraphs
{
public:
  // Copies the nodes and edges of each document changed since its last publish, node moves too, and drops the bodies of windows
  // that closed or moved to another path. True when the compile would read something else: an unmodified body compiles as its file.
  bool publish(dag::ConstSpan<OpenSubgraphDoc> docs, SubgraphCache &cache);
  // Each window's subgraph pins in order, redone when its graph or a node position moved; none while the schema fails, so
  // its instances keep theirs.
  void refreshInterfaces(dag::ConstSpan<OpenSubgraphDoc> docs);
  // Null when no open window shows that subgraph with a valid interface.
  const eastl::vector<GraphData::Pin> *findInterface(const char *stem) const;
  uint32_t getInterfaceGeneration() const { return interfaceGeneration; }

private:
  struct Published
  { // -V802 PVS sizes EASTL members as one byte; at their real sizes no member order is smaller
    eastl::string path;
    uint64_t revision = 0;
    uint64_t positions = 0;
    bool modified = false;
  };
  eastl::hash_map<int, Published> byTabId;

  struct Interface
  { // -V802 PVS sizes EASTL members as one byte; at their real sizes no member order is smaller
    eastl::string stem;
    uint64_t revision = ~uint64_t(0);
    uint64_t positions = ~uint64_t(0);
    eastl::vector<GraphData::Pin> pins;
    bool valid = false;
  };
  eastl::hash_map<int, Interface> interfaceByTabId;
  uint32_t interfaceGeneration = 0;
};
