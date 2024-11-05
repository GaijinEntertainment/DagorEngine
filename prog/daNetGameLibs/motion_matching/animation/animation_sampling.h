// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <anim/dag_animBlend.h>
#include <EASTL/fixed_vector.h>
#include <math/dag_geomTree.h>
#include "animation_clip.h"

// translation, rotation, scale of node
// keep changed status to optimize not changed nodes blending
struct NodeTSR
{
  vec3f translation;
  quat4f rotation;
  vec3f scale;
  uint8_t changeBit; // should be parallel bitarray
  __forceinline void set_translation(const vec3f &translation_)
  {
    translation = translation_;
    changeBit |= AnimV20::AnimBlender::RM_POS_B;
  }
  __forceinline void set_rotation(const quat4f &rotation_)
  {
    rotation = rotation_;
    changeBit |= AnimV20::AnimBlender::RM_ROT_B;
  }
  __forceinline void set_scale(const vec3f &scale_)
  {
    scale = scale_;
    changeBit |= AnimV20::AnimBlender::RM_SCL_B;
  }
};

constexpr int MAX_NODE_COUNT = 255;
using NodeTSRFixedArray = eastl::fixed_vector<NodeTSR, MAX_NODE_COUNT>;

void set_identity(NodeTSRFixedArray &nodes, const GeomNodeTree &tree);

void sample_animation(float t, const AnimationClip &clip, NodeTSRFixedArray &nodes);

RootMotion lerp_root_motion(const AnimationClip &clip, float time);

void update_tree(const NodeTSRFixedArray &nodes, GeomNodeTree &tree);

struct AnimationDataBase;
vec3f extract_center_of_mass(const AnimationDataBase &data_base, const GeomNodeTree &tree);
vec3f get_root_translation(const AnimationDataBase &data_base, const GeomNodeTree &tree);
vec3f get_root_direction(const AnimationDataBase &data_base, const GeomNodeTree &tree);
vec4f_const FORWARD_DIRECTION = DECL_VECFLOAT4(0.f, 0.f, -1.f, 0.f);
