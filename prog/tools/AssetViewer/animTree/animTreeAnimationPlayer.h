// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_geomTree.h>
#include <anim/dag_simpleNodeAnim.h>
#include <dag/dag_vector.h>
#include <util/dag_string.h>
#include <propPanel/c_common.h>

namespace PropPanel
{
class ContainerPropertyControl;
}

class ILodController;
class IObjEntity;
class DagorAsset;
class DagorAssetMgr;
struct AnimParamData;
enum class BlendNodeType;

struct AnimNodeData
{
  AnimNodeData(const char *node_name, dag::Index16 skel_idx, const SimpleNodeAnim &node_anim) :
    name(node_name), skelIdx(skel_idx), anim(node_anim)
  {}
  String name;
  dag::Index16 skelIdx;
  SimpleNodeAnim anim;
};

class AnimTreeAnimationPlayer
{
public:
  AnimTreeAnimationPlayer(int dyn_model_id, const DagorAssetMgr &mgr);

  void init(const DagorAsset &asset);
  void clear();
  void act(float dt);
  void setDynModel(const char *model_namemodel_name, const DagorAsset &asset);
  void resetDynModel();
  void reloadDynModel(const DagorAsset &asset);
  void setValuesFromCommonNode(PropPanel::ContainerPropertyControl *panel, const dag::Vector<AnimParamData> &params);
  void setValuesFormStillNode(PropPanel::ContainerPropertyControl *panel, const dag::Vector<AnimParamData> &params);
  void setValuesFormParametricNode(PropPanel::ContainerPropertyControl *panel, const dag::Vector<AnimParamData> &params);
  void loadA2dResource(PropPanel::ContainerPropertyControl *panel, const dag::Vector<AnimParamData> &params,
    PropPanel::TLeafHandle leaff, bool validate_without_error = false);
  void animate(int key);
  void animNodesFieldChanged(PropPanel::ContainerPropertyControl *panel, const dag::Vector<AnimParamData> &params,
    const AnimParamData &param);
  void changePlayingAnimation();
  void setSelectedA2dName(const char *name);
  void resetSelectedA2dName() { selectedA2dName.clear(); }
  int getDynModelId() const { return dynModelId; }
  String getModelName() const { return modelName; }
  bool getSelectionBox(BBox3 &box) const;
  float getAnimProgress() const { return animProgress; }
  bool isAnimInProgress() const { return animProgress >= 0.f && animProgress < 1.f; }

private:
  IObjEntity *entity = nullptr;
  ILodController *ctrl = nullptr;
  const GeomNodeTree *origGeomTree = nullptr;
  GeomNodeTree geomTree;
  String modelName;
  const int dynModelId;
  const DagorAssetMgr &assetMgr;

  AnimV20::AnimData *selectedA2d = nullptr;
  String selectedA2dName;
  dag::Vector<AnimNodeData> animNodes;
  float animProgress;

  BlendNodeType type;
  float addk;
  float mulk;
  float pStart;
  float pEnd;
  float animTime;
  int firstKey;
  int lastKey;
  bool loop;
};
