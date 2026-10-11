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

class AnimTreeEditor;

class IfMathDropHandler : public PropPanel::IDropTargetHandler
{
public:
  void setEditor(AnimTreeEditor *editor_arg, PropPanel::ContainerPropertyControl *plugin_panel);
  PropPanel::DragAndDropResult handleDropTarget(int target_pid, PropPanel::ContainerPropertyControl *panel) override;

private:
  AnimTreeEditor *editor = nullptr;
  PropPanel::ContainerPropertyControl *pluginPanel = nullptr;
};
