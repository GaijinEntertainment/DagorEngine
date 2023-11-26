//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/iterator.h>
#include <EASTL/type_traits.h>
#include "dag_intrin.h"


/// Visitor to iterate through the set bits of the initial integer value.
/// The operator* will give back the index of least significant set bit.
/// On most of the platforms it will use intrinsics to perform the calculation in 1 or 2 instructions.
/// E.g. in
/// for(auto i : LsbVisitor{0x4002})
///   print(i);
///
/// code example the block of for loop will execute only twice and will print out the *1* and *14*
///
/// 0100000000000010b
///               <--
template <bool is64Bit = false>
struct LeastSignificantSetBitVisitor
{
  struct Iterator
  {
    using iterator_category = eastl::input_iterator_tag;
    using value_type = eastl::conditional_t<is64Bit, uint64_t, uint32_t>;
    using difference_type = eastl::conditional_t<is64Bit, int64_t, int32_t>;
    using pointer = value_type const *;
    using reference = const value_type &;

    value_type operator*() const
    {
      // value = 0 means it is an end iterator, and in normal behaving code before dereferencing the iterator should check
      // with it != end(), so this operator will never be called when value is 0
      G_ASSERT(value);

      return __ctz_unsafe(value);
    }

    Iterator &operator++()
    {
      value = __blsr(value);
      return *this;
    }

    Iterator operator++(int)
    {
      const auto copy = *this;
      ++*this;
      return copy;
    }

    constexpr pointer operator->() const = delete;

    constexpr bool operator==(const Iterator &other) const { return value == other.value; }
    constexpr bool operator!=(const Iterator &other) const { return value != other.value; }

    value_type value = 0;
  };

  constexpr Iterator begin() const { return {value}; }
  constexpr Iterator end() const { return {}; }

  typename Iterator::value_type value = 0;
};

template <class T>
LeastSignificantSetBitVisitor(T value) -> LeastSignificantSetBitVisitor<sizeof(T) == 8>;

template <bool is64Bit>
struct LsbVisitor : LeastSignificantSetBitVisitor<is64Bit>
{};

template <class T>
LsbVisitor(T value) -> LsbVisitor<sizeof(T) == 8>;
