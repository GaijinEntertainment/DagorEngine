// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "animCtrlData.h"
#include "../childsDialog.h"
#include <propPanel/control/menu.h>
#include <propPanel/control/treeInterface.h>

class DagorAsset;

class CtrlTreeEventHandler : public PropPanel::ITreeControlEventHandler, public PropPanel::IMenuEventHandler
{
public:
  CtrlTreeEventHandler(dag::Vector<AnimCtrlData> &controllers, dag::Vector<String> &paths, ChildsDialog &dialog);
  bool onTreeContextMenu(PropPanel::ContainerPropertyControl &tree, int pcb_id, PropPanel::ITreeInterface &tree_interface) override;
  int onMenuItemClick(unsigned id) override;
  void setAsset(DagorAsset *cur_asset);

private:
  PropPanel::TLeafHandle selLeaf = nullptr;
  PropPanel::ContainerPropertyControl *ctrlsTree = nullptr;
  DagorAsset *asset = nullptr;
  ChildsDialog &dialog;
  dag::Vector<AnimCtrlData> &controllers;
  dag::Vector<String> &paths;
};
