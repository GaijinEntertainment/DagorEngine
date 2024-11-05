//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/iterator.h>
#include <EASTL/bitset.h>
#include <math/dag_intrin.h>


template <size_t N, typename WordType = EASTL_BITSET_WORD_TYPE_DEFAULT>
class Bitset : public eastl::bitset<N, WordType>
{
public:
  using base_type = eastl::bitset<N, WordType>;
  using this_type = Bitset<N, WordType>;
  using word_type = WordType;
  using size_type = typename base_type::size_type;

  using base_type::base_type;

  using base_type::kBitsPerWord;
  using base_type::kSize;
  using base_type::mWord;

  static constexpr size_t NW = BITSET_WORD_COUNT(N, WordType);

  Bitset(const base_type &value) : base_type{value} {}

  size_type find_first() const
  {
    for (auto &word : mWord)
    {
      if (word)
      {
        const size_type fbiw = __ctz_unsafe(word);
        return eastl::distance(mWord, eastl::addressof(word)) * kBitsPerWord + fbiw;
      }
    }
    return kSize;
  }

  size_type find_last() const
  {
    for (auto it = mWord + N - 1, end = mWord - 1; it != end; it--)
    {
      if (*it)
      {
        const size_type lbiw = __clz_unsafe(*it);
        return (N - eastl::distance(mWord, it)) * kBitsPerWord + lbiw;
      }
    }
    return kSize;
  }

  size_type find_first_and_reset()
  {
    for (auto &word : mWord)
    {
      if (word)
      {
        const size_type fbiw = __ctz_unsafe(word);
        word = __blsr(word);
        return eastl::distance(mWord, eastl::addressof(word)) * kBitsPerWord + fbiw;
      }
    }
    return kSize;
  }

private:
  struct IteratorBaseA
  {
    using this_type = IteratorBaseA;
    using iterator_category = eastl::input_iterator_tag;
    using difference_type = eastl::make_signed_t<WordType>;
    using value_type = WordType;
    using pointer = WordType const *;
    using reference = const WordType &;

    value_type operator*() const
    {
      // value = 0 means it is an end iterator, and in normal behaving code before dereferencing the iterator should check
      // with it != end(), so this operator will never be called when value is 0
      G_FAST_ASSERT(value);

      return __ctz_unsafe(value);
    }

    this_type &operator++()
    {
      value = __blsr(value);
      return *this;
    }

    this_type operator++(int)
    {
      const auto copy = *this;
      ++*this;
      return copy;
    }

    constexpr pointer operator->() const = delete;

    constexpr bool operator==(const this_type &other) const { return value == other.value; }
    constexpr bool operator!=(const this_type &other) const { return value != other.value; }

    value_type value = 0;
  };

  struct IteratorBaseB : IteratorBaseA
  {
    using base_type = IteratorBaseA;
    using this_type = IteratorBaseB;
    using value_type = typename base_type::value_type;
    using pointer = typename base_type::pointer;

    using base_type::value;

    value_type operator*() const { return index * kBitsPerWord + base_type::operator*(); }

    this_type &operator++()
    {
      value = __blsr(value);
      if (value == 0)
      {
        for (;;)
        {
          index++;

          if (index == NW)
            break;

          if (bitset.mWord[index])
          {
            value = bitset.mWord[index];
            break;
          }
        }
      }
      return *this;
    }

    constexpr bool operator==(const this_type &other) const { return value == other.value && index == other.index; }
    constexpr bool operator!=(const this_type &other) const { return !(*this == other); }

    size_t index = 0;
    const Bitset &bitset;
  };

public:
  struct Iterator : eastl::conditional_t<NW == 1, IteratorBaseA, IteratorBaseB>
  {};

  constexpr Iterator begin() const
  {
    if constexpr (NW == 1)
      return {mWord[0]};
    else
    {
      const size_t firstWordWithValue =
        eastl::distance(mWord, eastl::find_if(eastl::begin(mWord), eastl::end(mWord), [](const auto &x) { return x; }));
      if (firstWordWithValue < NW)
        return {mWord[firstWordWithValue], firstWordWithValue, *this};
      else
        return end();
    }
  }

  constexpr Iterator end() const
  {
    if constexpr (NW == 1)
      return {0};
    else
      return {0, NW, *this};
  }
};

// To ensure the layout doesn't change between eastl::bitset and Bitset,
// They should be interchangeable, only a few member function have been overwritten and optimized in Bitset
static_assert(sizeof(Bitset<32>) == sizeof(eastl::bitset<32>));
static_assert(sizeof(Bitset<1024>) == sizeof(eastl::bitset<1024>));
