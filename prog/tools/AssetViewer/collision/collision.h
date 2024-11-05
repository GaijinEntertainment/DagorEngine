// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../av_plugin.h"
#include "nodesProcessing.h"
#include <EditorCore/ec_interface.h>

#include <propPanel/c_control_event_handler.h>
#include <propPanel/control/treeInterface.h>
#include <sepGui/wndMenuInterface.h>
#include <3d/dag_resPtr.h>

class CollisionResource;
class GeomNodeTree;


class CollisionPlugin : public IGenEditorPlugin,
                        public PropPanel::ControlEventHandler,
                        public PropPanel::ITreeControlEventHandler,
                        public IMenuEventHandler
{
public:
  CollisionPlugin();
  ~CollisionPlugin() { end(); }


  // IGenEditorPlugin
  virtual const char *getInternalName() const { return "Collision"; }

  virtual void registered() {}
  virtual void unregistered() {}

  virtual bool begin(DagorAsset *asset);
  virtual bool end();

  virtual void clearObjects() {}
  virtual void onSaveLibrary();
  virtual void onLoadLibrary() {}

  virtual bool getSelectionBox(BBox3 &box) const;

  virtual void actObjects(float dt) {}
  virtual void beforeRenderObjects() {}
  virtual void renderObjects() {}
  virtual void renderTransObjects();

  virtual bool supportAssetType(const DagorAsset &asset) const;

  virtual void fillPropPanel(PropPanel::ContainerPropertyControl &panel);
  virtual void postFillPropPanel() {}

  virtual void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel);
  virtual void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel);
  virtual bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);

  void handleViewportPaint(IGenViewportWnd *wnd)
  {
    drawObjects(wnd);
    drawInfo(wnd);
  }

  virtual bool onTreeContextMenu(PropPanel::ContainerPropertyControl &tree, int pcb_id,
    PropPanel::ITreeInterface &tree_interface) override;
  virtual int onMenuItemClick(unsigned id) override;

public:
  CollisionPlugin *self;

protected:
  CollisionResource *collisionRes;
  GeomNodeTree *nodeTree;
  NodesProcessing nodesProcessing;
  UniqueTex faceOrientationRenderDepth;
  int selectedNodeId;
  bool drawNodeAnotate;
  bool showPhysCollidable;
  bool showTraceable;
  bool isSolidMatValid;
  bool drawSolid;
  bool showFaceOrientation;

  void drawObjects(IGenViewportWnd *wnd);
  void printKdopLog();
  void clearAssetStats();
  void fillAssetStats();
  void updateFaceOrientationRenderDepthFromCurRT();
  int getNodeIdx(PropPanel::ContainerPropertyControl *tree, PropPanel::TLeafHandle leaf);
};

void InitCollisionResource(const DagorAsset &asset, CollisionResource **collision_res, GeomNodeTree **node_tree);
void ReleaseCollisionResource(CollisionResource **collision_res, GeomNodeTree **node_tree);
void RenderCollisionResource(const CollisionResource &collision_res, GeomNodeTree *node_tree, bool show_phys_collidable = false,
  bool show_traceable = false, bool draw_solid = false, bool show_face_orientation = false, int selected_node_id = -1,
  bool edit_mode = false, const dag::Vector<bool> &hidden_nodes = {});
