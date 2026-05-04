// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/type_traits.h>
#include <EASTL/bonus/compressed_pair.h>
#include <EASTL/internal/memory_base.h>
#include <EASTL/iterator.h>
#include <vecmath/dag_vecMath.h>
#include <generic/dag_align.h>
#include <debug/dag_assert.h>
#include <string.h>
#include <id/bitSearch.h>
#include <id/idEnumerateView.h>


template <class EnumType, typename Allocator = EASTLAllocatorType>
struct IdIndexedFlags
{
  using index_type = EnumType;
  using SizeType = eastl::underlying_type_t<EnumType>;
  using size_type = SizeType;

  template <class OtherEnumType, class OtherAllocator>
  friend struct IdIndexedFlags;

  struct BitReference
  {
    uint32_t *word;
    uint32_t mask;
    operator bool() const { return (*word & mask) != 0; }
    BitReference &operator=(bool v)
    {
      if (v)
        *word |= mask;
      else
        *word &= ~mask;
      return *this;
    }
    BitReference &operator=(const BitReference &other) { return operator=(bool(other)); }
  };

  struct iterator
  {
    using iterator_category = eastl::random_access_iterator_tag;
    using value_type = BitReference;
    using difference_type = ptrdiff_t;
    using pointer = void;
    using reference = BitReference;
    using reference_type = BitReference;

    uint32_t *bitsData;
    SizeType pos;

    BitReference operator*() const { return BitReference{bitsData + pos / WORD_BITS, 1u << (pos % WORD_BITS)}; }
    iterator &operator++()
    {
      ++pos;
      return *this;
    }
    iterator &operator--()
    {
      --pos;
      return *this;
    }
    iterator operator++(int)
    {
      auto t = *this;
      ++pos;
      return t;
    }
    iterator operator--(int)
    {
      auto t = *this;
      --pos;
      return t;
    }
    bool operator==(const iterator &o) const { return pos == o.pos; }
    difference_type operator-(const iterator &o) const
    {
      return static_cast<difference_type>(pos) - static_cast<difference_type>(o.pos);
    }
  };

  struct const_iterator
  {
    using iterator_category = eastl::random_access_iterator_tag;
    using value_type = bool;
    using difference_type = ptrdiff_t;
    using pointer = void;
    using reference = bool;
    using reference_type = bool;

    const uint32_t *bitsData;
    SizeType pos;

    bool operator*() const { return (bitsData[pos / WORD_BITS] & (1u << (pos % WORD_BITS))) != 0; }
    const_iterator &operator++()
    {
      ++pos;
      return *this;
    }
    const_iterator &operator--()
    {
      --pos;
      return *this;
    }
    const_iterator operator++(int)
    {
      auto t = *this;
      ++pos;
      return t;
    }
    const_iterator operator--(int)
    {
      auto t = *this;
      --pos;
      return t;
    }
    bool operator==(const const_iterator &o) const { return pos == o.pos; }
    difference_type operator-(const const_iterator &o) const
    {
      return static_cast<difference_type>(pos) - static_cast<difference_type>(o.pos);
    }
  };

  iterator begin() { return iterator{bits, SizeType{0}}; }
  iterator end() { return iterator{bits, bitCount}; }
  const_iterator begin() const { return const_iterator{bits, SizeType{0}}; }
  const_iterator end() const { return const_iterator{bits, bitCount}; }

  IdIndexedFlags(const Allocator &alloc = Allocator{}) : capacityInWordsAndAlloc(0, alloc) {}

  IdIndexedFlags(size_t count, bool val, const Allocator &alloc = Allocator{}) : capacityInWordsAndAlloc(0, alloc)
  {
    resize(static_cast<SizeType>(count), val);
  }

  template <class OtherAllocator>
  IdIndexedFlags(const IdIndexedFlags<EnumType, OtherAllocator> &other, const Allocator &alloc = Allocator{}) :
    capacityInWordsAndAlloc(0, alloc)
  {
    if (other.size() > 0)
    {
      SizeType cap = bitCountToCapacity(other.bitCount);
      reallocate(cap);
      memcpy(bits, other.bits, cap * sizeof(uint32_t));
    }
    bitCount = other.size();
  }

  IdIndexedFlags(const IdIndexedFlags &other) : capacityInWordsAndAlloc(0, other.capacityInWordsAndAlloc.second())
  {
    if (other.bitCount > 0)
    {
      SizeType cap = bitCountToCapacity(other.bitCount);
      reallocate(cap);
      memcpy(bits, other.bits, cap * sizeof(uint32_t));
    }
    bitCount = other.bitCount;
  }

  IdIndexedFlags(IdIndexedFlags &&other) :
    bits{eastl::exchange(other.bits, nullptr)},
    capacityInWordsAndAlloc(eastl::exchange(other.capacityInWordsAndAlloc.first(), SizeType{0}),
      eastl::move(other.capacityInWordsAndAlloc.second())),
    bitCount{eastl::exchange(other.bitCount, SizeType{0})}
  {}

  IdIndexedFlags &operator=(const IdIndexedFlags &other)
  {
    if (this == &other)
      return *this;
    SizeType needed = bitCountToCapacity(other.bitCount);
    if (needed > capacityInWords())
    {
      deallocate();
      reallocate(needed);
    }
    if (other.bitCount > 0)
      memcpy(bits, other.bits, needed * sizeof(uint32_t));
    if (needed < capacityInWords())
      memset(bits + needed, 0, (capacityInWords() - needed) * sizeof(uint32_t));
    bitCount = other.bitCount;
    return *this;
  }

  IdIndexedFlags &operator=(IdIndexedFlags &&other)
  {
    if (this == &other)
      return *this;
    deallocate();
    bits = eastl::exchange(other.bits, nullptr);
    capacityInWordsAndAlloc.first() = eastl::exchange(other.capacityInWordsAndAlloc.first(), SizeType{0});
    bitCount = eastl::exchange(other.bitCount, SizeType{0});
    return *this;
  }

  ~IdIndexedFlags() { deallocate(); }

  BitReference operator[](EnumType key)
  {
    G_FAST_ASSERT(isMapped(key));
    auto idx = eastl::to_underlying(key);
    return BitReference{bits + idx / WORD_BITS, 1u << (idx % WORD_BITS)};
  }

  bool operator[](EnumType key) const
  {
    G_FAST_ASSERT(isMapped(key));
    auto idx = eastl::to_underlying(key);
    return (bits[idx / WORD_BITS] & (1u << (idx % WORD_BITS))) != 0;
  }

  void set(EnumType key, bool val)
  {
    if (!isMapped(key))
      resize(eastl::to_underlying(key) + 1, false);
    operator[](key) = val;
  }

  bool test(EnumType key, bool default_value = false) const
  {
    if (!isMapped(key))
      return default_value;
    return operator[](key);
  }

  SizeType size() const { return bitCount; }
  SizeType capacity() const { return capacityInWords() * WORD_BITS; }
  bool empty() const { return bitCount == 0; }
  bool isMapped(EnumType key) const { return eastl::to_underlying(key) < bitCount; }

  void reserve(size_t new_cap_bits)
  {
    const SizeType newCapNeeded = bitCountToCapacity(static_cast<SizeType>(new_cap_bits));
    if (newCapNeeded > capacityInWords())
      reallocate(newCapNeeded);
  }

  void resize(size_t new_count, bool val = false)
  {
    reserve(new_count);

    const SizeType newCount = static_cast<SizeType>(new_count);
    if (newCount > bitCount)
    {
      if (val)
        fillBits(bitCount, newCount);
    }
    else if (newCount < bitCount)
    {
      clearTail(newCount);
    }
    bitCount = newCount;
  }

  void assign(SizeType count, bool val)
  {
    reserve(count);

    if (val && count > 0)
    {
      memset(bits, 0xFF, byteSize(count));
      clearTail(count);
    }
    else if (bits)
    {
      memset(bits, 0, byteSize(bitCount));
    }
    bitCount = count;
  }

  void clear()
  {
    if (bits)
      memset(bits, 0, byteSize(bitCount));
    bitCount = 0;
  }

  struct TrueKeyIterator
  {
    using iterator_category = eastl::bidirectional_iterator_tag;
    using value_type = EnumType;
    using difference_type = ptrdiff_t;
    using pointer = const EnumType *;
    using reference = EnumType;

    const IdIndexedFlags *flags;
    SizeType pos;
    uint32_t currentSubword;

    TrueKeyIterator &operator++()
    {
      pos = bit_search<true, true>(flags->bits, static_cast<uint32_t>(flags->bitCount), pos + 1, currentSubword);
      return *this;
    }

    TrueKeyIterator operator++(int)
    {
      auto tmp = *this;
      ++(*this);
      return tmp;
    }

    TrueKeyIterator &operator--()
    {
      pos = bit_search<true, false>(flags->bits, static_cast<uint32_t>(flags->bitCount), pos - 1, currentSubword);
      return *this;
    }

    TrueKeyIterator operator--(int)
    {
      auto tmp = *this;
      --(*this);
      return tmp;
    }

    bool operator==(const TrueKeyIterator &o) const { return pos == o.pos; }
    EnumType operator*() const { return static_cast<EnumType>(pos); }
  };

  struct TrueKeyView
  {
    TrueKeyIterator b, e;
    TrueKeyIterator begin() const { return b; }
    TrueKeyIterator end() const { return e; }
  };

  TrueKeyView trueKeys() const
  {
    auto sz = static_cast<uint32_t>(bitCount);
    uint32_t sw = 0;
    uint32_t firstPos = bitCount > 0 ? bit_search<true, true>(bits, sz, 0, sw) : bitCount;
    return {TrueKeyIterator{this, static_cast<SizeType>(firstPos), sw}, TrueKeyIterator{this, bitCount, 0}};
  }

  bool all() const
  {
    if (bitCount == 0)
      return true;
    uint32_t sw = 0;
    SizeType firstZero = bit_search<false, true>(bits, static_cast<uint32_t>(bitCount), 0, sw);
    G_FAST_ASSERT(firstZero <= bitCount);
    return firstZero == bitCount;
  }

  IdEnumerateView<IdIndexedFlags> enumerate() & { return {*this}; }
  IdEnumerateView<const IdIndexedFlags> enumerate() const & { return {*this}; }

private:
  static constexpr SizeType WORD_BITS = sizeof(uint32_t) * CHAR_BIT;
  static constexpr SizeType WORDS_PER_VEC4 = sizeof(vec4f) / sizeof(uint32_t);

  uint32_t *bits = nullptr;
  eastl::compressed_pair<SizeType, Allocator> capacityInWordsAndAlloc{SizeType{0}, Allocator{}};
  SizeType bitCount = 0;

  SizeType capacityInWords() const { return capacityInWordsAndAlloc.first(); }
  size_t byteSize(SizeType bit_count) const { return (bit_count + CHAR_BIT - 1) / CHAR_BIT; }

  // We always keep capacity rounded up to vec4, so that we can safely use
  // SIMD loads/stores without worrying about going OOB at the end of storage.
  static SizeType bitCountToCapacity(SizeType bit_cnt)
  {
    SizeType words = (bit_cnt + WORD_BITS - 1) / WORD_BITS;
    return (words + WORDS_PER_VEC4 - 1) & ~(WORDS_PER_VEC4 - 1);
  }

  static bool tryResizeInplace(Allocator &a, void *p, size_t sz)
  {
    if constexpr (requires { a.resizeInplace(p, sz); })
      return a.resizeInplace(p, sz);
    else
      return false;
  }

  void reallocate(SizeType new_cap_in_words)
  {
    auto &alloc = capacityInWordsAndAlloc.second();

    if (bits && tryResizeInplace(alloc, bits, new_cap_in_words * sizeof(uint32_t)))
    {
      SizeType oldCap = capacityInWords();
      G_FAST_ASSERT(new_cap_in_words > oldCap);
      memset(bits + oldCap, 0, (new_cap_in_words - oldCap) * sizeof(uint32_t));
      capacityInWordsAndAlloc.first() = new_cap_in_words;
      return;
    }

    auto *newBits = static_cast<uint32_t *>(eastl::allocate_memory(alloc, new_cap_in_words * sizeof(uint32_t), alignof(vec4f), 0));

    if (bits) //-V1051
    {
      SizeType oldCap = capacityInWords();
      G_FAST_ASSERT(new_cap_in_words > oldCap);
      memcpy(newBits, bits, oldCap * sizeof(uint32_t));
      memset(newBits + oldCap, 0, (new_cap_in_words - oldCap) * sizeof(uint32_t));
      alloc.deallocate(bits, 0);
    }
    else
    {
      memset(newBits, 0, new_cap_in_words * sizeof(uint32_t));
    }

    bits = newBits;
    capacityInWordsAndAlloc.first() = new_cap_in_words;
  }

  void deallocate()
  {
    if (bits)
    {
      capacityInWordsAndAlloc.second().deallocate(bits, 0);
      bits = nullptr;
    }
  }

  // [from, to) range.
  void fillBits(SizeType from, SizeType to)
  {
    if (from >= to)
      return;
    SizeType firstW = from / WORD_BITS;
    SizeType lastW = (to - 1) / WORD_BITS;
    if (firstW == lastW)
    {
      uint32_t mask = (~0u << (from % WORD_BITS)) & (~0u >> (WORD_BITS - 1 - ((to - 1) % WORD_BITS)));
      bits[firstW] |= mask;
    }
    else
    {
      bits[firstW] |= ~0u << (from % WORD_BITS);
      memset(bits + firstW + 1, 0xFF, (lastW - firstW - 1) * sizeof(uint32_t));
      bits[lastW] |= ~0u >> (WORD_BITS - 1 - ((to - 1) % WORD_BITS));
    }
  }

  void clearTail(SizeType from)
  {
    SizeType firstW = from / WORD_BITS;
    if (from % WORD_BITS != 0)
    {
      bits[firstW] &= ~(~0u << (from % WORD_BITS));
      ++firstW;
    }
    memset(bits + firstW, 0, (capacityInWords() - firstW) * sizeof(uint32_t));
  }
};
