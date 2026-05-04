// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_intrin.h>
#include <generic/dag_carray.h>
#include <EASTL/vector.h>


namespace drv3d_dx12::resource_manager
{

template <uint32_t MaxSignatureCount>
class HeapSuballocatorImpl
{
public:
  void add(uint32_t signature, uint32_t index, uint64_t max_free_range_size);
  void remove(uint32_t signature, uint32_t index);
  void update(uint32_t signature, uint32_t index, uint64_t new_max_free_range_size);
  void clear();

  template <typename T>
  void iterateSuitableHeaps(uint32_t signature, uint64_t size, const T &try_allocate)
  {
    iterateSuitableHeaps(signatureToBuckets[signature], size, try_allocate);
  }

  template <typename T>
  void iterateSuitableHeapsInSameSizeBucketOnly(uint32_t signature, uint64_t size, const T &try_allocate)
  {
    iterateSuitableHeapsInSameSizeBucketOnly(signatureToBuckets[signature], size, try_allocate);
  }

  template <typename T>
  void iterateSuitableHeapsInBufferOrder(uint32_t signature, uint64_t size, const T &try_allocate)
  {
    iterateSuitableHeapsInBufferOrder(signatureToBuckets[signature], size, try_allocate);
  }

private:
  static constexpr uint32_t bucket_count = 32;

  struct HeapEntry
  {
    uint64_t maxFreeRangeSize : 40;
    uint64_t heapIndex : 24;
  };

  struct HeapPosition
  {
    uint32_t bucketIndex : 8;
    uint32_t indexInsideBucket : 24;
  };

  using Buckets = carray<dag::Vector<HeapEntry>, bucket_count>;

  void add(Buckets &buckets, uint32_t index, uint64_t max_free_range_size);
  void remove(Buckets &buckets, uint32_t index);
  void update(Buckets &buckets, uint32_t index, uint64_t new_max_free_range_size);

  template <typename T>
  void iterateSuitableHeaps(Buckets &buckets, uint64_t size, const T &try_allocate)
  {
    for (uint32_t bucketIndex = sizeToBucketIndex(size); bucketIndex <= maxValidBucketIndex; ++bucketIndex)
    {
      const auto &bucket = buckets[bucketIndex];
      for (auto entry : bucket)
      {
        if (size > entry.maxFreeRangeSize)
          continue;

        if (try_allocate(entry.heapIndex, entry.maxFreeRangeSize))
          return;
      }
    }
  }

  template <typename T>
  void iterateSuitableHeapsInSameSizeBucketOnly(Buckets &buckets, uint64_t size, const T &try_allocate)
  {
    const auto sizeBucket = sizeToBucketIndex(size);
    if (sizeBucket > maxValidBucketIndex)
    {
      return;
    }
    const auto &bucket = buckets[sizeBucket];
    for (auto entry : bucket)
    {
      if (size > entry.maxFreeRangeSize)
        continue;

      if (try_allocate(entry.heapIndex, entry.maxFreeRangeSize))
      {
        return;
      }
    }
  }

  template <typename T>
  void iterateSuitableHeapsInBufferOrder(Buckets &buckets, uint64_t size, const T &try_allocate)
  {
    const auto sizeBucket = sizeToBucketIndex(size);
    if (sizeBucket > maxValidBucketIndex)
    {
      return;
    }
    for (uint32_t index = 0; index < resourceIndexToHeapPosition.size(); ++index)
    {
      const auto pos = resourceIndexToHeapPosition[index];
      if (pos.bucketIndex < sizeBucket)
      {
        continue;
      }
      if (pos.bucketIndex >= buckets.size())
      {
        continue;
      }
      const auto &bucket = buckets[pos.bucketIndex];
      if (pos.indexInsideBucket >= bucket.size())
      {
        continue;
      }
      const auto entry = bucket[pos.indexInsideBucket];
      if (entry.heapIndex != index)
      {
        continue;
      }
      if (size > entry.maxFreeRangeSize)
      {
        continue;
      }
      if (try_allocate(index, entry.maxFreeRangeSize))
      {
        return;
      }
    }
  }

  static inline uint32_t sizeToBucketIndex(uint32_t size) { return 32 - __clz(size); }

  carray<Buckets, MaxSignatureCount> signatureToBuckets;
  dag::Vector<HeapPosition> resourceIndexToHeapPosition;

  uint32_t maxValidBucketIndex = 0;
};

} // namespace drv3d_dx12::resource_manager
