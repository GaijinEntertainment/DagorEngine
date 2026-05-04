//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/iterator.h>
#include <EASTL/utility.h>

namespace dag
{

// Iterates a range yielding consecutive adjacent pairs of elements:
// [a, b, c, d] -> (a, b), (b, c), (c, d). Ranges with fewer than two
// elements yield nothing.
template <class Container>
class PairwiseView
{
  using InnerIter = decltype(eastl::begin(eastl::declval<Container>()));
  using InnerValueType = decltype(*eastl::declval<InnerIter>());

public:
  template <class T>
  PairwiseView(T &&cont) : container_{eastl::forward<T>(cont)}
  {}

  class Iterator
  {
  public:
    using iterator_category = eastl::input_iterator_tag;
    using value_type = eastl::pair<InnerValueType, InnerValueType>;
    using difference_type = typename eastl::iterator_traits<InnerIter>::difference_type;
    using reference = value_type;
    using pointer = value_type *;

    Iterator(InnerIter cur, InnerIter end) : cur_{cur}, next_{cur}
    {
      if (next_ != end)
        ++next_;
    }

    Iterator &operator++()
    {
      cur_ = next_++;
      return *this;
    }
    Iterator operator++(int)
    {
      Iterator tmp = *this;
      ++*this;
      return tmp;
    }

    reference operator*() const { return {*cur_, *next_}; }

    // At the end, `next_` reaches the wrapped range's end for both iterators;
    // while iterating, `cur_` and `next_` advance in lockstep so comparing just
    // `next_` is sufficient.
    bool operator==(const Iterator &other) const { return next_ == other.next_; }

  private:
    InnerIter cur_;
    InnerIter next_;
  };

  auto begin() const { return Iterator(eastl::begin(container_), eastl::end(container_)); }
  auto begin() { return Iterator(eastl::begin(container_), eastl::end(container_)); }

  auto end() const { return Iterator(eastl::end(container_), eastl::end(container_)); }
  auto end() { return Iterator(eastl::end(container_), eastl::end(container_)); }

private:
  Container container_;
};

// For lvalue, we want a reference to the original container,
// while for an rvalue we actually want to store it inside the view.
template <class T>
PairwiseView(T &&) -> PairwiseView<T>;

} // namespace dag
