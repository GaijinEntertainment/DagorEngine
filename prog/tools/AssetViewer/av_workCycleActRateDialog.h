// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/container.h>
#include <workCycle/dag_workCycle.h>

class WorkCycleActRateDialog : public PropPanel::DialogWindow
{
public:
  WorkCycleActRateDialog() : PropPanel::DialogWindow(nullptr, hdpi::_pxScaled(250), hdpi::_pxScaled(100), "Set work cycle act rate") {}

  void fill()
  {
    getPanel()->createEditInt(CONTROL_ID, "Acts per second:", ::dagor_game_act_rate);
    getPanel()->setMinMaxStep(CONTROL_ID, 1.0f, 1000.0f, 1.0f);
  }

private:
  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override
  {
    G_UNUSED(panel);
    if (pcb_id == CONTROL_ID)
      ::dagor_set_game_act_rate(getPanel()->getInt(CONTROL_ID));
  }

  static const int CONTROL_ID = 1;
};
