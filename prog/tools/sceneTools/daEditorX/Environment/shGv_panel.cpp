// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shGV_panel.h"

#include <oldEditor/de_interface.h>
#include <oldEditor/de_workspace.h>

#include <libTools/util/strUtil.h>

#include <osApiWrappers/dag_direct.h>

#include <propPanel/control/panelWindow.h>
#include <propPanel/c_indirect.h>


EnvironmentPlugin::ShaderGlobVarsPanel::ShaderGlobVarsPanel(EnvironmentPlugin &plug, void *hwnd) : plugin(plug)
{
  propPanel = DAGORED2->createPropPanel(this, "Shader Global Vars");
  scheme = propPanel->createSceme();

  pluginShgvBlk = plugin.isAcesPlugin ? &plugin.currentEnvironmentAces.shaderVarsBlk : &plugin.shgvBlk;
}


EnvironmentPlugin::ShaderGlobVarsPanel::~ShaderGlobVarsPanel()
{
  del_it(scheme);
  DAGORED2->deleteCustomPanel(propPanel);
  propPanel = NULL;
}


void EnvironmentPlugin::ShaderGlobVarsPanel::getSchemeBlk(DataBlock &blk)
{
  //== TO REFACTOR:
  const DeWorkspace &wsp = DAGORED2->getWorkspace();
  blk.setFrom(&wsp.getShGlobVarsScheme());
}


void EnvironmentPlugin::ShaderGlobVarsPanel::setGameBlk(const DataBlock &blk)
{
  ShaderGlobal::set_vars_from_blk(blk, true);

  pluginShgvBlk->setFrom(&blk);

  DAGORED2->repaint();
}


void EnvironmentPlugin::ShaderGlobVarsPanel::fillPanel()
{
  DataBlock gameBlk;
  gameBlk.setFrom(pluginShgvBlk);

  DataBlock schemeBlk;
  getSchemeBlk(schemeBlk);

  scheme->load(schemeBlk, true);
  scheme->setParams(propPanel, gameBlk);

  // for (int i = 0; i < params.getParamsCount(); ++i)
  //   params.setEnableByInd(i, false);

  if (scheme->getParams(propPanel, gameBlk))
    setGameBlk(gameBlk);
}


void EnvironmentPlugin::ShaderGlobVarsPanel::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  DataBlock gameBlk;
  gameBlk.setFrom(pluginShgvBlk);

  if (scheme->getParams(propPanel, gameBlk))
    setGameBlk(gameBlk);
}
