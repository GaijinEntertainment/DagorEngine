// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <memory/dag_framemem.h>

#include <backend/resourceScheduling/layoutUpdate.h>


namespace dafg
{

struct HeapPolicySettings
{
  uint64_t defragmentMinWaste = 16u << 20;
  uint64_t relocationMaxBytes = 64u << 20;
  uint32_t defragmentDelayFrames = 300;
  uint32_t defragmentCheckIntervalFrames = 60;
  uint32_t defragmentMaxWaitFrames = 600;
  float defragmentPlacedAnewFraction = 0.5f;
  float defragmentWasteFraction = 0.25f;
  float defragmentWasteFractionStop = 0.15f;
  float relocationWasteRatio = 1.f;
  float defragmentHeadroom = 0.25f;
};

struct DefragmentationDecision
{
  bool defragment = false;
  bool relocate = false;
  bool sizeToContent = false;
};

// decide_defragmentation writes only the check and delay frames. The caller
// owns the waste left after a defragmentation, and changes the pending decision
// only through arm_defragmentation and disarm_defragmentation.
struct DefragmentationState
{
  int64_t lastCheckFrame = eastl::numeric_limits<int64_t>::min() / 2;
  int64_t wastefulSinceFrame = -1;
  int64_t pendingSince = -1;
  uint64_t recoverableAfterDefragmentation = 0;
  DefragmentationDecision pending{};
};

inline void arm_defragmentation(DefragmentationState &state, const DefragmentationDecision &decision, uint32_t frame)
{
  state.pending = decision;
  state.pendingSince = decision.defragment ? static_cast<int64_t>(frame) : -1;
}

inline DefragmentationDecision disarm_defragmentation(DefragmentationState &state)
{
  state.pendingSince = -1;
  return eastl::exchange(state.pending, DefragmentationDecision{});
}

inline bool is_pending_overdue(const DefragmentationState &state, const HeapPolicySettings &settings, uint32_t frame)
{
  return state.pending.defragment && static_cast<int64_t>(frame) - state.pendingSince >= settings.defragmentMaxWaitFrames;
}

struct HeapWaste
{
  HeapIndex heap = HeapIndex::Invalid;
  bool pinned = false;
  uint64_t free = 0;
  uint64_t locked = 0;
  uint64_t content = 0;

  uint64_t recoverable(bool relocate) const { return free + (relocate ? locked : 0); }
};

// `free` comes back without moving preserved rects, `locked` only by moving them
struct ClassWaste
{
  uint64_t free = 0;
  uint64_t locked = 0;
  uint64_t preservedBytes = 0;
  dag::Vector<HeapWaste, framemem_allocator> heaps;

  uint64_t recoverable(bool relocate) const { return free + (relocate ? locked : 0); }
};

// Every heap when no smaller subset is enough. Empty when nothing is worth
// reclaiming.
dag::Vector<HeapIndex, framemem_allocator> select_heaps_to_defragment(const ClassWaste &waste, uint64_t reserved_now, bool relocate,
  const HeapPolicySettings &settings);

uint64_t measure_defragmentation_burst(const ClassWaste &waste, eastl::span<const HeapIndex> chosen, bool relocate);

inline bool is_burst_affordable(uint64_t burst, uint64_t free_budget, const HeapPolicySettings &settings)
{
  const uint64_t headroom = static_cast<uint64_t>(static_cast<double>(burst) * settings.defragmentHeadroom);
  return burst <= free_budget && headroom <= free_budget - burst;
}

inline bool should_defragment_on_placed_anew(size_t in_layout, size_t placed_anew, const HeapPolicySettings &settings)
{
  const size_t total = in_layout + placed_anew;
  return total > 0 && static_cast<float>(placed_anew) >= settings.defragmentPlacedAnewFraction * static_cast<float>(total);
}

inline void reserve_for_ghosts(ClassWaste &waste, uint64_t ghost_bytes)
{
  const uint64_t fromFree = eastl::min(ghost_bytes, waste.free);
  waste.free -= fromFree;
  waste.locked -= eastl::min(ghost_bytes - fromFree, waste.locked);
}

inline bool should_release_empty_class(bool empty, uint64_t ghost_bytes) { return empty && ghost_bytes == 0; }

inline bool is_relocation_worth_it(const ClassWaste &waste, const HeapPolicySettings &settings)
{
  return waste.preservedBytes > 0 && waste.preservedBytes <= settings.relocationMaxBytes &&
         waste.locked >= settings.defragmentMinWaste &&
         static_cast<float>(waste.locked) >= settings.relocationWasteRatio * static_cast<float>(waste.preservedBytes);
}

inline bool is_defragmentation_check_due(const DefragmentationState &state, const HeapPolicySettings &settings, uint32_t frame)
{
  return static_cast<int64_t>(frame) - state.lastCheckFrame >= settings.defragmentCheckIntervalFrames;
}

// When a resize keeps the contents of a heap, a preserved rect locks no waste
ClassWaste measure_recoverable_waste(const Layout &layout, HeapClass heap_class, const Preservation &preservation,
  bool resize_keeps_contents = false);

inline bool is_class_wasteful(const DefragmentationState &state, uint64_t reserved_now, const ClassWaste &waste, bool relocate,
  const HeapPolicySettings &settings, bool started)
{
  const uint64_t recoverable = waste.recoverable(relocate);
  const float fraction = started ? settings.defragmentWasteFractionStop : settings.defragmentWasteFraction;
  return recoverable > state.recoverableAfterDefragmentation && recoverable >= settings.defragmentMinWaste &&
         recoverable >= static_cast<uint64_t>(reserved_now * fraction);
}

DefragmentationDecision decide_defragmentation(DefragmentationState &state, uint64_t reserved_now, const ClassWaste &waste,
  const HeapPolicySettings &settings, uint32_t frame);

} // namespace dafg
