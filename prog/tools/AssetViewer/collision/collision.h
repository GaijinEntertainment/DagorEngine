// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../av_plugin.h"
#include "nodesProcessing.h"
#include "physMatLegend.h"
#include <EditorCore/ec_interface.h>

#include <propPanel/c_control_event_handler.h>
#include <propPanel/control/menu.h>
#include <propPanel/control/customControl.h>
#include <propPanel/control/treeInterface.h>
#include <3d/dag_resPtr.h>
#include <generic/dag_span.h>
#include <math/dag_geomTree.h>

class CollisionResource;
class IObjEntity;
class TMatrix;
class TMatrix4;


class CollisionPlugin : public IGenEditorPlugin,
                        public PropPanel::ControlEventHandler,
                        public PropPanel::ITreeControlEventHandler,
                        public PropPanel::IMenuEventHandler,
                        public PropPanel::ICustomControl
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
  void renderTransObjects(BaseTexture *) override;

  bool supportAssetType(const DagorAsset &asset) const override;

  void fillPropPanel(PropPanel::ContainerPropertyControl &panel) override;
  void postFillPropPanel() override {}
  void onPropPanelClear(PropPanel::ContainerPropertyControl &panel) override;

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
  void customControlUpdate(int id) override; // the phys material legend swatches

public:
  CollisionPlugin *self;

protected:
  CollisionResource *collisionRes;
  GeomNodeTreeUniquePtr nodeTree;
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
  bool showDegenerativeTriangles;
  bool colorByPhysMat = true;

  String modelAssetName;
  IObjEntity *modelEntity = nullptr;
  bool showModel = false;
  DagorAsset *curAsset = nullptr;
  dag::Vector<DegenerativeNodeData> degenerativeNodes;
  PhysMatLegend physMats;

  void drawObjects(IGenViewportWnd *wnd);
  void fillPhysMatPanel(PropPanel::ContainerPropertyControl &panel);
  void updatePhysMatPanel(PropPanel::ContainerPropertyControl &panel);
  void setSelectedNodeId(int id); // every selection change refreshes the phys material rows and the selected text
  void updateSelectionPanels(PropPanel::ContainerPropertyControl &panel);
  String selectedNodeInfoCaption() const;
  void printKdopLog();
  void clearAssetStats();
  void fillAssetStats();
  void updateFaceOrientationRenderDepth(BaseTexture *color_target);
  int getNodeIdx(PropPanel::ContainerPropertyControl *tree, PropPanel::TLeafHandle leaf);
  void updateModel();
};

// Spans point at caller-owned data; they must outlive the RenderCollisionResource call only.
struct CollisionRenderOptions
{
  bool showBbox = false;
  bool showPhysCollidable = false;
  bool showTraceable = false;
  bool drawSolid = false;
  bool showFaceOrientation = false;
  bool showDegenerativeTriangles = false;
  bool colorByPhysMat = false; // each face takes its material's color; the phys/trace/degenerate modes still override
  dag::ConstSpan<DegenerativeNodeData> degenerativeNodes; // empty = no degenerate markers
  int selectedNodeId = -1;
  bool editMode = false;
  dag::ConstSpan<bool> hiddenNodes;        // empty = no node is hidden; else one flag per node of getAllNodes(), by node index
  const PhysMatLegend *physMats = nullptr; // the material colors and the filter; null = legacy colors, no filter
};

void InitCollisionResource(const DagorAsset &asset, CollisionResource **collision_res, GeomNodeTreeUniquePtr &node_tree);
void ReleaseCollisionResource(CollisionResource **collision_res, GeomNodeTreeUniquePtr &node_tree);
void RenderCollisionResource(const CollisionResource &collision_res, GeomNodeTree *node_tree, const TMatrix &view_tm,
  const TMatrix4 &proj_tm, const CollisionRenderOptions &opts);
