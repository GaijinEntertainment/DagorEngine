#ifndef __GAIJIN_HML_PLUGIN_SEL_TEX_DLG__
#define __GAIJIN_HML_PLUGIN_SEL_TEX_DLG__
#pragma once


#include "hmlPlugin.h"
#include <propPanel2/comWnd/dialog_window.h>


class HmapLandPlugin::HmlSelTexDlg : public ControlEventHandler
{
public:
  HmlSelTexDlg(HmapLandPlugin &plug, const char *selected, int bpp);

  bool execute();

  const char *getSetTex() const { return selTex; }

  virtual void onChange(int pcb_id, PropertyContainerControlBase *panel);
  virtual void onClick(int pcb_id, PropertyContainerControlBase *panel);
  virtual void onDoubleClick(int pcb_id, PropertyContainerControlBase *panel);

private:
  HmapLandPlugin &plugin;
  Tab<String> texNames1, texNames8;
  Tab<String> *texNamesCur;
  String selTex;
  int reqBpp;

  void rebuildTexList(const char *sel);
};


#endif //__GAIJIN_HML_PLUGIN_SEL_TEX_DLG__
