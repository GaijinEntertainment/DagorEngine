// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/listBoxInterface.h>
#include <propPanel/control/menu.h>

namespace PropPanel
{
class ContainerPropertyControl;
} // namespace PropPanel

class AnimTreeEditor;

class NodeListEventHandler : public PropPanel::IListBoxControlEventHandler, public PropPanel::IMenuEventHandler
{
public:
  int onMenuItemClick(unsigned id) override;
  void setEventHandler(PropPanel::ContainerPropertyControl *panel, AnimTreeEditor *event_handler);

protected:
  void createEditMenu(PropPanel::IListBoxInterface &list_box_interface);
  virtual void editSelectedNode() = 0;

  PropPanel::ContainerPropertyControl *pluginPanel = nullptr;
  AnimTreeEditor *eventHandler = nullptr;
};
