// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <oldEditor/de_interface.h>
#include <oldEditor/de_common_interface.h>
#include <ioSys/dag_dataBlock.h>
#include "decl.h"


class ssviewplugin::Plugin : public IGenEditorPlugin, public IRenderingService
{
public:
  static Plugin *self;

  static bool prepareRequiredServices();

public:
  Plugin();
  ~Plugin() override;

  // IGenEditorPlugin
  const char *getInternalName() const override { return "ssview"; }
  const char *getMenuCommandName() const override { return "Streaming Scene View"; }
  const char *getHelpUrl() const override { return NULL; }

  int getRenderOrder() const override { return 101; }
  int getBuildOrder() const override { return 0; }

  bool showInTabs() const override { return false; }
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

  bool onPluginMenuClick(unsigned id) override { return false; }

  // IRenderingService
  void renderGeometry(Stage stage) override;

private:
  bool isVisible;
  bool isDebugVisible;
  String streamingFolder, streamingBlkPath;
  DataBlock strmBlk;
  SceneHolder *streamingScene;

  bool loadScene(const char *streaming_folder);
};
