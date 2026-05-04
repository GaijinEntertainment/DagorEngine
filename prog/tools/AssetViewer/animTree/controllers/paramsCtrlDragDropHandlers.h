// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/dragAndDropHandler.h>

namespace PropPanel
{
class ContainerPropertyControl;
}

class IfMathDragHandler : public PropPanel::IDragSourceHandler
{
public:
  void setPanel(PropPanel::ContainerPropertyControl *panel);
  void onBeginDrag(int pcb_id) override;
  ImGuiDragDropFlags getDragDropFlags() override;

private:
  PropPanel::ContainerPropertyControl *pluginPanel = nullptr;
};

class AnimTreePlugin;

class IfMathDropHandler : public PropPanel::IDropTargetHandler
{
public:
  void setPlugin(AnimTreePlugin *plugin_arg, PropPanel::ContainerPropertyControl *plugin_panel);
  PropPanel::DragAndDropResult handleDropTarget(int target_pid, PropPanel::ContainerPropertyControl *panel) override;

private:
  AnimTreePlugin *plugin = nullptr;
  PropPanel::ContainerPropertyControl *pluginPanel = nullptr;
};
