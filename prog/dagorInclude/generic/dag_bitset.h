//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/iterator.h>
#include <EASTL/bitset.h>
#include <math/dag_intrin.h>
#include "dag_reverseView.h"


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

  static constexpr size_t NUM_WORDS = BITSET_WORD_COUNT(N, WordType);
  static constexpr bool SINGLE_WORD = (NUM_WORDS == 1);
  static constexpr WordType LAST_WORD_MASK = [] {
    constexpr size_t VALID_BITS = (N == 0) ? 0 : ((N - 1) % kBitsPerWord) + 1;
    if constexpr (VALID_BITS == 0 || VALID_BITS == kBitsPerWord)
      return ~WordType(0);
    else
      return (WordType(1) << VALID_BITS) - WordType(1);
  }();

  Bitset(const base_type &value) : base_type{value} {}

  [[nodiscard]] size_type find_first() const
  {
    for (size_t offset = 0; auto word : mWord)
    {
      if (!word)
      {
        offset += kBitsPerWord;
        continue;
      }
      const size_type fbiw = __ctz_unsafe(word);
      return offset + fbiw;
    }
    return kSize;
  }

  [[nodiscard]] size_type find_last() const
  {
    for (size_t offset = NUM_WORDS * kBitsPerWord; auto word : dag::ReverseView{mWord})
    {
      offset -= kBitsPerWord;
      if (!word)
        continue;
      const size_type lbiw = __clz_unsafe(word);
      return offset + (kBitsPerWord - 1 - lbiw);
    }
    return kSize;
  }

  [[nodiscard]] size_type find_first_and_reset()
  {
    for (size_t offset = 0; auto &word : mWord)
    {
      if (!word)
      {
        offset += kBitsPerWord;
        continue;
      }
      const size_type fbiw = __ctz_unsafe(word);
      word = __blsr(word);
      return offset + fbiw;
    }
    return kSize;
  }

private:
  struct SmallIterator
  {
    using this_type = SmallIterator;
    using iterator_category = eastl::input_iterator_tag;
    using difference_type = eastl::make_signed_t<size_type>;
    using value_type = size_type;
    using pointer = size_type const *;
    using reference = const size_type &;

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

    value_type value;
  };

  template <auto fetchWord>
  struct LargeIteratorImpl
  {
    using this_type = LargeIteratorImpl;
    using iterator_category = eastl::input_iterator_tag;
    using difference_type = eastl::make_signed_t<size_type>;
    using value_type = size_type;
    using pointer = size_type const *;
    using reference = const size_type &;

    value_type operator*() const
    {
      // value = 0 means it is an end iterator, and in normal behaving code before dereferencing the iterator should check
      // with it != end(), so this operator will never be called when value is 0
      G_FAST_ASSERT(value);

      return baseValue + __ctz_unsafe(value);
    }

    this_type &operator++()
    {
      value = __blsr(value);
      return advance();
    }

    this_type operator++(int)
    {
      const this_type copy = *this;
      ++(*this);
      return copy;
    }

    constexpr pointer operator->() const = delete;

    constexpr bool operator==(const this_type &other) const { return value == other.value; }
    constexpr bool operator!=(const this_type &other) const { return value != other.value; }

    this_type &advance()
    {
      if (value)
        return *this;

      const WordType *const wLast = wordPtr - baseValue / kBitsPerWord + (NUM_WORDS - 1);

      for (baseValue += kBitsPerWord; baseValue < kBitsPerWord * NUM_WORDS; baseValue += kBitsPerWord)
      {
        ++wordPtr;
        if (const auto w = fetchWord(wordPtr, wLast))
        {
          value = w;
          break;
        }
      }

      return *this;
    }

    value_type value;
    size_type baseValue;
    const WordType *wordPtr;
  };

  struct Range
  {
    size_type start, end;
  };

  struct SmallRangeIterator
  {
    using this_type = SmallRangeIterator;
    using iterator_category = eastl::input_iterator_tag;
    using difference_type = eastl::make_signed_t<size_type>;
    using value_type = Range;
    using pointer = const Range *;
    using reference = const Range &;

    value_type operator*() const
    {
      G_FAST_ASSERT(value);
      return {currentStart, currentStart + __ctz(~value)};
    }

    this_type &operator++()
    {
      const auto maskedValue = value & (value + 1);
      const auto nextRangeStart = __ctz(maskedValue);
      // When maskedValue == 0, __ctz returns 64; masking the shift amount
      // maps that to >> 0, so 0 >> 0 == 0, which is the end sentinel -- no branch needed.
      value = maskedValue >> (nextRangeStart & (kBitsPerWord - 1));
      currentStart += nextRangeStart;
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

    WordType value;
    size_type currentStart;
  };

  template <auto fetchWord>
  struct LargeRangeIteratorImpl
  {
    using this_type = LargeRangeIteratorImpl;
    using iterator_category = eastl::input_iterator_tag;
    using difference_type = eastl::make_signed_t<size_type>;
    using value_type = Range;
    using pointer = const Range *;
    using reference = const Range &;

    value_type operator*() const
    {
      G_FAST_ASSERT(hasRange);
      return {currentStart, rangeEnd};
    }

    this_type &operator++() { return advanceFrom(rangeEnd); }

    this_type operator++(int)
    {
      const this_type copy = *this;
      ++(*this);
      return copy;
    }

    constexpr pointer operator->() const = delete;

    constexpr bool operator==(const this_type &other) const { return hasRange == other.hasRange; }
    constexpr bool operator!=(const this_type &other) const { return hasRange != other.hasRange; }

    this_type &advanceFrom(size_type search_from)
    {
      const WordType *const wEnd = pWord + NUM_WORDS;
      const WordType *const wLast = wEnd - 1;
      const WordType *wp = pWord + search_from / kBitsPerWord;
      const size_t bi = search_from % kBitsPerWord;

      WordType word = (wp < wEnd) ? (fetchWord(wp, wLast) & (~WordType(0) << bi)) : WordType(0);
      while (!word)
      {
        if (++wp >= wEnd)
        {
          hasRange = false;
          return *this;
        }
        word = fetchWord(wp, wLast);
      }

      const size_t rangeStartBit = __ctz_unsafe(word);
      currentStart = size_t(wp - pWord) * kBitsPerWord + rangeStartBit;

      if (const WordType maskedWord = ~fetchWord(wp, wLast) & (~WordType(0) << rangeStartBit))
      {
        rangeEnd = size_t(wp - pWord) * kBitsPerWord + __ctz_unsafe(maskedWord);
        return *this;
      }

      for (++wp; wp < wEnd; ++wp)
      {
        if (const WordType maskedWord = ~fetchWord(wp, wLast))
        {
          rangeEnd = size_t(wp - pWord) * kBitsPerWord + __ctz_unsafe(maskedWord);
          return *this;
        }
      }

      rangeEnd = N;
      return *this;
    }

    bool hasRange;
    size_type currentStart;
    size_type rangeEnd;
    const WordType *const pWord;
  };

  template <typename Iter, auto firstWord>
  struct RangeViewImpl
  {
    constexpr Iter begin() const
    {
      if constexpr (SINGLE_WORD)
      {
        const WordType fw = firstWord(mWord);
        if (fw)
        {
          const auto start = __ctz_unsafe(fw);
          return {fw >> start, start};
        }
        return {0};
      }
      else
      {
        return Iter{true, 0, 0, mWord}.advanceFrom(0);
      }
    }

    constexpr Iter end() const { return {0}; }

    const decltype(base_type::mWord) &mWord;
  };

  using LargeIterator = LargeIteratorImpl<[](const WordType *p, const WordType *) -> WordType { return *p; }>;

  using Iterator = eastl::conditional_t<SINGLE_WORD, SmallIterator, LargeIterator>;

  using InvertedLargeIterator = LargeIteratorImpl<[](const WordType *p, const WordType *wLast) -> WordType {
    return (p == wLast) ? (~*p & LAST_WORD_MASK) : ~*p;
  }>;

  using InvertedIterator = eastl::conditional_t<SINGLE_WORD, SmallIterator, InvertedLargeIterator>;

  using LargeRangeIterator = LargeRangeIteratorImpl<[](const WordType *p, const WordType *) -> WordType { return *p; }>;

  using RangeIterator = eastl::conditional_t<SINGLE_WORD, SmallRangeIterator, LargeRangeIterator>;

  using InvertedLargeRangeIterator = LargeRangeIteratorImpl<[](const WordType *p, const WordType *wLast) -> WordType {
    return (p == wLast) ? (~*p & LAST_WORD_MASK) : ~*p;
  }>;

  using InvertedRangeIterator = eastl::conditional_t<SINGLE_WORD, SmallRangeIterator, InvertedLargeRangeIterator>;

  struct InvertedView
  {
    constexpr InvertedIterator begin() const
    {
      if constexpr (SINGLE_WORD)
        return {~mWord[0] & LAST_WORD_MASK};
      else
        return InvertedLargeIterator{~mWord[0], 0, mWord}.advance();
    }

    constexpr InvertedIterator end() const { return {0}; }

    const decltype(base_type::mWord) &mWord;
  };

  using RangeView = RangeViewImpl<RangeIterator, [](const auto &mWord) -> WordType { return mWord[0]; }>;

  using InvertedRangeView =
    RangeViewImpl<InvertedRangeIterator, [](const auto &mWord) -> WordType { return ~mWord[0] & LAST_WORD_MASK; }>;

public:
  constexpr Iterator begin() const
  {
    if constexpr (SINGLE_WORD)
      return {mWord[0]};
    else
      return Iterator{mWord[0], 0, mWord}.advance();
  }

  constexpr Iterator end() const { return {0}; }

  constexpr InvertedView inverted() const { return {mWord}; }

  constexpr RangeView ranges() const { return {mWord}; }

  constexpr InvertedRangeView invertedRanges() const { return {mWord}; }
};

// To ensure the layout doesn't change between eastl::bitset and Bitset,
// They should be interchangeable, only a few member function have been overwritten and optimized in Bitset
static_assert(sizeof(Bitset<32>) == sizeof(eastl::bitset<32>));
static_assert(sizeof(Bitset<1024>) == sizeof(eastl::bitset<1024>));
