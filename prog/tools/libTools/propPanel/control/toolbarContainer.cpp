// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <propPanel/control/toolbarContainer.h>
#include "toolbar.h"

namespace PropPanel
{

ToolbarContainerPropertyControl::ToolbarContainerPropertyControl(int id, ControlEventHandler *event_handler,
  ContainerPropertyControl *parent, int x, int y, hdpi::Px w, hdpi::Px h) :
  ContainerPropertyControl(id, event_handler, parent, x, y, w, h)
{}

ContainerPropertyControl *ToolbarContainerPropertyControl::createToolbarPanel(int id, bool use_tight_button_placement, bool new_line,
  int toolbar_control_width)
{
  ToolbarPropertyControl *newControl =
    new ToolbarPropertyControl(mEventHandler, this, id, use_tight_button_placement, toolbar_control_width);
  newControl->setToolbarScalePercent(toolbarScalePercent);
  addControl(newControl, new_line);
  return newControl;
}

void ToolbarContainerPropertyControl::setToolbarScalePercent(int scale_percent)
{
  toolbarScalePercent = scale_percent;

  for (PropertyControlBase *control : mControlArray)
    if (control->getImguiControlType() == (int)ControlType::Toolbar)
      static_cast<ToolbarPropertyControl *>(control)->setToolbarScalePercent(scale_percent);
}

void ToolbarContainerPropertyControl::updateImgui()
{
  ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * toolbarScalePercent * 0.01f);
  ContainerPropertyControl::updateImgui();
  ImGui::PopFont();
}

} // namespace PropPanel