#ifndef __GAIJIN_HEIGHTMAPLAND_PANEL__
#define __GAIJIN_HEIGHTMAPLAND_PANEL__
#pragma once


#include "hmlPlugin.h"

#include <propPanel2/comWnd/panel_window.h>

class HmapLandPlugin::HmapLandPanel : public ControlEventHandler
{
public:
  HmapLandPanel(HmapLandPlugin &p);

  void fillPanel(bool refill = true, bool schedule_regen = false, bool hold_pos = false);
  void updateLightGroup();
  bool showPropPanel(bool show);

  bool isVisible() { return (mPanelWindow) ? true : false; }

  void setPanelWindow(CPanelWindow *panel) { mPanelWindow = panel; }
  CPanelWindow *getPanelWindow() const { return mPanelWindow; }

  // ControlEventHandler

  virtual void onClick(int pcb_id, PropPanel2 *panel);
  virtual void onChange(int pcb_id, PropPanel2 *panel);
  virtual void onPostEvent(int pcb_id, PropPanel2 *panel);

private:
  HmapLandPlugin &plugin;
  CPanelWindow *mPanelWindow;
};


#endif //__GAIJIN_GRASS_PANEL__
