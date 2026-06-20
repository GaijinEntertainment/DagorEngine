//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
#include <generic/dag_smallTab.h>
#include <scene/dag_visTree.h>

struct HierVisBuildNode
{
  uint32_t flags;
  uint16_t first, last;
  BBox3 bbox;
  BSphere3 bsph;
  float maxSubobjRad;
};

class BuildVisTree
{
public:
  SmallTab<HierVisNode> nodes;
  int leavesCount = 0; // number of leaves - first N of nodes are leaves

  void build(dag::Span<HierVisBuildNode> leaves);

protected:
  static void buildHierVisData(SmallTab<HierVisNode> &buildList, dag::Span<HierVisBuildNode> leaves);
  static void recursiveBuild(SmallTab<HierVisNode> &buildList, int node_id);
};
