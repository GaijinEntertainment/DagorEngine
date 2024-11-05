// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "nodeMaskData.h"
#include <propPanel/control/treeInterface.h>
#include <sepGui/wndMenuInterface.h>

class DagorAsset;

class NodeMaskTreeEventHandler : public PropPanel::ITreeControlEventHandler, public IMenuEventHandler
{
public:
  NodeMaskTreeEventHandler(dag::Vector<NodeMaskData> &node_masks, dag::Vector<String> &paths);
  virtual bool onTreeContextMenu(PropPanel::ContainerPropertyControl &tree, int pcb_id,
    PropPanel::ITreeInterface &tree_interface) override;
  virtual int onMenuItemClick(unsigned id) override;
  void setAsset(DagorAsset *cur_asset);

private:
  PropPanel::TLeafHandle selLeaf = nullptr;
  PropPanel::ContainerPropertyControl *masksTree = nullptr;
  DagorAsset *asset = nullptr;
  dag::Vector<NodeMaskData> &nodeMasks;
  dag::Vector<String> &paths;
};
