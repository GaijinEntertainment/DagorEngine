// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <oldEditor/de_interface.h>
#include <oldEditor/de_collision.h>
#include <de3_baseInterfaces.h>
#include <oldEditor/de_common_interface.h>
#include "objEd_csg.h"
#include <libTools/dagFileRW/dagFileNode.h>

class CSGPlugin : public IGenEditorPlugin, public IGenEventHandlerWrapper, public IGatherStaticGeometry, public IPostProcessGeometry
{
public:
  static CSGPlugin *self;
  ObjEd objEd;

  CSGPlugin();
  ~CSGPlugin() override;

  // IGenEventHandler
  IGenEventHandler *getWrappedHandler() override { return &objEd; }

  // IGenEditorPlugin
  const char *getInternalName() const override { return "csg"; }
  const char *getMenuCommandName() const override { return "CSG"; }
  const char *getHelpUrl() const override { return "/html/Plugins/CSG/index.htm"; }

  void processGeometry(StaticGeometryContainer &container) override;

  int getRenderOrder() const override { return 101; }
  int getBuildOrder() const override { return 0; }

  bool showInTabs() const override { return true; }
  bool showSelectAll() const override { return true; }

  bool acceptSaveLoad() const override { return true; }

  void registered() override {}
  void unregistered() override {}
  void beforeMainLoop() override {}

  bool begin(int toolbar_id, unsigned menu_id) override;
  bool end() override;
  void onNewProject() override {}
  IGenEventHandler *getEventHandler() override { return this; }

  void setVisible(bool vis) override;
  bool getVisible() const override;
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
  void handleViewportAcceleratorCommand(unsigned id) override;
  void registerMenuAccelerators() override;

  void gatherStaticVisualGeometry(StaticGeometryContainer &cont) override {}

  void gatherStaticCollisionGeomGame(StaticGeometryContainer &cont) override;

  void gatherStaticCollisionGeomEditor(StaticGeometryContainer &cont) override;

  void gatherStaticEnviGeometry(StaticGeometryContainer &container) override {}

private:
  bool isVisible;
};
