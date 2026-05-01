//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_smallTab.h>
#include <generic/dag_tab.h>
#include <anim/dag_simpleNodeAnim.h>

class DynamicRenderableSceneInstance;
class TMatrix;
class GeomNodeTree;

namespace AnimV20
{
class AnimData;
};

struct NodeAnimationInfo
{
  SimpleNodeAnim nodeAnim;
  int nodeId;

  int timeMin;
  int timeMax;

  NodeAnimationInfo(const SimpleNodeAnim &node_anim, int node_id, int time_min, int time_max);

  void updateTm(float anim_t, const TMatrix &render_space_tm, DynamicRenderableSceneInstance *instances);
};

struct AnimatableDynModel
{
  Tab<NodeAnimationInfo> animatedNodes;
  float curAnimProgress;
  float animTimeMult;

  DynamicRenderableSceneInstance *instance;

  AnimatableDynModel() : instance(NULL), curAnimProgress(0.f), animTimeMult(1.f) {}

  bool init(DynamicRenderableSceneInstance *inst, AnimV20::AnimData *anim);
  void update(float dt);
  void updateTm(const TMatrix &render_space_tm);
};
