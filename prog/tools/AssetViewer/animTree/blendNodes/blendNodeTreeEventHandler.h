// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "blendNodeData.h"
#include <propPanel/control/menu.h>
#include <propPanel/control/treeInterface.h>

class DagorAsset;

class BlendNodeTreeEventHandler : public PropPanel::ITreeControlEventHandler, public PropPanel::IMenuEventHandler
{
public:
  BlendNodeTreeEventHandler(dag::Vector<BlendNodeData> &blend_nodes, dag::Vector<String> &paths);
  bool onTreeContextMenu(PropPanel::ContainerPropertyControl &tree, int pcb_id, PropPanel::ITreeInterface &tree_interface) override;
  int onMenuItemClick(unsigned id) override;
  void setAsset(DagorAsset *cur_asset);

private:
  PropPanel::TLeafHandle selLeaf = nullptr;
  PropPanel::ContainerPropertyControl *nodesTree = nullptr;
  DagorAsset *asset = nullptr;
  dag::Vector<BlendNodeData> &blendNodes;
  dag::Vector<String> &paths;

  const char *getA2dName();
};
