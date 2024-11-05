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
class FixedVectorMap
  : protected FixedVectorSet<eastl::pair<K, V>, inplace_count, allow_overflow, eastl::use_first<eastl::pair<K, V>>, Allocator, Counter>
{
public:
  typedef FixedVectorSet<eastl::pair<K, V>, inplace_count, allow_overflow, eastl::use_first<eastl::pair<K, V>>, Allocator, Counter>
    base_type;
  typedef typename base_type::value_type value_type;
  typedef typename base_type::shallow_copy_t shallow_copy_t;
  typedef typename base_type::const_iterator const_iterator;
  typedef typename base_type::iterator iterator;
  typedef typename base_type::reverse_iterator reverse_iterator;
  typedef typename base_type::const_reverse_iterator const_reverse_iterator;
  typedef typename base_type::size_type size_type;
  typedef typename base_type::difference_type difference_type;
  using base_type::base_type;
  using base_type::begin;
  using base_type::capacity;
  using base_type::cbegin;
  using base_type::cend;
  using base_type::clear;
  using base_type::data;
  using base_type::empty;
  using base_type::end;
  using base_type::erase;
  using base_type::getShallowCopy;
  using base_type::insert;
  using base_type::reserve;
  using base_type::shrink_to_fit;
  using base_type::size;

  typedef V mapped_type;
  typedef K key_type;

  // No references here as both key and value are supposed to be extremely
  // small types, i.e. integers.
  const_iterator find(key_type key) const { return base_type::find({key, {}}); }
  mapped_type &operator[](key_type key) { return base_type::insert({key, {}}).first->second; }
  size_type erase(key_type key) { return base_type::erase({key, {}}); }

  bool contains(key_type key) const { return base_type::contains({key, {}}); }
  // This compiles faster than a variadic template but serves exactly the same purpose.
  eastl::pair<iterator, bool> emplace(key_type key, mapped_type value) { return base_type::insert({key, value}); }
};

} // namespace dag
