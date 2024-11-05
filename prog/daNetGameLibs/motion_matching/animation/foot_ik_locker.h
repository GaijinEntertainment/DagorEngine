// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/array.h>
#include <EASTL/bitvector.h>
#include <daECS/core/componentTypes.h>
#include <util/dag_index16.h>


class GeomNodeTree;
class MotionMatchingController;
struct AnimationDataBase;
struct AnimationClip;

struct FootIKLockerNodes
{
  struct LegNodes
  {
    dag::Index16 toeNodeId;
    dag::Index16 heelNodeId;
    dag::Index16 kneeNodeId;
    dag::Index16 hipNodeId;
  };
  static constexpr int NUM_LEGS = 2;
  eastl::array<LegNodes, NUM_LEGS> legs;
};

struct FootIKLockerClipData
{
  eastl::bitvector<> lockStates;
  bool needLock(int leg_id, int frame_id) const
  {
    return lockStates.empty() ? false : lockStates[frame_id * FootIKLockerNodes::NUM_LEGS + leg_id];
  }
};

void load_foot_ik_locker_states(ecs::EntityId data_base_eid,
  FootIKLockerClipData &out,
  const AnimationClip &clip,
  const DataBlock &clip_block,
  const GeomNodeTree &tree);
FootIKLockerNodes get_foot_ik_locker_nodes(ecs::EntityId data_base_eid, const GeomNodeTree &tree);

struct FootIKLockerControllerState
{
  struct LegState
  {
    bool isLocked = false;
    bool lockAllowed = false;
    vec3f lockedPosition = v_zero();
    vec3f posOffset = v_zero();
  };
  eastl::array<LegState, FootIKLockerNodes::NUM_LEGS> legs = {};
};

ECS_DECLARE_RELOCATABLE_TYPE(FootIKLockerNodes);
