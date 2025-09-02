// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/panelWindow.h>

enum
{
  PROPBAR_EDITOR_WTYPE = 132,
};

class CollisionPlugin;
struct TpsPhysmatInfo;
struct CollisionBuildSettings;
struct E3DCOLOR;

class CollisionPropPanelClient : public PropPanel::ControlEventHandler
{
public:
  CollisionPropPanelClient(CollisionPlugin *_plg, CollisionBuildSettings &_stg, int &phys_eng_type);
  void setPanelParams();
  bool showPropPanel(bool show);

  bool isVisible() { return (mPanelWindow) ? true : false; }

  void setPanelWindow(PropPanel::PanelWindowPropertyControl *panel) { mPanelWindow = panel; }
  PropPanel::PanelWindowPropertyControl *getPanelWindow() const { return mPanelWindow; }

private:
  PropPanel::PanelWindowPropertyControl *mPanelWindow;
  CollisionPlugin *plugin;
  CollisionBuildSettings &stg;
  int &curPhysEngType;

  // ControlEventHandler

  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  void setCollisionParams();
  void fillPanel();
  void changeColor(int ind, E3DCOLOR new_color);
};
