#pragma once

#include <dag/dag_vector.h>
#include <debug/dag_assert.h>
#include "idEnumerateView.h"


template <class EnumType, class ValueType, typename Allocator = EASTLAllocatorType>
struct IdIndexedMapping : dag::Vector<ValueType, Allocator, true, eastl::underlying_type_t<EnumType>>
{
  using index_type = EnumType;
  using Container = dag::Vector<ValueType, Allocator, true, eastl::underlying_type_t<EnumType>>;
  using Container::Container;

  ValueType &operator[](EnumType key) { return Container::operator[](eastl::to_underlying(key)); }

  const ValueType &operator[](EnumType key) const { return Container::operator[](eastl::to_underlying(key)); }

  // Hide parent's emplace_back: it's not useful and leads to errors
  auto emplace_back(...) = delete;

  // Appends a new mapping element and returns it's id and reference to it.
  // Should only be used when generating ids.
  template <class... Args>
  eastl::pair<EnumType, ValueType &> appendNew(Args &&...args)
  {
    G_FAST_ASSERT(Container::size() < eastl::numeric_limits<eastl::underlying_type_t<EnumType>>::max());
    auto newIndex = static_cast<EnumType>(Container::size());
    return {newIndex, Container::emplace_back(eastl::forward<Args>(args)...)};
  }

  // Automatically expands
  void set(EnumType key, ValueType value)
  {
    expandMapping(key);
    operator[](key) = value;
  }

  // Automatically expands
  ValueType &get(EnumType key, ValueType value)
  {
    expandMapping(key, value);
    return operator[](key);
  }

  ValueType &get(EnumType key)
  {
    expandMapping(key);
    return operator[](key);
  }

  bool isMapped(EnumType key) const { return eastl::to_underlying(key) < Container::size(); }

  void expandMapping(EnumType key, ValueType value)
  {
    if (!isMapped(key))
      Container::resize(eastl::to_underlying(key) + 1, value);
  }

  // Has to be an overload to support move-only types
  void expandMapping(EnumType key)
  {
    if (!isMapped(key))
      Container::resize(eastl::to_underlying(key) + 1);
  }

  IdEnumerateView<IdIndexedMapping> enumerate() & { return {*this}; }
  IdEnumerateView<const IdIndexedMapping> enumerate() const & { return {*this}; }
};
