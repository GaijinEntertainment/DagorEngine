// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "animation_clip.h"
#include <anim/dag_animBlend.h>
#include <EASTL/vector.h>
#include <EASTL/vector_map.h>
#include <util/dag_oaHashNameMap.h>
#include <daECS/core/componentTypes.h>
#include <math/dag_Point4.h>
#include <gameRes/dag_gameResources.h>

struct AnimationRootMotionConfig
{
  eastl::string rootNodeName;
  eastl::vector_map<eastl::string, float> rootMotionDirections;      // nodeName -> weight
  eastl::vector_map<eastl::string, Point4> rootMotionCenterOfMasses; // nodeName -> (offset.xyz, weight)
};

struct TagPreset
{
  FeatureWeights weights;
  int requiredTagIdx;
  float animationBlendTime;
  float linearVelocityViscosity;
  float angularVelocityViscosity;
  float metricaToleranceMin;
  float metricaToleranceMax;
  float metricaToleranceDecayTime;
  float presetBlendTime;
};

using TagPresetVector = eastl::vector<TagPreset>;
using AnimationClipVector = dag::Vector<AnimationClip>;

struct RootRotationInfo
{
  quat4f defaultRotation;
  float weight;
};
struct AnimationDataBase
{
  ecs::StringList nodesName;
  ecs::FloatList predictionTimes;
  AnimationClipVector clips;
  int nodeCount = 0; // we have copy of this values in each clip for readability
  int trajectorySize = 0;
  int featuresSize = 0;
  TagPresetVector tagsPresets;
  dag::Vector<vec4f> featuresAvg, featuresStd;
  int normalizationGroupsCount = 0;

  dag::Index16 rootNode;
  // Prefer this node if it exists in a2d, otherwise fallback to root motion calculation based on skeleton nodes
  eastl::string rootMotionA2dNode;
  // For calculating the facing direction of the character in root motion
  eastl::vector_map<dag::Index16, RootRotationInfo> rootRotations;
  // For calculating the center of mass in root motion
  eastl::vector_map<dag::Index16, vec4f> nodeCenterOfMasses; // nodeId -> (offset.xyz, avg weight)
  vec4f defaultCenterOfMass = v_zero();                      // for default pose

  dag::Vector<eastl::pair<int, float>> pbcWeightOverrides; // pairs (pbcId, pbcWeight)

  dag::Vector<uint32_t> animGraphNodeTagsRemap;
  dag::Vector<dag::Index16> footLockerNodes;
  Ptr<AnimV20::AnimationGraph> referenceAnimGraph;
  int animGraphTagsParamId = -1;
  int footLockerParamId = -1;

  eastl::unique_ptr<GameResource, GameResDeleter> referenceSkeleton;
  void setReferenceSkeleton(const GeomNodeTree *skeleton)
  {
    referenceSkeleton.reset(reinterpret_cast<GameResource *>(const_cast<GeomNodeTree *>(skeleton)));
    ::game_resource_add_ref(referenceSkeleton.get());
  }
  const GeomNodeTree *getReferenceSkeleton() const { return reinterpret_cast<const GeomNodeTree *>(referenceSkeleton.get()); }

  // store here only pow index
  OAHashNameMap<false> tags;
  int playOnlyFromStartTag = -1;
  int getTagsCount() const { return tags.nameCount(); }
  int getTagIdx(const char *tag_name) const { return tags.getNameId(tag_name); }
  const char *getTagName(int tag_idx) const { return tags.getName(tag_idx); }
  bool hasAnimGraphTags() const { return !animGraphNodeTagsRemap.empty(); }
  bool hasAvailableAnimations(const AnimationFilterTags &tags) const;
  void changePBCWeightOverride(int pbc_id, float weight);
  static void requestResources(const char *, const ecs::resource_request_cb_t &res_cb);
};
ECS_DECLARE_BOXED_TYPE(AnimationDataBase);


const float TICKS_PER_SECOND = 30.f;
const vec4f V_TICKS_PER_SECOND = v_splats(TICKS_PER_SECOND);

ECS_DECLARE_RELOCATABLE_TYPE(FrameFeatures);
ECS_DECLARE_RELOCATABLE_TYPE(AnimationFilterTags);

void calculate_acceleration_struct(dag::Vector<AnimationClip> &clips, int features);
