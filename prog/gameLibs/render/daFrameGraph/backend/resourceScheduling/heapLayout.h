// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/array.h>
#include <EASTL/span.h>
#include <dag/dag_vector.h>
#include <dag/dag_vectorMap.h>

#include <id/idIndexedMapping.h>
#include <id/idIndexedFlags.h>
#include <backend/intermediateRepresentation.h>
#include <backend/resourceScheduling/resourceLifetimes.h>
#include <backend/resourceScheduling/resourceSchedulingTypes.h>


namespace dafg
{

// An arc of the frame ring whose end is not after its start wraps the ring
bool do_arcs_overlap(uint32_t a_start, uint32_t a_end, uint32_t b_start, uint32_t b_end);

struct Timeline
{
  uint32_t timepointsPerFrame = 0;

  uint32_t length() const { return SCHEDULE_FRAME_WINDOW * timepointsPerFrame; }
  uint32_t moment(LifetimePoint point) const { return point.frame * timepointsPerFrame + point.timepoint; }
  uint32_t duration(const ResourceLifetime &lifetime) const;

  bool overlap(const ResourceLifetime &a, const ResourceLifetime &b) const;
};

struct HeapClass
{
  ResourceHeapGroup *group = nullptr;
  // Aliasing barriers of untracked resources are only synced against other
  // untracked ones, so tracked and untracked resources never share a heap.
  bool untracked = false;

  bool operator==(const HeapClass &other) const = default;
};

struct HeapClassLess
{
  bool operator()(const HeapClass &a, const HeapClass &b) const
  {
    return a.group != b.group ? eastl::less<ResourceHeapGroup *>{}(a.group, b.group) : a.untracked < b.untracked;
  }
};

struct LayoutRect
{
  FrameResource what{};
  uint64_t offset = 0;
  uint64_t size = 0;
  uint64_t align = 1;
  ResourceLifetime lifetime{};
  ResourceDescription::HashT hash = 0;

  uint64_t end() const { return offset + size; }
};

inline bool spans_frames(const ResourceLifetime &lifetime) { return lifetime.firstUse.frame != lifetime.release.frame; }

struct WantedRect
{
  FrameResource what{};
  uint64_t size = 0;
  uint64_t align = 1;
  ResourceLifetime lifetime{};
  ResourceDescription::HashT hash = 0;
  HeapClass heapClass{};
  // Invalid when the frame resource itself changed. The allocator then places
  // a new object, even at the same offset.
  AllocationLocation wasAt = NOWHERE;
};

struct LayoutHeap
{
  HeapClass heapClass{};
  uint64_t reservedSize = 0;
  dag::Vector<LayoutRect> rects;
  // History rects, by the frame of their frame resource
  eastl::array<uint32_t, SCHEDULE_FRAME_WINDOW> spanning{};
};

struct LocationLookup
{
  eastl::array<IdIndexedMapping<intermediate::ResourceIndex, AllocationLocation>, SCHEDULE_FRAME_WINDOW> locationOf;

  AllocationLocation operator[](FrameResource what) const
  {
    return locationOf[what.frame].isMapped(what.resIdx) ? locationOf[what.frame][what.resIdx] : NOWHERE;
  }
  void put(FrameResource what, AllocationLocation location)
  {
    locationOf[what.frame].expandMapping(what.resIdx, NOWHERE);
    locationOf[what.frame][what.resIdx] = location;
  }
};

using LayoutHeaps = IdIndexedMapping<HeapIndex, LayoutHeap>;
using HeapsOfClass = dag::VectorMap<HeapClass, dag::Vector<HeapIndex>, HeapClassLess>;

// Rects of a heap stay sorted by offset.
struct Layout
{
  LayoutHeaps heaps;
  HeapsOfClass heapsOfClass;
  LocationLookup where;
  eastl::array<IdIndexedFlags<intermediate::ResourceIndex>, SCHEDULE_FRAME_WINDOW> spanning;

  // Reuses a heap slot that has no rects and no memory. A batch of new heaps
  // passes the slots it already took in `in_use`.
  HeapIndex addHeap(HeapClass heap_class, eastl::span<const HeapIndex> in_use = {});
  eastl::span<const HeapIndex> heapsOf(HeapClass heap_class) const;

  void reserveResources(size_t resource_count);

  bool has(FrameResource what) const { return where[what].heap != HeapIndex::Invalid; }
  const LayoutRect *find(FrameResource what) const;
  LayoutRect *find(FrameResource what);

  void add(HeapIndex heap, const LayoutRect &rect);
  // The caller also puts the rect into the rects of the heap
  void track(HeapIndex heap, const LayoutRect &rect);
  LayoutRect take(FrameResource what);
  void setLifetime(FrameResource what, const ResourceLifetime &lifetime);
};

template <class Visit>
void for_each_untracked_alias(const Layout &layout, const Visit &visit)
{
  for (const auto &heap : layout.heaps)
  {
    if (!heap.heapClass.untracked)
      continue;
    for (size_t i = 0; i < heap.rects.size(); ++i)
      for (size_t j = i + 1; j < heap.rects.size() && heap.rects[j].offset < heap.rects[i].end(); ++j)
        visit(heap.rects[i].what, heap.rects[j].what);
  }
}

// Frame resources whose object the allocator must place again or drop
using PlacementsChanged = eastl::array<IdIndexedFlags<intermediate::ResourceIndex>, SCHEDULE_FRAME_WINDOW>;

WantedRect convert_to_wanted(const LayoutRect &rect, HeapClass heap_class, AllocationLocation was_at = NOWHERE);

uint64_t measure_used_size(const LayoutHeap &heap);

size_t count_rects(const Layout &layout, HeapClass heap_class);

bool is_layout_valid(const Layout &layout, const Timeline &timeline);

} // namespace dafg
