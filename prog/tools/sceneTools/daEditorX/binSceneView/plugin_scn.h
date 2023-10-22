#pragma once

#include <EditorCore/ec_interface.h>
#include <oldEditor/de_interface.h>
#include <oldEditor/de_common_interface.h>
#include <oldEditor/de_clipping.h>
#include <de3_levelBinLoad.h>
#include <ioSys/dag_dataBlock.h>

//==============================================================================

class AcesScene;

class BinSceneViewPlugin : public IGenEditorPlugin,
                           public IRenderingService,
                           public IPluginAutoSave,
                           public ControlEventHandler,
                           public ILevelBinLoader,
                           public IEnvironmentSettings,
                           public IDagorEdCustomCollider
{
public:
  BinSceneViewPlugin();
  ~BinSceneViewPlugin();

  // IGenEditorPlugin
  virtual const char *getInternalName() const { return "bin_scene_view"; }
  virtual const char *getMenuCommandName() const { return "Scene view"; }
  virtual const char *getHelpUrl() const { return NULL; }

  virtual int getRenderOrder() const { return 102; }
  virtual int getBuildOrder() const { return 1; }

  virtual bool showInTabs() const { return streamingScene != NULL; }
  virtual bool showSelectAll() const { return true; }

  virtual bool acceptSaveLoad() const { return true; }

  virtual void registered();
  virtual void unregistered();
  virtual void beforeMainLoop() {}

  virtual bool begin(int toolbar_id, unsigned menu_id);
  virtual bool end();
  virtual void onNewProject() {}
  virtual IGenEventHandler *getEventHandler() { return NULL; }

  virtual void setVisible(bool vis);
  virtual bool getVisible() const { return isVisible; }
  virtual bool getSelectionBox(BBox3 &box) const { return false; }
  virtual bool getStatusBarPos(Point3 &pos) const { return false; }

  virtual void clearObjects();
  virtual void saveObjects(DataBlock &blk, DataBlock &local_data, const char *base_path);
  virtual void loadObjects(const DataBlock &blk, const DataBlock &local_data, const char *base_path);
  virtual void selectAll() {}
  virtual void deselectAll() {}

  virtual void actObjects(float dt);
  virtual void beforeRenderObjects(IGenViewportWnd *vp);
  virtual void renderObjects();
  virtual void renderTransObjects();

  virtual void *queryInterfacePtr(unsigned huid);

  virtual bool onPluginMenuClick(unsigned id);

  virtual bool catchEvent(unsigned ev_huid, void *userData);

  // IRenderingService
  virtual void renderGeometry(Stage stage);

  // ControlEventHandler
  virtual void onClick(int pcb_id, PropPanel2 *panel);

  // IPluginAutoSave
  virtual void autoSaveObjects(DataBlock &local_data);

  // ILevelBinLoader
  virtual void changeLevelBinary(const char *bin_fn);
  virtual const char *getLevelBinary() { return binFn; }

  // IDagorEdCustomCollider
  virtual bool traceRay(const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm);
  virtual bool shadowRayHitTest(const Point3 &p, const Point3 &dir, real maxt);
  virtual const char *getColliderName() const { return "Binary scene view plugin"; }
  virtual bool isColliderVisible() const { return getVisible(); }

  // IEnvironmentSettings.
  virtual void getEnvironmentSettings(DataBlock &) {}
  virtual void setEnvironmentSettings(DataBlock &blk);

  static bool prepareRequiredServices() { return true; }

  // SceneHolder *getScene() { return streamingScene; };
  AcesScene *getScene() { return streamingScene; };

private:
  bool isVisible;
  bool isDebugVisible;
  bool showSplines;
  bool showNavMesh;
  bool showFrt, showLrt, showStaticGeom;
  bool showSplinePoints;
  float maxPointVisDist;
  float maxFrtVisDist;
  bool showFrtWire;
  Tab<SimpleString> recentFn;
  String streamingFolder, streamingBlkPath, binFn;
  DataBlock strmBlk;
  AcesScene *streamingScene;
  int toolBarId;

  bool loadScene(const char *streaming_folder);
};

static BinSceneViewPlugin *plugin = NULL;