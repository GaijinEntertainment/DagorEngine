// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <dasModules/aotEcs.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/dasManagedTab.h>
#include <dasModules/aotAnimchar.h>
#include <anim/dag_animPostBlendCtrl.h>
#include "../animation/animationDataBase.h"
#include "../animation/animationSampling.h"
#include "../animation/motionMatchingController.h"
#include "../animation/motionMatching.h"

MAKE_TYPE_FACTORY(AnimationClip, AnimationClip);
MAKE_TYPE_FACTORY(AnimationDataBase, AnimationDataBase);
MAKE_TYPE_FACTORY(AnimationFilterTags, AnimationFilterTags);
MAKE_TYPE_FACTORY(MotionMatchingController, MotionMatchingController);
MAKE_TYPE_FACTORY(BoneInertialInfo, BoneInertialInfo);
MAKE_TYPE_FACTORY(FeatureWeights, FeatureWeights);
MAKE_TYPE_FACTORY(TagPreset, TagPreset);
MAKE_TYPE_FACTORY(RootMotion, RootMotion);
MAKE_TYPE_FACTORY(FootLockerIKCtrlLegData, AnimV20::FootLockerIKCtrl::LegData)
MAKE_TYPE_FACTORY(FrameFeatures, FrameFeatures);
MAKE_TYPE_FACTORY(MatchingResult, MatchingResult)

DAS_BIND_VECTOR(AnimationClipVector, AnimationClipVector, AnimationClip, "AnimationClipVector");

DAS_BIND_VECTOR(TagPresetVector, TagPresetVector, TagPreset, "TagPresetVector");
DAS_BIND_VECTOR(MotionMatchingSearchFeatures, MotionMatchingSearchFeatures, das::float4, "MotionMatchingSearchFeatures");


inline void clearAnimations(MotionMatchingController &controller) { controller.currentClipInfo = {}; }

inline int get_root_node_id(const AnimationDataBase &data_base) { return data_base.rootNode.index(); }

inline void init_feature_weights(FeatureWeights &weights, int node_count, int trajectory_size)
{
  weights.rootPositions.resize(trajectory_size, 0.f);
  weights.rootDirections.resize(trajectory_size, 0.f);

  weights.nodePositions.resize(node_count, 0.f);
  weights.nodeVelocities.resize(node_count, 0.f);
}

inline void commit_feature_weights(FeatureWeights &weights)
{
  G_ASSERT(weights.nodePositions.size() == weights.nodeVelocities.size());
  G_ASSERT(weights.rootPositions.size() == weights.rootDirections.size());
  // order is matter! see FrameFeature

  int nodeCount = weights.nodePositions.size();
  int trajectoryPointCount = weights.rootPositions.size();

  FeaturesSizes sizes = caclulate_features_sizes(nodeCount, trajectoryPointCount);

  weights.featureWeights.assign(sizes.featuresSizeInVec4f, v_zero());

  vec4f *data = weights.featureWeights.data();
  auto rootPositions = make_span(reinterpret_cast<Point2 *>(data), trajectoryPointCount);
  auto rootDirections = make_span(reinterpret_cast<Point2 *>(data) + trajectoryPointCount, trajectoryPointCount);
  auto nodePositions = make_span(reinterpret_cast<Point3 *>(data + sizes.trajectorySizeInVec4f), nodeCount);
  auto nodeVelocities = make_span(reinterpret_cast<Point3 *>(data + sizes.trajectorySizeInVec4f) + nodeCount, nodeCount);

  for (int i = 0; i < trajectoryPointCount; i++)
  {
    float p = weights.rootPositions[i];
    float d = weights.rootDirections[i];
    rootPositions[i] = Point2(p, p);
    rootDirections[i] = Point2(d, d);
  }

  for (int i = 0; i < nodeCount; i++)
  {
    float p = weights.nodePositions[i];
    float v = weights.nodeVelocities[i];
    nodePositions[i] = Point3(p, p, p);
    nodeVelocities[i] = Point3(v, v, v);
  }
}

extern void motion_matching_update_anim_tree_foot_locker(AnimV20::AnimcharBaseComponent &animchar,
  const MotionMatchingController &controller);

namespace bind_dascript
{
inline void anim_state_holder_get_foot_locker_legs(AnimV20::IAnimStateHolder *anim_state, const AnimationDataBase &data_base,
  const das::TBlock<void, das::TArray<AnimV20::FootLockerIKCtrl::LegData>> &block, das::Context *context, das::LineInfoArg *at)
{
  if (data_base.footLockerParamId < 0)
    return;
  AnimV20::FootLockerIKCtrl::LegData *legsData =
    static_cast<AnimV20::FootLockerIKCtrl::LegData *>(anim_state->getInlinePtr(data_base.footLockerParamId));
  das::Array arr;
  das::array_mark_locked(arr, (char *)legsData, data_base.footLockerNodes.size());
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline void anim_state_holder_iterate_foot_locker_legs_const(const AnimV20::IAnimStateHolder *anim_state,
  const AnimationDataBase &data_base, const das::TBlock<void, const AnimV20::FootLockerIKCtrl::LegData &> &block,
  das::Context *context, das::LineInfoArg *at)
{
  if (data_base.footLockerParamId < 0)
    return;
  const AnimV20::FootLockerIKCtrl::LegData *legsData =
    static_cast<const AnimV20::FootLockerIKCtrl::LegData *>(anim_state->getInlinePtr(data_base.footLockerParamId));
  vec4f arg;
  for (int i = 0; i < data_base.footLockerNodes.size(); ++i)
  {
    arg = das::cast<const AnimV20::FootLockerIKCtrl::LegData &>::from(legsData[i]);
    context->invoke(block, &arg, nullptr, at);
  }
}

inline bool animation_database_need_lock_foot(const AnimationDataBase &database, int clip_idx, int frame_idx, int leg_idx)
{
  return database.clips[clip_idx].footLockerStates.needLock(leg_idx, frame_idx, database.footLockerNodes.size());
}

inline int get_post_blend_controller_idx(const AnimationDataBase &data_base, const char *pbc_name)
{
  if (!pbc_name)
    return -1;
  const AnimV20::AnimationGraph *anim_graph = data_base.referenceAnimGraph;
  for (int pbcIdx = 0, pbcCnt = anim_graph->getPBCCount(); pbcIdx < pbcCnt; ++pbcIdx)
  {
    const char *pbcName = anim_graph->getPBCName(pbcIdx);
    if (pbcName && !strcmp(pbc_name, pbcName))
      return pbcIdx;
  }
  return -1;
}

inline void simple_spring_damper_exact_q(das::float4 &q, das::float3 &v, das::float4 q_goal, float halflife, float dt)
{
  quat4f q_tmp = q;
  vec3f v_tmp = v;
  ::simple_spring_damper_exact_q(q_tmp, v_tmp, q_goal, halflife, dt);
  q = q_tmp;
  v = v_tmp;
}

template <class T>
inline das::Array span_to_das_array(dag::Span<T> span)
{
  das::Array arr;
  das::array_mark_locked(arr, (char *)span.data(), span.size());
  arr.flags = 0;
  return arr;
}

inline void get_trajectory_features(FrameFeatures &features, int frame,
  const das::TBlock<void, das::TArray<das::float2>, das::TArray<das::float2>> &block, das::Context *context, das::LineInfoArg *at)
{
  TrajectoryFeature trajectory = features.get_trajectory_feature(frame);
  das::Array trajectoryPos = span_to_das_array(trajectory.rootPositions);
  das::Array trajectoryDir = span_to_das_array(trajectory.rootDirections);
  vec4f args[2];
  args[0] = das::cast<das::Array *>::from(&trajectoryPos);
  args[1] = das::cast<das::Array *>::from(&trajectoryDir);
  context->invoke(block, args, nullptr, at);
}

inline void get_node_features(FrameFeatures &features, int frame,
  const das::TBlock<void, das::TArray<das::float3>, das::TArray<das::float3>> &block, das::Context *context, das::LineInfoArg *at)
{
  NodeFeature nodeFeatures = features.get_node_feature(frame);
  das::Array nodePos = span_to_das_array(nodeFeatures.nodePositions);
  das::Array nodeVel = span_to_das_array(nodeFeatures.nodeVelocities);
  vec4f args[2];
  args[0] = das::cast<das::Array *>::from(&nodePos);
  args[1] = das::cast<das::Array *>::from(&nodeVel);
  context->invoke(block, args, nullptr, at);
}
} // namespace bind_dascript
