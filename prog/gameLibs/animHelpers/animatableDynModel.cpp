// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <animHelpers/animatableDynModel.h>

#include <shaders/dag_dynSceneRes.h>
#include <math/dag_TMatrix.h>
#include <math/dag_geomTree.h>
#include <math/dag_mathUtils.h>

NodeAnimationInfo::NodeAnimationInfo(const SimpleNodeAnim &node_anim, int node_id, int time_min, int time_max) :
  nodeAnim(node_anim), nodeId(node_id), timeMin(time_min), timeMax(time_max)
{}

void NodeAnimationInfo::updateTm(float anim_t, const TMatrix &render_space_tm, DynamicRenderableSceneInstance *instance)
{
  int animKey = lerp(timeMin, timeMax, saturate(anim_t));
  TMatrix resTm;
  nodeAnim.calcAnimTm(resTm, animKey);
  instance->setNodeWtm(nodeId, render_space_tm * resTm);
}


bool AnimatableDynModel::init(DynamicRenderableSceneInstance *inst, AnimV20::AnimData *anim)
{
  if (!anim || !inst)
    return false;
  instance = inst;
  iterate_names(instance->getLodsResource()->getNames().node, [&](int nodeId, const char *nodeName) {
    SimpleNodeAnim nodeAnim;
    if (!nodeAnim.init(anim, nodeName))
      return;

    int timeMin, timeMax;
    nodeAnim.calcTimeMinMax(timeMin, timeMax);
    if (timeMax <= timeMin)
      return;

    animatedNodes.push_back(NodeAnimationInfo(nodeAnim, nodeId, timeMin, timeMax));
  });
  return true;
}

void AnimatableDynModel::update(float dt) { curAnimProgress = fmodf(curAnimProgress + dt * animTimeMult, 1.f); }

void AnimatableDynModel::updateTm(const TMatrix &render_space_tm)
{
  for (int i = 0; i < animatedNodes.size(); ++i)
    animatedNodes[i].updateTm(curAnimProgress, render_space_tm, instance);
}
