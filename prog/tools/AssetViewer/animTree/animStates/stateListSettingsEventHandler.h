// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "animStatesData.h"
#include "../controllers/animCtrlData.h"
#include "../blendNodes/blendNodeData.h"
#include <propPanel/control/listBoxInterface.h>
#include <propPanel/control/menu.h>

namespace PropPanel
{
class ContainerPropertyControl;
class ControlEventHandler;
} // namespace PropPanel

class DagorAsset;

class StateListSettingsEventHandler : public PropPanel::IListBoxControlEventHandler, public PropPanel::IMenuEventHandler
{
public:
  StateListSettingsEventHandler(dag::Vector<AnimStatesData> &states, dag::Vector<AnimCtrlData> &controllers,
    dag::Vector<BlendNodeData> &nodes, dag::Vector<String> &paths);
  bool onListBoxContextMenu(int pcb_id, PropPanel::IListBoxInterface &list_box_interface) override;
  int onMenuItemClick(unsigned id) override;
  void setPluginEventHandler(PropPanel::ContainerPropertyControl *panel, DagorAsset *cur_asset,
    PropPanel::ControlEventHandler *event_handler);
  void editSelectedNode();

private:
  dag::Vector<AnimStatesData> &states;
  dag::Vector<AnimCtrlData> &controllers;
  dag::Vector<BlendNodeData> &nodes;
  dag::Vector<String> &paths;
  DagorAsset *asset = nullptr;

  PropPanel::ContainerPropertyControl *pluginPanel = nullptr;
  PropPanel::ControlEventHandler *pluginEventHandler = nullptr;
};
