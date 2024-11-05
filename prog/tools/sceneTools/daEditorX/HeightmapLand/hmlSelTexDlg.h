// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "hmlPlugin.h"
#include <propPanel/commonWindow/dialogWindow.h>


class HmapLandPlugin::HmlSelTexDlg : public PropPanel::ControlEventHandler
{
public:
  HmlSelTexDlg(HmapLandPlugin &plug, const char *selected, int bpp);

  bool execute();

  const char *getSetTex() const { return selTex; }

  virtual void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel);
  virtual void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel);
  virtual void onDoubleClick(int pcb_id, PropPanel::ContainerPropertyControl *panel);

private:
  HmapLandPlugin &plugin;
  Tab<String> texNames1, texNames8;
  Tab<String> *texNamesCur;
  String selTex;
  int reqBpp;

  void rebuildTexList(const char *sel);
};
