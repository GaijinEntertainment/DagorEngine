#pragma once

#include "../av_plugin.h"
#include <EditorCore/ec_interface.h>
#include <de3_objEntity.h>
#include <propPanel2/c_control_event_handler.h>
#include <anim/dag_simpleNodeAnim.h>
#include <math/dag_geomTree.h>
#include <EASTL/map.h>
#include <EASTL/vector.h>
#include <EASTL/string.h>

class SimpleString;
class ILodController;
namespace AnimV20
{
class AnimV20;
}

class A2dPlugin : public IGenEditorPlugin, public ControlEventHandler
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

  virtual const char *getInternalName() const override { return "A2d viewer"; }

  virtual void registered() override {}
  virtual void unregistered() override {}

  virtual bool begin(DagorAsset *asset) override;
  virtual bool end() override;

  virtual void clearObjects() override {}
  virtual void onSaveLibrary() override {}
  virtual void onLoadLibrary() override {}

  virtual bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  virtual void onClick(int pcb_id, PropertyContainerControlBase *panel) override;
  virtual void onChange(int pid, PropertyContainerControlBase *panel) override;
  virtual bool getSelectionBox(BBox3 &box) const override;

  virtual void actObjects(float dt) override;
  virtual void beforeRenderObjects() override {}
  virtual void renderObjects() override {}
  virtual void renderTransObjects() override;
  virtual void handleViewportPaint(IGenViewportWnd *wnd) override;

  virtual bool supportAssetType(const DagorAsset &asset) const override;

  virtual void fillPropPanel(PropertyContainerControlBase &panel) override;
  virtual void postFillPropPanel() override {}
};
