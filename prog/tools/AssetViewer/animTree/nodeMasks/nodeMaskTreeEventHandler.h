// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "nodeMaskData.h"
#include <propPanel/control/menu.h>
#include <propPanel/control/treeInterface.h>

class DagorAsset;

class NodeMaskTreeEventHandler : public PropPanel::ITreeControlEventHandler, public PropPanel::IMenuEventHandler
{
public:
  NodeMaskTreeEventHandler(dag::Vector<NodeMaskData> &node_masks, dag::Vector<String> &paths);
  bool onTreeContextMenu(PropPanel::ContainerPropertyControl &tree, int pcb_id, PropPanel::ITreeInterface &tree_interface) override;
  int onMenuItemClick(unsigned id) override;
  void setAsset(DagorAsset *cur_asset);

private:
  PropPanel::TLeafHandle selLeaf = nullptr;
  PropPanel::ContainerPropertyControl *masksTree = nullptr;
  DagorAsset *asset = nullptr;
  dag::Vector<NodeMaskData> &nodeMasks;
  dag::Vector<String> &paths;
};
