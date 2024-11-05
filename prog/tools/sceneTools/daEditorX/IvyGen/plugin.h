// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_ObjectEditor.h>
#include "ivyObjectsEditor.h"

#include <oldEditor/de_interface.h>
#include <oldEditor/de_clipping.h>


class IvyGenPlugin : public IGenEditorPlugin, public IGenEventHandlerWrapper, public IGatherStaticGeometry
{
public:
  static IvyGenPlugin *self;

  IvyGenPlugin();
  ~IvyGenPlugin();

  // IGenEditorPlugin
  virtual const char *getInternalName() const { return "ivyGen"; }
  virtual const char *getMenuCommandName() const { return "IvyGen"; }
  virtual const char *getHelpUrl() const { return "/html/Plugins/IvyGen/index.htm"; }

  virtual int getRenderOrder() const { return 101; }
  virtual int getBuildOrder() const { return 0; }

  virtual bool showInTabs() const { return true; }
  virtual bool showSelectAll() const { return true; }

  virtual bool acceptSaveLoad() const { return true; }

  virtual void registered() {}
  virtual void unregistered() {}
  virtual void beforeMainLoop() {}

  virtual bool begin(int toolbar_id, unsigned menu_id);
  virtual bool end();
  virtual void onNewProject() {}
  virtual IGenEventHandler *getEventHandler() { return this; }

  virtual void setVisible(bool vis) { isVisible = vis; }
  virtual bool getVisible() const { return isVisible; }
  virtual bool getSelectionBox(BBox3 &box) const { return objEd.getSelectionBox(box); }
  virtual bool getStatusBarPos(Point3 &pos) const { return false; }

  virtual void clearObjects();
  virtual void saveObjects(DataBlock &blk, DataBlock &local_data, const char *base_path);
  virtual void loadObjects(const DataBlock &blk, const DataBlock &local_data, const char *base_path);
  virtual void selectAll() { objEd.selectAll(); }
  virtual void deselectAll() { objEd.unselectAll(); }

  virtual void actObjects(float dt);
  virtual void beforeRenderObjects(IGenViewportWnd *vp);
  virtual void renderObjects();
  virtual void renderTransObjects();

  virtual void *queryInterfacePtr(unsigned huid);

  virtual bool onPluginMenuClick(unsigned id) { return false; }
  virtual void handleViewportAcceleratorCommand(unsigned id) override;
  virtual void registerMenuAccelerators() override;

  virtual void gatherStaticVisualGeometry(StaticGeometryContainer &cont) { gatherStaticGeometry(cont, 0); }

  virtual void gatherStaticCollisionGeomGame(StaticGeometryContainer &cont) { gatherStaticGeometry(cont, 0); }

  virtual void gatherStaticCollisionGeomEditor(StaticGeometryContainer &cont) { gatherStaticGeometry(cont, 0); }

  virtual void gatherStaticEnviGeometry(StaticGeometryContainer &container) {}
  void gatherStaticGeometry(StaticGeometryContainer &cont, int flags) { objEd.gatherStaticGeometry(cont, flags); }

  // IGenEventHandler
  virtual IGenEventHandler *getWrappedHandler() { return &objEd; }

private:
  IvyObjectEditor objEd;
  bool isVisible, firstBegin;
};
