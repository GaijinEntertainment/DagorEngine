// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../av_plugin.h"
#include <EditorCore/ec_interface.h>
#include <de3_objEntity.h>
#include <propPanel/c_control_event_handler.h>
#include <anim/dag_simpleNodeAnim.h>
#include <math/dag_geomTree.h>
#include <EASTL/map.h>
#include <EASTL/vector.h>
#include <EASTL/string.h>

class SimpleString;
class ILodController;

class A2dPlugin : public IGenEditorPlugin, public PropPanel::ControlEventHandler
{
public:
  struct AnimNode
  {
    SimpleString name;
    dag::Index16 skelIdx;
    SimpleNodeAnim anim;
    int prsMask = 0;
    TMatrix tm;
  };

private:
  struct RefDynmodel
  {
    RefDynmodel(const char *ref_name);
    int refTypeId;
    SimpleString refTypeName;
    SimpleString modelName;
    IObjEntity *entity;
    const GeomNodeTree *origTree;
    GeomNodeTree tree;
    ILodController *ctrl;

    void reload(const SimpleString &model_name, DagorAsset *asset);
    void reset();
    void updateNodeTree(const dag::Vector<AnimNode> &animNodes, bool additive);
    bool ready() const { return entity && origTree && ctrl; }
  };

  A2dPlugin *self;
  DagorAsset *asset;
  AnimV20::AnimData *a2d;
  RefDynmodel ref;
  dag::Vector<AnimNode> animNodes; // nodes in a2d
  int selectedNode = -1;

  float currAnimProgress; // 0.f - 1.f
  float animTimeMul;
  int animTimeMin;
  int animTimeMax;

  bool loopedPlay;
  bool drawSkeleton;
  bool drawA2dnodes;
  bool disableAdditivity;

  bool isAnimInProgress() const { return (currAnimProgress < 1.f) && (currAnimProgress >= 0.f); }
  void resetAnimProgress(float defined_progress);
  void processAnimProgress(float dt);
  bool checkOrderOfTrackLabels() const;
  bool checkDuplicatedTrackLabels() const;
  void setupAnimNodesForRefSkeleton();
  void setPoseFromTrackLabel(int key_label_idx);
  void updateAnimNodes(int a2d_time);
  void selectAnimNode(const Point3 &p, const Point3 &dir);

public:
  A2dPlugin();
  ~A2dPlugin() override;

  const char *getInternalName() const override { return "A2d viewer"; }

  void registered() override {}
  void unregistered() override {}

  bool begin(DagorAsset *asset) override;
  bool end() override;

  void clearObjects() override {}
  void onSaveLibrary() override {}
  void onLoadLibrary() override {}

  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  void onChange(int pid, PropPanel::ContainerPropertyControl *panel) override;
  bool getSelectionBox(BBox3 &box) const override;

  void actObjects(float dt) override;
  void beforeRenderObjects() override {}
  void renderObjects() override {}
  void renderTransObjects() override;
  void handleViewportPaint(IGenViewportWnd *wnd) override;

  bool supportAssetType(const DagorAsset &asset) const override;

  void fillPropPanel(PropPanel::ContainerPropertyControl &panel) override;
  void postFillPropPanel() override {}
};
