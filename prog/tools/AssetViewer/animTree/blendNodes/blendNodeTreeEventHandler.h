// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "blendNodeData.h"
#include <propPanel/control/treeInterface.h>
#include <sepGui/wndMenuInterface.h>

class DagorAsset;

class BlendNodeTreeEventHandler : public PropPanel::ITreeControlEventHandler, public IMenuEventHandler
{
public:
  BlendNodeTreeEventHandler(dag::Vector<BlendNodeData> &blend_nodes, dag::Vector<String> &paths);
  virtual bool onTreeContextMenu(PropPanel::ContainerPropertyControl &tree, int pcb_id,
    PropPanel::ITreeInterface &tree_interface) override;
  virtual int onMenuItemClick(unsigned id) override;
  void setAsset(DagorAsset *cur_asset);

private:
  PropPanel::TLeafHandle selLeaf = nullptr;
  PropPanel::ContainerPropertyControl *nodesTree = nullptr;
  DagorAsset *asset = nullptr;
  dag::Vector<BlendNodeData> &blendNodes;
  dag::Vector<String> &paths;

  const char *getA2dName();
};
