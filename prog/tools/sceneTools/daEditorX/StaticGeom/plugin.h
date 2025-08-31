// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <oldEditor/de_interface.h>
#include <oldEditor/de_collision.h>
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
  ~StaticGeometryPlugin() override;

  // IGenEditorPlugin interface implementation
  const char *getInternalName() const override { return "staticGeometry"; }
  const char *getMenuCommandName() const override { return "SGeometry"; }
  const char *getHelpUrl() const override { return "/html/Plugins/SGeometry/index.htm"; }

  int getRenderOrder() const override { return 5; }
  int getBuildOrder() const override { return 0; }
  bool showInTabs() const override { return true; }

  void registered() override;
  void unregistered() override;
  void beforeMainLoop() override {}

  void registerMenuAccelerators() override;
  bool begin(int toolbar_id, unsigned menu_id) override;
  bool end() override { return true; }
  IGenEventHandler *getEventHandler() override { return this; }

  void setVisible(bool vis) override;
  bool getVisible() const override { return objsVisible; }
  bool getSelectionBox(BBox3 &bounds) const override;
  bool getStatusBarPos(Point3 &pos) const override { return false; }

  void clearObjects() override;
  void onNewProject() override {}
  void saveObjects(DataBlock &blk, DataBlock &local_data, const char *base_path) override;
  void loadObjects(const DataBlock &blk, const DataBlock &local_data, const char *base_path) override;
  bool acceptSaveLoad() const override { return true; }

  void selectAll() override {}
  void deselectAll() override {}

  void actObjects(float dt) override {}
  void beforeRenderObjects(IGenViewportWnd *vp) override {}
  void renderObjects() override {}
  void renderTransObjects() override;

  void *queryInterfacePtr(unsigned huid) override;

  bool onPluginMenuClick(unsigned id) override;

  // IGenEventHandler interface implementation
  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseRBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseCBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseCBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseWheel(IGenViewportWnd *wnd, int wheel_d, int x, int y, int key_modif) override;
  bool handleMouseDoubleClick(IGenViewportWnd *wnd, int x, int y, int key_modif) override;
  void handleViewportPaint(IGenViewportWnd *wnd) override;
  void handleViewChange(IGenViewportWnd *wnd) override;

  // from IGatherStaticGeometry
  void gatherStaticVisualGeometry(StaticGeometryContainer &container) override;
  void gatherStaticEnviGeometry(StaticGeometryContainer &container) override;
  void gatherStaticCollisionGeomGame(StaticGeometryContainer &container) override;
  void gatherStaticCollisionGeomEditor(StaticGeometryContainer &container) override;

  // IOccluderGeomProvider
  void gatherOccluders(Tab<TMatrix> &occl_boxes, Tab<Quad> &occl_quads) override;
  void renderOccluders(const Point3 &camPos, float max_dist) override {}

  // from IRenderingService
  void renderGeometry(Stage stage) override;

  // IDagorEdCustomCollider
  bool traceRay(const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm) override;
  bool shadowRayHitTest(const Point3 &p, const Point3 &dir, real maxt) override;
  const char *getColliderName() const override { return getMenuCommandName(); }
  bool isColliderVisible() const override { return getVisible(); }

  // ILightingChangeClient
  void onLightingChanged() override {}
  void onLightingSettingsChanged() override {}

  // IFileChangedNotify
  void onFileChanged(int file_name_id) override;

  // ControlEventHandler
  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

protected:
  GeomObject *geom;

  String lastImportFilename;

  bool objsVisible, loaded, collisionBuilt;


private:
  bool doResetGeometry;
  bool loadingFailed;
  collisionpreview::Collision collision;

  String getPluginFilePath(const char *fileName);

  void loadGeometry();
  void resetGeometry();

  String getImportDagPath() const;
  void importDag();
};
