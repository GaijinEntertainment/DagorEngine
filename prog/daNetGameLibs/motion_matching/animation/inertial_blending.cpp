// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <anim/dag_animBlend.h>
#include <anim/dag_animKeyInterp.h>
#include "animation_clip.h"
#include "animation_sampling.h"
#include "inertial_blending.h"

void inertialize_pose_transition(
  BoneInertialInfo &offset, const BoneInertialInfo &current, const BoneInertialInfo &goal, const dag::Vector<float> &node_weights)
{

  // Transition all the inertializers for each other bone
  for (int i = 0, n = offset.position.size(); i < n; i++)
  {
    if (node_weights[i] > 0.0f)
    {
      vec3f goalVelocity = goal.velocity[i];
      inertialize_transition_v3(offset.position[i], offset.velocity[i], current.position[i], current.velocity[i], goal.position[i],
        goalVelocity);

      vec3f goalAngularVelocity = goal.angular_velocity[i];
      inertialize_transition_q(offset.rotation[i], offset.angular_velocity[i], current.rotation[i], current.angular_velocity[i],
        goal.rotation[i], goalAngularVelocity);
    }
  }
}

void inertialize_pose_update(BoneInertialInfo &result,
  BoneInertialInfo &offset,
  const BoneInertialInfo &current,
  const dag::Vector<float> &node_weights,
  const float halflife,
  const float dt)
{


  // Then we update the inertializers for the rest of the bones
  for (int i = 0, n = offset.position.size(); i < n; i++)
  {
    if (node_weights[i] > 0.0f)
    {
      decay_spring_damper_exact_v3(offset.position[i], offset.velocity[i], halflife, dt);
      result.position[i] = v_add(offset.position[i], current.position[i]);
      decay_spring_damper_exact_q(offset.rotation[i], offset.angular_velocity[i], halflife, dt);
      result.rotation[i] = v_quat_mul_quat(offset.rotation[i], current.rotation[i]);
    }
  }
}

void apply_root_motion_correction(float t, const AnimationClip &clip, dag::Index16 root_idx, BoneInertialInfo &info)
{
  if (clip.inPlaceAnimation)
    return;
  int rootId = root_idx.index();
  RootMotion rootTm = lerp_root_motion(clip, t);
  quat4f invRot = v_quat_conjugate(rootTm.rotation);

  vec3f pos = v_quat_mul_vec3(invRot, v_sub(info.position[rootId], rootTm.translation));
  info.position[rootId] = pos;
  info.velocity[rootId] = v_quat_mul_vec3(invRot, info.velocity[rootId]);
  info.rotation[rootId] = v_norm4(v_quat_mul_quat(invRot, info.rotation[rootId]));
}

void extract_frame_info(float t, const AnimationClip &clip, BoneInertialInfo &info)
{
  constexpr int TIME_FramesPerSec = AnimV20::TIME_TicksPerSec >> AnimV20::TIME_SubdivExp;

  int a2dTime1 = t * AnimV20::TIME_TicksPerSec;
  int a2dTime2 = a2dTime1 + AnimV20::TIME_TicksPerSec / TIME_FramesPerSec;

  vec4f dtInv = v_safediv(V_C_ONE, v_splats(TIME_FramesPerSec));

  AnimV20Math::PrsAnimSampler<AnimV20Math::DefaultConfig> sampler(clip.animation);

  for (const AnimationClip::Point3Channel &channel : clip.channelTranslation)
  {
    sampler.seekTicks(a2dTime1);
    vec3f p1 = sampler.samplePosTrack(channel.first);
    sampler.seekTicks(a2dTime2);
    vec3f p2 = sampler.samplePosTrack(channel.first);

    int nodeIdx = channel.second.index();
    info.position[nodeIdx] = p1;
    info.velocity[nodeIdx] = v_mul(v_sub(p2, p1), dtInv);
  }

  for (const AnimationClip::QuaternionChannel &channel : clip.channelRotation)
  {
    sampler.seekTicks(a2dTime1);
    vec3f r1 = sampler.sampleRotTrack(channel.first);
    sampler.seekTicks(a2dTime2);
    vec3f r2 = sampler.sampleRotTrack(channel.first);

    int nodeIdx = channel.second.index();
    info.rotation[nodeIdx] = r1;
    vec3f w = quat_log(v_quat_mul_quat(r1, v_quat_conjugate(r2)));
    info.angular_velocity[nodeIdx] = v_mul(w, dtInv);
  }
}