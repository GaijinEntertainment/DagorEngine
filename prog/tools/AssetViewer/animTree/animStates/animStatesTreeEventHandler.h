// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "animStatesData.h"
#include "../childsDialog.h"
#include "../controllers/animCtrlData.h"
#include <propPanel/control/menu.h>
#include <propPanel/control/treeInterface.h>
#include <propPanel/c_control_event_handler.h>

class DagorAsset;

class AnimStatesTreeEventHandler : public PropPanel::ITreeControlEventHandler, public PropPanel::IMenuEventHandler
{
public:
  AnimStatesTreeEventHandler(dag::Vector<AnimStatesData> &states, dag::Vector<AnimCtrlData> &ctrls, dag::Vector<String> &paths,
    ChildsDialog &dialog);
  bool onTreeContextMenu(PropPanel::ContainerPropertyControl &tree, int pcb_id, PropPanel::ITreeInterface &tree_interface) override;
  int onMenuItemClick(unsigned id) override;
  void setAsset(DagorAsset *cur_asset);
  void setPluginEventHandler(PropPanel::ContainerPropertyControl *panel, PropPanel::ControlEventHandler *event_handler);
  String findFullPath();
  String findResolvedFolderPath();
  PropPanel::TLeafHandle findIncludeLeafByStateData();

private:
  PropPanel::TLeafHandle selLeaf = nullptr;
  PropPanel::ContainerPropertyControl *statesTree = nullptr;
  PropPanel::ContainerPropertyControl *ctrlsTree = nullptr;
  DagorAsset *asset = nullptr;
  const AnimStatesData *selData = nullptr;
  dag::Vector<AnimStatesData> &states;
  dag::Vector<AnimCtrlData> &ctrls;
  dag::Vector<String> &paths;
  ChildsDialog &dialog;

  PropPanel::ContainerPropertyControl *pluginPanel = nullptr;
  PropPanel::ControlEventHandler *pluginEventHandler = nullptr;
};
