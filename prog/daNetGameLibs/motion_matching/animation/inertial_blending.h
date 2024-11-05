// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/bitvector.h>
#include <dag/dag_vector.h>
#include <memory/dag_framemem.h>
#include "spring.h"


static inline void inertialize_transition_v3(
  vec3f &off_x, vec3f &off_v, const vec3f src_x, const vec3f src_v, const vec3f dst_x, const vec3f dst_v)
{
  off_x = v_sub(v_add(src_x, off_x), dst_x); // off_x = (src_x + off_x) - dst_x;
  off_v = v_sub(v_add(src_v, off_v), dst_v); // off_v = (src_v + off_v) - dst_v;
}

static inline void inertialize_transition_q(
  quat4f &off_x, vec3f &off_v, const quat4f src_x, const vec3f src_v, const quat4f dst_x, const vec3f dst_v)
{
  off_x = v_quat_abs(v_quat_mul_quat(v_quat_mul_quat(off_x, src_x), v_quat_conjugate(dst_x)));
  off_v = v_sub(v_add(off_v, src_v), dst_v);
}


struct BoneInertialInfo
{
  dag::Vector<vec3f> position;
  dag::Vector<vec3f> velocity;
  dag::Vector<quat4f> rotation;
  dag::Vector<vec3f> angular_velocity;
  void set_bone_count(int count)
  {
    position.assign(count, v_zero());
    velocity.assign(count, v_zero());
    rotation.assign(count, V_C_UNIT_0001);
    angular_velocity.assign(count, v_zero());
  }
};


// This function transitions the inertializer for
// the full character. It takes as input the current
// offsets, as well as the root transition locations,
// current root state, and the full pose information
// for the pose being transitioned from (src) as well
// as the pose being transitioned to (dst) in their
// own animation spaces.
void inertialize_pose_transition(BoneInertialInfo &offset,
  const BoneInertialInfo &current,
  const BoneInertialInfo &goal,
  const eastl::bitvector<framemem_allocator> &position_dirty,
  const eastl::bitvector<framemem_allocator> &rotation_dirty);

// This function updates the inertializer states. Here
// it outputs the smoothed animation (input plus offset)
// as well as updating the offsets themselves. It takes
// as input the current playing animation as well as the
// root transition locations, a halflife, and a dt
void inertialize_pose_update(BoneInertialInfo &result,
  BoneInertialInfo &offset,
  const BoneInertialInfo &current,
  const dag::Vector<float> &node_weights,
  const float halflife,
  const float dt);

struct AnimationClip;

void extract_frame_info(float t,
  const AnimationClip &clip,
  BoneInertialInfo &info,
  eastl::bitvector<framemem_allocator> &position_dirty,
  eastl::bitvector<framemem_allocator> &rotation_dirty);

// We need this correction because current mocap animations are not centered. Root node
// will be transformed to local model space and animation become in-place. Most likely we
// can perform this transformation during a2d export in future.
void apply_root_motion_correction(float t,
  const AnimationClip &clip,
  dag::Index16 root_idx,
  BoneInertialInfo &info,
  const eastl::bitvector<framemem_allocator> &position_dirty,
  const eastl::bitvector<framemem_allocator> &rotation_dirty);
