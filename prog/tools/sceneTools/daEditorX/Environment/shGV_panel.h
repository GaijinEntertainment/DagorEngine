#ifndef __GAIJIN_DAGORED_SHGV_PANEL_H__
#define __GAIJIN_DAGORED_SHGV_PANEL_H__
#pragma once


#include "environmentPlugin.h"

#include <propPanel2/c_panel_base.h>

class CPanelWindow;


class EnvironmentPlugin::ShaderGlobVarsPanel : public ControlEventHandler
{
public:
  ShaderGlobVarsPanel(EnvironmentPlugin &plugin, void *hwnd);
  ~ShaderGlobVarsPanel();

  CPanelWindow *getPanel()
  {
    G_ASSERT(propPanel);
    return propPanel;
  }

  virtual void fillPanel();

private:
  void setGameBlk(const DataBlock &blk);
  void getSchemeBlk(DataBlock &blk);

  // ControlEventHandler
  virtual void onChange(int pcb_id, PropPanel2 *panel);

  CPanelWindow *propPanel;
  EnvironmentPlugin &plugin;
  PropPanelScheme *scheme;

  DataBlock *pluginShgvBlk;
};

#endif
