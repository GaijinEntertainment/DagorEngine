// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "animStatesData.h"
#include <propPanel/control/treeInterface.h>
#include <sepGui/wndMenuInterface.h>

class DagorAsset;

class AnimStatesTreeEventHandler : public PropPanel::ITreeControlEventHandler, public IMenuEventHandler
{
public:
  AnimStatesTreeEventHandler(dag::Vector<AnimStatesData> &states, dag::Vector<String> &paths);
  virtual bool onTreeContextMenu(PropPanel::ContainerPropertyControl &tree, int pcb_id,
    PropPanel::ITreeInterface &tree_interface) override;
  virtual int onMenuItemClick(unsigned id) override;
  void setAsset(DagorAsset *cur_asset);

private:
  PropPanel::TLeafHandle selLeaf = nullptr;
  PropPanel::ContainerPropertyControl *statesTree = nullptr;
  DagorAsset *asset = nullptr;
  dag::Vector<AnimStatesData> &states;
  dag::Vector<String> &paths;
};
