// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/bitvector.h>


struct AnimationDataBase;
struct AnimationClip;
class DataBlock;

struct FootLockerClipData
{
  eastl::bitvector<> lockStates;
  bool needLock(int leg_id, int frame_id, int num_legs) const
  {
    return lockStates.empty() ? false : lockStates[frame_id * num_legs + leg_id];
  }
};

void load_foot_locker_states(
  const AnimationDataBase &database, FootLockerClipData &out, const AnimationClip &clip, const DataBlock &clip_block);
