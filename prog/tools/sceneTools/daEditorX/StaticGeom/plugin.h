// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <oldEditor/de_interface.h>
#include <oldEditor/de_clipping.h>
#include <oldEditor/de_common_interface.h>
#include <libTools/staticGeom/staticGeometryContainer.h>
#include <de3_occluderGeomProvider.h>
#include <de3_collisionPreview.h>

#include <de3_fileTracker.h>


class BaseTexture;
typedef BaseTexture Texture;
class GeomObject;


class StaticGeometryPlugin : public IGenEditorPlugin,
                             public IGenEventHandler,
                             public IGatherStaticGeometry,
                             public IOccluderGeomProvider,
                             public IDagorEdCustomCollider,
                             public IRenderingService,
                             public IFileChangedNotify,
                             public ILightingChangeClient,
                             public PropPanel::ControlEventHandler
{
public:
  StaticGeometryPlugin();
  ~StaticGeometryPlugin();

  // IGenEditorPlugin interface implementation
  virtual const char *getInternalName() const { return "staticGeometry"; }
  virtual const char *getMenuCommandName() const { return "SGeometry"; }
  virtual const char *getHelpUrl() const { return "/html/Plugins/SGeometry/index.htm"; }

  virtual int getRenderOrder() const { return 5; }
  virtual int getBuildOrder() const { return 0; }
  virtual bool showInTabs() const { return true; }

  virtual void registered();
  virtual void unregistered();
  virtual void beforeMainLoop() {}

  virtual bool begin(int toolbar_id, unsigned menu_id);
  virtual bool end() { return true; }
  virtual IGenEventHandler *getEventHandler() { return this; }

  virtual void setVisible(bool vis);
  virtual bool getVisible() const { return objsVisible; }
  virtual bool getSelectionBox(BBox3 &bounds) const;
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
  virtual void renderTransObjects();

  virtual void *queryInterfacePtr(unsigned huid);

  virtual bool onPluginMenuClick(unsigned id);

  // IGenEventHandler interface implementation
  // virtual void handleButtonClick ( int btn_id, CtlBtnTemplate *btn, bool btn_pressed ) {}
  virtual void handleKeyPress(IGenViewportWnd *wnd, int vk, int modif);
  virtual void handleKeyRelease(IGenViewportWnd *wnd, int vk, int modif);
  virtual bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseRBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseCBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseCBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseWheel(IGenViewportWnd *wnd, int wheel_d, int x, int y, int key_modif);
  virtual bool handleMouseDoubleClick(IGenViewportWnd *wnd, int x, int y, int key_modif);
  virtual void handleViewportPaint(IGenViewportWnd *wnd);
  virtual void handleViewChange(IGenViewportWnd *wnd);
  // virtual void handleChildCreate ( CtlWndObject  *child ) {}
  // virtual void handleChildClose ( CtlWndObject  *child ) {}

  // from IGatherStaticGeometry
  virtual void gatherStaticVisualGeometry(StaticGeometryContainer &container);
  virtual void gatherStaticEnviGeometry(StaticGeometryContainer &container);
  virtual void gatherStaticCollisionGeomGame(StaticGeometryContainer &container);
  virtual void gatherStaticCollisionGeomEditor(StaticGeometryContainer &container);

  // IOccluderGeomProvider
  virtual void gatherOccluders(Tab<TMatrix> &occl_boxes, Tab<Quad> &occl_quads);
  virtual void renderOccluders(const Point3 &camPos, float max_dist) {}

  // from IRenderingService
  virtual void renderGeometry(Stage stage);

  // IDagorEdCustomCollider
  virtual bool traceRay(const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm);
  virtual bool shadowRayHitTest(const Point3 &p, const Point3 &dir, real maxt);
  virtual const char *getColliderName() const { return getMenuCommandName(); }
  virtual bool isColliderVisible() const { return getVisible(); }

  // ILightingChangeClient
  virtual void onLightingChanged() {}
  void onLightingSettingsChanged() override {}

  // IFileChangedNotify
  virtual void onFileChanged(int file_name_id);

  // ControlEventHandler
  virtual void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel);

protected:
  GeomObject *geom;

  String lastImportFilename;
  String sceneFileName;

  bool objsVisible, loaded, collisionBuilt;


private:
  bool doResetGeometry;
  bool loadingFailed;
  collisionpreview::Collision collision;

  String StaticGeometryPlugin::getPluginFilePath(const char *fileName);

  void loadGeometry();
  void resetGeometry();

  String getImportDagPath() const;
  void importDag();
};
