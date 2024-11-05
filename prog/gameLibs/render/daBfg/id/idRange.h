// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/type_traits.h>
#include <EASTL/iterator.h>


template <class EnumType>
class IdRange
{
  using Underlying = eastl::underlying_type_t<EnumType>;

public:
  IdRange(Underlying end) : begin_{0}, end_{end} {}
  IdRange(Underlying beg, Underlying end) : begin_{beg}, end_{end} { G_ASSERT(beg <= end); }

  class Iterator
  {
    friend class IdRange;

    Iterator(Underlying value) : current_{value} {}

  public:
    using value_type = EnumType;
    using reference = EnumType;
    using pointer = void;
    using iterator_category = eastl::input_iterator_tag;
    using difference_type = eastl::make_signed<Underlying>;

    EnumType operator*() const { return static_cast<EnumType>(current_); }
    // Yeah, this is intentional. Casting &current_ to a pointer
    // is possible, but reading it will incur UB until C++20 :)
    void operator->() const {}

    Iterator operator++()
    {
      ++current_;
      return *this;
    }
    Iterator operator++(int)
    {
      auto copy = *this;
      ++current_;
      return copy;
    }

    Iterator operator--()
    {
      --current_;
      return *this;
    }
    Iterator operator--(int)
    {
      auto copy = *this;
      --current_;
      return copy;
    }

    friend bool operator==(Iterator a, Iterator b) { return a.current_ == b.current_; }
    friend bool operator!=(Iterator a, Iterator b) { return a.current_ != b.current_; }

  private:
    Underlying current_;
  };

  Iterator begin() const { return Iterator(begin_); }
  Iterator end() const { return Iterator(end_); }

private:
  Underlying begin_;
  Underlying end_;
};
