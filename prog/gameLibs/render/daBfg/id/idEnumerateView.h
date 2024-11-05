// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/iterator.h>
#include <EASTL/tuple.h>


template <class Iter, class = void>
struct ReferenceTypePicker
{
  using Type = typename eastl::iterator_traits<Iter>::reference;
};

// Workaround for a "bug" inside eastl::bitvector, whose reference type
// is declared incorrectly
template <class T>
struct ReferenceTypePicker<T, eastl::void_t<typename T::reference_type>>
{
  using Type = typename T::reference_type;
};

// Inspired by P2164
template <class Container>
struct IdEnumerateView
{
  using Base = eastl::remove_cv_t<Container>;
  using BaseIterator = decltype(eastl::declval<Container>().begin());

  using size_type = typename Base::size_type;
  using index_type = typename Base::index_type;
  using value_type = eastl::tuple<const index_type, typename ReferenceTypePicker<BaseIterator>::Type>;

  struct iterator;

  iterator begin() { return {0, container.begin()}; }
  iterator end() { return {container.size(), container.end()}; }

  iterator begin() const { return {0, container.begin()}; }
  iterator end() const { return {container.size(), container.end()}; }

  Container &container;
};

template <class Container>
struct IdEnumerateView<Container>::iterator
{

  // No need to provide anything stronger than a forward iterator, as
  // we only support a single usecase:
  // `for (auto[i, obj] : idIndexedThing.enumerate())`
  using iterator_category = eastl::bidirectional_iterator_tag;
  using difference_type = typename eastl::iterator_traits<BaseIterator>::difference_type;
  using value_type = typename IdEnumerateView<Container>::value_type;
  using reference = value_type;
  using pointer = void;


  auto operator*() { return reference{static_cast<index_type>(pos), *current}; }
  iterator &operator++()
  {
    ++current;
    ++pos;
    return *this;
  }
  iterator &operator--()
  {
    --current;
    --pos;
    return *this;
  }
  iterator operator++(int)
  {
    iterator tmp = *this;
    ++*this;
    return tmp;
  }
  iterator operator--(int)
  {
    iterator tmp = *this;
    --*this;
    return tmp;
  }

  friend bool operator==(const iterator &fst, const iterator &snd) { return fst.current == snd.current; }
  friend bool operator!=(const iterator &fst, const iterator &snd) { return !(fst == snd); }

  decltype(eastl::declval<Container>().size()) pos;
  BaseIterator current;
};
