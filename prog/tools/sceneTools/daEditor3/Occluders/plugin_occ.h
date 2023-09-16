#pragma once


#include "decl.h"
#include "objEd_occ.h"
#include <oldEditor/de_interface.h>
#include <de3_baseInterfaces.h>
#include <de3_occluderGeomProvider.h>
#include <oldEditor/de_common_interface.h>
#include <math/dag_mesh.h>
#include <util/dag_simpleString.h>


class occplugin::Plugin : public IGenEditorPlugin,
                          public IGenEventHandlerWrapper,
                          public IBinaryDataBuilder,
                          public IOccluderGeomProvider
{
public:
  static Plugin *self;

  Plugin();
  ~Plugin();

  // IGenEditorPlugin
  virtual const char *getInternalName() const { return "occluders"; }
  virtual const char *getMenuCommandName() const { return "Occluders"; }
  virtual const char *getHelpUrl() const { return NULL; }

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

  virtual bool onPluginMenuClick(unsigned id);

  // IGenEventHandler
  virtual IGenEventHandler *getWrappedHandler() { return &objEd; }

  // IBinaryDataBuilder
  virtual bool validateBuild(int target, ILogWriter &rep, PropPanel2 *params);
  virtual bool addUsedTextures(ITextureNumerator &tn) { return true; }
  virtual bool buildAndWrite(BinDumpSaveCB &cwr, const ITextureNumerator &tn, PropPanel2 *pp);
  virtual bool checkMetrics(const DataBlock &metrics_blk) { return true; }
  virtual bool useExportParameters() const { return true; }
  virtual void fillExportPanel(PropPanel2 &params);

  // IOccluderGeomProvider
  virtual void gatherOccluders(Tab<TMatrix> &occl_boxes, Tab<Quad> &occl_quads);
  virtual void renderOccluders(const Point3 &camPos, float max_dist) {}

private:
  ObjEd objEd;
  Tab<SimpleString> disPlugins;

  bool isVisible;
  Tab<TMatrix> occlBoxes;
  Tab<Quad> occlQuads;

  bool renderExternalOccluders;
  float renderExternalOccludersDist;
};
