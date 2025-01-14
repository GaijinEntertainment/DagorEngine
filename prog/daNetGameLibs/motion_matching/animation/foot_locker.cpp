// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_geomTree.h>
#include <ioSys/dag_dataBlock.h>
#include "foot_locker.h"
#include "animation_sampling.h"
#include "animation_data_base.h"
#include <memory/dag_framemem.h>


static vec3f transform_upto_root(const NodeTSRFixedArray &nodes, const GeomNodeTree &tree, dag::Index16 target_node)
{
  vec3f pos = nodes[target_node.index()].translation;
  for (dag::Index16 parent = tree.getParentNodeIdx(target_node); parent; parent = tree.getParentNodeIdx(parent))
    pos = v_add(nodes[parent.index()].translation,
      v_quat_mul_vec3(nodes[parent.index()].rotation, v_mul(nodes[parent.index()].scale, pos)));
  return pos;
}

void load_foot_locker_states(
  const AnimationDataBase &database, FootLockerClipData &out, const AnimationClip &clip, const DataBlock &clip_block)
{
  const GeomNodeTree &tree = *database.getReferenceSkeleton();
  out.lockStates.clear();
  const DataBlock *lockParams = clip_block.getBlockByName("use_foot_ik_locker");
  if (!lockParams)
    return;
  float velocityThreshold = lockParams->getReal("velocity_threshold", 0.3);
  vec4f threshold = v_set_x(sqr(velocityThreshold / TICKS_PER_SECOND));
  int medianFilterRadius = lockParams->getInt("median_filter_radius", 2);

  const dag::Vector<dag::Index16> &footNodes = database.footLockerNodes;
  int numLegs = footNodes.size();
  if (lockParams->getBool("always_locked", false))
  {
    out.lockStates.resize(numLegs * clip.tickDuration, true);
    return;
  }
  dag::Vector<vec3f, framemem_allocator> prevPos(numLegs);
  NodeTSRFixedArray nodes(tree.nodeCount());
  set_identity(nodes, tree);
  sample_animation(0.0, clip, nodes);
  eastl::bitvector<framemem_allocator> tmpLockStates(numLegs * clip.tickDuration, false);
  for (int legId = 0; legId < numLegs; ++legId)
    prevPos[legId] = transform_upto_root(nodes, tree, footNodes[legId]);
  for (int frame = 1; frame < clip.tickDuration; ++frame)
  {
    float timeInSeconds = clip.duration * frame / clip.tickDuration;
    sample_animation(timeInSeconds, clip, nodes);
    for (int legId = 0; legId < numLegs; ++legId)
    {
      vec3f curPos = transform_upto_root(nodes, tree, footNodes[legId]);
      if (v_test_vec_x_lt(v_length3_sq_x(v_sub(curPos, prevPos[legId])), threshold))
      {
        tmpLockStates[(frame - 1) * numLegs + legId] = true;
        tmpLockStates[frame * numLegs + legId] = true;
      }
      prevPos[legId] = curPos;
    }
  }

  out.lockStates.resize(tmpLockStates.size());
  for (int legId = 0; legId < numLegs; ++legId)
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
          sum += tmpLockStates[j * numLegs + legId];
        }
        for (int i = frame + medianFilterRadius; i > to; --i)
        {
          int j = i % clip.tickDuration;
          sum += tmpLockStates[j * numLegs + legId];
        }
      }
      for (int i = from; i <= to; ++i)
        sum += tmpLockStates[i * numLegs + legId];
      out.lockStates[frame * numLegs + legId] = sum > medianFilterRadius;
    }
  }
}
