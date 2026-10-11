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
} // namespace PropPanel

class DagorAsset;
class AnimTreeEditor;

class StateListSettingsEventHandler : public PropPanel::IListBoxControlEventHandler, public PropPanel::IMenuEventHandler
{
public:
  StateListSettingsEventHandler(dag::Vector<AnimStatesData> &states, dag::Vector<AnimCtrlData> &controllers,
    dag::Vector<BlendNodeData> &nodes, dag::Vector<String> &paths);
  bool onListBoxContextMenu(int pcb_id, PropPanel::IListBoxInterface &list_box_interface) override;
  int onMenuItemClick(unsigned id) override;
  void setEventHandler(PropPanel::ContainerPropertyControl *panel, DagorAsset *cur_asset, AnimTreeEditor *event_handler);
  void editSelectedNode();

private:
  dag::Vector<AnimStatesData> &states;
  dag::Vector<AnimCtrlData> &controllers;
  dag::Vector<BlendNodeData> &nodes;
  dag::Vector<String> &paths;
  DagorAsset *asset = nullptr;

  PropPanel::ContainerPropertyControl *pluginPanel = nullptr;
  AnimTreeEditor *eventHandler = nullptr;
};
