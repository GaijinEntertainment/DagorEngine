//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// specialized wrapper around FixedVectorSet that contains
// key-value pairs and sorts by key only
//
// see dag_fixedVectorSet.h for more info

#include <generic/dag_fixedVectorSet.h>


namespace dag
{

template <typename K, typename V, size_t inplace_count, bool allow_overflow = true, typename Allocator = MidmemAlloc,
  typename Counter = uint32_t>
class FixedVectorMap : protected FixedVectorSet<eastl::pair<const K, V>, inplace_count, allow_overflow,
                         eastl::use_first<eastl::pair<const K, V>>, Allocator, Counter>
{
public:
  typedef FixedVectorSet<eastl::pair<const K, V>, inplace_count, allow_overflow, eastl::use_first<eastl::pair<const K, V>>, Allocator,
    Counter>
    base_type;
  typedef typename base_type::value_type value_type;
  typedef typename base_type::shallow_copy_t shallow_copy_t;
  typedef typename base_type::iterator iterator;
  typedef typename base_type::const_iterator const_iterator;
  typedef typename base_type::size_type size_type;
  typedef typename base_type::difference_type difference_type;

  typedef V mapped_type;
  typedef K key_type;

  using base_type::base_type;

  // pair<const K, V> is not copy-assignable, so the base assign (which uses
  // operator= on elements) cannot be used. Provide explicit copy assignment
  // via copy-construct + move-assign, which only uses placement new and memcpy.
  FixedVectorMap() = default;
  FixedVectorMap(const FixedVectorMap &) = default;
  FixedVectorMap(FixedVectorMap &&) = default;
  FixedVectorMap &operator=(FixedVectorMap &&) = default;
  FixedVectorMap &operator=(const FixedVectorMap &other)
  {
    if (this != &other)
    {
      FixedVectorMap tmp(other);
      *this = static_cast<FixedVectorMap &&>(tmp);
    }
    return *this;
  }

  using base_type::capacity;
  using base_type::clear;
  using base_type::data;
  using base_type::empty;
  using base_type::getShallowCopy;
  using base_type::reserve;
  using base_type::shrink_to_fit;
  using base_type::size;

  // FixedVectorSet only exposes const iterators. Since value_type is
  // pair<const K, V>, mutable iterators are safe: keys stay immutable.
  iterator begin() { return data(); }
  iterator end() { return data() + size(); }
  const_iterator begin() const { return base_type::cbegin(); }
  const_iterator end() const { return base_type::cend(); }
  const_iterator cbegin() const { return begin(); }
  const_iterator cend() const { return end(); }

  // No references here as both key and value are supposed to be extremely
  // small types, i.e. integers.
  const_iterator find(key_type key) const { return base_type::find({key, {}}); }
  iterator find(key_type key)
  {
    auto cit = base_type::find({key, {}});
    if (cit == base_type::cend())
      return end();
    return begin() + (cit - base_type::cbegin());
  }
  mapped_type &operator[](key_type key) { return base_type::insert({key, {}}).first->second; }

  eastl::pair<iterator, bool> insert(value_type v) { return base_type::insert(v); }

  size_type erase(key_type key) { return base_type::erase({key, {}}); }
  iterator erase(const_iterator it) { return base_type::erase(it); }
  iterator erase(const_iterator first, const_iterator last) { return base_type::erase(first, last); }

  bool contains(key_type key) const { return base_type::contains({key, {}}); }
  // This compiles faster than a variadic template but serves exactly the same purpose.
  eastl::pair<iterator, bool> emplace(key_type key, mapped_type value) { return base_type::insert({key, value}); }

  friend bool operator==(const FixedVectorMap &, const FixedVectorMap &) = default;
};

} // namespace dag
