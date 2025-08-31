// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <backend/resourceScheduling/packer.h>

#include <debug/dag_assert.h>
#include <memory/dag_framemem.h>
#include <dag/dag_vector.h>
#include <dag/dag_vectorSet.h>
#include <generic/dag_sort.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/numeric_limits.h>
#include <EASTL/optional.h>
#include <EASTL/numeric.h>
#include <EASTL/sort.h>
#include <EASTL/set.h>


namespace dafg
{

using MemoryOffset = uint64_t;
using MemorySize = uint64_t;
using TimePoint = uint32_t;

struct Allocation
{
  MemoryOffset offset;
  MemorySize size;
  // Those variables are a trick that is central to enabling wraparound
  // and pinned resource allocation. When allocation is freed, it shall only be
  // reused for resources who's lifetime ends before availableUntil.
  TimePoint availableUntil;

  static constexpr TimePoint ALWAYS_AVAILABLE = eastl::numeric_limits<TimePoint>::max();
};

struct GreedyAllocator
{
  GreedyAllocator(MemorySize maxHeapSize) : maxHeapSize(maxHeapSize) {}
  // Result guaranteed to fit resource with proper alignment
  // if heap has enough space.
  EA_NODISCARD Allocation allocate(const PackerInput::Resource &resource);
  // NOTE: cannot be called after regular allocate AND must be called in
  // ascending pinned offset order.
  EA_NODISCARD Allocation allocatePinned(const PackerInput::Resource &resource);
  void deallocate(Allocation alloc);
  EA_NODISCARD MemorySize heapSize() const { return totalHeapSize; }

  bool isEmpty() const { return currentHeapSize == 0; }

private:
  static constexpr MemorySize ALLOCATION_SPLIT_THRESHOLD = 4096;

  struct AllocSizeCmp
  {
    bool operator()(const Allocation &a, const Allocation &b) { return a.size < b.size; }
  };

  using FreeList = eastl::multiset<Allocation, AllocSizeCmp, framemem_allocator>;
  FreeList freeList;

  struct AllocOffsetCmp
  {
    bool operator()(const FreeList::iterator &a, const FreeList::iterator &b) { return a->offset < b->offset; }
  };
  dag::VectorSet<FreeList::iterator, AllocOffsetCmp, framemem_allocator> freeListByOffset;

  MemorySize currentHeapSize = 0;
  MemorySize totalHeapSize = 0;
  MemorySize maxHeapSize = 0;
};

struct Segment
{
  uint32_t start;
  uint32_t end;
};

static bool is_wrapping(const Segment &seg) { return seg.end <= seg.start; }

static bool is_wrapping(const PackerInput::Resource &res) { return res.end <= res.start; }

// Non-wrapping with non-wrapping case
static bool segments_disjoint_nwnw(Segment a, Segment b) { return a.end <= b.start || b.end <= a.start; }

// Wrapping with non-wrapping case
static bool segments_disjoint_wnw(Segment a, Segment b) { return a.end <= b.start && b.end <= a.start; }

static bool segments_disjoint(Segment a, Segment b)
{
  if (is_wrapping(b))
    eastl::swap(a, b);

  if (is_wrapping(b))
    return false;

  if (is_wrapping(a))
    return segments_disjoint_wnw(a, b);
  else
    return segments_disjoint_nwnw(a, b);
}

struct GreedyScanlinePacker
{
  template <class T>
  using OverflowAlloc = eastl::fixed_vector_allocator<sizeof(T), 32, alignof(T), 0, true, framemem_allocator>;

  template <class T>
  using FixedFramememVector = dag::Vector<T, OverflowAlloc<T>>;

  template <class T>
  using FramememVector = dag::Vector<T, framemem_allocator>;

  enum class EvType : uint32_t
  {
    Start,
    End
  };
  enum class ResType : uint32_t
  {
    Normal,
    Pinned
  };
  struct Event
  {
    uint32_t resIndex : 30;
    EvType evType : 1;
    ResType resType : 1;
  };

  using Timeline = FramememVector<Event>;

  FixedFramememVector<uint32_t> findWrapping(eastl::span<const PackerInput::Resource> resources)
  {
    FixedFramememVector<uint32_t> result;
    for (uint32_t i = 0; i < resources.size(); ++i)
      if (resources[i].size > 0 && resources[i].align > 0 && is_wrapping(resources[i]) && resources[i].pin == PackerInput::NO_PIN)
        result.push_back(i);
    return result;
  }

  FixedFramememVector<uint32_t> findPinned(eastl::span<const PackerInput::Resource> resources)
  {
    FixedFramememVector<uint32_t> result;
    for (uint32_t i = 0; i < resources.size(); ++i)
      if (resources[i].pin != PackerInput::NO_PIN)
        result.push_back(i);
    return result;
  }

  PackerOutput operator()(PackerInput input)
  {
    FRAMEMEM_REGION;

    const Timeline timeline = generateTimeline(input);

    auto wrappingRes = findWrapping(input.resources);

    // Heuristic: sort by start so that the available area is less jagged
    // Bad: fragmentation
    //    start     split
    //      v         v
    //      [         |         )
    //              [ |       )
    //   [            |         )
    //              [ |       )
    //        [       |         )
    fast_sort(wrappingRes, [&input](uint32_t a, uint32_t b) { return input.resources[a].start > input.resources[b].start; });

    auto pinnedResources = findPinned(input.resources);
    fast_sort(pinnedResources, [&input](uint32_t a, uint32_t b) { return input.resources[a].pin < input.resources[b].pin; });

    return scanlinePack(timeline, wrappingRes, pinnedResources, input);
  }

  PackerOutput scanlinePack(const Timeline &timeline, eastl::span<uint32_t> wrappingResources, eastl::span<uint32_t> pinnedResources,
    const PackerInput &input)
  {
    GreedyAllocator allocator(input.maxHeapSize);

    FramememVector<eastl::optional<Allocation>> resourceAllocations(input.resources.size());

    for (auto idx : pinnedResources)
      resourceAllocations[idx] = allocator.allocatePinned(input.resources[idx]);

    for (uint32_t idx : wrappingResources)
      resourceAllocations[idx] = allocator.allocate(input.resources[idx]);

    // Just in case
    calculatedOffsets.clear();
    calculatedOffsets.resize(input.resources.size(), PackerOutput::NOT_ALLOCATED);

    for (auto [resIdx, evType, resType] : timeline)
    {
      switch (evType)
      {
        case EvType::End:
        {
          G_ASSERT_CONTINUE(resourceAllocations[resIdx].has_value());
          Allocation allocation = *resourceAllocations[resIdx];

          // When the allocator fails to allocate space for a resource due to
          // max heap size constraint, it returns a zero size/offset allocation.
          // Hence we simply set the resulting offset to NOT_SCHEDULED here.
          if (DAGOR_LIKELY(allocation.size == 0 && allocation.offset == 0))
          {
            G_ASSERT(resType != ResType::Pinned);
            calculatedOffsets[resIdx] = PackerOutput::NOT_SCHEDULED;
            continue;
          }

          // Pinned resources might be contained within allocations with
          // padding, so we don't use the actual allocation offset but the initial pin.
          if (resType == ResType::Normal)
            calculatedOffsets[resIdx] = input.resources[resIdx].doAlign(allocation.offset);
          else
            calculatedOffsets[resIdx] = input.resources[resIdx].pin;

          allocator.deallocate(allocation);
          break;
        }

        case EvType::Start:
        {
          // Pinned & wrapping resources are already allocated
          if (EA_LIKELY(resType == ResType::Normal && !is_wrapping(input.resources[resIdx])))
            resourceAllocations[resIdx] = allocator.allocate(input.resources[resIdx]);
          break;
        }
      }
    }

    PackerOutput result;
    result.offsets = calculatedOffsets;
    result.heapSize = allocator.heapSize();
    return result;
  }

  Timeline generateTimeline(const PackerInput &input)
  {
    Timeline result;
    result.reserve(2 * input.resources.size());

    struct EvWithInfo
    {
      Event event;
      TimePoint time;
      MemorySize size;
    };
    FramememVector<EvWithInfo> events;
    events.reserve(2 * input.resources.size());
    for (uint32_t i = 0; i < input.resources.size(); ++i)
    {
      const auto &res = input.resources[i];
      const bool pinned = res.pin != PackerInput::NO_PIN;
      if (res.size == 0)
      {
        // Pinning a 0 size is an ew edge case
        G_ASSERT(!pinned);
        continue;
      }
      G_ASSERT_CONTINUE(res.align != 0);
      G_ASSERT_CONTINUE(!pinned || res.end <= res.start);
      events.push_back({Event{i, EvType::Start, pinned ? ResType::Pinned : ResType::Normal}, res.start, res.size});
      events.push_back({Event{i, EvType::End, pinned ? ResType::Pinned : ResType::Normal}, res.end, res.size});
    }

    fast_sort(events, [](const EvWithInfo &fst, const EvWithInfo &snd) {
      if (fst.time != snd.time)
        return fst.time < snd.time;
      // Ends go before starts to ensure that resources are deallocated
      // before new memory is attempted to be allocated
      if (fst.event.evType != snd.event.evType)
        return fst.event.evType == EvType::End && snd.event.evType == EvType::Start;

      // It is preferable to allocate fat resources first because otherwise
      // small ones might fill in gaps which big resources could've fit in
      return fst.size > snd.size;
    });

    for (const auto &[ev, t, s] : events)
      result.push_back(ev);

    return result;
  }

private:
  // NOTE: this intentionally does not use framemem allocator,
  // we wouldn't be able to wipe memory after packing is done
  // otherwise.
  dag::Vector<MemoryOffset> calculatedOffsets;
};

Allocation GreedyAllocator::allocate(const PackerInput::Resource &res)
{
  auto candidate = freeList.lower_bound(Allocation{0, res.size, 0});

  for (; candidate != freeList.end(); ++candidate)
  {
    const TimePoint availUntil = candidate->availableUntil;

    const bool sizeFits = res.sizeWithPadding(candidate->offset) <= candidate->size;
    if (!sizeFits)
      continue;

    const bool available =
      availUntil == Allocation::ALWAYS_AVAILABLE || segments_disjoint(Segment{res.start, res.end}, Segment{availUntil, 0});

    if (!available)
      continue;

    break;
  }

  // No luck, expand heap
  if (candidate == freeList.end())
  {
    const MemorySize allocSize = res.sizeWithPadding(currentHeapSize);

    // Extra bad luck, no heap space left
    if (DAGOR_UNLIKELY(allocSize > maxHeapSize || currentHeapSize > maxHeapSize - allocSize))
    {
      Allocation emptyResult{0, 0, Allocation::ALWAYS_AVAILABLE};
      return emptyResult;
    }

    Allocation result{currentHeapSize, allocSize, is_wrapping(res) ? res.start : Allocation::ALWAYS_AVAILABLE};

    currentHeapSize += result.size;

    totalHeapSize = eastl::max(currentHeapSize, totalHeapSize);
    return result;
  }

  // If the allocation block is a lot bigger than the resource
  // size, we can cut off the end of the block and put it back.
  const auto cutAllocationHigh = [this](const PackerInput::Resource &res, Allocation &result) {
    const MemorySize usedSpace = res.sizeWithPadding(result.offset);
    if (result.size - usedSpace >= ALLOCATION_SPLIT_THRESHOLD)
    {
      Allocation leftover = result;
      leftover.offset += usedSpace;
      leftover.size -= usedSpace;

      deallocate(leftover);

      result.size = usedSpace;
    }
  };

  Allocation result = *candidate;
  freeListByOffset.erase(candidate);
  freeList.erase(candidate);

  cutAllocationHigh(res, result);

  if (is_wrapping(res))
  {
    result.availableUntil = res.start;
  }
  return result;
}

Allocation GreedyAllocator::allocatePinned(const PackerInput::Resource &res)
{
  // Only sequential allocation allowed, this is sort of a "setup" for the allocator
  G_ASSERT(currentHeapSize <= res.pin);

  Allocation result{res.pin, res.size, res.start};

  const MemorySize padBefore = res.pin - currentHeapSize;
  if (padBefore >= ALLOCATION_SPLIT_THRESHOLD)
  {
    freeListByOffset.insert(freeList.insert(Allocation{currentHeapSize, res.pin - currentHeapSize, Allocation::ALWAYS_AVAILABLE}));
  }
  else
  {
    result.offset = currentHeapSize;
    result.size += padBefore;
  }

  totalHeapSize = currentHeapSize = res.pin + res.size;

  return result;
}

static bool allocs_mergeable(const Allocation &a, const Allocation &b)
{
  // NOTE: if both chunks are available currently, we merge them
  return a.offset + a.size == b.offset && a.availableUntil == b.availableUntil;
}

void GreedyAllocator::deallocate(Allocation alloc)
{
  // Basically go through nodes with close offsets in order
  // and merge them with the current one if they are completely
  // adjacent. STL crimes follow.

  auto it = freeList.insert(alloc);
  const auto lb = freeListByOffset.lower_bound(it);
  freeList.erase(it);

  auto runStart = eastl::make_reverse_iterator(lb);
  while (runStart != freeListByOffset.rend() && allocs_mergeable(**runStart, alloc))
  {
    alloc.offset = (*runStart)->offset;
    alloc.size += (*runStart)->size;
    ++runStart;
  }
  auto runEnd = lb;
  while (runEnd != freeListByOffset.end() && allocs_mergeable(alloc, **runEnd))
  {
    alloc.size += (*runEnd)->size;
    ++runEnd;
  }

  for (auto i = runStart.base(); i != runEnd; ++i)
    freeList.erase(*i);
  freeListByOffset.erase(runStart.base(), runEnd);

  // Drop allocations that are at the end of memory and **don't have
  // availability restrictions**.
  if (alloc.offset + alloc.size == currentHeapSize && alloc.availableUntil == Allocation::ALWAYS_AVAILABLE)
    currentHeapSize -= alloc.size;
  else
    freeListByOffset.insert(freeList.insert(alloc));
}


Packer make_greedy_scanline_packer() { return GreedyScanlinePacker{}; }

} // namespace dafg
