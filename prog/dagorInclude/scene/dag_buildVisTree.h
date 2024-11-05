//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
#include <generic/dag_smallTab.h>
#include <scene/dag_visTree.h>

/// building
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
  BuildVisTree() : leavesCount(0) {}
  SmallTab<HierVisNode, MidmemAlloc> nodes;
  int leavesCount; // number of leaves - first N of nodes are leaves

  void build(HierVisBuildNode *leaves, int leaves_count, bool clr_lp = true);
  void clearLastPlaneWord();

protected:
  static Tab<HierVisNode> buildList;
  static void buildHierVisData(HierVisBuildNode *leaves, int leaves_count);
  static void recursiveBuild(int node_id);
};
