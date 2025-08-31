// Copyright (C) Gaijin Games KFT.  All rights reserved.
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
  ~Plugin() override;

  // IGenEditorPlugin
  const char *getInternalName() const override { return "occluders"; }
  const char *getMenuCommandName() const override { return "Occluders"; }
  const char *getHelpUrl() const override { return NULL; }

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

  bool onPluginMenuClick(unsigned id) override;
  void handleViewportAcceleratorCommand(unsigned id) override;
  void registerMenuAccelerators() override;

  // IGenEventHandler
  IGenEventHandler *getWrappedHandler() override { return &objEd; }

  // IBinaryDataBuilder
  bool validateBuild(int target, ILogWriter &rep, PropPanel::ContainerPropertyControl *params) override;
  bool addUsedTextures(ITextureNumerator &tn) override { return true; }
  bool buildAndWrite(BinDumpSaveCB &cwr, const ITextureNumerator &tn, PropPanel::ContainerPropertyControl *pp) override;
  bool checkMetrics(const DataBlock &metrics_blk) override { return true; }
  bool useExportParameters() const override { return true; }
  void fillExportPanel(PropPanel::ContainerPropertyControl &params) override;

  // IOccluderGeomProvider
  void gatherOccluders(Tab<TMatrix> &occl_boxes, Tab<Quad> &occl_quads) override;
  void renderOccluders(const Point3 &camPos, float max_dist) override {}

private:
  ObjEd objEd;
  Tab<SimpleString> disPlugins;

  bool isVisible;
  Tab<TMatrix> occlBoxes;
  Tab<Quad> occlQuads;

  bool renderExternalOccluders;
  float renderExternalOccludersDist;
};
