// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "buffer_components.h"

namespace drv3d_dx12::resource_manager
{

void BufferHeap::Heap::init(uint64_t allocation_size)
{
  G_ASSERT(freeRanges.empty());
  freeRanges.push_back(make_value_range<uint64_t>(0, allocation_size));

  maxFreeRangeSize = allocation_size;
  notifyMaxFreeRangeSizeChanged();
}

void BufferHeap::Heap::applyFirstAllocation(uint64_t payload_size)
{
  G_ASSERT(hasNoAllocations());
  if (freeRanges.front().stop == payload_size)
  {
    freeRanges.pop_back();
    maxFreeRangeSize = 0;
  }
  else
  {
    freeRanges.front().start = payload_size;
    maxFreeRangeSize = freeRanges.front().size();
  }
  notifyMaxFreeRangeSizeChanged();
}

// This allocate is a bit more simple than a general purpose allocation as it has the goal to
// keep similarly aligned things in the same buffer heaps together So allocation from this will
// fail if no free range is found that has a matching offset / address alignment.
ValueRange<uint64_t> BufferHeap::Heap::allocate(uint64_t size, uint64_t alignment)
{
  if (freeRanges.empty())
  {
    return {};
  }
  auto ref = eastl::find_if(begin(freeRanges), end(freeRanges),
    [size, alignment](auto range) //
    { return (0 == (range.start % alignment)) && (range.size() >= size); });
  if (ref == end(freeRanges))
  {
    return {};
  }
  auto s = ref->start;
  if (ref->size() == size)
  {
    freeRanges.erase(ref);
  }
  else
  {
    ref->start += size;
  }
  updateMaxFreeRangeSize();
  return make_value_range<uint64_t>(s, size);
}

void BufferHeap::Heap::rangeAllocate(ValueRange<uint64_t> range)
{
  auto ref = eastl::find_if(begin(freeRanges), end(freeRanges), [range](auto free_range) { return range.isSubRangeOf(free_range); });
  G_ASSERT(ref != end(freeRanges));
  if (ref == end(freeRanges))
  {
    return;
  }

  auto followup = ref->cutOut(range);
  if (!followup.empty())
  {
    if (ref->empty())
    {
      *ref = followup;
    }
    else
    {
      freeRanges.insert(ref + 1, followup);
    }
  }
  else if (ref->empty())
  {
    freeRanges.erase(ref);
  }

  updateMaxFreeRangeSize();
}

bool BufferHeap::Heap::free(ValueRange<uint64_t> range)
{
  free_list_insert_and_coalesce(freeRanges, range);
  updateMaxFreeRangeSize();
  return freeRanges.front().size() == bufferMemory.size;
}

void BufferHeap::Heap::updateMaxFreeRangeSize()
{
  maxFreeRangeSize = 0;
  for (auto range : freeRanges)
    maxFreeRangeSize = ::max<uint64_t>(maxFreeRangeSize, range.size());
  notifyMaxFreeRangeSizeChanged();
}

void BufferHeap::Heap::notifyMaxFreeRangeSizeChanged()
{
  if (suballocator)
    suballocator->onHeapUpdated(*this);
}

} // namespace drv3d_dx12::resource_manager