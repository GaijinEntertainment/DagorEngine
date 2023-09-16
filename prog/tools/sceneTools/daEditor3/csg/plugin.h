#ifndef __GAIJIN_IVYGEN_PLUGIN__
#define __GAIJIN_IVYGEN_PLUGIN__
#pragma once


#include <oldEditor/de_interface.h>
#include <oldEditor/de_clipping.h>
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
  ~CSGPlugin();

  // IGenEventHandler
  virtual IGenEventHandler *getWrappedHandler() { return &objEd; }

  // IGenEditorPlugin
  virtual const char *getInternalName() const { return "csg"; }
  virtual const char *getMenuCommandName() const { return "CSG"; }
  virtual const char *getHelpUrl() const { return "/html/Plugins/CSG/index.htm"; }

  virtual void processGeometry(StaticGeometryContainer &container);

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

  virtual void setVisible(bool vis);
  virtual bool getVisible() const;
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

  virtual bool onPluginMenuClick(unsigned id) { return false; }

  virtual void gatherStaticVisualGeometry(StaticGeometryContainer &cont) {}

  virtual void gatherStaticCollisionGeomGame(StaticGeometryContainer &cont);

  virtual void gatherStaticCollisionGeomEditor(StaticGeometryContainer &cont);

  virtual void gatherStaticEnviGeometry(StaticGeometryContainer &container) {}

private:
  bool isVisible;
};


#endif //__GAIJIN_IVYGEN_PLUGIN__
