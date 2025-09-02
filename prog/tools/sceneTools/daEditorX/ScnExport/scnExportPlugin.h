// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <oldEditor/de_interface.h>
#include <oldEditor/de_common_interface.h>
#include <de3_interface.h>
#include <libTools/staticGeom/geomObject.h>
#include <sceneBuilder/nsb_StdTonemapper.h>

#include <coolConsole/coolConsole.h>
#include <EditorCore/ec_IGizmoObject.h>
#include <util/dag_oaHashNameMap.h>

class ScnExportPlugin;

class ScnExportPlugin : public IGenEditorPlugin, public IRenderingService, public IRenderOnCubeTex, public IBinaryDataBuilder
{
public:
  ScnExportPlugin();
  ~ScnExportPlugin() override;

  const char *getInternalName() const override { return "scnExport"; }
  const char *getMenuCommandName() const override { return "ScnExport"; }
  const char *getHelpUrl() const override { return "/html/Plugins/ScnExport/index.htm"; }

  int getRenderOrder() const override { return -200; }
  int getBuildOrder() const override { return 0; }
  bool showInTabs() const override { return false; }

  void registered() override
  {
    isVisible = true;
    clearObjects();
  }
  void unregistered() override {}
  void beforeMainLoop() override {}

  bool begin(int toolbar_id, unsigned menu_id) override;
  bool end() override { return true; }

  void setVisible(bool vis) override
  {
    isVisible = vis;
    if (bsSrv)
      bsSrv->setServiceVisible(vis);
  }
  bool getVisible() const override { return isVisible; }

  bool getSelectionBox(BBox3 &) const override { return false; }
  bool getStatusBarPos(Point3 &pos) const override { return false; }

  void clearObjects() override;
  void onNewProject() override {}
  void saveObjects(DataBlock &blk, DataBlock &local_data, const char *base_path) override;
  void loadObjects(const DataBlock &blk, const DataBlock &local_data, const char *base_path) override;
  bool acceptSaveLoad() const override { return true; }

  void selectAll() override {}
  void deselectAll() override {}

  void actObjects(float dt) override {}
  void beforeRenderObjects(IGenViewportWnd *vp) override {}
  void renderObjects() override {}
  void renderTransObjects() override {}

  void *queryInterfacePtr(unsigned huid) override;

  bool catchEvent(unsigned event_huid, void *userData) override { return bsSrv ? bsSrv->catchEvent(event_huid, userData) : false; }
  bool onPluginMenuClick(unsigned id) override;

  // command handlers
  IGenEventHandler *getEventHandler() override { return NULL; }

  // IBinaryDataBuilder implemenatation
  bool validateBuild(int target, ILogWriter &rep, PropPanel::ContainerPropertyControl *params) override;
  bool addUsedTextures(ITextureNumerator &tn) override;
  bool buildAndWrite(BinDumpSaveCB &cwr, const ITextureNumerator &tn, PropPanel::ContainerPropertyControl *pp) override;
  bool useExportParameters() const override { return true; }
  void fillExportPanel(PropPanel::ContainerPropertyControl &params) override;
  bool checkMetrics(const DataBlock &metrics_blk) override { return true; }

  // IRenderingService interface
  void renderGeometry(Stage stage) override { return bsRendGeom->renderGeometry(stage); }

  // IRenderOnCubeTex interface
  void prepareCubeTex(bool renderEnvi, bool renderLit, bool renderStreamLit) override
  {
    bsRendCube->prepareCubeTex(renderEnvi, renderLit, renderStreamLit);
  }

private:
  bool buildLmScene(Tab<int> &plugs, unsigned target);
  bool exportLms(const char *fname, CoolConsole &con, Tab<int> &plugs);

  bool isVisible;
  IEditorService *bsSrv;
  IRenderingService *bsRendGeom;
  IRenderOnCubeTex *bsRendCube;
  StaticSceneBuilder::Splitter splitter;

  FastNameMap disabledGamePlugins;
};

extern bool check_geom_provider(IGenEditorPlugin *p);
extern int count_geom_provider();
