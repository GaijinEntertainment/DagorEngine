// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <propPanel/control/toolbarContainer.h>
#include "toolbar.h"

namespace PropPanel
{

ToolbarContainerPropertyControl::ToolbarContainerPropertyControl(int id, ControlEventHandler *event_handler,
  ContainerPropertyControl *parent, int x, int y, hdpi::Px w, hdpi::Px h) :
  ContainerPropertyControl(id, event_handler, parent, x, y, w, h), toolbarControlWidth(Constants::TOOLBAR_DEFAULT_CONTROL_WIDTH)
{}

ContainerPropertyControl *ToolbarContainerPropertyControl::createToolbarPanel(int id, const char caption[], bool new_line)
{
  ToolbarPropertyControl *newControl = new ToolbarPropertyControl(mEventHandler, this, id, caption);
  newControl->setToolbarControlWidth(toolbarControlWidth);
  addControl(newControl, new_line);
  return newControl;
}

void ToolbarContainerPropertyControl::setToolbarControlWidth(int width)
{
  toolbarControlWidth = width;

  for (PropertyControlBase *control : mControlArray)
    if (control->getImguiControlType() == (int)ControlType::Toolbar)
      static_cast<ToolbarPropertyControl *>(control)->setToolbarControlWidth(width);
}

} // namespace PropPanel