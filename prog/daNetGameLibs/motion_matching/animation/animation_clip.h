// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>
#include <EASTL/vector_map.h>
#include <dag/dag_vector.h>
#include <anim/dag_animChannels.h> //AnimV20::AnimData::Anim
#include "motion_matching_features.h"
#include "motion_matching_tags.h"
#include "foot_locker.h"
#include <util/dag_index16.h>

struct RootMotion
{
  // modelToWorld * boneToModel = modelToWorld * (RootTM * RootTM^-1) * boneToModel
  // RootTM = translationTM * rotationTM
  // RootTM^-1 * boneToModel = (rotationTM^-1 * translationTM^-1) * boneToModel = in-place animation
  vec3f translation = V_C_UNIT_0001;
  quat4f rotation = V_C_UNIT_0001;
  RootMotion inv() const
  {
    quat4f invRot = v_quat_conjugate(rotation);
    return {v_quat_mul_vec3(invRot, v_neg(translation)), invRot};
  }
  RootMotion operator*(const RootMotion &other) const
  {
    return {v_add(translation, v_quat_mul_vec3(rotation, other.translation)), v_quat_mul_quat(rotation, other.rotation)};
  }
  vec3f transformPosition(vec3f v) const { return v_add(translation, v_quat_mul_vec3(rotation, v)); }
  vec3f transformDirection(vec3f dir) const { return v_quat_mul_vec3(rotation, dir); }
};

struct AnimationInterval
{
  uint32_t from, to;
  Tags tags;
};


// this constants gives good result, but it can be not optimal
constexpr int BOUNDS_SMALL_SIZE = 8;
constexpr int BOUNDS_LARGE_SIZE = 32;
struct AnimationClip
{
  eastl::string name;
  float duration;
  int tickDuration;
  int featuresNormalizationGroup;

  dag::Vector<AnimationInterval> intervals;

  eastl::string nextClipName;
  int nextClip;
  bool looped;
  bool inPlaceAnimation;
  const AnimV20::AnimData *animation;
  using Point3Channel = eastl::pair</*animation node id*/ dag::Index16, /*node index*/ dag::Index16>;
  using QuaternionChannel = eastl::pair</*animation node id*/ dag::Index16, /*node index*/ dag::Index16>;
  dag::Vector<Point3Channel> channelTranslation;
  dag::Vector<QuaternionChannel> channelRotation;
  dag::Vector<Point3Channel> channelScale;
  dag::Vector<float> nodeMask;
  vec3f rootScale;
  Point2 playSpeedMultiplierRange;
  int animTreeTimer;
  float animTreeTimerCycle;


  dag::Vector<RootMotion> rootMotion;
  FootLockerClipData footLockerStates;

  FrameFeatures features;
  dag::Vector<vec4f> boundsSmallMin; // features.size() / BOUNDS_SMALL_SIZE
  dag::Vector<vec4f> boundsSmallMax; // features.size() / BOUNDS_SMALL_SIZE
  dag::Vector<vec4f> boundsLargeMin; // features.size() / BOUNDS_LARGE_SIZE
  dag::Vector<vec4f> boundsLargeMax; // features.size() / BOUNDS_LARGE_SIZE

  int getInterval(int frame) const
  {
    for (int i = 0; i < intervals.size(); i++)
      if (frame < intervals[i].to)
        return i;
    return -1;
  }
};