// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "animCtrlData.h"
#include "../blendNodes/blendNodeData.h"
#include <propPanel/control/listBoxInterface.h>
#include <propPanel/control/menu.h>

namespace PropPanel
{
class ContainerPropertyControl;
class ControlEventHandler;
} // namespace PropPanel

class DagorAsset;

class CtrlListSettingsEventHandler : public PropPanel::IListBoxControlEventHandler, public PropPanel::IMenuEventHandler
{
public:
  CtrlListSettingsEventHandler(dag::Vector<AnimCtrlData> &controllers, dag::Vector<BlendNodeData> &nodes);
  bool onListBoxContextMenu(int pcb_id, PropPanel::IListBoxInterface &list_box_interface) override;
  int onMenuItemClick(unsigned id) override;
  void setPluginEventHandler(PropPanel::ContainerPropertyControl *panel, PropPanel::ControlEventHandler *event_handler);
  void editSelectedNode();

private:
  dag::Vector<AnimCtrlData> &controllers;
  dag::Vector<BlendNodeData> &nodes;

  PropPanel::ContainerPropertyControl *pluginPanel = nullptr;
  PropPanel::ControlEventHandler *pluginEventHandler = nullptr;
};
