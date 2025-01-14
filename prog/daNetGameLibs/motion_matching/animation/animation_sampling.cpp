// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "animation_data_base.h"
#include "animation_sampling.h"
#include <anim/dag_animKeyInterp.h>
#include <math/dag_mathAng.h>

void set_identity(NodeTSRFixedArray &nodes, const GeomNodeTree &tree)
{
  for (unsigned i = 0, n = tree.nodeCount(); i < n; i++)
  {
    nodes[i].translation = tree.getNodeTm(dag::Index16(i)).col3;
    nodes[i].rotation = V_C_UNIT_0001;
    nodes[i].scale = V_C_ONE;
    nodes[i].changeBit = 0;
  }
}

void sample_animation(float t, const AnimationClip &clip, NodeTSRFixedArray &nodes)
{
  AnimV20Math::PrsAnimSampler<AnimV20Math::OneShotConfig> sampler(clip.animation, t);

  for (const AnimationClip::Point3Channel &translation : clip.channelTranslation)
  {
    nodes[translation.second.index()].set_translation(sampler.samplePosTrack(translation.first));
  }
  for (const AnimationClip::QuaternionChannel &rotation : clip.channelRotation)
  {
    nodes[rotation.second.index()].set_rotation(sampler.sampleRotTrack(rotation.first));
  }
  for (const AnimationClip::Point3Channel &scale : clip.channelScale)
  {
    nodes[scale.second.index()].set_scale(sampler.sampleSclTrack(scale.first));
  }
  if (clip.inPlaceAnimation)
  {
    RootMotion rootMotion = lerp_root_motion(clip, t);
    nodes[0].set_translation(rootMotion.translation);
    nodes[0].set_rotation(rootMotion.rotation);
  }
}

RootMotion lerp_root_motion(const AnimationClip &clip, float time)
{
  float ticks = clip.tickDuration * time / clip.duration;
  int frame = int(ticks);
  if (frame >= clip.tickDuration)
    return clip.rootMotion[clip.tickDuration];
  float fraction = ticks - float(frame);

  int prevFrame = frame;
  int nextFrame = prevFrame + 1;

  const RootMotion &prevRoot = clip.rootMotion[prevFrame];
  const RootMotion &nextRoot = clip.rootMotion[nextFrame];

  RootMotion ret;
  ret.rotation = v_quat_qslerp(fraction, prevRoot.rotation, nextRoot.rotation);
  ret.translation = v_lerp_vec4f(v_splats(fraction), prevRoot.translation, nextRoot.translation);
  return ret;
}

void update_tree(const NodeTSRFixedArray &nodes, GeomNodeTree &tree)
{
  for (unsigned i = 0, n = tree.nodeCount(); i < n; i++)
    if (nodes[i].changeBit)
      v_mat44_compose(tree.getNodeTm(dag::Index16(i)), nodes[i].translation, nodes[i].rotation, nodes[i].scale);
  tree.invalidateWtm();
  tree.calcWtm();
}

vec3f extract_center_of_mass(const AnimationDataBase &data_base, const GeomNodeTree &tree)
{
  vec3f ret = v_zero();
  vec4f centerOfMassW = v_zero();
  for (const auto &itr : data_base.nodeCenterOfMasses)
  {
    const mat44f &tm = tree.getNodeWtmRel(itr.first);
    vec3f p = v_mat44_mul_vec3p(tm, itr.second);
    vec4f w = v_splat_w(itr.second);
    ret = v_madd(p, w, ret);
    centerOfMassW = v_add(centerOfMassW, w);
  }
  return v_div(ret, centerOfMassW);
}

vec3f get_root_translation(const AnimationDataBase &data_base, const GeomNodeTree &tree)
{
  // these constants can be made into animation clip parameters if needed
  constexpr bool allow_root_motion_Y = true;
  constexpr bool allow_root_motion_XZ = false;

  vec3f centerOfMass = extract_center_of_mass(data_base, tree);

  vec3f translation = v_sub(centerOfMass, data_base.defaultCenterOfMass);

  if (allow_root_motion_XZ)
    translation = v_and(translation, V_CI_MASK0101);
  if (allow_root_motion_Y)
    translation = v_and(translation, V_CI_MASK1011);
  return translation;
}

vec3f get_root_direction(const AnimationDataBase &data_base, const GeomNodeTree &tree)
{
  // these constants can be made into animation clip parameters if needed

  constexpr bool allow_root_rotation_Y = false;

  if (!allow_root_rotation_Y)
  {
    float totalW = 0;
    for (const auto &itr : data_base.rootRotations)
      totalW += itr.second.weight;
    float invTotalW = safeinv(totalW);
    quat4f heading = V_C_UNIT_0001;
    for (const auto &itr : data_base.rootRotations)
    {
      const mat44f &tm = tree.getNodeWtmRel(itr.first);
      vec3f translation;
      vec3f scale;
      quat4f rot;
      v_mat4_decompose(tm, translation, rot, scale);
      // Delta from the default pose
      rot = v_quat_mul_quat(rot, v_quat_conjugate(itr.second.defaultRotation));
      quat4f twist, swing;
      v_quat_decompose_twist_swing(rot, V_C_UNIT_0100, twist, swing);

      quat4f r = v_quat_qslerp(itr.second.weight * invTotalW, V_C_UNIT_0001, twist);
      heading = v_quat_mul_quat(r, heading);
    }
    return v_quat_mul_vec3(heading, FORWARD_DIRECTION);
  }
  return FORWARD_DIRECTION;
}
