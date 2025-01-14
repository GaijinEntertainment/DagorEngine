// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <backend/resourceScheduling/packer.h>

#include <debug/dag_assert.h>
#include <memory/dag_framemem.h>
#include <dag/dag_vector.h>
#include <util/dag_stlqsort.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/numeric_limits.h>
#include <EASTL/optional.h>
#include <EASTL/numeric.h>
#include <EASTL/sort.h>
#include <EASTL/set.h>


namespace dabfg
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
  // reused for resources who's lifetime doesn't interesect
  // with [availableUntil, availableSince) interval.
  TimePoint availableUntil;
  TimePoint availableSince;

  static constexpr TimePoint ALWAYS_AVAILABLE = eastl::numeric_limits<TimePoint>::max();
};

static bool allocs_mergeable(const Allocation &a, const Allocation &b)
{
  return a.offset + a.size == b.offset && a.availableUntil == b.availableUntil && a.availableSince == b.availableSince;
}

struct GreedyAllocator
{
  GreedyAllocator(MemorySize maxHeapSize) : maxHeapSize(maxHeapSize) {}
  // Result guaranteed to fit resource with proper alignment
  // if heap has enough space.
  EA_NODISCARD Allocation allocate(const PackerInput::Resource &resource, TimePoint splitTime);
  void deallocate(Allocation alloc);
  EA_NODISCARD MemorySize heapSize() const { return totalHeapSize; }

  bool isEmpty() const { return currentHeapSize == 0; }

private:
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
  eastl::set<FreeList::iterator, AllocOffsetCmp, framemem_allocator> freeListByOffset;

  MemorySize currentHeapSize = 0;
  MemorySize totalHeapSize = 0;
  MemorySize maxHeapSize = 0;
};

struct Segment
{
  uint32_t start;
  uint32_t end;
};

// Imagine that we cyclically shifted time by `-time`. This
// function returns true iff a resources would be a wrap-around one
// in such a scenario.
static bool is_wrapping(const Segment &seg, TimePoint time)
{
  if (seg.start < seg.end)
    // {     [     |     )        }
    return seg.start < time && time <= seg.end;
  else
    // {       )          [  |    }
    return time <= seg.end || seg.start < time;
}

static bool is_wrapping(const PackerInput::Resource &res, TimePoint time) { return is_wrapping(Segment{res.start, res.end}, time); }

// Non-wrapping with non-wrapping case
static bool segments_disjoint_nwnw(Segment a, Segment b) { return a.end <= b.start || b.end <= a.start; }

// Wrapping with non-wrapping case
static bool segments_disjoint_wnw(Segment a, Segment b) { return a.end <= b.start && b.end <= a.start; }

static bool is_wrapping(Segment s) { return s.start >= s.end; }

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

static bool is_hinted(const PackerInput::Resource &res) { return res.offsetHint != PackerInput::NO_HINT; }

// Cheaper than operator%, but only works when time+offset < timelineSize*2
static TimePoint shift_by_offset(TimePoint time, TimePoint offset, TimePoint timelineSize)
{
  return offset + time < timelineSize ? offset + time : offset + time - timelineSize;
}

struct GreedyScanlinePacker
{
  template <class T>
  using OverflowAlloc = eastl::fixed_vector_allocator<sizeof(T), 32, alignof(T), 0, true, framemem_allocator>;

  template <class T>
  using FixedFramememVector = dag::Vector<T, OverflowAlloc<T>>;

  template <class T>
  using FramememVector = dag::Vector<T, framemem_allocator>;

  struct TimeMoment
  {
    // NOTE: memory from resources that ended can be reused on the same
    // time moment!
    FixedFramememVector<uint32_t> endingResources;
    FixedFramememVector<uint32_t> startingResources;
  };

  using Timeline = FramememVector<TimeMoment>;

  PackerOutput operator()(PackerInput input)
  {
    FRAMEMEM_REGION;

    // Split time is chosen so that a minimal amount of resources
    // are active at that time.
    const TimePoint splitTime = calculateBestSplittingTime(input);
    const Timeline timeline = generateTimeline(input);

    auto wrappingRes = findWrapping(input.resources, splitTime);
    auto pinnedRes = findPinned(input.resources);
    // Heuristic: sort by start so that the available area is less jagged

    // Bad: fragmentation
    //    start     split
    //      v         v
    //      [         |         )
    //              [ |       )
    //   [            |         )
    //              [ |       )
    //        [       |         )
    stlsort::sort(wrappingRes.begin(), wrappingRes.end(),
      [&input, minusTime = input.timelineSize - splitTime](uint32_t a, uint32_t b) {
        const TimePoint aStartShifted = shift_by_offset(input.resources[a].start, minusTime, input.timelineSize);
        const TimePoint bStartShifted = shift_by_offset(input.resources[b].start, minusTime, input.timelineSize);
        return aStartShifted > bStartShifted;
      });
    stlsort::sort(pinnedRes.begin(), pinnedRes.end(),
      [&input](uint32_t a, uint32_t b) { return input.resources[a].offsetHint < input.resources[b].offsetHint; });
    return scanlinePack(timeline, splitTime, wrappingRes, pinnedRes, input);
  }

  PackerOutput scanlinePack(const Timeline &timeline, TimePoint startTime, eastl::span<uint32_t> wrappingResources,
    eastl::span<uint32_t> pinnedRes, const PackerInput &input)
  {
    GreedyAllocator allocator(input.maxHeapSize);

    FramememVector<eastl::optional<Allocation>> resourceAllocations(input.resources.size());

    // Firstly pinned resources are allocated to ensure
    // that their offset hints are not taken by other resources.
    // Then we allocate resources that intersect the split time upfront.
    // Pinned and wrapping resources create allocation before
    // timeline iteration, so they have [availableUntil, availableSince) interval constrain,
    // therefore avoiding other resources accidentally overlapping with them.
    // And because of this interval, we can deallocate immediately after allocation.
    for (uint32_t idx : pinnedRes)
    {
      const auto allocation = allocator.allocate(input.resources[idx], startTime);
      resourceAllocations[idx] = allocation;
      allocator.deallocate(allocation);
    }
    for (uint32_t idx : wrappingResources)
    {
      if (!is_hinted(input.resources[idx]))
      {
        const auto allocation = allocator.allocate(input.resources[idx], startTime);
        resourceAllocations[idx] = allocation;
        allocator.deallocate(allocation);
      }
    }

    // Just in case
    calculatedOffsets.clear();
    calculatedOffsets.resize(input.resources.size(), PackerOutput::NOT_ALLOCATED);

    for (TimePoint i = 0; i < timeline.size(); ++i)
    {
      const TimePoint time = shift_by_offset(i, startTime, timeline.size());

      const auto &moment = timeline[time];

      // Deallocate old resources first, freeing space, then allocate
      // new ones.
      for (uint32_t endingIndex : moment.endingResources)
      {
        const auto &res = input.resources[endingIndex];

        auto allocation = eastl::move(resourceAllocations[endingIndex]);
        G_ASSERT_CONTINUE(allocation.has_value());

        // When the allocator fails to allocate space for a resource due to
        // max heap size constraint, it returns a zero size/offset allocation.
        // Hence we simply set the resulting offset to NOT_SCHEDULED here.
        if (DAGOR_LIKELY(allocation->size != 0 || allocation->offset != 0))
        {
          // If allocation respects hint, assign it as output offset.
          if (is_hinted(res) &&
              DAGOR_LIKELY(res.sizeWithHint(allocation->offset) <= allocation->size && res.offsetHint >= allocation->offset))
          {
            calculatedOffsets[endingIndex] = res.offsetHint;
          }
          else
          {
            calculatedOffsets[endingIndex] = res.doAlign(allocation->offset);
          }
          // Wrapping and pinned resources were already deallocated and
          // their allocations have availability interval
          if (!is_hinted(res) && !is_wrapping(res, startTime))
            allocator.deallocate(*allocation);
        }
        else
        {
          calculatedOffsets[endingIndex] = PackerOutput::NOT_SCHEDULED;
        }
      }

      for (uint32_t startingIndex : moment.startingResources)
      {
        const auto &res = input.resources[startingIndex];
        // Wrapping and pinned resources were already allocated, a repeated
        // allocation might place them in a different location, or even
        // new memory, therefore increasing totalHeapSize without it
        // actually being used for anything.
        if (EA_LIKELY(!is_wrapping(res, startTime) && !is_hinted(res)))
          resourceAllocations[startingIndex] = allocator.allocate(res, startTime);
      }
    }

    PackerOutput result;
    result.offsets = calculatedOffsets;
    result.heapSize = allocator.heapSize();
    return result;
  }

  // Finds all resources that are wrapping w.r.t. offset `-time`
  FixedFramememVector<uint32_t> findWrapping(eastl::span<const PackerInput::Resource> resources, uint32_t time)
  {
    FixedFramememVector<uint32_t> result;
    for (uint32_t i = 0; i < resources.size(); ++i)
      if (resources[i].size > 0 && resources[i].align > 0 && is_wrapping(resources[i], time))
        result.push_back(i);
    return result;
  }

  TimePoint calculateBestSplittingTime(const PackerInput &input)
  {
    FramememVector<int> timelineBalance(input.timelineSize, 0);
    for (auto &res : input.resources)
    {
      ++timelineBalance[res.start];
      --timelineBalance[res.end];
    }

    eastl::partial_sum(timelineBalance.begin(), timelineBalance.end(), timelineBalance.begin());

    // If this line was here, `timelineBalance[t]` would be precisely
    // the amount of resources alive at time `t`, but it would not
    // impact the result due to monotonicity.
    // for (auto& b : timelineBalance)
    //  b += wrappingResourceCount;
    // Code in comment intentionally left here for clarity.


    // Start from the end ensure that the result is 0 for cases with no
    // wraparound textures. The off by 1 is intentional here.
    const TimePoint minIndex = eastl::min_element(timelineBalance.rbegin(), timelineBalance.rend()).base() - timelineBalance.begin();

    return minIndex != input.timelineSize ? minIndex : 0;
  }

  FixedFramememVector<uint32_t> findPinned(eastl::span<const PackerInput::Resource> resources)
  {
    FixedFramememVector<uint32_t> result;
    for (uint32_t i = 0; i < resources.size(); ++i)
      if (is_hinted(resources[i]))
        result.push_back(i);
    return result;
  }

  Timeline generateTimeline(const PackerInput &input)
  {
    Timeline result(input.timelineSize);
    for (uint32_t i = 0; i < input.resources.size(); ++i)
    {
      const auto &res = input.resources[i];
      if (res.size == 0)
        continue;
      G_ASSERT_CONTINUE(res.align != 0);
      G_ASSERT_CONTINUE(res.start < result.size() && res.end < result.size());
      result[res.start].startingResources.emplace_back(i);
      result[res.end].endingResources.emplace_back(i);
    }
    return result;
  }

private:
  // NOTE: this intentionally does not use framemem allocator,
  // we wouldn't be able to wipe memory after packing is done
  // otherwise.
  dag::Vector<MemoryOffset> calculatedOffsets;
};

Allocation GreedyAllocator::allocate(const PackerInput::Resource &res, TimePoint splitTime)
{
  G_ASSERT(res.offsetHint == PackerInput::NO_HINT || res.offsetHint % res.align == 0);
  Allocation fakeAlloc{0, res.size, 0};
  if (is_hinted(res))
    fakeAlloc = {res.offsetHint, 0, 0};

  auto candidate = freeList.lower_bound(fakeAlloc);
  auto candidateNoHintRespected = freeList.end();

  while (candidate != freeList.end())
  {
    const TimePoint availUntil = candidate->availableUntil;
    const TimePoint availSince = candidate->availableSince;

    const bool available =
      availUntil == Allocation::ALWAYS_AVAILABLE || segments_disjoint(Segment{res.start, res.end}, Segment{availUntil, availSince});

    const bool sizeFits = res.sizeWithPadding(candidate->offset) <= candidate->size;
    const bool hintFits = res.sizeWithHint(candidate->offset) <= candidate->size;
    const bool hintRespected = !is_hinted(res) || (candidate->offset <= res.offsetHint && hintFits);
    const bool containsHint = candidate->offset <= res.offsetHint && candidate->offset + candidate->size > res.offsetHint;
    // If a candidate is located at the end of the heap and contains a resource offset hint,
    // but is too small for resource, increase the size of the allocation.
    if (is_hinted(res) && available && containsHint && !hintFits && candidate->offset + candidate->size == currentHeapSize)
    {
      Allocation result = *candidate;
      freeListByOffset.erase(candidate);
      freeList.erase(candidate);
      currentHeapSize += res.sizeWithHint(result.offset) - result.size;
      result.size = res.sizeWithHint(result.offset);
      result.availableUntil = res.start;

      totalHeapSize = eastl::max(currentHeapSize, totalHeapSize);
      return result;
    }

    // Save a possible candidate that does not satisfy the offset hint
    // if there are no appropriate ones.
    if (available && sizeFits && !hintRespected && candidateNoHintRespected == freeList.end())
      candidateNoHintRespected = candidate;
    // NOTE: we could search for an alloc that would fit this resource
    // with ANY padding instead, but that might lead to a bad chain
    // reaction of excessive allocations.
    if (available && sizeFits && hintRespected)
      break;

    ++candidate;
  }

  if (is_hinted(res) && candidate == freeList.end() && res.offsetHint < currentHeapSize)
    candidate = candidateNoHintRespected;

  static constexpr MemorySize ALLOCATION_SPLIT_THRESHOLD = 4096;
  const auto cutAllocationRight = [this](const PackerInput::Resource &res, Allocation &result) {
    const bool hintFits = res.sizeWithHint(result.offset) <= result.size;
    const bool hintRespected = result.offset <= res.offsetHint && hintFits;
    const MemorySize usedSpace = hintFits && hintRespected ? res.sizeWithHint(result.offset) : res.sizeWithPadding(result.offset);
    if (result.size - usedSpace >= ALLOCATION_SPLIT_THRESHOLD)
    {
      Allocation leftover = result;
      leftover.offset += usedSpace;
      leftover.size -= usedSpace;

      freeListByOffset.insert(freeList.insert(leftover));

      result.size = usedSpace;
    }
  };
  const auto cutAllocationLeft = [this](const PackerInput::Resource &res, Allocation &result, uint32_t splitOffset,
                                   TimePoint leftoverAvailableUntil, TimePoint leftoverAvailableSince) {
    if (result.offset <= splitOffset && splitOffset - result.offset >= ALLOCATION_SPLIT_THRESHOLD)
    {
      Allocation leftover = {result.offset, splitOffset - result.offset, leftoverAvailableUntil, leftoverAvailableSince};

      freeListByOffset.insert(freeList.insert(leftover));

      result.offset = res.offsetHint;
      result.size -= leftover.size;
    }
  };

  // If the allocation block is a lot bigger than the resource
  // size, cut off only a part of it.

  if (candidate != freeList.end())
  {
    Allocation result = *candidate;
    freeListByOffset.erase(candidate);
    freeList.erase(candidate);

    // The candidate may not respect hint and it's invalid to cut them on the left.
    const bool hintFits = res.sizeWithHint(result.offset) <= result.size;
    const bool hintRespected = result.offset <= res.offsetHint && hintFits;
    if (is_hinted(res) && hintRespected)
      cutAllocationLeft(res, result, res.offsetHint, result.availableUntil, result.availableSince);

    cutAllocationRight(res, result);

    if (is_wrapping(res, splitTime) || is_hinted(res))
    {
      if (result.availableSince == Allocation::ALWAYS_AVAILABLE)
        result.availableSince = res.end;

      result.availableUntil = res.start;
    }
    return result;
  }
  const MemorySize allocSize = res.sizeWithHint(currentHeapSize);

  if (DAGOR_UNLIKELY(allocSize > maxHeapSize || currentHeapSize > maxHeapSize - allocSize))
  {
    G_ASSERTF(
      res.sizeWithPadding(currentHeapSize) > maxHeapSize || currentHeapSize > maxHeapSize - res.sizeWithPadding(currentHeapSize),
      "Resource with an offset hint do not fit into heap size limits");

    Allocation emptyResult{0, 0, Allocation::ALWAYS_AVAILABLE};
    return emptyResult;
  }

  Allocation result{currentHeapSize, allocSize,
    // Setting this to a fixed value for non-wrapping non-hinted resources
    // allows us to defragment the free list
    is_wrapping(res, splitTime) || is_hinted(res) ? res.start : Allocation::ALWAYS_AVAILABLE,
    is_wrapping(res, splitTime) || is_hinted(res) ? res.end : Allocation::ALWAYS_AVAILABLE};

  currentHeapSize += result.size;
  if (is_hinted(res))
    cutAllocationLeft(res, result, res.offsetHint, Allocation::ALWAYS_AVAILABLE, Allocation::ALWAYS_AVAILABLE);

  totalHeapSize = eastl::max(currentHeapSize, totalHeapSize);
  return result;
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

  for (auto i = runStart.base(); i != runEnd;)
  {
    freeList.erase(*i);
    i = freeListByOffset.erase(i);
  }

  // Drop allocations that are at the end of memory and **don't have
  // availability restrictions**.
  if (alloc.offset + alloc.size == currentHeapSize && alloc.availableUntil == Allocation::ALWAYS_AVAILABLE)
    currentHeapSize -= alloc.size;
  else
    freeListByOffset.insert(freeList.insert(alloc));
}


Packer make_greedy_scanline_packer() { return GreedyScanlinePacker{}; }

} // namespace dabfg
