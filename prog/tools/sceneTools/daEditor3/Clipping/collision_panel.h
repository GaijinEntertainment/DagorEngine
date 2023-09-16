#ifndef __GAIJIN_DAGORED_COLLISION_PANEL_H__
#define __GAIJIN_DAGORED_COLLISION_PANEL_H__
#pragma once

#include <propPanel2/comWnd/panel_window.h>

enum
{
  PROPBAR_EDITOR_WTYPE = 132,
};

class ClippingPlugin;
struct TpsPhysmatInfo;
struct CollisionBuildSettings;
struct E3DCOLOR;

class CollisionPropPanelClient : public ControlEventHandler
{
public:
  CollisionPropPanelClient(ClippingPlugin *_plg, CollisionBuildSettings &_stg, int &phys_eng_type);
  void setPanelParams();
  bool showPropPanel(bool show);

  bool isVisible() { return (mPanelWindow) ? true : false; }

  void setPanelWindow(CPanelWindow *panel) { mPanelWindow = panel; }
  CPanelWindow *getPanelWindow() const { return mPanelWindow; }

private:
  CPanelWindow *mPanelWindow;
  ClippingPlugin *plugin;
  CollisionBuildSettings &stg;
  int &curPhysEngType;

  // ControlEventHandler

  virtual void onChange(int pcb_id, PropPanel2 *panel);

  void setCollisionParams();
  virtual void fillPanel();
  void changeColor(int ind, E3DCOLOR new_color);
};

#endif
