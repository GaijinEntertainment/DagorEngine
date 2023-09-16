#pragma once

#include "../av_plugin.h"
#include "nodesProcessing.h"
#include <EditorCore/ec_interface.h>

#include <propPanel2/c_control_event_handler.h>

class CollisionResource;
class GeomNodeTree;


class CollisionPlugin : public IGenEditorPlugin, public ControlEventHandler
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

  virtual void fillPropPanel(PropertyContainerControlBase &panel);
  virtual void postFillPropPanel() {}

  virtual void onClick(int pcb_id, PropertyContainerControlBase *panel);
  virtual void onChange(int pcb_id, PropertyContainerControlBase *panel);
  virtual bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);

  void handleViewportPaint(IGenViewportWnd *wnd)
  {
    drawObjects(wnd);
    drawInfo(wnd);
  }

public:
  CollisionPlugin *self;

protected:
  CollisionResource *collisionRes;
  GeomNodeTree *nodeTree;
  NodesProcessing nodesProcessing;
  int selectedNodeId;
  bool drawNodeAnotate;
  bool showPhysCollidable;
  bool showTraceable;
  bool isSolidMatValid;
  bool drawSolid;

  void drawObjects(IGenViewportWnd *wnd);
  void printKdopLog();
  void clearAssetStats();
  void fillAssetStats();
};

void InitCollisionResource(const DagorAsset &asset, CollisionResource **collision_res, GeomNodeTree **node_tree);
void ReleaseCollisionResource(CollisionResource **collision_res, GeomNodeTree **node_tree);
void RenderCollisionResource(const CollisionResource &collision_res, GeomNodeTree *node_tree, bool show_phys_collidable = false,
  bool show_traceable = false, bool draw_solid = false, int selected_node_id = -1, bool edit_mode = false,
  const dag::Vector<bool> &hidden_nodes = {});
