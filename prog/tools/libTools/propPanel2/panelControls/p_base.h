// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#pragma once

#include "../windowControls/w_simple_controls.h"
#include <propPanel2/c_panel_base.h>

class BasicPropertyControl : public PropertyControlBase
{
public:
  BasicPropertyControl(int id, ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int x, int y, hdpi::Px w,
    hdpi::Px h);
  virtual ~BasicPropertyControl();
  void setTooltip(const char text[]);

protected:
  void initTooltip(WindowBase *window);

private:
  Tab<WTooltip *> tooltips;
};
