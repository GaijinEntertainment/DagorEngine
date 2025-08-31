// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "ecsObjectEditor.h"

#include <EditorCore/ec_interface_ex.h>
#include <oldEditor/de_interface.h>
#include <de3_baseInterfaces.h>

#include <EASTL/unique_ptr.h>

class ECSEditorPlugin : public IGenEditorPlugin, public IGenEventHandlerWrapper, public IOnExportNotify
{
public:
  // IGenEditorPlugin
  const char *getInternalName() const override { return "ecsEditor"; }
  const char *getMenuCommandName() const override { return "ECS Editor"; }
  const char *getHelpUrl() const override { return "/html/Plugins/ECSEditor/index.htm"; }

  int getRenderOrder() const override { return 300; }
  int getBuildOrder() const override { return 0; }

  bool showInTabs() const override { return true; }
  bool showSelectAll() const override { return true; }

  bool acceptSaveLoad() const override { return true; }

  void registered() override {}
  void unregistered() override {}
  void loadSettings(const DataBlock &global_settings, const DataBlock &per_app_settings) override;
  void saveSettings(DataBlock &global_settings, DataBlock &per_app_settings) override;
  void beforeMainLoop() override {}

  bool begin(int toolbar_id, unsigned menu_id) override;
  bool end() override;
  void onNewProject() override {}
  IGenEventHandler *getEventHandler() override { return this; }

  void setVisible(bool vis) override;
  bool getVisible() const override { return isVisible; }
  bool getSelectionBox(BBox3 &box) const override { return objEd->getSelectionBox(box); }
  bool getStatusBarPos(Point3 &pos) const override { return false; }

  void clearObjects() override;
  void saveObjects(DataBlock &blk, DataBlock &local_data, const char *base_path) override;
  void loadObjects(const DataBlock &blk, const DataBlock &local_data, const char *base_path) override;
  void selectAll() override { objEd->selectAll(); }
  void deselectAll() override { objEd->unselectAll(); }

  void actObjects(float dt) override;
  void beforeRenderObjects(IGenViewportWnd *vp) override;
  void renderObjects() override;
  void renderTransObjects() override;
  void handleViewportPaint(IGenViewportWnd *wnd) override;
  void updateImgui() override;

  void *queryInterfacePtr(unsigned huid) override;

  bool onPluginMenuClick(unsigned id) override;
  void handleViewportAcceleratorCommand(unsigned id) override;
  void registerMenuAccelerators() override;

  // IGenEventHandler
  IGenEventHandler *getWrappedHandler() override { return objEd.get(); }

  // IOnExportNotify interface
  void onBeforeExport(unsigned target_code) override;
  void onAfterExport(unsigned target_code) override;

  ECSObjectEditor *getObjectEditor() const { return objEd.get(); }

  void setViewportMessage(const char *message, bool auto_hide);

private:
  String getDefaultSceneBlkPath() const;
  String getCurrentSceneBlkPath() const;
  int getRecentFileIndex(const char *path) const;
  void addToRecentList(const char *path);
  void updateMenu();
  void loadSceneManually(const char *custom_path = nullptr);

  static constexpr float VIEWPORT_MESSAGE_AUTO_HIDE_DELAY_IN_SECONDS = 5.0f;

  eastl::unique_ptr<ECSObjectEditor> objEd;
  String customSceneBlkPath;
  dag::Vector<String> recentCustomPaths;
  int menuId = 0;
  bool isVisible = false;

  String viewportMessage;
  float viewportMessageLifeTimeInSeconds = -1.0f;
};
