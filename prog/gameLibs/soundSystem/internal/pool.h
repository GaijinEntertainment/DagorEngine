// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/type_traits.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>
#include <EASTL/array.h>
#include <debug/dag_assert.h>
#include <math/dag_adjpow2.h>
#include <new>
#include <type_traits>

namespace sndsys
{
template <typename T>
struct ResolvePoolValueType
{};

template <>
struct ResolvePoolValueType<int32_t>
{
  using index_type = uint16_t;
  using generation_type = uint16_t;
};

template <>
struct ResolvePoolValueType<uint32_t>
{
  using index_type = uint16_t;
  using generation_type = uint16_t;
};

// NOT thread safe
// PoolType is used to reduce collision chance among handles of different types.
// It is expected and most likely that the fixed part will be sufficient and additional arrays will not grow.
template <typename Event, typename ValueType, size_t GrowSize, size_t PoolType, size_t MaxPoolTypes,
  size_t LeakWarningThreshold = 2048>
class Pool
{
public:
  using event_type = Event;
  using value_type = ValueType;

  static constexpr value_type invalid_value = 0;

private:
  using resolve_value_type_t = ResolvePoolValueType<value_type>;
  using index_type = typename resolve_value_type_t::index_type;
  using generation_type = typename resolve_value_type_t::generation_type;

  static constexpr size_t pool_type = PoolType;
  static constexpr size_t max_pool_types = MaxPoolTypes;
  G_STATIC_ASSERT(pool_type < max_pool_types);
  static constexpr size_t array_size = GrowSize;
  G_STATIC_ASSERT(array_size > 0 && is_pow2(array_size));
  static constexpr size_t array_size_minus_one = array_size - 1;
  static constexpr size_t array_size_log2 = get_const_log2(array_size);
  static constexpr size_t max_slots = eastl::numeric_limits<index_type>::max();
  static constexpr size_t max_generation = eastl::numeric_limits<generation_type>::max();
  static constexpr size_t min_free_slots_grow_threshold = min(array_size, (size_t)8);
  G_STATIC_ASSERT(min_free_slots_grow_threshold > 0);
  static constexpr size_t leak_warning_threshold = LeakWarningThreshold;

  static constexpr inline bool is_used_generation(generation_type generation) { return generation & 1; }

  static constexpr inline bool is_free_generation(generation_type generation) { return !is_used_generation(generation); }

  // Advance a free generation to the next used generation (even -> odd).
  // Skips 0 to prevent collisions with invalid_value and start_generation edge cases.
  static inline generation_type next_used_generation(generation_type gen)
  {
    G_ASSERT(is_free_generation(gen));
    ++gen;        // even -> odd (used)
    if (gen == 0) // wrapped: 0xFFFF -> 0x0000, skip to 1 (still odd, used)
      gen = 1;
    G_ASSERT(is_used_generation(gen));
    return gen;
  }

  // Advance a used generation to the next free generation (odd -> even).
  // Skips 0 to prevent a node's generation from matching start_generation
  // after wrap-around, which would cause ABA collisions with never-used nodes.
  static inline generation_type next_free_generation(generation_type gen)
  {
    G_ASSERT(is_used_generation(gen));
    ++gen;        // odd -> even (free)
    if (gen == 0) // wrapped: 0xFFFF -> 0x0000, advance to 2 (still even, free, non-zero)
      gen = 2;
    G_ASSERT(is_free_generation(gen));
    G_ASSERT(gen != 0); // generation 0 is reserved for invalid/uninitialized state
    return gen;
  }

  static constexpr generation_type pool_type_based_generation = pool_type * max_generation / max_pool_types;
  static constexpr generation_type start_generation = pool_type_based_generation - (pool_type_based_generation & 1);
  G_STATIC_ASSERT(is_free_generation(start_generation));

  struct Node
  {
    union
    {
      Node *nextFree;
      std::aligned_storage_t<sizeof(event_type), alignof(event_type)> storage;
    };

    generation_type generation = start_generation;
  };

  using Array = eastl::array<Node, array_size>;

  eastl::vector<eastl::unique_ptr<Array>> arrays;
  Array fixedArray;

  size_t total = 0;
  size_t freeCount = 0;

  Node *firstFree = nullptr;
  Node *lastFree = nullptr;

  // ---- handle packing helpers ----

  static constexpr size_t index_bits = sizeof(index_type) * 8;
  static inline value_type pack(index_type idx, generation_type gen) { return (value_type(gen) << index_bits) | idx; }
  static inline index_type unpack_index(value_type v) { return index_type(v); }
  static inline generation_type unpack_generation(value_type v) { return generation_type(v >> index_bits); }

  // --------------------------------

  inline bool can_pop_front() const { return firstFree != nullptr; }

  inline Node &pop_front_free()
  {
    G_ASSERT(firstFree);
    Node &node = *firstFree;
    if (firstFree != lastFree)
    {
      G_ASSERT(firstFree->nextFree && lastFree);
      firstFree = firstFree->nextFree;
    }
    else
    {
      G_ASSERT(!firstFree->nextFree);
      firstFree = lastFree = nullptr;
    }
    --freeCount;
    node.nextFree = nullptr;
#if DAGOR_DBGLEVEL > 0
    G_ASSERT(count_num_free_nodes() == freeCount);
#endif
    return node;
  }

  inline void push_back_free(Node &node)
  {
    node.nextFree = nullptr;
    if (lastFree)
    {
      G_ASSERT(firstFree && !lastFree->nextFree);
      lastFree->nextFree = &node;
      lastFree = &node;
    }
    else
    {
      G_ASSERT(!firstFree);
      firstFree = lastFree = &node;
    }
    ++freeCount;
  }

  inline void init_array(Array &arr, size_t count)
  {
    G_ASSERT(count <= array_size && total + count <= max_slots);
    for (size_t i = 0; i < count; ++i)
      push_back_free(arr[i]);
    total += count;
  }

  inline void grow()
  {
    G_ASSERT(total <= max_slots);
    const size_t add = min(array_size, max_slots - total);

    if (total < leak_warning_threshold && total + add >= leak_warning_threshold)
    {
      logerr("sndsys: handle leak warning: more then %zu handles allocated, total: %zu, arrays: %zu, pool type: %zu",
        leak_warning_threshold, total + add, arrays.size(), pool_type);
    }

    auto arr = eastl::make_unique<Array>();
    Array &ref = *arr;
    arrays.emplace_back(eastl::move(arr));
    init_array(ref, add);
  }

  inline size_t count_num_free_nodes() const
  {
    size_t count = 0;
    for (const Node *node = firstFree; node; node = node->nextFree)
    {
      G_UNREFERENCED(node);
      ++count;
    }
    return count;
  }

  inline bool need_grow() const { return freeCount < min_free_slots_grow_threshold; }

  inline void grow_on_demand()
  {
    if (need_grow())
      grow();
  }

  static inline event_type &get_event(Node &node) { return *std::launder(reinterpret_cast<event_type *>(&node.storage)); }

  template <typename... Args>
  inline void construct(Node &node, Args &&...args)
  {
    new (&node.storage) event_type(eastl::forward<Args>(args)...);
  }
  inline void destroy(event_type &event) { event.~event_type(); }

  inline Node &get_node_from_index(intptr_t idx)
  {
    const size_t arrIdx = size_t(idx) >> array_size_log2;
    Array &arr = arrIdx != 0 ? *arrays[arrIdx - 1] : fixedArray;
    return arr[idx & array_size_minus_one];
  }

  inline Node *get_node_from_value(value_type value)
  {
    const generation_type gen = unpack_generation(value);
    const index_type idx = unpack_index(value);
    if (is_used_generation(gen))
    {
      if (size_t(idx) >= total)
      {
        logerr("sndsys: Handle value %lld contains garbage(index %zu out of range, total %zu)", int64_t(value), size_t(idx), total);
        return nullptr;
      }
      Node &node = get_node_from_index(idx);
      if (node.generation == gen)
        return &node;
    }
    return nullptr;
  }

  inline intptr_t get_node_index(const Node *node) const
  {
    if (node >= fixedArray.begin() && node < fixedArray.end())
      return node - fixedArray.begin(); // in most cases will stop here
    intptr_t idx = fixedArray.size();
    for (auto &it : arrays)
    {
      if (node >= it->begin() && node < it->end())
        return idx + (node - it->begin());
      idx += it->size();
    }
    return -1;
  }

public:
  event_type *get(value_type value)
  {
    if (Node *node = get_node_from_value(value))
      return &get_event(*node);
    return nullptr;
  }

  inline intptr_t get_node_index(value_type value) { return unpack_index(value); }

  template <typename... Args>
  event_type *emplace(value_type &value, Args &&...args)
  {
    value = invalid_value;

    grow_on_demand();

    if (!can_pop_front())
    {
      logerr("sndsys: max pool capacity exceeded, total: %zu, arrays: %zu, pool type: %zu", total, arrays.size(), pool_type);
      return nullptr;
    }

    Node &node = pop_front_free();

    // free -> to used generation
    G_STATIC_ASSERT(eastl::is_unsigned<generation_type>::value);
    node.generation = next_used_generation(node.generation);
    G_ASSERT(is_used_generation(node.generation));

    const intptr_t nodeIndex = get_node_index(&node);
    G_ASSERT(nodeIndex >= 0);

    value = pack(index_type(nodeIndex), node.generation);

    construct(node, eastl::forward<Args>(args)...);

    return &get_event(node);
  }

  void reject(value_type value)
  {
    if (Node *node = get_node_from_value(value))
    {
      destroy(get_event(*node));
      // used -> to free generation
      G_STATIC_ASSERT(eastl::is_unsigned<generation_type>::value);
      node->generation = next_free_generation(node->generation);
      // push back, pop front to reduce generation overflow
      push_back_free(*node);
      G_ASSERT(is_free_generation(node->generation));
    }
  }

  template <typename Callable>
  void enumerate(Callable cb)
  {
    intptr_t idx = 0;
    for (Node &node : fixedArray)
    {
      if (idx >= total)
        return;
      if (is_used_generation(node.generation))
        cb(get_event(node), pack(index_type(idx), node.generation));
      ++idx;
    }
    for (auto &arr : arrays)
      for (Node &node : *arr)
      {
        if (idx >= total)
          return;
        if (is_used_generation(node.generation))
          cb(get_event(node), pack(index_type(idx), node.generation));
        ++idx;
      }
  }

  inline size_t get_free() const { return freeCount; }

  inline size_t get_used() const { return total - get_free(); }

  inline size_t get_total() const { return total; }

private:
  void teardown()
  {
    enumerate([&](event_type &evt, value_type) { destroy(evt); });
    arrays.clear();
    firstFree = lastFree = nullptr;
    freeCount = 0;
    total = 0;
    for (Node &node : fixedArray)
      if (is_used_generation(node.generation))
        ++node.generation; // advance to free, preserve generation history to avoid ABA on reuse
  }

public:
  void close()
  {
    teardown();
    init_array(fixedArray, array_size);
  }

  Pool(const Pool &) = delete;
  Pool(Pool &&) = delete;
  Pool &operator=(const Pool &) = delete;
  Pool &operator=(Pool &&) = delete;

  Pool() { init_array(fixedArray, array_size); }
  ~Pool() { teardown(); }
}; // class Pool
} // namespace sndsys
