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

ValueRange<uint64_t> BufferHeap::Heap::allocate(uint64_t size, uint64_t alignment)
{
  if (freeRanges.empty())
  {
    return {};
  }
  // tries to find a range that has enough space and fulfills the alignment requirement either by allocating from the front or the back
  auto ref = eastl::find_if(freeRanges.begin(), freeRanges.end(), [size, alignment](auto range) {
    if (range.size() < size)
    {
      return false;
    }
    if (0 == (range.start % alignment))
    {
      return true;
    }
    return 0 == ((range.stop - size) % alignment);
  });
  if (freeRanges.end() == ref)
  {
    return {};
  }
  uint64_t offset;
  if (0 == (ref->start % alignment))
  {
    // from the front
    offset = ref->start;
    if (ref->size() == size)
    {
      freeRanges.erase(ref);
    }
    else
    {
      ref->start += size;
    }
  }
  else
  {
    // from the back
    offset = ref->stop - size;
    // size can not be equal, otherwise alignment from the front would be true
    ref->stop -= size;
  }
  updateMaxFreeRangeSize();
  return make_value_range<uint64_t>(offset, size);
}

ValueRange<uint64_t> BufferHeap::Heap::allocateExact(uint64_t size, uint64_t alignment)
{
  if (freeRanges.empty())
  {
    return {};
  }
  auto ref = eastl::find_if(freeRanges.begin(), freeRanges.end(),
    [size, alignment](auto range) { return (0 == (range.start % alignment)) && (range.size() == size); });
  if (freeRanges.end() == ref)
  {
    return {};
  }
  auto s = ref->start;
  freeRanges.erase(ref);
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