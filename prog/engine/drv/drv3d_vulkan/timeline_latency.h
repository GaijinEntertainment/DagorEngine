// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// sometimes we don't want to wait for specific jobs, saving tracking memory and CPU time
// in this cases we rely on system wide timeline logic that have defined latency and dependencies
// this module helps working with this logic on higher level
// avoiding comparative errors and clutter

namespace drv3d_vulkan
{

struct TimelineLatency
{
  // common logic
  //   for completion of current vs old
  static bool isCompleted(size_t old_id, size_t current_id) { return old_id < current_id; }
  //   for completed index from current with latency
  static size_t getLastCompleted(size_t current_id, const size_t latency)
  {
    if (current_id < (latency + 1))
      return 0;
    return current_id - latency - 1;
  }
  //   next index of work on which ring buffered resource used in current work may be reused
  static size_t nextRingedIndex(size_t current_id, const size_t latency) { return current_id + latency; }

  // replay latency and helpers

  // shows latency between replay work and its GPU compeletion
  // if resource used on replay x, it may be reused after frame x + replayToGPUCompletion
  static constexpr size_t replayToGPUCompletion = GPU_TIMELINE_HISTORY_SIZE + MAX_PENDING_REPLAY_ITEMS;
  // for latency x we need x+1 elements to form "reuse" ring buffer
  static constexpr size_t replayToGPUCompletionRingBufferSize = replayToGPUCompletion + 1;

  static bool isReplayCompleted(size_t replay_id, size_t current_replay_id)
  {
    return isCompleted(replay_id + replayToGPUCompletion, current_replay_id);
  }
  static size_t getLastCompletedReplay(size_t replay_id) { return getLastCompleted(replay_id, replayToGPUCompletion); }
  static size_t getNextRingedReplay(size_t replay_id) { return nextRingedIndex(replay_id, replayToGPUCompletion); }
};

} // namespace drv3d_vulkan
