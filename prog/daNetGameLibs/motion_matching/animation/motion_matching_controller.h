// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <anim/dag_animBlend.h>
#include <dag/dag_vector.h>
#include "inertial_blending.h"
#include "animation_clip.h"
#include "animation_data_base.h"

// it's usefull ot disable mm work while debug new animations
#define DISABLE_MOTION_MATCHING 0

class MotionMatchingController : public AnimV20::IMotionMatchingController
{
public:
  struct NodePRS
  {
    vec3f position = v_zero();
    quat4f rotation = V_C_UNIT_0001;
    vec3f scale = V_C_UNIT_1110;
  };
  struct CurrentClipInfo
  {
    int clip = -1;
    int frame = -1;
    float linearBlendProgress = 0.f;
  };
  const AnimationDataBase *dataBase = nullptr; // set by setup
  vec3f rootPosition = v_zero();
  quat4f rootRotation = V_C_UNIT_0001;
  NodePRS rootPRS; // applied to actual animchar transform which can be not same as `rootPosition` + `rootRotation`
  BoneInertialInfo offset;
  BoneInertialInfo currentAnimation;
  BoneInertialInfo resultAnimation;
  dag::Vector<float> perNodeWeights;
  AnimationFilterTags currentTags;
  CurrentClipInfo currentClipInfo;
  unsigned nodeCount = 0;
  float motionMatchingWeight = 0.0f; // max of perNodeWeights
  float lastTransitionTime = 0.0f;
  float transitionBlendTime = 0.0f;
  float playSpeedMult = 1.0f;

  bool useTagsFromAnimGraph = false;

  // customizable parameters to keep the animation root node close to the actual animchar transform,
  // but at the same time follow the movements from the animation clip as much as possible.
  bool rootSynchronization = true;
  bool rootAdjustment = false;
  bool rootClamping = false;

  // root adjustment parameters
  float rootAdjustmentVelocityRatio = 0.5;
  float rootAdjustmentPosTime = 0.2;
  float rootAdjustmentAngVelocityRatio = 0.5;
  float rootAdjustmentRotTime = 0.4;

  // root clamping parameters
  float rootClampingMaxDistance = 0.1;
  float rootClampingMaxAngle = M_PI_2;


  void setup(const AnimationDataBase &data_base, const AnimV20::AnimcharBaseComponent &animchar);

  bool getPose(AnimV20::AnimBlender::TlsContext &tls, const Tab<AnimV20::AnimMap> &anim_map) const override;

  void playAnimation(int clip_index, int frame_index, bool need_transition);

  void updateAnimationProgress(float dt);
  void updateRoot(float dt,
    const AnimV20::AnimcharBaseComponent &animchar,
    const Point3 &animchar_velocity,
    const Point3 &animchar_angular_velocity);

  bool hasActiveAnimation() const { return currentClipInfo.clip >= 0; }
  int getCurrentClip() const { return currentClipInfo.clip; }
  int getCurrentFrame() const { return currentClipInfo.frame; }
  float getCurrentFrameProgress() const { return currentClipInfo.linearBlendProgress; }
  bool willCurrentAnimationEnd(float dt);

  float getFrameTimeInSeconds(uint32_t clip_idx, uint32_t frame, float blend_progress) const
  {
    G_ASSERT_RETURN(dataBase && clip_idx < dataBase->clips.size(), 0.f);
    const AnimationClip &clip = dataBase->clips[clip_idx];
    return clip.duration * (frame + blend_progress) / clip.tickDuration;
  }
};

// can't be copy by memcpy
ECS_DECLARE_BOXED_TYPE(MotionMatchingController);