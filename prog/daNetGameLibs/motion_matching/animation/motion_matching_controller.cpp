// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/anim/anim.h>
#include <anim/dag_animKeyInterp.h>
#include <util/dag_convar.h>

#include "animation_data_base.h"
#include "animation_sampling.h"
#include "motion_matching_controller.h"

CONSOLE_BOOL_VAL("motion_matching", debug_force_disable_post_blend_controllers, false);
CONSOLE_BOOL_VAL("motion_matching", debug_disable_motion_matching_blend, false);

void MotionMatchingController::setup(const AnimationDataBase &data_base, const AnimV20::AnimcharBaseComponent &animchar)
{
  nodeCount = animchar.getOriginalNodeTree()->nodeCount();
  dataBase = &data_base;
}

bool MotionMatchingController::getPose(AnimV20::AnimBlender::TlsContext &tls, const Tab<AnimV20::AnimMap> &anim_map) const
{
  if (motionMatchingWeight == 0.f || debug_disable_motion_matching_blend.get())
    return false;

  for (auto [pbcId, pbcWeight] : dataBase->pbcWeightOverrides)
    if (pbcId >= 0 && pbcId < tls.pbcWt.size())
      tls.pbcWt[pbcId] = lerp(tls.pbcWt[pbcId], pbcWeight, motionMatchingWeight);

  if (debug_force_disable_post_blend_controllers)
    mem_set_0(tls.pbcWt);

  uint16_t rootId = dataBase->rootNode.index();
  for (int i = 0, e = anim_map.size(); i < e; ++i)
  {
    AnimV20::AnimMap animMap = anim_map[i];
    if (!animMap.geomId.valid())
      continue;
    uint16_t geomId = animMap.geomId.index();
    float nodeWeight = perNodeWeights[geomId];
    if (nodeWeight < 1e-6f)
      continue;
    int16_t animId = animMap.animId;


    AnimV20::AnimBlender::PrsResult &chPrs = tls.chPrs[animId];

    AnimV20::AnimBlender::NodeWeight &wtPos = tls.wtPos[animId];
    wtPos.totalNum = 1;
    wtPos.wTotal = nodeWeight;
    AnimV20::AnimBlender::WeightedNode &chPos = tls.chPos[animId];
    chPos.readyFlg = AnimV20::AnimBlender::RM_POS_B;
    chPos.blendWt[0] = nodeWeight;
    chPrs.pos = (geomId == rootId ? rootPRS.position : resultAnimation.position[geomId]);

    AnimV20::AnimBlender::NodeWeight &wtRot = tls.wtRot[animId];
    wtRot.totalNum = 1;
    wtRot.wTotal = nodeWeight;
    AnimV20::AnimBlender::WeightedNode &chRot = tls.chRot[animId];
    chRot.readyFlg = AnimV20::AnimBlender::RM_ROT_B;
    chRot.blendWt[0] = nodeWeight;
    chPrs.rot = (geomId == rootId ? rootPRS.rotation : resultAnimation.rotation[geomId]);

    if (geomId == rootId)
    {
      AnimV20::AnimBlender::NodeWeight &wtScl = tls.wtScl[animId];
      wtScl.totalNum = 1;
      wtScl.wTotal = nodeWeight;
      AnimV20::AnimBlender::WeightedNode &chScl = tls.chScl[animId];
      chScl.readyFlg = AnimV20::AnimBlender::RM_SCL_B;
      chScl.blendWt[0] = nodeWeight;
      chPrs.scl = rootPRS.scale;
    }
  }
  return true;
}


void MotionMatchingController::playAnimation(int clip_index, int frame_index, bool need_transition)
{
  currentClipInfo.clip = clip_index;
  currentClipInfo.frame = frame_index;
  currentClipInfo.linearBlendProgress = 0.f;

  const AnimationClip &nextClip = dataBase->clips[clip_index];
  // We probably want blending here, but it looks like all clips will have (1,1,-1) scale.
  // And I also don't understand why we need scale in animations at all, such constants can be premultiplied.
  rootPRS.scale = nextClip.rootScale;

  if (need_transition)
  {
    lastTransitionTime = 0.f;
    // reuse memory for nextAnimation, resultAnimation is not needed here and will be recalculated in `updateAnimationProgress()`
    BoneInertialInfo &nextAnimation = resultAnimation;
    float timeInSeconds = getFrameTimeInSeconds(clip_index, frame_index, 0.f);
    extract_frame_info(timeInSeconds, nextClip, nextAnimation);
    apply_root_motion_correction(timeInSeconds, nextClip, dataBase->rootNode, nextAnimation);

    // caclulate offset between currentAnimation and nextAnimation
    // we will decay offset for smooth transition
    inertialize_pose_transition(offset, currentAnimation, nextAnimation, perNodeWeights);
  }
}


static bool next_frame(int &clip_idx, int &frame, int delta, const AnimationDataBase &data_base)
{
  frame += delta;
  const AnimationClip &clip = data_base.clips[clip_idx];
  if (frame >= clip.tickDuration)
  {
#if DISABLE_MOTION_MATCHING
    frame = 0;
    clip_idx = (clip_idx + 1) % data_base.clips.size();
    return false;
#endif
    if (clip.nextClip != -1)
    {
      frame %= clip.tickDuration;
      clip_idx = clip.nextClip;
      frame = min(frame, data_base.clips[clip.nextClip].tickDuration - 1);
    }
    else
    {
      frame = clip.tickDuration - 1;
      return true;
    }
  }
  return false;
}

bool MotionMatchingController::willCurrentAnimationEnd(float dt)
{
  if (!hasActiveAnimation())
    return true;
  int framesPassed = int(getCurrentFrameProgress() + dt * TICKS_PER_SECOND * playSpeedMult);
  const AnimationClip &curClip = dataBase->clips[getCurrentClip()];
  if (curClip.nextClip == -1 && getCurrentFrame() + framesPassed >= curClip.tickDuration)
    return true;
  return false;
}

void MotionMatchingController::updateAnimationProgress(float dt)
{
  if (!hasActiveAnimation())
    return;
  currentClipInfo.linearBlendProgress += dt * TICKS_PER_SECOND * playSpeedMult;
  lastTransitionTime += dt;
  if (currentClipInfo.linearBlendProgress >= 1.f)
  {
    int skipFrames = (int)currentClipInfo.linearBlendProgress;
    currentClipInfo.linearBlendProgress -= skipFrames;
    bool animationFinished = next_frame(currentClipInfo.clip, currentClipInfo.frame, skipFrames, *dataBase);
    if (animationFinished)
      currentClipInfo.linearBlendProgress = 1.f;
  }

  int curClipIdx = currentClipInfo.clip;
  float timeInSeconds = getFrameTimeInSeconds(curClipIdx, currentClipInfo.frame, currentClipInfo.linearBlendProgress);
  extract_frame_info(timeInSeconds, dataBase->clips[curClipIdx], currentAnimation);
  apply_root_motion_correction(timeInSeconds, dataBase->clips[curClipIdx], dataBase->rootNode, currentAnimation);
  // decay offset here. Result animation is offset + currentAnimation
  inertialize_pose_update(resultAnimation, offset, currentAnimation, perNodeWeights, transitionBlendTime * 0.5f, dt);
}

void MotionMatchingController::updateRoot(
  float dt, const AnimV20::AnimcharBaseComponent &animchar, const Point3 &animchar_velocity, const Point3 &animchar_angular_velocity)
{
  mat44f animcharTm;
  animchar.getTm(animcharTm);
  if (rootSynchronization || !hasActiveAnimation())
  {
    vec3f scale;
    v_mat4_decompose(animcharTm, rootPosition, rootRotation, scale);
  }
  else
  {
    const CurrentClipInfo &curAnimation = currentClipInfo;
    float prevTime = getFrameTimeInSeconds(curAnimation.clip, curAnimation.frame, curAnimation.linearBlendProgress);
    float nextTime = prevTime + dt * playSpeedMult;
    RootMotion prevRoot, nextRoot;
    if (!dataBase->clips[curAnimation.clip].looped || nextTime < dataBase->clips[curAnimation.clip].duration)
    {
      prevRoot = lerp_root_motion(dataBase->clips[curAnimation.clip], prevTime);
      nextRoot = lerp_root_motion(dataBase->clips[curAnimation.clip], nextTime);
    }
    else
    {
      prevRoot = dataBase->clips[curAnimation.clip].rootMotion[0];
      nextRoot = lerp_root_motion(dataBase->clips[curAnimation.clip], nextTime - prevTime);
    }
    vec3f deltaPos = v_sub(nextRoot.translation, prevRoot.translation);
    quat4f animSpaceToWorldSpace = v_quat_mul_quat(rootRotation, v_quat_conjugate(prevRoot.rotation));
    rootPosition = v_add(rootPosition, v_quat_mul_vec3(animSpaceToWorldSpace, deltaPos));
    rootPosition = v_perm_xbzw(rootPosition, animcharTm.col3); // don't have movement along Y axis in animations
    quat4f prevToNextQuat = v_quat_mul_quat(nextRoot.rotation, v_quat_conjugate(prevRoot.rotation));
    rootRotation = v_norm4(v_quat_mul_quat(prevToNextQuat, rootRotation));

    quat4f animcharRotation = v_quat_from_mat43(animcharTm);
    if (rootAdjustment)
    {
      vec3f adjustmentPos = v_sub(animcharTm.col3, rootPosition);
      adjustmentPos = damp_adjustment_exact_v3(adjustmentPos, rootAdjustmentPosTime * 0.5, dt);
      if (rootAdjustmentVelocityRatio > 0)
      {
        vec4f maxAdjustmentPos = v_splats(rootAdjustmentVelocityRatio * length(animchar_velocity) * dt);
        if (v_test_vec_x_gt(v_length3_x(adjustmentPos), maxAdjustmentPos))
          adjustmentPos = v_mul(maxAdjustmentPos, v_norm3_safe(adjustmentPos, v_zero()));
      }
      rootPosition = v_add(rootPosition, adjustmentPos);

      quat4f adjustmentRot = v_quat_abs(v_quat_mul_quat(animcharRotation, v_quat_conjugate(rootRotation)));
      adjustmentRot = damp_adjustment_exact_q(adjustmentRot, rootAdjustmentRotTime * 0.5, dt);
      if (rootAdjustmentAngVelocityRatio > 0)
      {
        float maxAdjustmentRot = rootAdjustmentAngVelocityRatio * length(animchar_angular_velocity) * dt;
        float adjustmentAngle = v_extract_w(v_mul(V_C_TWO, v_acos(v_min(adjustmentRot, V_C_ONE))));
        if (adjustmentAngle > maxAdjustmentRot)
          adjustmentRot = v_quat_from_unit_vec_ang(v_norm3(adjustmentRot), v_splats(maxAdjustmentRot));
      }
      rootRotation = v_quat_mul_quat(adjustmentRot, rootRotation);
    }
    if (rootClamping)
    {
      vec3f clampingPos = v_sub(rootPosition, animcharTm.col3);
      if (v_extract_x(v_length3_sq_x(clampingPos)) > rootClampingMaxDistance * rootClampingMaxDistance)
      {
        clampingPos = v_mul(v_splats(rootClampingMaxDistance), v_norm3(clampingPos));
        rootPosition = v_add(animcharTm.col3, clampingPos);
      }

      quat4f clampingRot = v_quat_abs(v_quat_mul_quat(rootRotation, v_quat_conjugate(animcharRotation)));
      float clampingAngle = v_extract_w(v_mul(V_C_TWO, v_acos(v_min(clampingRot, V_C_ONE))));
      if (clampingAngle > rootClampingMaxAngle)
      {
        vec4f rotAxis = v_sel(v_norm3(clampingRot), V_C_UNIT_0100, v_sub(v_dot3(clampingRot, clampingRot), V_C_EPS_VAL));
        clampingRot = v_quat_from_unit_vec_ang(rotAxis, v_splats(rootClampingMaxAngle));
        rootRotation = v_quat_mul_quat(clampingRot, animcharRotation);
      }
    }
    G_ASSERT(!v_test_xyzw_nan(rootRotation));
  }
  if (hasActiveAnimation())
  {
    int curFrame = currentClipInfo.frame;
    const AnimationClip &curClip = dataBase->clips[currentClipInfo.clip];
    if (curFrame == 0)
      curFrame = 1;
    vec3f posDelta = v_sub(curClip.rootMotion[curFrame].translation, curClip.rootMotion[curFrame - 1].translation);
    float animationVelocity = v_extract_x(v_length3_x(v_mul(posDelta, V_TICKS_PER_SECOND)));
    float characterVelocity = length(animchar_velocity);
    float characterRotationSpeed = length(animchar_angular_velocity);
    if (animationVelocity > 0.001f)
      playSpeedMult = characterVelocity / animationVelocity;
    else if (characterRotationSpeed > 0.1f) // no need to adjust animation speed when character is standing still and not rotating
    {
      quat4f rotDelta = v_quat_abs(
        v_quat_mul_quat(curClip.rootMotion[curFrame].rotation, v_quat_conjugate(curClip.rootMotion[curFrame - 1].rotation)));
      float rotationSpeed = v_extract_w(v_mul(v_acos(v_clamp(rotDelta, v_splats(-1.0f), V_C_ONE)), v_splats(TICKS_PER_SECOND)));
      playSpeedMult = rotationSpeed > 0.001f ? characterRotationSpeed / rotationSpeed : 1.0f;
    }
    playSpeedMult = clamp(playSpeedMult, curClip.playSpeedMultiplierRange.x, curClip.playSpeedMultiplierRange.y);
  }
}
