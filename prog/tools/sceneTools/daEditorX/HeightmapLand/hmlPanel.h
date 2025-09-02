// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "hmlPlugin.h"

namespace PropPanel
{
class PanelWindowPropertyControl;
}

class HmapLandPlugin::HmapLandPanel : public PropPanel::ControlEventHandler
{
public:
  HmapLandPanel(HmapLandPlugin &p);

  void fillPanel(bool refill = true, bool schedule_regen = false, bool hold_pos = false);
  void updateLightGroup();
  bool showPropPanel(bool show);

  bool isVisible() { return (mPanelWindow) ? true : false; }

  void setPanelWindow(PropPanel::PanelWindowPropertyControl *panel) { mPanelWindow = panel; }
  PropPanel::PanelWindowPropertyControl *getPanelWindow() const { return mPanelWindow; }

  // ControlEventHandler

  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  void onPostEvent(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

private:
  HmapLandPlugin &plugin;
  PropPanel::PanelWindowPropertyControl *mPanelWindow;
};
