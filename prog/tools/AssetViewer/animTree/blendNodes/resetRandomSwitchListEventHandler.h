// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../controllers/animCtrlData.h"
#include "../controllers/ctrlType.h"
#include <propPanel/control/listBoxInterface.h>
#include <propPanel/control/menu.h>
#include <propPanel/c_common.h>

namespace PropPanel
{
class ContainerPropertyControl;
class ControlEventHandler;
} // namespace PropPanel

class ResetRandomSwitchListEventHandler : public PropPanel::IListBoxControlEventHandler, public PropPanel::IMenuEventHandler
{
public:
  ResetRandomSwitchListEventHandler(dag::Vector<AnimCtrlData> &controllers);
  bool onListBoxContextMenu(int pcb_id, PropPanel::IListBoxInterface &list_box_interface) override;
  int onMenuItemClick(unsigned id) override;
  void setPluginEventHandler(PropPanel::ContainerPropertyControl *panel, PropPanel::ControlEventHandler *event_handler);
  void editSelectedNode();

private:
  dag::Vector<AnimCtrlData> &controllers;
  PropPanel::ContainerPropertyControl *pluginPanel = nullptr;
  PropPanel::ControlEventHandler *pluginEventHandler = nullptr;
};
