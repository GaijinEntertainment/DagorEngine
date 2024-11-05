// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "environmentPlugin.h"

#include <propPanel/control/container.h>

namespace PropPanel
{
class PanelWindowPropertyControl;
}


class EnvironmentPlugin::ShaderGlobVarsPanel : public PropPanel::ControlEventHandler
{
public:
  ShaderGlobVarsPanel(EnvironmentPlugin &plugin, void *hwnd);
  ~ShaderGlobVarsPanel();

  PropPanel::PanelWindowPropertyControl *getPanel()
  {
    G_ASSERT(propPanel);
    return propPanel;
  }

  virtual void fillPanel();

private:
  void setGameBlk(const DataBlock &blk);
  void getSchemeBlk(DataBlock &blk);

  // ControlEventHandler
  virtual void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel);

  PropPanel::PanelWindowPropertyControl *propPanel;
  EnvironmentPlugin &plugin;
  PropPanel::PropPanelScheme *scheme;

  DataBlock *pluginShgvBlk;
};
