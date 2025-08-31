// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../av_plugin.h"
#include "../av_appwnd.h"

#include <EditorCore/ec_interface.h>

#include <propPanel/c_control_event_handler.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_index16.h>

class OLAppWindow;
class BaseTexture;
class GeomNodeTree;


class NodesPlugin : public IGenEditorPlugin, public PropPanel::ControlEventHandler
{
public:
  enum PresentationType
  {
    FIG_TYPE_CIRCLE = 1,
    FIG_TYPE_SPHERE,
  };


  NodesPlugin() : geomNodeTree(NULL), presentationType(FIG_TYPE_SPHERE), nodeTreePanel(NULL)
  {
    drawLinks = true;
    drawRootLinks = true;
    drawNodeAnotate = true;

    radius = 0.05;

    DataBlock appBlk(::get_app().getWorkspace().getAppPath());
    nodeFilterMasksBlk = *appBlk.getBlockByNameEx("nodeFilterMasks");
  }

  ~NodesPlugin() override { end(); }

  // IGenEditorPlugin
  const char *getInternalName() const override { return "nodesPreview"; }

  void registered() override {}
  void unregistered() override {}

  bool begin(DagorAsset *asset) override;
  bool end() override;

  void clearObjects() override {}
  void onSaveLibrary() override {}
  void onLoadLibrary() override {}

  bool getSelectionBox(BBox3 &box) const override;

  void actObjects(float dt) override {}
  void beforeRenderObjects() override {}
  void renderObjects() override {}
  void renderTransObjects() override;

  bool supportAssetType(const DagorAsset &asset) const override;

  void fillPropPanel(PropPanel::ContainerPropertyControl &panel) override;
  void postFillPropPanel() override {}

  // ControlEventHandler
  void onChange(int pid, PropPanel::ContainerPropertyControl *panel) override;
  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  void handleViewportPaint(IGenViewportWnd *wnd) override
  {
    drawObjects(wnd);
    drawInfo(wnd);
  }
  void selectNode(dag::Index16 node);

protected:
  dag::Index16 findObject(const Point3 &p, const Point3 &dir);
  BBox3 bbox;


private:
  GeomNodeTree *geomNodeTree;

  bool drawLinks, drawRootLinks, drawNodeAnotate;
  real radius;
  float axisLen = 0.f;
  PresentationType presentationType;
  DataBlock nodeFilterMasksBlk;
  Tab<bool> nodesIsEnabledArray;

  PropPanel::ContainerPropertyControl *nodeTreePanel;

  void drawObjects(IGenViewportWnd *wnd);

  void applyMask(int index, PropPanel::ContainerPropertyControl *panel);
  void updateAllMaskFilters(PropPanel::ContainerPropertyControl *panel);
};
