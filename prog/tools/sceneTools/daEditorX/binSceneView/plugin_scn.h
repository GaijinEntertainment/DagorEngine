// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_interface.h>
#include <oldEditor/de_interface.h>
#include <oldEditor/de_common_interface.h>
#include <oldEditor/de_collision.h>
#include <de3_levelBinLoad.h>
#include <ioSys/dag_dataBlock.h>

//==============================================================================

class AcesScene;

class BinSceneViewPlugin : public IGenEditorPlugin,
                           public IRenderingService,
                           public IPluginAutoSave,
                           public PropPanel::ControlEventHandler,
                           public ILevelBinLoader,
                           public IEnvironmentSettings,
                           public IDagorEdCustomCollider
{
public:
  BinSceneViewPlugin();
  ~BinSceneViewPlugin() override;

  // IGenEditorPlugin
  const char *getInternalName() const override { return "bin_scene_view"; }
  const char *getMenuCommandName() const override { return "Scene view"; }
  const char *getHelpUrl() const override { return NULL; }

  int getRenderOrder() const override { return 102; }
  int getBuildOrder() const override { return 1; }

  bool showInTabs() const override { return streamingScene != NULL; }
  bool showSelectAll() const override { return true; }

  bool acceptSaveLoad() const override { return true; }

  void registered() override;
  void unregistered() override;
  void beforeMainLoop() override {}

  bool begin(int toolbar_id, unsigned menu_id) override;
  bool end() override;
  void onNewProject() override {}
  IGenEventHandler *getEventHandler() override { return NULL; }

  void setVisible(bool vis) override;
  bool getVisible() const override { return isVisible; }
  bool getSelectionBox(BBox3 &box) const override { return false; }
  bool getStatusBarPos(Point3 &pos) const override { return false; }

  void clearObjects() override;
  void saveObjects(DataBlock &blk, DataBlock &local_data, const char *base_path) override;
  void loadObjects(const DataBlock &blk, const DataBlock &local_data, const char *base_path) override;
  void selectAll() override {}
  void deselectAll() override {}

  void actObjects(float dt) override;
  void beforeRenderObjects(IGenViewportWnd *vp) override;
  void renderObjects() override;
  void renderTransObjects() override;

  void *queryInterfacePtr(unsigned huid) override;

  bool onPluginMenuClick(unsigned id) override;

  bool catchEvent(unsigned ev_huid, void *userData) override;

  // IRenderingService
  void renderGeometry(Stage stage) override;

  // ControlEventHandler
  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  // IPluginAutoSave
  void autoSaveObjects(DataBlock &local_data) override;

  // ILevelBinLoader
  void changeLevelBinary(const char *bin_fn) override;
  const char *getLevelBinary() override { return binFn; }

  // IDagorEdCustomCollider
  bool traceRay(const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm) override;
  bool shadowRayHitTest(const Point3 &p, const Point3 &dir, real maxt) override;
  const char *getColliderName() const override { return "Binary scene view plugin"; }
  bool isColliderVisible() const override { return getVisible(); }

  // IEnvironmentSettings.
  void getEnvironmentSettings(DataBlock &) override {}
  void setEnvironmentSettings(DataBlock &blk) override;

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