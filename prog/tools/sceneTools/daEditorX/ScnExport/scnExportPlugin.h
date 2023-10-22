#ifndef __GAIJIN_DYNAMIC_LIGHTING_PLUGIN__
#define __GAIJIN_DYNAMIC_LIGHTING_PLUGIN__
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
  ~ScnExportPlugin();

  virtual const char *getInternalName() const { return "scnExport"; }
  virtual const char *getMenuCommandName() const { return "ScnExport"; }
  virtual const char *getHelpUrl() const { return "/html/Plugins/ScnExport/index.htm"; }

  virtual int getRenderOrder() const { return -200; }
  virtual int getBuildOrder() const { return 0; }
  virtual bool showInTabs() const { return false; }

  virtual void registered()
  {
    isVisible = true;
    clearObjects();
  }
  virtual void unregistered() {}
  virtual void beforeMainLoop() {}

  virtual bool begin(int toolbar_id, unsigned menu_id);
  virtual bool end() { return true; }

  virtual void setVisible(bool vis)
  {
    isVisible = vis;
    if (bsSrv)
      bsSrv->setServiceVisible(vis);
  }
  virtual bool getVisible() const { return isVisible; }

  virtual bool getSelectionBox(BBox3 &) const { return false; }
  virtual bool getStatusBarPos(Point3 &pos) const { return false; }

  virtual void clearObjects();
  virtual void onNewProject() {}
  virtual void saveObjects(DataBlock &blk, DataBlock &local_data, const char *base_path);
  virtual void loadObjects(const DataBlock &blk, const DataBlock &local_data, const char *base_path);
  virtual bool acceptSaveLoad() const { return true; }

  virtual void selectAll() {}
  virtual void deselectAll() {}

  virtual void actObjects(float dt) {}
  virtual void beforeRenderObjects(IGenViewportWnd *vp) {}
  virtual void renderObjects() {}
  virtual void renderTransObjects() {}

  virtual void *queryInterfacePtr(unsigned huid);

  virtual bool catchEvent(unsigned event_huid, void *userData) { return bsSrv ? bsSrv->catchEvent(event_huid, userData) : false; }
  virtual bool onPluginMenuClick(unsigned id);

  // command handlers
  virtual IGenEventHandler *getEventHandler() { return NULL; }

  // IBinaryDataBuilder implemenatation
  virtual bool validateBuild(int target, ILogWriter &rep, PropPanel2 *params);
  virtual bool addUsedTextures(ITextureNumerator &tn);
  virtual bool buildAndWrite(BinDumpSaveCB &cwr, const ITextureNumerator &tn, PropPanel2 *pp);
  virtual bool useExportParameters() const { return true; }
  virtual void fillExportPanel(PropPanel2 &params);
  virtual bool checkMetrics(const DataBlock &metrics_blk) { return true; }

  // IRenderingService interface
  virtual void renderGeometry(Stage stage) { return bsRendGeom->renderGeometry(stage); }

  // IRenderOnCubeTex interface
  void prepareCubeTex(bool renderEnvi, bool renderLit, bool renderStreamLit)
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

#endif //__GAIJIN_DYNAMIC_LIGHTING_PLUGIN__
