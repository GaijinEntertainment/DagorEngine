//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

// file implements a vector_set<> for relocatable keys
// with fixed_vector like semantics (allowing overflow)
//
// note that for now this uses linear search, which is faster for sizes
// that fit into 1-2 cache lines, but slower otherwise
//
// see dag_relocatableFixedVector.h for more info

#include <generic/dag_relocatableFixedVector.h>


namespace dag
{

template <typename T, size_t inplace_count, bool allow_overflow = true, typename ExtractKey = eastl::use_self<T>,
  typename Allocator = MidmemAlloc, typename Counter = uint32_t>
class FixedVectorSet : protected RelocatableFixedVector<T, inplace_count, allow_overflow, Allocator, Counter>
{
public:
  typedef RelocatableFixedVector<T, inplace_count, allow_overflow, Allocator, Counter> base_type;
  typedef typename base_type::value_type value_type;
  typedef typename base_type::shallow_copy_t shallow_copy_t;
  typedef typename base_type::const_iterator const_iterator;
  typedef typename base_type::iterator iterator;
  typedef typename base_type::reverse_iterator reverse_iterator;
  typedef typename base_type::const_reverse_iterator const_reverse_iterator;
  typedef eastl::pair<iterator, bool> insert_return_type;

  typedef typename base_type::size_type size_type;
  typedef typename base_type::difference_type difference_type;
  using base_type::base_type; // bring in the constructor
  using base_type::capacity;
  using base_type::cbegin;
  using base_type::cend;
  using base_type::clear;
  using base_type::data;
  using base_type::empty;
  using base_type::erase;
  using base_type::getShallowCopy;
  using base_type::reserve;
  using base_type::shrink_to_fit;
  using base_type::size;

  template <typename InputIterator>
  FixedVectorSet(InputIterator first, InputIterator last);

  const_iterator begin() const { return cbegin(); } // it is unsafe to iterate over non-const values
  const_iterator end() const { return cend(); }
  const value_type &operator[](uint32_t idx) const { return base_type::operator[](idx); }

  const_iterator lower_bound(value_type v) const;
  const_iterator find(value_type v) const;
  bool contains(value_type v) const;
  size_type count(value_type v) const { return (size_type)contains(v); }
  insert_return_type insert(value_type v);
  size_type erase(value_type v)
  {
    auto it = find(v);
    if (it == end())
      return 0;
    erase(it);
    return (size_type)1;
  }
};

template <typename T, size_t N, bool O, typename E, typename A, typename C>
template <typename InputIterator>
inline FixedVectorSet<T, N, O, E, A, C>::FixedVectorSet(InputIterator first, InputIterator last)
{
  reserve(eastl::distance(last, first));
  // This is equivalent to an insertion sort
  for (; first != last; ++first)
    insert(*first);
}

template <typename T, size_t N, bool O, typename E, typename A, typename C>
inline typename FixedVectorSet<T, N, O, E, A, C>::const_iterator FixedVectorSet<T, N, O, E, A, C>::lower_bound(value_type v) const
{
  E extractKey;

  const const_iterator s = cbegin();
  const const_iterator e = s + size(); // calling cend would cause a branch
  const_iterator lb = s;

  // use linear search instead of binary as it is faster in small counts (~16), which is our typical case
  // if ever incorrect assumption, binary search is super easy to implement
  for (; lb != e && extractKey(*lb) < extractKey(v); ++lb)
    ;

  return lb;
}

template <typename T, size_t N, bool O, typename E, typename A, typename C>
inline typename FixedVectorSet<T, N, O, E, A, C>::const_iterator FixedVectorSet<T, N, O, E, A, C>::find(value_type v) const
{
  E extractKey;

  const const_iterator s = cbegin();
  const const_iterator e = s + size();

  for (const_iterator lb = s; lb != e; ++lb)
    if (extractKey(v) == extractKey(*lb))
      return lb;
    else if (extractKey(v) < extractKey(*lb))
      break;

  return e;
}

template <typename T, size_t N, bool O, typename E, typename A, typename C>
inline bool FixedVectorSet<T, N, O, E, A, C>::contains(value_type v) const
{
  E extractKey;

  const const_iterator s = cbegin();
  const const_iterator e = s + size();

  for (const_iterator lb = s; lb != e; ++lb)
    if (extractKey(v) == extractKey(*lb))
      return true;
    else if (extractKey(v) < extractKey(*lb))
      break;

  return false;
}

template <typename T, size_t N, bool O, typename E, typename A, typename C>
inline typename FixedVectorSet<T, N, O, E, A, C>::insert_return_type FixedVectorSet<T, N, O, E, A, C>::insert(value_type v)
{
  E extractKey;

  const auto s = base_type::begin();
  const auto e = s + size();
  auto lb = s;

  for (; lb != e; ++lb)
    if (extractKey(v) == extractKey(*lb))
      return {lb, false}; // we already have `v`
    else if (extractKey(v) < extractKey(*lb))
      break;

  lb = base_type::insert(lb, v);
  return {lb, true};
}

template <typename T, size_t N, bool O, typename E, typename A, typename C>
struct is_type_relocatable<FixedVectorSet<T, N, O, E, A, C>, typename eastl::enable_if_t<is_type_relocatable<T>::value>>
  : public eastl::true_type
{};

} // namespace dag
