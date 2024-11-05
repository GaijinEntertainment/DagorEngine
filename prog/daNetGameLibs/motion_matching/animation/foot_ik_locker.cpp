// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_geomTree.h>
#include <ioSys/dag_dataBlock.h>
#include "foot_ik_locker.h"
#include "animation_sampling.h"
#include "animation_data_base.h"


ECS_REGISTER_RELOCATABLE_TYPE(FootIKLockerNodes, nullptr);

static vec3f transform_upto_root(const NodeTSRFixedArray &nodes, const GeomNodeTree &tree, dag::Index16 target_node)
{
  vec3f pos = nodes[target_node.index()].translation;
  for (dag::Index16 parent = tree.getParentNodeIdx(target_node); parent; parent = tree.getParentNodeIdx(parent))
    pos = v_add(nodes[parent.index()].translation,
      v_quat_mul_vec3(nodes[parent.index()].rotation, v_mul(nodes[parent.index()].scale, pos)));
  return pos;
}

void load_foot_ik_locker_states(ecs::EntityId data_base_eid,
  FootIKLockerClipData &out,
  const AnimationClip &clip,
  const DataBlock &clip_block,
  const GeomNodeTree &tree)
{
  out.lockStates.clear();
  const DataBlock *lockParams = clip_block.getBlockByName("use_foot_ik_locker");
  if (!lockParams)
    return;
  float velocityThreshold = lockParams->getReal("velocity_threshold", 0.3);
  vec4f threshold = v_set_x(sqr(velocityThreshold / TICKS_PER_SECOND));
  int medianFilterRadius = lockParams->getInt("median_filter_radius", 2);

  FootIKLockerNodes footNodes = get_foot_ik_locker_nodes(data_base_eid, tree);
  if (!footNodes.legs[0].toeNodeId || !footNodes.legs[1].toeNodeId)
    return;
  if (lockParams->getBool("always_locked", false))
  {
    out.lockStates.resize(footNodes.NUM_LEGS * clip.tickDuration, true);
    return;
  }
  eastl::array<vec3f, FootIKLockerNodes::NUM_LEGS> prevPos;
  NodeTSRFixedArray nodes(tree.nodeCount());
  set_identity(nodes, tree);
  sample_animation(0.0, clip, nodes);
  eastl::bitvector<> tmpLockStates;
  tmpLockStates.resize(footNodes.NUM_LEGS * clip.tickDuration, false);
  for (int legId = 0; legId < footNodes.NUM_LEGS; ++legId)
    prevPos[legId] = transform_upto_root(nodes, tree, footNodes.legs[legId].toeNodeId);
  for (int frame = 1; frame < clip.tickDuration; ++frame)
  {
    float timeInSeconds = clip.duration * frame / clip.tickDuration;
    sample_animation(timeInSeconds, clip, nodes);
    for (int legId = 0; legId < footNodes.NUM_LEGS; ++legId)
    {
      vec3f curPos = transform_upto_root(nodes, tree, footNodes.legs[legId].toeNodeId);
      if (v_test_vec_x_lt(v_length3_sq_x(v_sub(curPos, prevPos[legId])), threshold))
      {
        tmpLockStates[(frame - 1) * footNodes.NUM_LEGS + legId] = true;
        tmpLockStates[frame * footNodes.NUM_LEGS + legId] = true;
      }
      prevPos[legId] = curPos;
    }
  }

  out.lockStates.resize(tmpLockStates.size());
  for (int legId = 0; legId < footNodes.NUM_LEGS; ++legId)
  {
    for (int frame = 0; frame < clip.tickDuration; ++frame)
    {
      int sum = 0;
      int from = max(frame - medianFilterRadius, 0);
      int to = min(frame + medianFilterRadius, clip.tickDuration - 1);
      if (clip.looped)
      {
        for (int i = frame - medianFilterRadius; i < from; ++i)
        {
          int j = (i % clip.tickDuration + clip.tickDuration) % clip.tickDuration;
          sum += tmpLockStates[j * footNodes.NUM_LEGS + legId];
        }
        for (int i = frame + medianFilterRadius; i > to; --i)
        {
          int j = i % clip.tickDuration;
          sum += tmpLockStates[j * footNodes.NUM_LEGS + legId];
        }
      }
      for (int i = from; i <= to; ++i)
        sum += tmpLockStates[i * footNodes.NUM_LEGS + legId];
      out.lockStates[frame * footNodes.NUM_LEGS + legId] = sum > medianFilterRadius;
    }
  }
}
