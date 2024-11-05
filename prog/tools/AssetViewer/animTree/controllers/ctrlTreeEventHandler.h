// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "animCtrlData.h"
#include "ctrlChildsDialog.h"
#include <propPanel/control/treeInterface.h>
#include <sepGui/wndMenuInterface.h>

class DagorAsset;

class CtrlTreeEventHandler : public PropPanel::ITreeControlEventHandler, public IMenuEventHandler
{
public:
  CtrlTreeEventHandler(dag::Vector<AnimCtrlData> &controllers, dag::Vector<String> &paths, CtrlChildsDialog &dialog);
  virtual bool onTreeContextMenu(PropPanel::ContainerPropertyControl &tree, int pcb_id,
    PropPanel::ITreeInterface &tree_interface) override;
  virtual int onMenuItemClick(unsigned id) override;
  void setAsset(DagorAsset *cur_asset);

private:
  PropPanel::TLeafHandle selLeaf = nullptr;
  PropPanel::ContainerPropertyControl *ctrlsTree = nullptr;
  DagorAsset *asset = nullptr;
  CtrlChildsDialog &dialog;
  dag::Vector<AnimCtrlData> &controllers;
  dag::Vector<String> &paths;
};
