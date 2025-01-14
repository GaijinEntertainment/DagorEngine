// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "animStatesData.h"
#include <propPanel/control/treeInterface.h>
#include <propPanel/c_control_event_handler.h>
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
  void setPluginEventHandler(PropPanel::ContainerPropertyControl *panel, PropPanel::ControlEventHandler *event_handler);

private:
  PropPanel::TLeafHandle selLeaf = nullptr;
  PropPanel::ContainerPropertyControl *statesTree = nullptr;
  DagorAsset *asset = nullptr;
  dag::Vector<AnimStatesData> &states;
  dag::Vector<String> &paths;

  PropPanel::ContainerPropertyControl *pluginPanel = nullptr;
  PropPanel::ControlEventHandler *pluginEventHandler = nullptr;
};
