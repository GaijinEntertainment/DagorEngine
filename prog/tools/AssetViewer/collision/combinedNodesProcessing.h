// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "collisionNodesSettings.h"
#include <propPanel/control/container.h>

struct CombinedNode
{
  dag::Vector<Point3> vertices;
  dag::Vector<uint16_t> indices;
  BBox3 boundingBox;
  BSphere3 boundingSphere;
};

class CombinedNodesProcessing
{
public:
  SelectedNodesSettings selectedSettings;
  CombinedNode selectedNode;
  dag::Vector<CombinedNode> nodes;

  void init(CollisionResource *collision_res, PropPanel::ContainerPropertyControl *prop_panel);
  void calcSelectedCombinedNode();
  void calcCombinedNode(const SelectedNodesSettings &settings);
  void setSelectedNodesSettings(SelectedNodesSettings &&selected_nodes);
  void addSelectedCombinedNode();
  void clearSelectedNode();
  void renderCombinedNodes(bool is_faded, bool draw_solid, const dag::Vector<SelectedNodesSettings> &combined_settings);
  void updateSeletectedSettings();

private:
  PropPanel::ContainerPropertyControl *panel = nullptr;
  CollisionResource *collisionRes = nullptr;
};
