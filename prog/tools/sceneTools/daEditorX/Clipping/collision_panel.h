// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/panelWindow.h>

enum
{
  PROPBAR_EDITOR_WTYPE = 132,
};

class ClippingPlugin;
struct TpsPhysmatInfo;
struct CollisionBuildSettings;
struct E3DCOLOR;

class CollisionPropPanelClient : public PropPanel::ControlEventHandler
{
public:
  CollisionPropPanelClient(ClippingPlugin *_plg, CollisionBuildSettings &_stg, int &phys_eng_type);
  void setPanelParams();
  bool showPropPanel(bool show);

  bool isVisible() { return (mPanelWindow) ? true : false; }

  void setPanelWindow(PropPanel::PanelWindowPropertyControl *panel) { mPanelWindow = panel; }
  PropPanel::PanelWindowPropertyControl *getPanelWindow() const { return mPanelWindow; }

private:
  PropPanel::PanelWindowPropertyControl *mPanelWindow;
  ClippingPlugin *plugin;
  CollisionBuildSettings &stg;
  int &curPhysEngType;

  // ControlEventHandler

  virtual void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel);

  void setCollisionParams();
  virtual void fillPanel();
  void changeColor(int ind, E3DCOLOR new_color);
};
