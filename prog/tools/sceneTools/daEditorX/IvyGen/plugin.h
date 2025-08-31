// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_ObjectEditor.h>
#include "ivyObjectsEditor.h"

#include <oldEditor/de_interface.h>
#include <oldEditor/de_collision.h>


class IvyGenPlugin : public IGenEditorPlugin, public IGenEventHandlerWrapper, public IGatherStaticGeometry
{
public:
  static IvyGenPlugin *self;

  IvyGenPlugin();
  ~IvyGenPlugin() override;

  // IGenEditorPlugin
  const char *getInternalName() const override { return "ivyGen"; }
  const char *getMenuCommandName() const override { return "IvyGen"; }
  const char *getHelpUrl() const override { return "/html/Plugins/IvyGen/index.htm"; }

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

  void setVisible(bool vis) override { isVisible = vis; }
  bool getVisible() const override { return isVisible; }
  bool getSelectionBox(BBox3 &box) const override { return objEd.getSelectionBox(box); }
  bool getStatusBarPos(Point3 &pos) const override { return false; }

  void clearObjects() override;
  void saveObjects(DataBlock &blk, DataBlock &local_data, const char *base_path) override;
  void loadObjects(const DataBlock &blk, const DataBlock &local_data, const char *base_path) override;
  void selectAll() override { objEd.selectAll(); }
  void deselectAll() override { objEd.unselectAll(); }

  void actObjects(float dt) override;
  void beforeRenderObjects(IGenViewportWnd *vp) override;
  void renderObjects() override;
  void renderTransObjects() override;

  void *queryInterfacePtr(unsigned huid) override;

  bool onPluginMenuClick(unsigned id) override { return false; }
  void handleViewportAcceleratorCommand(unsigned id) override;
  void registerMenuAccelerators() override;

  void gatherStaticVisualGeometry(StaticGeometryContainer &cont) override { gatherStaticGeometry(cont, 0); }

  void gatherStaticCollisionGeomGame(StaticGeometryContainer &cont) override { gatherStaticGeometry(cont, 0); }

  void gatherStaticCollisionGeomEditor(StaticGeometryContainer &cont) override { gatherStaticGeometry(cont, 0); }

  void gatherStaticEnviGeometry(StaticGeometryContainer &container) override {}
  void gatherStaticGeometry(StaticGeometryContainer &cont, int flags) { objEd.gatherStaticGeometry(cont, flags); }

  // IGenEventHandler
  IGenEventHandler *getWrappedHandler() override { return &objEd; }

private:
  IvyObjectEditor objEd;
  bool isVisible, firstBegin;
};
