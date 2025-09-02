// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/span.h>
#include <EASTL/tuple.h>
#include <EASTL/unique_ptr.h>
#include <dag/dag_vector.h>
#include <math/dag_intrin.h>
#include <math/dag_adjpow2.h>
#include <debug/dag_log.h>


namespace concurrent_range_pool_detail
{
// Strong typedef required to specialize dag::is_type_relocatable without any issues.
template <class T>
struct FixedArray : eastl::unique_ptr<T[]>
{};
} // namespace concurrent_range_pool_detail

template <class T>
struct dag::is_type_relocatable<concurrent_range_pool_detail::FixedArray<T>> : eastl::true_type
{};

/// @brief This is a pool for ranges of `T`s. Each range allocated is guaranteed to be contiguous
/// in memory and to NEVER be relocated. This allows one to use it in concurrent code safely in the
/// following manner: one thread always allocates new ranges and "sends" them somehow to other threads.
/// These other threads can then independently work with distinct ranges without any synchronization.
///
/// @tparam T The type of the elements in the pool.
/// @tparam INITIAL_CAPACITY_LOG2 Log2 of initial capacity of the pool.
/// @tparam BUCKET_COUNT Amount of buckets we are going to use.
/// @tparam ALIGN_TO_CHUNKS If true, ranges will be grouped into chunks of
///         size 2 ^ \p INITIAL_CAPACITY_LOG2 and no range will cross a chunk boundary.
///         This allows for batch-processing of several ranges within a single chunk,
///         e.g., to upload them to a GPU.
template <class T, uint32_t INITIAL_CAPACITY_LOG2, uint32_t BUCKET_COUNT = 8, bool ALIGN_TO_CHUNKS = false>
class ConcurrentRangePool
{
  static constexpr uint32_t INITIAL_CAPACITY = 1u << INITIAL_CAPACITY_LOG2;
  static_assert(INITIAL_CAPACITY >= 256, "INITIAL_CAPACITY must be at least 256");
  static constexpr uint32_t OFFSET_BITS_FOR_FIRST_BUCKET = INITIAL_CAPACITY_LOG2;
  static constexpr uint32_t BUCKET_AND_OFFSET_BITS = 24;
  static constexpr uint32_t MAX_BUCKET_COUNT = BUCKET_AND_OFFSET_BITS - OFFSET_BITS_FOR_FIRST_BUCKET;
  static_assert(BUCKET_COUNT > 0, "BUCKET_COUNT must be non-zero!");
  static_assert(BUCKET_COUNT <= MAX_BUCKET_COUNT, "BUCKET_COUNT must be less than MAX_BUCKET_COUNT");

public:
  /// Compact representation for a range of elements of the pool.
  enum class Range : uint32_t
  {
    /// Invalid range, will cause asserts.
    Invalid = 0,
    /// Empty range, can be viewed without any problems.
    Empty = 1 << OFFSET_BITS_FOR_FIRST_BUCKET,
  };
  /// Compact representation for a chunk of INITIAL_CAPACITY elements of this pool,
  /// which are comprised of several ranges laid out contiguously in memory.
  /// A "tail" of the chunk might not be allocated yet but is guaranteed to contain
  /// default-initialized elements (zeros for trivial types).
  enum class ChunkIdx : uint32_t
  {
  };

  ConcurrentRangePool()
  {
    buckets.reserve(BUCKET_COUNT);
    buckets.emplace_back().reset(new T[INITIAL_CAPACITY]{});
  }

  ConcurrentRangePool(const ConcurrentRangePool &) = delete;
  ConcurrentRangePool &operator=(const ConcurrentRangePool &) = delete;

  ConcurrentRangePool(ConcurrentRangePool &&other) :
    buckets{eastl::move(other.buckets)},
    firstFreeInLastBucket{eastl::exchange(other.firstFreeInLastBucket, 0)},
    totalElems{eastl::exchange(other.totalElems, 0)}
  {}

  ConcurrentRangePool &operator=(ConcurrentRangePool &&other)
  {
    if (this == &other)
      return *this;

    buckets = eastl::move(other.buckets);
    firstFreeInLastBucket = eastl::exchange(other.firstFreeInLastBucket, 0);
    totalElems = eastl::exchange(other.totalElems, 0);

    return *this;
  }


  /// @brief Allocates a new range of the given size.
  /// @warning If the amount of elements exceeds 2^INITIAL_CPACITY + ... + 2^(INITIAL_CAPACITY + BUCKET_COUNT - 1),
  /// you will get a logerr, a data race, and possibly a crash.
  /// @param size The size of the range to allocate.
  Range allocate(uint8_t size)
  {
    if constexpr (ALIGN_TO_CHUNKS) // Align if we cross chunk boundary
      if ((firstFreeInLastBucket + size) % INITIAL_CAPACITY != 0 &&
          firstFreeInLastBucket / INITIAL_CAPACITY != (firstFreeInLastBucket + size) / INITIAL_CAPACITY)
        firstFreeInLastBucket = ((firstFreeInLastBucket + INITIAL_CAPACITY - 1) / INITIAL_CAPACITY) * INITIAL_CAPACITY;

    if (DAGOR_UNLIKELY(firstFreeInLastBucket + size > capacity(buckets.size() - 1)))
    {
      T *newBucket = new T[capacity(buckets.size())]{};
      buckets.emplace_back().reset(newBucket);
      if (DAGOR_UNLIKELY(buckets.size() > BUCKET_COUNT))
        logerr("Too many allocations in RangePool! This is a data race! Expect crashes!");
      firstFreeInLastBucket = 0;
    }

    const auto result = make_range(size, buckets.size() - 1, firstFreeInLastBucket);

    firstFreeInLastBucket += size;
    totalElems += size;

    return result;
  }

  /// @brief returns the size of the range.
  /// @param r The range to get the size of.
  /// @return The size of the range.
  static uint32_t range_size(Range r) { return eastl::get<0>(break_range(r)); }

  /// @brief returns the total number of elements allocated (sum of all range sizes).
  /// @warning Cannot be called concurrently with allocate.
  /// @warning This is not the same as the total amount of allocated T elements,
  ///          some of them might be unused.
  /// @return The total number of elements allocated.
  uint32_t totalElements() const { return totalElems; }

  /// @brief returns a view of the range previously allocated.
  /// @param r The range to view.
  /// @return A span to the memory of the range.
  eastl::span<T> view(Range r)
  {
    const auto [size, bucket, offset] = break_range(r);
    return {buckets.data()[bucket].get() + offset, size};
  }

  /// @brief returns a view of the range previously allocated.
  /// @param r The range to view.
  /// @return A span to the memory of the range.
  eastl::span<const T> view(Range r) const
  {
    const auto [size, bucket, offset] = break_range(r);
    return {buckets.data()[bucket].get() + offset, size};
  }

  /// @brief returns an index of the chunk that contains the given range.
  /// @note This is only available if ALIGN_TO_CHUNKS is true, and the chunk
  /// indices are **sequential**.
  /// @param r The range to get the chunk of.
  /// @return The chunk that contains \p r.
  ChunkIdx chunkOf(Range r) const /* requires ALIGN_TO_CHUNK */
  {
    G_FAST_ASSERT(ALIGN_TO_CHUNKS);
    const auto [size, bucket, offset] = break_range(r);
    G_FAST_ASSERT(size > 0);
    return ChunkIdx{(1 << bucket) - 1 + offset / INITIAL_CAPACITY};
  }

  /// @brief returns an offset of a range inside of it's chunk.
  /// @note This is only available if ALIGN_TO_CHUNKS is true.
  /// @param r The range to get the offset of.
  /// @return The offset of \p r inside of it's chunk.
  uint32_t offsetInChunk(Range r) const /* requires ALIGN_TO_CHUNK */
  {
    G_FAST_ASSERT(ALIGN_TO_CHUNKS);
    const auto [size, bucket, offset] = break_range(r);
    G_FAST_ASSERT(size > 0);
    return offset % INITIAL_CAPACITY;
  }

  /// @brief returns a view of the chunk. Elements in the chunk which were not
  /// allocated yet are guaranteed to be default-initialized (zeroed out in case of trivial types).
  /// @param c The chunk to view.
  /// @return A span to the memory of the chunk.
  eastl::span<const T, INITIAL_CAPACITY> viewChunk(ChunkIdx c) const /* requires ALIGN_TO_CHUNK */
  {
    const uint32_t raw = eastl::to_underlying(c);
    const uint32_t bucket = get_log2i(raw + 1);
    const uint32_t offset = (raw + 1 - (1 << bucket)) * INITIAL_CAPACITY;
    return {buckets.data()[bucket].get() + offset, INITIAL_CAPACITY};
  }

  void clear()
  {
    buckets.clear();
    buckets.emplace_back().reset(new T[INITIAL_CAPACITY]{});
    totalElems = 0;
    firstFreeInLastBucket = 0;
  }

private:
  static uint32_t capacity(uint32_t bucket) { return INITIAL_CAPACITY << bucket; }
  static Range make_range(uint8_t size, uint8_t bucket, uint16_t offset)
  {
    G_FAST_ASSERT(bucket < MAX_BUCKET_COUNT && offset + size <= capacity(bucket));
    return static_cast<Range>(
      (static_cast<uint32_t>(size) << BUCKET_AND_OFFSET_BITS) | (uint32_t{1} << (OFFSET_BITS_FOR_FIRST_BUCKET + bucket)) | offset);
  }
  static eastl::tuple<uint8_t, uint8_t, uint16_t> break_range(Range r)
  {
    // Encoding schema (see concurrentElementPool for an explanation):
    // count of zeros encodes bucket no.
    //           _____
    //   size   |    |    offset into bkt
    // xxxxxxxx 0000001y yyyyyyyy yyyyyyyy
    G_FAST_ASSERT(r != Range::Invalid);
    const uint32_t raw = eastl::to_underlying(r);
    const uint8_t size = static_cast<uint8_t>(raw >> BUCKET_AND_OFFSET_BITS);
    const uint32_t rest = raw & ((1 << BUCKET_AND_OFFSET_BITS) - 1);
    const uint32_t zeroBitsCount = __clz_unsafe(rest) - CHAR_BIT * sizeof(uint8_t);
    const uint8_t bucket = static_cast<uint8_t>(MAX_BUCKET_COUNT - 1 - zeroBitsCount);
    const uint32_t offsetBitsCount = BUCKET_AND_OFFSET_BITS - zeroBitsCount - 1;
    const uint16_t offset = static_cast<uint16_t>(rest & ((1 << offsetBitsCount) - 1));
    G_FAST_ASSERT(bucket < MAX_BUCKET_COUNT && offset + size <= capacity(bucket));
    return {size, bucket, offset};
  }

private:
  // NOTE: ranges are stored SPARSELY inside of here, and we don't keep track of
  // where the uninitialized gaps are, so it is impossible to iterate this!
  dag::Vector<concurrent_range_pool_detail::FixedArray<T>> buckets;
  uint32_t totalElems = 0;
  uint32_t firstFreeInLastBucket = 0;
};
