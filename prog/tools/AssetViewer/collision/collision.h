// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../av_plugin.h"
#include "nodesProcessing.h"
#include <EditorCore/ec_interface.h>

#include <propPanel/c_control_event_handler.h>
#include <propPanel/control/menu.h>
#include <propPanel/control/treeInterface.h>
#include <3d/dag_resPtr.h>

class CollisionResource;
class GeomNodeTree;
class IObjEntity;


class CollisionPlugin : public IGenEditorPlugin,
                        public PropPanel::ControlEventHandler,
                        public PropPanel::ITreeControlEventHandler,
                        public PropPanel::IMenuEventHandler
{
public:
  CollisionPlugin();
  ~CollisionPlugin() override { end(); }


  // IGenEditorPlugin
  const char *getInternalName() const override { return "Collision"; }

  void registered() override {}
  void unregistered() override {}

  bool begin(DagorAsset *asset) override;
  bool end() override;

  void clearObjects() override {}
  void onSaveLibrary() override;
  void onLoadLibrary() override {}

  bool getSelectionBox(BBox3 &box) const override;

  void actObjects(float dt) override {}
  void beforeRenderObjects() override {}
  void renderObjects() override {}
  void renderTransObjects() override;

  bool supportAssetType(const DagorAsset &asset) const override;

  void fillPropPanel(PropPanel::ContainerPropertyControl &panel) override;
  void postFillPropPanel() override {}

  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;

  void handleViewportPaint(IGenViewportWnd *wnd) override
  {
    drawObjects(wnd);
    drawInfo(wnd);
  }

  bool onTreeContextMenu(PropPanel::ContainerPropertyControl &tree, int pcb_id, PropPanel::ITreeInterface &tree_interface) override;
  int onMenuItemClick(unsigned id) override;

public:
  CollisionPlugin *self;

protected:
  CollisionResource *collisionRes;
  GeomNodeTree *nodeTree;
  NodesProcessing nodesProcessing;
  UniqueTex faceOrientationRenderDepth;
  int selectedNodeId;
  bool drawNodeAnotate;
  bool showBbox;
  bool showPhysCollidable;
  bool showTraceable;
  bool isSolidMatValid;
  bool drawSolid;
  bool showFaceOrientation;

  String modelAssetName;
  IObjEntity *modelEntity = nullptr;
  bool showModel = false;

  void drawObjects(IGenViewportWnd *wnd);
  void printKdopLog();
  void clearAssetStats();
  void fillAssetStats();
  void updateFaceOrientationRenderDepthFromCurRT();
  int getNodeIdx(PropPanel::ContainerPropertyControl *tree, PropPanel::TLeafHandle leaf);
  void updateModel();
};

void InitCollisionResource(const DagorAsset &asset, CollisionResource **collision_res, GeomNodeTree **node_tree);
void ReleaseCollisionResource(CollisionResource **collision_res, GeomNodeTree **node_tree);
void RenderCollisionResource(const CollisionResource &collision_res, GeomNodeTree *node_tree, bool show_bbox = false,
  bool show_phys_collidable = false, bool show_traceable = false, bool draw_solid = false, bool show_face_orientation = false,
  int selected_node_id = -1, bool edit_mode = false, const dag::Vector<bool> &hidden_nodes = {});
