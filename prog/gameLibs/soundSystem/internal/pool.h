// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/type_traits.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>
#include <EASTL/array.h>
#include <debug/dag_assert.h>
#include <math/dag_adjpow2.h>

namespace sndsys
{
template <typename T>
struct ResolvePoolValueType
{};

template <>
struct ResolvePoolValueType<int32_t>
{
  typedef uint16_t index_type;
  typedef uint16_t generation_type;
};

template <>
struct ResolvePoolValueType<uint32_t>
{
  typedef uint16_t index_type;
  typedef uint16_t generation_type;
};

// PoolType is used to reduce collision chance among handles of different types
template <typename Event, typename ValueType, size_t GrowSize, size_t PoolType, size_t MaxPoolTypes,
  size_t LeakWarningThreshold = 2048>
class Pool
{
public:
  typedef Event event_type;
  typedef ValueType value_type;
  static constexpr value_type invalid_value = 0;

private:
  typedef ResolvePoolValueType<value_type> resolve_value_type_t;
  typedef typename resolve_value_type_t::index_type index_type;
  typedef typename resolve_value_type_t::generation_type generation_type;

  static constexpr size_t pool_type = PoolType;
  static constexpr size_t max_pool_types = MaxPoolTypes;
  G_STATIC_ASSERT(pool_type < max_pool_types);
  static constexpr size_t array_size = GrowSize;
  G_STATIC_ASSERT(array_size > 0 && is_pow2(array_size));
  static constexpr size_t array_size_minus_one = array_size - 1;
  static constexpr size_t array_size_log2 = get_const_log2(array_size);
  static constexpr size_t max_slots = eastl::numeric_limits<index_type>::max();
  static constexpr size_t max_generation = eastl::numeric_limits<generation_type>::max();
  static constexpr size_t min_free_slots_grow_threshold = min(array_size, (size_t)4);
  G_STATIC_ASSERT(min_free_slots_grow_threshold > 0);
  static constexpr size_t leak_warning_threshold = LeakWarningThreshold;

  static constexpr inline bool is_used_generation(generation_type generation) { return generation & 1; }

  static constexpr inline bool is_free_generation(generation_type generation) { return !is_used_generation(generation); }

  static constexpr generation_type pool_type_based_generation = pool_type * max_generation / max_pool_types;
  static constexpr generation_type start_generation = pool_type_based_generation - (pool_type_based_generation & 1);
  G_STATIC_ASSERT(is_free_generation(start_generation));

  struct Node
  {
    union
    {
      alignas(event_type) uint8_t event[sizeof(event_type)];
      Node *nextFree;
    };
    generation_type generation = start_generation;
  };

  typedef eastl::array<Node, array_size> Array;
  eastl::vector<eastl::unique_ptr<Array>> arrays;
  Array fixedArray;

  size_t total = 0;
  Node *firstFree = nullptr;
  Node *lastFree = nullptr;

  struct ValueParts
  {
    union
    {
      struct
      {
        generation_type generation;
        index_type index;
      };
      value_type value;
    };
    ValueParts() = delete;
    ValueParts(value_type value) : value(value) {}
    ValueParts(index_type index, generation_type generation) : index(index), generation(generation) {}
  };

  inline bool can_pop_front() const { return firstFree != nullptr; }

  inline Node &pop_front()
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
    return node;
  }

  inline void push_back(Node &node)
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
  }

  inline void init_array(Array &arr, size_t count)
  {
    G_ASSERT(count <= array_size && total + count <= max_slots);
    for (intptr_t idx = 0; idx < count; ++idx)
      push_back(arr[idx]);
    total += count;
  }

  inline void grow()
  {
    G_ASSERT(total <= max_slots);
    size_t add = min(array_size, max_slots - total);

    if (total < leak_warning_threshold && total + add >= leak_warning_threshold)
    {
      logerr("sndsys: handle leak warning: more then %d handles allocated, total: %d, arrays: %d, pool type: %d",
        leak_warning_threshold, total + add, arrays.size(), pool_type);
    }

    for (; add != 0;)
    {
      Array &arr = *arrays.emplace_back(eastl::make_unique<Array>());
      const size_t num = min(add, array_size);
      init_array(arr, num);
      add -= num;
    }
  }

  template <size_t MaxCount = 0>
  inline size_t get_num_free_nodes() const
  {
    size_t count = 0;
    for (const Node *node = firstFree; node; node = node->nextFree)
    {
      G_UNREFERENCED(node);
      ++count;
      if constexpr (MaxCount > 0)
      {
        if (count >= MaxCount)
          break;
      }
    }
    return count;
  }

  inline bool need_grow() { return get_num_free_nodes<min_free_slots_grow_threshold>() < min_free_slots_grow_threshold; }

  inline void grow_on_demand()
  {
    if (need_grow())
      grow();
  }

  static inline event_type &get_event(Node &node) { return (event_type &)node.event; }

  template <typename... Args>
  inline void construct(Node &node, Args &&...args)
  {
    new (node.event) event_type(eastl::forward<Args>(args)...);
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
    const ValueParts parts(value);
    if (is_used_generation(parts.generation))
    {
      if (size_t(parts.index) >= total)
      {
        logerr("sndsys: Handle value %lld contains garbage(parts.index %d is out of range, %d nodes total)", int64_t(value),
          size_t(parts.index), total);
        return nullptr;
      }
      Node &node = get_node_from_index(parts.index);
      if (node.generation == parts.generation)
        return &node;
    }
    return nullptr;
  }

  inline intptr_t get_node_index(const Node *node) const
  {
    if (node >= fixedArray.begin() && node < fixedArray.end())
      return node - fixedArray.begin();
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

  template <typename... Args>
  event_type *emplace(value_type &value, Args &&...args)
  {
    value = invalid_value;

    grow_on_demand();

    if (!can_pop_front())
    {
      logerr("sndsys: max pool capacity exceeded, can not grow anymore, total: %d, arrays: %d, pool type: %d", total, arrays.size(),
        pool_type);
      return nullptr;
    }

    Node &node = pop_front();

    // free -> to used generation
    G_STATIC_ASSERT(eastl::is_unsigned<generation_type>::value);
    ++node.generation;
    if (node.generation == 0)
      node.generation = 1;
    G_STATIC_ASSERT(is_used_generation(1));
    G_ASSERT(is_used_generation(node.generation));

    const intptr_t nodeIndex = get_node_index(&node);
    G_ASSERT(nodeIndex >= 0);

    value = ValueParts(index_type(nodeIndex), node.generation).value;

    construct(node, eastl::forward<Args>(args)...);

    return &get_event(node);
  }

  void reject(value_type value)
  {
    if (Node *node = get_node_from_value(value))
    {
      destroy(get_event(*node));
      // push back, pop front to reduce generation overflow
      push_back(*node);

      // used -> to free generation
      G_STATIC_ASSERT(eastl::is_unsigned<generation_type>::value);
      ++node->generation;
      G_STATIC_ASSERT(is_free_generation(0));
      G_ASSERT(is_free_generation(node->generation));
    }
  }

  template <typename Callable>
  inline void enumerate(Callable c)
  {
    for (intptr_t idx = 0; idx < total; ++idx)
    {
      Node &node = get_node_from_index(idx);
      if (is_used_generation(node.generation))
        c(get_event(node), ValueParts(index_type(idx), node.generation).value);
    }
  }

  inline size_t get_free() const { return get_num_free_nodes(); }

  inline size_t get_used() const { return total - get_free(); }

  inline size_t get_total() const { return total; }

  void close()
  {
    enumerate([&](event_type &evt, value_type) { destroy(evt); });
    arrays.clear();
    firstFree = lastFree = nullptr;
    total = 0;
  }

  Pool(const Pool &) = delete;
  Pool(Pool &&) = delete;
  Pool &operator=(const Pool &) = delete;
  Pool &operator=(Pool &&) = delete;

  Pool() { init_array(fixedArray, array_size); }
}; // class Pool
} // namespace sndsys
