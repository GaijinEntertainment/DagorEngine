// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/span.h>
#include <EASTL/tuple.h>
#include <EASTL/unique_ptr.h>
#include <generic/dag_relocatableFixedVector.h>
#include <math/dag_intrin.h>


namespace concurrent_element_pool_detail
{
// Strong typedef required to specialize dag::is_type_relocatable without any issues.
template <class T>
struct FixedArray : eastl::unique_ptr<T[]>
{};
} // namespace concurrent_element_pool_detail

template <class T>
struct dag::is_type_relocatable<concurrent_element_pool_detail::FixedArray<T>> : eastl::true_type
{};

/// @brief This is a pool of `T`s. Elements of it are guaranteed to be stable in memory
/// after allocation, which allows for concurrent indexing and allocation. It is also possible to
/// iterate it, but not concurrently with allocate.
///
/// @tparam I A strong typedef (enum class) to use as an index into this pool.
/// @tparam T The type of the elements in the pool.
/// @tparam INITIAL_CAPACITY_LOG2 Log2 of the initial capacity of the pool.
/// @tparam BUCKET_COUNT Amount of buckets we are going to use.
template <class I, class T, uint32_t INITIAL_CAPACITY_LOG2, uint32_t BUCKET_COUNT = 8>
class ConcurrentElementPool
{
  static_assert(eastl::is_enum_v<I>, "I must be an enum class!");
  static constexpr uint32_t INITIAL_CAPACITY = 1u << INITIAL_CAPACITY_LOG2;
  static constexpr uint32_t OFFSET_BITS_FOR_FIRST_BUCKET = INITIAL_CAPACITY_LOG2;
  static constexpr uint32_t BUCKET_AND_OFFSET_BITS = sizeof(eastl::underlying_type_t<I>) * CHAR_BIT;
  static constexpr uint32_t MAX_BUCKET_COUNT = BUCKET_AND_OFFSET_BITS - OFFSET_BITS_FOR_FIRST_BUCKET - 1;
  static_assert(BUCKET_COUNT > 0, "BUCKET_COUNT must be non-zero!");
  static_assert(BUCKET_COUNT <= MAX_BUCKET_COUNT, "BUCKET_COUNT must be less than MAX_BUCKET_COUNT");
  static_assert(MAX_BUCKET_COUNT <= 255, "MAX_BUCKET_COUNT must fit into a byte!");
  static_assert(eastl::to_underlying(I::Invalid) == 0, "Invalid ID must be represented as 0!");
  static_assert(sizeof(I) <= sizeof(uint32_t), "I must fit into a uint32_t!");
  static_assert(eastl::is_unsigned_v<eastl::underlying_type_t<I>>, "I must be represented via an unsigned type!");

  template <bool>
  class Iterator;

public:
  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;

  /// Identifier for elements of this pool.
  /// @warning the identifiers are NOT sequential, use toIndex and
  /// fromIndex to convert to/from sequential indices.
  using Id = I;

  ConcurrentElementPool()
  {
    buckets.reserve(BUCKET_COUNT);
    buckets.emplace_back().reset(new T[INITIAL_CAPACITY]{});
  }

  ConcurrentElementPool(const ConcurrentElementPool &) = delete;
  ConcurrentElementPool &operator=(const ConcurrentElementPool &) = delete;

  ConcurrentElementPool(ConcurrentElementPool &&other) :
    buckets{eastl::move(other.buckets)},
    firstFreeInLastBucket{eastl::exchange(other.firstFreeInLastBucket, 0)},
    totalElems{eastl::exchange(other.totalElems, 0)}
  {}

  ConcurrentElementPool &operator=(ConcurrentElementPool &&other)
  {
    if (this == &other)
      return *this;

    buckets = eastl::move(other.buckets);
    firstFreeInLastBucket = eastl::exchange(other.firstFreeInLastBucket, 0);
    totalElems = eastl::exchange(other.totalElems, 0);

    return *this;
  }


  /// @brief Allocates an element in the pool
  /// @warning If the amount of elements exceeds 2^INITIAL_CPACITY + ... + 2^(INITIAL_CAPACITY + BUCKET_COUNT - 1),
  /// you will get a logerr, a data race, and possibly a crash.
  /// @return The index of the allocated element.
  Id allocate()
  {
    if (DAGOR_UNLIKELY(firstFreeInLastBucket >= capacity(buckets.size() - 1)))
    {
      T *newBucket = new T[capacity(buckets.size())]{};
      buckets.emplace_back().reset(newBucket);
      if (DAGOR_UNLIKELY(buckets.size() > BUCKET_COUNT))
        logerr("Too many allocations in ElementPool! This is a data race! Expect crashes!");
      firstFreeInLastBucket = 0;
    }

    const auto result = make_index(buckets.size() - 1, firstFreeInLastBucket);

    ++firstFreeInLastBucket;
    ++totalElems;

    return result;
  }

  /// @brief Pre-allocates elements such that indices up to and including \p id are valid.
  /// @param id The index to pre-allocate up to.
  void ensureSpace(Id id)
  {
    const auto [bucket, offset] = break_index(id);
    while (buckets.size() <= bucket || firstFreeInLastBucket <= offset)
      allocate();
  }

  /// @brief returns the total number of elements allocated.
  /// @warning Cannot be called concurrently with allocate.
  /// @return The total number of elements allocated.
  uint32_t totalElements() const { return totalElems; }

  /// @brief accesses an element.
  /// @param id The index of an element.
  /// @return A reference to the element.
  const T &operator[](Id id) const
  {
    const auto [bucket, offset] = break_index(id);
    return buckets[bucket][offset];
  }

  /// @brief accesses an element.
  /// @param id The index of an element.
  /// @return A reference to the element.
  T &operator[](Id id)
  {
    const auto [bucket, offset] = break_index(id);
    return buckets[bucket][offset];
  }

  /// @brief returns a sequential index corresponding to the given Id
  /// @param id The Id to convert to an index
  /// @return The index corresponding to the Id
  static eastl::underlying_type_t<Id> to_index(Id id)
  {
    const auto [bucket, offset] = break_index(id);
    return INITIAL_CAPACITY * ((1 << bucket) - 1) + offset;
  }

  /// @brief Constructs an Id back from an index
  /// @param idx The index to convert to an Id
  /// @return The Id corresponding to the index
  static Id from_index(eastl::underlying_type_t<Id> idx)
  {
    const uint32_t bucket = get_log2i(idx / INITIAL_CAPACITY + 1);
    const uint32_t offset = idx - ((1 << bucket) - 1) * INITIAL_CAPACITY;
    return make_index(bucket, offset);
  }

  iterator begin() { return iterator{this, 0, 0}; }
  iterator end() { return iterator{this, buckets.size() - 1, firstFreeInLastBucket}; }
  const_iterator begin() const { return const_iterator{this, 0, 0}; }
  const_iterator end() const { return const_iterator{this, buckets.size() - 1, firstFreeInLastBucket}; }
  const_iterator cbegin() const { return const_iterator{this, 0, 0}; }
  const_iterator cend() const { return const_iterator{this, buckets.size() - 1, firstFreeInLastBucket}; }

  /// @brief Clears the pool.
  void clear()
  {
    buckets.clear();
    buckets.emplace_back().reset(new T[INITIAL_CAPACITY]{});
    totalElems = 0;
    firstFreeInLastBucket = 0;
  }

  /// @brief Iterates over all (id, element) pairs in the pool.
  /// @tparam F A callable type that takes an Id and a reference to a T.
  /// @param f The callable object to call for each element.
  template <class F>
  void iterateWithIds(const F &f)
  {
    for (uint32_t i = 0, ie = buckets.size(); i < ie; ++i)
      for (uint32_t j = 0, je = size(i); j < je; ++j)
        f(make_index(i, j), buckets[i][j]);
  }

  /// @brief Finds an element in the pool by a predicate.
  /// @tparam P A callable type that takes a const reference to a T and returns a bool.
  /// @param p The predicate to use for finding the element.
  template <class P>
  Id findIf(const P &p) const
  {
    for (uint32_t i = 0, ie = buckets.size(); i < ie; ++i)
      for (uint32_t j = 0, je = size(i); j < je; ++j)
        if (p(buckets[i][j]))
          return make_index(i, j);
    return Id::Invalid;
  }

private:
  static uint32_t capacity(uint32_t bucket) { return INITIAL_CAPACITY << bucket; }
  uint32_t size(uint32_t bucket) const { return bucket == buckets.size() - 1 ? firstFreeInLastBucket : capacity(bucket); }

  static Id make_index(uint8_t bucket, uint16_t offset)
  {
    G_FAST_ASSERT(bucket < MAX_BUCKET_COUNT && offset < capacity(bucket));
    // NOTE: see break_index for the encoding
    return static_cast<Id>((uint32_t{1} << (OFFSET_BITS_FOR_FIRST_BUCKET + bucket)) | offset);
  }

  static eastl::tuple<uint8_t, uint16_t> break_index(Id id)
  {
    // Format of IDs is as follows:
    // 1) Some amount of 0s which encodes the bucket.
    //    The less we 0s we have, the higher bucket index we mean
    //    and hence the more bits we have to store the offset into the bucket.
    // 2) A single 1 as a separator.
    // 3) Offset into the bucket. The amount of bits here is always capacity(bucket).
    //
    //    bucket no.
    //  ______________
    // |             |    offset into bkt
    // 00000000 0000001y yyyyyyyy yyyyyyyy
    G_FAST_ASSERT(id != Id::Invalid);
    const uint32_t raw = eastl::to_underlying(id);
    const uint32_t zeroBitsCount = __clz_unsafe(raw) - CHAR_BIT * (sizeof(uint32_t) - sizeof(Id));
    const uint8_t bucket = static_cast<uint8_t>(MAX_BUCKET_COUNT - zeroBitsCount);
    const uint32_t offsetBitsCount = BUCKET_AND_OFFSET_BITS - zeroBitsCount - 1;
    const uint16_t offset = static_cast<uint16_t>(raw & ((1 << offsetBitsCount) - 1));
    // sanity checks against memory corruption
    G_FAST_ASSERT(bucket < MAX_BUCKET_COUNT && offset < capacity(bucket));
    return {bucket, offset};
  }

private:
  dag::RelocatableFixedVector<concurrent_element_pool_detail::FixedArray<T>, BUCKET_COUNT> buckets;
  uint32_t totalElems = 0;
  uint32_t firstFreeInLastBucket = 0;
};


template <class I, class T, uint32_t INITIAL_CAPACITY, uint32_t BUCKET_COUNT>
template <bool IS_CONST>
class ConcurrentElementPool<I, T, INITIAL_CAPACITY, BUCKET_COUNT>::Iterator
{
  using OwnerPtr = eastl::conditional_t<IS_CONST, const ConcurrentElementPool *, ConcurrentElementPool *>;

  Iterator(OwnerPtr pool, uint32_t bucket_, uint32_t offset_) : pool{pool}, bucket{bucket_}, offset{offset_} {}

  friend class ConcurrentElementPool;

public:
  using difference_type = ptrdiff_t;
  using iterator_category = eastl::bidirectional_iterator_tag;
  using value_type = T;
  using pointer = eastl::conditional_t<IS_CONST, const T *, T *>;
  using reference = eastl::conditional_t<IS_CONST, const T &, T &>;

  Iterator() = default;

  Iterator &operator++()
  {
    if (DAGOR_UNLIKELY(offset + 1 >= ConcurrentElementPool::capacity(bucket)))
    {
      ++bucket;
      offset = 0;
    }
    else
      ++offset;

    return *this;
  }

  Iterator operator++(int)
  {
    Iterator copy = *this;
    ++*this;
    return copy;
  }

  Iterator &operator--()
  {
    if (DAGOR_UNLIKELY(offset == 0))
    {
      --bucket;
      offset = ConcurrentElementPool::capacity(bucket) - 1;
    }
    else
      --offset;

    return *this;
  }

  Iterator operator--(int)
  {
    Iterator copy = *this;
    --*this;
    return copy;
  }

  friend bool operator==(const Iterator &fst, const Iterator &snd) { return fst.bucket == snd.bucket && fst.offset == snd.offset; }
  friend bool operator!=(const Iterator &fst, const Iterator &snd) { return !(fst == snd); }

  reference operator*() const { return pool->buckets[bucket][offset]; }
  pointer operator->() const { return &pool->buckets[bucket][offset]; }

protected:
  OwnerPtr pool;
  uint32_t bucket = 0;
  uint32_t offset = 0;
};
