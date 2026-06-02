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

  ContainerPropertyControl *createToolbarPanel(int id = 0, bool use_tight_button_placement = false, bool new_line = true,
    int toolbar_control_width = Constants::TOOLBAR_DEFAULT_CONTROL_WIDTH) override;

  void setToolbarScalePercent(int scale_percent);
  int getToolbarScalePercent() const { return toolbarScalePercent; }

  void updateImgui() override;

private:
  int toolbarScalePercent = 100;
};

} // namespace PropPanel