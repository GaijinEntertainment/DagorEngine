// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <backend/resourceScheduling/heapLayout.h>


namespace layout_test
{

inline int FAKE_GROUP_STORAGE;
inline ResourceHeapGroup *const GROUP = reinterpret_cast<ResourceHeapGroup *>(&FAKE_GROUP_STORAGE);

inline const dafg::HeapClass TRACKED{
  .group = GROUP,
  .untracked = false,
};

inline int FAKE_GROUP_2_STORAGE;
inline ResourceHeapGroup *const GROUP_2 = reinterpret_cast<ResourceHeapGroup *>(&FAKE_GROUP_2_STORAGE);

inline const dafg::HeapClass TRACKED_2{
  .group = GROUP_2,
  .untracked = false,
};

inline constexpr size_t RESOURCE_COUNT = 8;

inline constexpr uint32_t TIMEPOINTS = 10;
inline constexpr uint64_t ALIGN = 256;
inline constexpr uint64_t KB = 1024;

inline const dafg::Timeline TIMELINE{
  .timepointsPerFrame = TIMEPOINTS,
};

inline dafg::ResourceLifetime make_lifetime(uint32_t first_frame, uint32_t first, uint32_t release_frame, uint32_t release)
{
  return {
    .firstUse =
      {
        .frame = first_frame,
        .timepoint = first,
      },
    .release =
      {
        .frame = release_frame,
        .timepoint = release,
      },
  };
}

inline dafg::ResourceLifetime live_within(uint32_t frame, uint32_t first, uint32_t release)
{
  return make_lifetime(frame, first, frame, release);
}

inline dafg::ResourceLifetime live_across_frames(uint32_t first, uint32_t release) { return make_lifetime(0, first, 1, release); }

inline dafg::FrameResource make_frame_resource(uint32_t res, uint32_t frame = 0)
{
  return {
    .resIdx = static_cast<dafg::intermediate::ResourceIndex>(res),
    .frame = frame,
  };
}

inline dafg::LayoutRect make_rect(uint32_t res, uint64_t offset, uint64_t size, dafg::ResourceLifetime lifetime, uint32_t frame = 0)
{
  return {
    .what = make_frame_resource(res, frame),
    .offset = offset,
    .size = size,
    .align = ALIGN,
    .lifetime = lifetime,
    .hash = res,
  };
}

inline dafg::HeapIndex add_heap(dafg::Layout &layout, dafg::HeapClass heap_class, uint64_t reserved_size,
  eastl::span<const dafg::LayoutRect> rects)
{
  layout.reserveResources(RESOURCE_COUNT);
  const auto idx = layout.addHeap(heap_class);
  layout.heaps[idx].reservedSize = reserved_size;
  for (const auto &r : rects)
    layout.add(idx, r);
  return idx;
}

inline const dafg::LayoutHeap &get_heap_at(const dafg::Layout &layout, uint32_t index)
{
  return layout.heaps[static_cast<dafg::HeapIndex>(index)];
}

} // namespace layout_test
