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


  NodesPlugin() : geomNodeTree(NULL), presentationType(FIG_TYPE_SPHERE)
  {
    drawLinks = true;
    drawRootLinks = true;
    drawNodeAnotate = true;

    radius = 0.05;

    DataBlock appBlk(::get_app().getWorkspace().getAppPath());
    nodeFilterMasksBlk = *appBlk.getBlockByNameEx("nodeFilterMasks");
  }

  ~NodesPlugin() { end(); }

  // IGenEditorPlugin
  virtual const char *getInternalName() const { return "nodesPreview"; }

  virtual void registered() {}
  virtual void unregistered() {}

  virtual bool begin(DagorAsset *asset);
  virtual bool end();

  virtual void clearObjects() {}
  virtual void onSaveLibrary() {}
  virtual void onLoadLibrary() {}

  virtual bool getSelectionBox(BBox3 &box) const;

  virtual void actObjects(float dt) {}
  virtual void beforeRenderObjects() {}
  virtual void renderObjects() {}
  virtual void renderTransObjects();

  virtual bool supportAssetType(const DagorAsset &asset) const;

  virtual void fillPropPanel(PropPanel::ContainerPropertyControl &panel);
  virtual void postFillPropPanel() {}

  // ControlEventHandler
  virtual void onChange(int pid, PropPanel::ContainerPropertyControl *panel);
  virtual void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  void handleViewportPaint(IGenViewportWnd *wnd)
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

  void drawObjects(IGenViewportWnd *wnd);

  void applyMask(int index, PropPanel::ContainerPropertyControl *panel);
  void updateAllMaskFilters(PropPanel::ContainerPropertyControl *panel);
};
