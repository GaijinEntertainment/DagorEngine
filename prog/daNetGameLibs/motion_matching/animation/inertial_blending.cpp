// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <anim/dag_animBlend.h>
#include <anim/dag_animKeyInterp.h>
#include "animation_clip.h"
#include "animation_sampling.h"
#include "inertial_blending.h"

void inertialize_pose_transition(BoneInertialInfo &offset,
  const BoneInertialInfo &current,
  const BoneInertialInfo &goal,
  const eastl::bitvector<framemem_allocator> &position_dirty,
  const eastl::bitvector<framemem_allocator> &rotation_dirty)
{

  // Transition all the inertializers for each other bone
  for (int i = 0, n = offset.position.size(); i < n; i++)
  {
    if (position_dirty[i])
    {
      vec3f goalVelocity = goal.velocity[i];
      inertialize_transition_v3(offset.position[i], offset.velocity[i], current.position[i], current.velocity[i], goal.position[i],
        goalVelocity);
    }

    if (rotation_dirty[i])
    {
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

static vec3f get_key(const AnimV20::AnimChanPoint3 *channel, int time)
{
  float keyT;
  const AnimV20::AnimKeyPoint3 *key = channel->findKey(time, &keyT);
  return keyT > 0 ? AnimV20Math::interp_key(key[0], v_splats(keyT)) : key[0].p;
}

static vec3f get_key(const AnimV20::AnimChanQuat *channel, int time)
{
  float keyT;
  const AnimV20::AnimKeyQuat *key = channel->findKey(time, &keyT);
  return keyT > 0 ? AnimV20Math::interp_key(key[0], key[1], keyT) : key[0].p;
}

void apply_root_motion_correction(float t,
  const AnimationClip &clip,
  dag::Index16 root_idx,
  BoneInertialInfo &info,
  const eastl::bitvector<framemem_allocator> &position_dirty,
  const eastl::bitvector<framemem_allocator> &rotation_dirty)
{
  if (clip.inPlaceAnimation)
    return;
  int rootId = root_idx.index();
  RootMotion rootTm = lerp_root_motion(clip, t);
  quat4f invRot = v_quat_conjugate(rootTm.rotation);

  if (position_dirty[rootId])
  {
    vec3f pos = v_quat_mul_vec3(invRot, v_sub(info.position[rootId], rootTm.translation));
    info.position[rootId] = pos;
    info.velocity[rootId] = v_quat_mul_vec3(invRot, info.velocity[rootId]);
  }

  if (rotation_dirty[rootId])
    info.rotation[rootId] = v_norm4(v_quat_mul_quat(invRot, info.rotation[rootId]));
}

void extract_frame_info(float t,
  const AnimationClip &clip,
  BoneInertialInfo &info,
  eastl::bitvector<framemem_allocator> &position_dirty,
  eastl::bitvector<framemem_allocator> &rotation_dirty)
{
  constexpr int TIME_TicksPerSec = AnimV20::TIME_TicksPerSec >> AnimV20::TIME_SubdivExp;

  int a2dTime1 = t * AnimV20::TIME_TicksPerSec;
  int a2dTime2 = a2dTime1 + AnimV20::TIME_TicksPerSec / TIME_TicksPerSec;

  vec4f dtInv = v_safediv(V_C_ONE, v_splats(TIME_TicksPerSec));

  for (const AnimationClip::Point3Channel &channel : clip.channelTranslation)
  {
    vec3f p1 = get_key(channel.first, a2dTime1);
    vec3f p2 = get_key(channel.first, a2dTime2);

    int nodeIdx = channel.second.index();
    info.position[nodeIdx] = p1;
    info.velocity[nodeIdx] = v_mul(v_sub(p2, p1), dtInv);
    position_dirty.set(nodeIdx, true);
  }

  for (const AnimationClip::QuaternionChannel &channel : clip.channelRotation)
  {
    quat4f r1 = get_key(channel.first, a2dTime1);
    quat4f r2 = get_key(channel.first, a2dTime2);

    int nodeIdx = channel.second.index();
    info.rotation[nodeIdx] = r1;
    vec3f w = quat_log(v_quat_mul_quat(r1, v_quat_conjugate(r2)));
    info.angular_velocity[nodeIdx] = v_mul(w, dtInv);
    rotation_dirty.set(nodeIdx, true);
  }
}