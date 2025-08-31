// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "buffer_components.h"

namespace drv3d_dx12::resource_manager
{
template <uint32_t MaxSignatureCount>
void HeapSuballocatorImpl<MaxSignatureCount>::add(uint32_t signature, uint32_t index, uint64_t max_free_range_size)
{
  add(signatureToBuckets[signature], index, max_free_range_size);
}

template <uint32_t MaxSignatureCount>
void HeapSuballocatorImpl<MaxSignatureCount>::remove(uint32_t signature, uint32_t index)
{
  if (maxValidBucketIndex != 0)
    remove(signatureToBuckets[signature], index);
}

template <uint32_t MaxSignatureCount>
void HeapSuballocatorImpl<MaxSignatureCount>::update(uint32_t signature, uint32_t index, uint64_t new_max_free_range_size)
{
  update(signatureToBuckets[signature], index, new_max_free_range_size);
}

template <uint32_t MaxSignatureCount>
void HeapSuballocatorImpl<MaxSignatureCount>::clear()
{
  for (auto &buckets : signatureToBuckets)
    for (auto &bucket : buckets)
      bucket.clear();

  resourceIndexToHeapPosition.clear();
  maxValidBucketIndex = 0;
}

template <uint32_t MaxSignatureCount>
void HeapSuballocatorImpl<MaxSignatureCount>::add(Buckets &buckets, uint32_t index, uint64_t max_free_range_size)
{
  if (index >= resourceIndexToHeapPosition.size())
    resourceIndexToHeapPosition.resize(index + 1);

  const uint32_t bucketIndex = sizeToBucketIndex(max_free_range_size);
  resourceIndexToHeapPosition[index].bucketIndex = bucketIndex;
  resourceIndexToHeapPosition[index].indexInsideBucket = buckets[bucketIndex].size();
  buckets[bucketIndex].push_back({max_free_range_size, index});

  maxValidBucketIndex = max(maxValidBucketIndex, bucketIndex);
}

template <uint32_t MaxSignatureCount>
void HeapSuballocatorImpl<MaxSignatureCount>::remove(Buckets &buckets, uint32_t index)
{
  auto &position = resourceIndexToHeapPosition[index];
  if (position.indexInsideBucket + 1 < buckets[position.bucketIndex].size())
  {
    auto &entry = buckets[position.bucketIndex][position.indexInsideBucket];
    entry = buckets[position.bucketIndex].back();
    resourceIndexToHeapPosition[entry.heapIndex] = position;
  }
  buckets[position.bucketIndex].pop_back();
}

template <uint32_t MaxSignatureCount>
void HeapSuballocatorImpl<MaxSignatureCount>::update(Buckets &buckets, uint32_t index, uint64_t new_max_free_range_size)
{
  const auto &position = resourceIndexToHeapPosition[index];
  if (position.bucketIndex == sizeToBucketIndex(new_max_free_range_size))
  {
    buckets[position.bucketIndex][position.indexInsideBucket].maxFreeRangeSize = new_max_free_range_size;
  }
  else
  {
    remove(buckets, index);
    add(buckets, index, new_max_free_range_size);
  }
}


void BufferHeap::HeapSuballocator::onHeapCreated(Heap &heap)
{
  if (!heap.canSubAllocateFrom())
    return;

  heap.setSuballocator(this);
  impl.add(getSignature(heap), heap.index(), heap.getMaxFreeRangeSize());
}

void BufferHeap::HeapSuballocator::onHeapUpdated(const Heap &heap)
{
  impl.update(getSignature(heap), heap.index(), heap.getMaxFreeRangeSize());
}

void BufferHeap::HeapSuballocator::onHeapDestroyed(Heap &heap)
{
  if (!heap.canSubAllocateFrom())
    return;

  heap.setSuballocator(nullptr);
  impl.remove(getSignature(heap), heap.index());
}

void BufferHeap::HeapSuballocator::clear() { impl.clear(); }

eastl::pair<BufferHeap::Heap *, ValueRange<uint64_t>> BufferHeap::HeapSuballocator::trySuballocate(dag::Vector<Heap> &buffer_heaps,
  ResourceHeapProperties properties, uint64_t size, D3D12_RESOURCE_FLAGS flags, uint32_t offset_alignment)
{
  if (flags != D3D12_RESOURCE_FLAG_NONE)
    return {nullptr, {}};

  const uint32_t signature = properties.raw;

  Heap *selectedHeap = nullptr;
  ValueRange<uint64_t> allocationRange;
  impl.iterateSuitableHeaps(signature, size,
    [&buffer_heaps, &selectedHeap, &allocationRange, size, offset_alignment, signature](uint32_t heap_index,
      uint64_t max_free_range_size) {
      G_UNUSED(max_free_range_size);
      G_UNUSED(signature);
      Heap &heap = buffer_heaps[heap_index];

      G_ASSERT(heap.canSubAllocateFrom());
      G_ASSERT(heap.getMaxFreeRangeSize() == max_free_range_size);
      G_ASSERT(getSignature(heap) == signature);

      auto range = heap.allocate(size, offset_alignment);
      if (range.empty())
        return false;

      selectedHeap = &heap;
      allocationRange = range;

      return true;
    });

  return {selectedHeap, allocationRange};
}

uint32_t BufferHeap::HeapSuballocator::getSignature(const Heap &heap) { return heap.getHeapID().group; }

} // namespace drv3d_dx12::resource_manager