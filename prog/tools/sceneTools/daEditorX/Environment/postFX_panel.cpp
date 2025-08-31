// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "postFX_panel.h"

#include <oldEditor/de_interface.h>
#include <de3_dynRenderService.h>
#include <osApiWrappers/dag_direct.h>
#include <EditorCore/ec_IEditorCore.h>
#include <EditorCore/ec_wndGlobal.h>

#include <propPanel/control/panelWindow.h>
#include <propPanel/c_indirect.h>
#include <libTools/util/strUtil.h>


EnvironmentPlugin::PostfxPanel::PostfxPanel(EnvironmentPlugin &plug, void *hwnd) : plugin(plug), scheme(NULL)
{
  propPanel = DAGORED2->createPropPanel(this, "PostFX settings");
  scheme = propPanel->createSceme();

  pluginPostfxBlk = plugin.isAcesPlugin ? &plugin.currentEnvironmentAces.postfxBlk : &plugin.postfxBlk;
}


EnvironmentPlugin::PostfxPanel::~PostfxPanel()
{
  del_it(scheme);
  DAGORED2->deleteCustomPanel(propPanel);
  propPanel = NULL;
}


void EnvironmentPlugin::PostfxPanel::getSchemeBlk(DataBlock &blk)
{
  blk.load(::make_full_path(sgg::get_exe_path_full(), "../commonData/postfx.blk"));
}


void EnvironmentPlugin::PostfxPanel::setGameBlk(const DataBlock &blk, bool save)
{
  if (save)
    pluginPostfxBlk->setFrom(&blk);

  EDITORCORE->queryEditorInterface<IDynRenderService>()->restartPostfx(*pluginPostfxBlk);

  DAGORED2->repaint();
}


void EnvironmentPlugin::PostfxPanel::fillPanel()
{
  DataBlock gameBlk;
  gameBlk.setFrom(pluginPostfxBlk);

  DataBlock schemeBlk;
  getSchemeBlk(schemeBlk);
  bool enable = gameBlk.getBlockByNameEx("hdr")->getBool("enabled", false);

  scheme->load(schemeBlk, true);
  propPanel->disableFillAutoResize();
  scheme->setParams(propPanel, gameBlk);
  propPanel->restoreFillAutoResize();

  // for (int i = 0; i < params.getParamsCount(); ++i)
  //   params.setEnableByInd(i, false);
}


void EnvironmentPlugin::PostfxPanel::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  DataBlock gameBlk;
  gameBlk.setFrom(pluginPostfxBlk);

  if (scheme->getParams(propPanel, gameBlk))
  {
    DataBlock *hdrBlk = gameBlk.getBlockByName("hdr");
    if (hdrBlk)
    {
      float bloomSourceMax = hdrBlk->getReal("bloomSourceMax", 1.f);
      if (bloomSourceMax <= 0.f)
        hdrBlk->setReal("bloomSourceMax", 1.f);

      int bloomScale = hdrBlk->getInt("bloomScale", 8);
      if (bloomScale < 4)
        bloomScale = 4;
      if (bloomScale > 4 && bloomScale < 8)
        bloomScale = 8;
      if (bloomScale > 8)
        bloomScale = 16;
      hdrBlk->setInt("bloomScale", bloomScale);

      DataBlock *realHdrBlk = hdrBlk->getBlockByName("real");
      if (realHdrBlk)
      {
        int bloomScale = realHdrBlk->getInt("bloomScale", 8);
        if (bloomScale < 4)
          bloomScale = 4;
        if (bloomScale > 4 && bloomScale < 8)
          bloomScale = 8;
        if (bloomScale > 8)
          bloomScale = 16;
        realHdrBlk->setInt("bloomScale", bloomScale);
      }

      DataBlock *fakeHdrBlk = hdrBlk->getBlockByName("fake");
      if (fakeHdrBlk)
      {
        int bloomScale = fakeHdrBlk->getInt("bloomScale", 8);
        if (bloomScale < 4)
          bloomScale = 4;
        if (bloomScale > 4 && bloomScale < 8)
          bloomScale = 8;
        if (bloomScale > 8)
          bloomScale = 16;
        fakeHdrBlk->setInt("bloomScale", bloomScale);
      }
    }

    setGameBlk(gameBlk, true);

    const DataBlock *demonBlk = gameBlk.getBlockByName("DemonPostFx");

    if (demonBlk && demonBlk->getBool("autosyncWithLightPlugin", false))
      plugin.onLightingSettingsChanged(true);
  }
}
