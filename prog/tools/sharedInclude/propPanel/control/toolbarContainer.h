//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <propPanel/control/container.h>

namespace PropPanel
{

class ToolbarContainerPropertyControl : public ContainerPropertyControl
{
public:
  ToolbarContainerPropertyControl(int id, ControlEventHandler *event_handler, ContainerPropertyControl *parent, int x = 0, int y = 0,
    hdpi::Px w = hdpi::Px(0), hdpi::Px h = hdpi::Px(0));

  ContainerPropertyControl *createToolbarPanel(int id = 0, bool use_tight_button_placement = false, bool new_line = true) override;

  void setToolbarControlWidth(int width);

private:
  int toolbarControlWidth;
};

} // namespace PropPanel