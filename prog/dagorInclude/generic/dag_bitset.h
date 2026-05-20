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
    for (auto it = mWord + NW - 1, end = mWord - 1; it != end; it--)
    {
      if (*it)
      {
        const size_type lbiw = __clz_unsafe(*it);
        return eastl::distance(mWord, it) * kBitsPerWord + (kBitsPerWord - 1 - lbiw);
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
  // Padding bits in the last word are 0 in the original; without this mask their inverses would yield indices >= N.
  static constexpr WordType getLastWordMask()
  {
    constexpr size_t validBits = (N == 0) ? 0 : ((N - 1) % base_type::kBitsPerWord) + 1;
    if constexpr (validBits == base_type::kBitsPerWord || validBits == 0)
      return ~WordType(0);
    else
      return (WordType(1) << validBits) - WordType(1);
  }

  static constexpr WordType invertedWord(WordType w, size_t index)
  {
    return (index + 1 == NW) ? WordType{~w & getLastWordMask()} : WordType{~w};
  }

  struct SmallIterator
  {
    using this_type = SmallIterator;
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

  struct LargeIterator : SmallIterator
  {
    using base_type = SmallIterator;
    using this_type = LargeIterator;
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

  using Iterator = eastl::conditional_t<NW == 1, SmallIterator, LargeIterator>;

  struct InvertedLargeIterator : LargeIterator
  {
    using base_type = LargeIterator;
    using this_type = InvertedLargeIterator;
    using value_type = typename base_type::value_type;
    using base_type::bitset;
    using base_type::index;
    using base_type::value;

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

          const WordType w = invertedWord(bitset.mWord[index], index);
          if (w)
          {
            value = w;
            break;
          }
        }
      }
      return *this;
    }
  };

  using InvertedIterator = eastl::conditional_t<NW == 1, SmallIterator, InvertedLargeIterator>;

  struct InvertedView
  {
    constexpr InvertedIterator begin() const
    {
      if constexpr (NW == 1)
        return {invertedWord(bitset.mWord[0], 0)};
      else
      {
        for (size_t i = 0; i < NW; ++i)
        {
          const WordType w = invertedWord(bitset.mWord[i], i);
          if (w)
            return {w, i, bitset};
        }
        return end();
      }
    }

    constexpr InvertedIterator end() const
    {
      if constexpr (NW == 1)
        return {0};
      else
        return {0, NW, bitset};
    }

    const Bitset &bitset;
  };

  struct Range
  {
    size_type start, end;
  };

  struct SmallRangeIterator : SmallIterator
  {
    using base_type = SmallIterator;
    using this_type = SmallRangeIterator;
    using difference_type = eastl::make_signed_t<size_type>;
    using value_type = Range;
    using pointer = const Range *;
    using reference = const Range &;
    using base_type::value;

    constexpr value_type operator*() const
    {
      // value = 0 means it is an end iterator, and in normal behaving code before dereferencing the iterator should check
      // with it != end(), so this operator will never be called when value is 0
      G_FAST_ASSERT(value);

      auto currentEnd = __ctz(~value);
      return {current_start, current_start + currentEnd};
    }

    constexpr this_type &operator++()
    {
      auto maskedValue = value & value + 1;
      auto currentEnd = __ctz(maskedValue);
      // When maskedValue == 0, __ctz returns kBitsPerWord; masking the shift amount
      // maps that to >> 0, so 0 >> 0 == 0, which is the end sentinel — no branch needed.
      value = maskedValue >> (currentEnd & (kBitsPerWord - 1));
      current_start += currentEnd;
      return *this;
    }

    constexpr this_type operator++(int)
    {
      const auto copy = *this;
      ++*this;
      return copy;
    }

    size_type current_start = 0;
  };

  struct LargeRangeIterator : SmallRangeIterator
  {
    using base_type = SmallRangeIterator;
    using this_type = LargeRangeIterator;
    using value_type = Range;
    using base_type::current_start;
    using base_type::value;

    value_type operator*() const
    {
      G_FAST_ASSERT(value);
      return {current_start, range_end};
    }

    this_type &operator++()
    {
      advanceFrom(range_end);
      return *this;
    }

    this_type operator++(int)
    {
      const auto copy = *this;
      ++*this;
      return copy;
    }

    constexpr bool operator==(const this_type &other) const { return current_start == other.current_start; }
    constexpr bool operator!=(const this_type &other) const { return !(*this == other); }

    void advanceFrom(size_type searchFrom)
    {
      size_t wi = searchFrom / kBitsPerWord;
      size_t bi = searchFrom % kBitsPerWord;

      // Find next set bit at or after searchFrom
      WordType w = (wi < NW) ? (bitset.mWord[wi] & (~WordType(0) << bi)) : WordType(0);
      while (!w)
      {
        bi = 0;
        if (++wi >= NW)
        {
          current_start = N;
          value = 0;
          return;
        }
        w = bitset.mWord[wi];
      }

      current_start = wi * kBitsPerWord + __ctz_unsafe(w);
      const size_t rangeStartBit = current_start - wi * kBitsPerWord;

      // Find first 0 at or after current_start within word wi
      const WordType zeros = ~bitset.mWord[wi] & (~WordType(0) << rangeStartBit);
      if (zeros)
      {
        range_end = wi * kBitsPerWord + __ctz_unsafe(zeros);
        value = 1;
        return;
      }

      // Range continues into subsequent words; padding bits in the last word
      // are 0 and will terminate the range at exactly N when reached
      for (++wi; wi < NW; ++wi)
      {
        const WordType z = ~bitset.mWord[wi];
        if (z)
        {
          range_end = wi * kBitsPerWord + __ctz_unsafe(z);
          value = 1;
          return;
        }
      }

      // All words were all-ones (only possible when N is a multiple of kBitsPerWord)
      range_end = N;
      value = 1;
    }

    size_type range_end = 0;
    const Bitset &bitset;
  };

  using RangeIterator = eastl::conditional_t<NW == 1, SmallRangeIterator, LargeRangeIterator>;

  struct InvertedLargeRangeIterator : LargeRangeIterator
  {
    using base_type = LargeRangeIterator;
    using this_type = InvertedLargeRangeIterator;
    using base_type::bitset;
    using base_type::current_start;
    using base_type::range_end;
    using base_type::value;

    this_type &operator++()
    {
      advanceFromInverted(range_end);
      return *this;
    }

    this_type operator++(int)
    {
      const auto copy = *this;
      ++*this;
      return copy;
    }

    void advanceFromInverted(size_type searchFrom)
    {
      size_t wi = searchFrom / kBitsPerWord;
      size_t bi = searchFrom % kBitsPerWord;

      // Find next clear bit (set in inverted word) at or after searchFrom
      WordType iw = (wi < NW) ? (invertedWord(bitset.mWord[wi], wi) & (~WordType(0) << bi)) : WordType(0);
      while (!iw)
      {
        bi = 0;
        if (++wi >= NW)
        {
          current_start = N;
          value = 0;
          return;
        }
        iw = invertedWord(bitset.mWord[wi], wi);
      }

      current_start = wi * kBitsPerWord + __ctz_unsafe(iw);
      const size_t rangeStartBit = current_start - wi * kBitsPerWord;

      // Find first set bit (= end of clear range) at or after current_start.
      // ~invertedWord(w, i) == w for non-last words; for the last word padding
      // bits become 1, so the range naturally terminates at N.
      const WordType term = ~invertedWord(bitset.mWord[wi], wi) & (~WordType(0) << rangeStartBit);
      if (term)
      {
        range_end = wi * kBitsPerWord + __ctz_unsafe(term);
        value = 1;
        return;
      }

      for (++wi; wi < NW; ++wi)
      {
        const WordType t = ~invertedWord(bitset.mWord[wi], wi);
        if (t)
        {
          range_end = wi * kBitsPerWord + __ctz_unsafe(t);
          value = 1;
          return;
        }
      }

      range_end = N;
      value = 1;
    }
  };

  using InvertedRangeIterator = eastl::conditional_t<NW == 1, SmallRangeIterator, InvertedLargeRangeIterator>;

  struct RangeView
  {
    constexpr RangeIterator begin() const
    {
      if constexpr (NW == 1)
      {
        if (bitset.mWord[0])
        {
          auto start = __ctz_unsafe(bitset.mWord[0]);
          return {bitset.mWord[0] >> start, start};
        }
      }
      else
      {
        LargeRangeIterator it{0, 0, 0, bitset};
        it.advanceFrom(0);
        return it;
      }
      return end();
    }

    constexpr RangeIterator end() const
    {
      if constexpr (NW == 1)
        return {0};
      else
        return {0, N, 0, bitset};
    }

    const Bitset &bitset;
  };

  struct InvertedRangeView
  {
    constexpr InvertedRangeIterator begin() const
    {
      if constexpr (NW == 1)
      {
        const WordType iw = invertedWord(bitset.mWord[0], 0);
        if (iw)
        {
          const auto start = __ctz_unsafe(iw);
          return {iw >> start, start};
        }
      }
      else
      {
        InvertedLargeRangeIterator it{0, 0, 0, bitset};
        it.advanceFromInverted(0);
        return it;
      }
      return end();
    }

    constexpr InvertedRangeIterator end() const
    {
      if constexpr (NW == 1)
        return {0};
      else
        return {0, N, 0, bitset};
    }

    const Bitset &bitset;
  };

public:
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

  constexpr InvertedView inverted() const { return {*this}; }

  constexpr RangeView ranges() const { return {*this}; }

  constexpr InvertedRangeView invertedRanges() const { return {*this}; }
};

// To ensure the layout doesn't change between eastl::bitset and Bitset,
// They should be interchangeable, only a few member function have been overwritten and optimized in Bitset
static_assert(sizeof(Bitset<32>) == sizeof(eastl::bitset<32>));
static_assert(sizeof(Bitset<1024>) == sizeof(eastl::bitset<1024>));
