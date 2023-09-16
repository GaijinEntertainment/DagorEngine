#ifndef __GAIJIN_DAGORED_POSTFX_PANEL_H__
#define __GAIJIN_DAGORED_POSTFX_PANEL_H__
#pragma once


#include "environmentPlugin.h"

#include <propPanel2/c_panel_base.h>

class CPanelWindow;


class EnvironmentPlugin::PostfxPanel : public ControlEventHandler
{
public:
  PostfxPanel(EnvironmentPlugin &plugin, void *hwnd);
  ~PostfxPanel();

  CPanelWindow *getPanel()
  {
    G_ASSERT(propPanel);
    return propPanel;
  }

  virtual void fillPanel();

private:
  void setGameBlk(const DataBlock &blk, bool save);
  void getSchemeBlk(DataBlock &blk);

  // ControlEventHandler
  virtual void onChange(int pcb_id, PropPanel2 *panel);

  CPanelWindow *propPanel;
  EnvironmentPlugin &plugin;
  PropPanelScheme *scheme;

  DataBlock *pluginPostfxBlk;
};

#endif
