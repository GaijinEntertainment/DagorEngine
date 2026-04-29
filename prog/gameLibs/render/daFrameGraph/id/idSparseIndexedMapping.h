// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/type_traits.h>
#include <EASTL/memory.h>
#include <EASTL/utility.h>
#include <EASTL/tuple.h>
#include <EASTL/bonus/compressed_pair.h>
#include <EASTL/string.h>
#include <util/dag_globDef.h>
#include <math/dag_intrin.h>
#include <vecmath/dag_vecMath.h>
#include <generic/dag_zip.h>
#include <debug/dag_assert.h>
#include <id/idIndexedFlags.h>
#include <id/bitSearch.h>


// Basically an Id -> Value map that is supposed to have
// very few holes in it.
template <class EnumType, class ValueType, typename Allocator = EASTLAllocatorType>
class IdSparseIndexedMapping
{
  using SizeType = eastl::underlying_type_t<EnumType>;
  using WordType = vec4f;
  static constexpr size_t WORD_BITS = sizeof(WordType) * CHAR_BIT;
  static constexpr size_t SUBWORD_BITS = sizeof(float) * CHAR_BIT;

  template <class OtherEnumType, class OtherValueType, typename OtherAllocator>
  friend class IdSparseIndexedMapping;

public:
  using index_type = EnumType;
  using value_type = ValueType;

  IdSparseIndexedMapping(const Allocator &allocator = Allocator()) : capacityInWordsAndAllocator(0, allocator) {}

  IdSparseIndexedMapping(const IdSparseIndexedMapping &other) :
    capacityInWordsAndAllocator(other.capacityInWordsAndAllocator), size(other.size), usedCount(other.usedCount)
  {
    copyInitFrom(other.elements, other.usedBits);
  }
  IdSparseIndexedMapping &operator=(const IdSparseIndexedMapping &other)
  {
    if (this == &other)
      return *this;

    // TODO: don't re-allocate if we have enough capacity
    destroyAndDeallocate();

    elements = nullptr;
    usedBits = nullptr;
    capacityInWordsAndAllocator = other.capacityInWordsAndAllocator;
    size = other.size;
    usedCount = other.usedCount;
    copyInitFrom(other.elements, other.usedBits);
    return *this;
  }

  IdSparseIndexedMapping(IdSparseIndexedMapping &&other) :
    elements(eastl::exchange(other.elements, nullptr)),
    usedBits(eastl::exchange(other.usedBits, nullptr)),
    capacityInWordsAndAllocator(eastl::exchange(other.capacityInWordsAndAllocator.first(), 0),
      // TODO: handle stateful allocators properly
      eastl::exchange(other.capacityInWordsAndAllocator.second(), Allocator{})),
    size(eastl::exchange(other.size, 0)),
    usedCount(eastl::exchange(other.usedCount, 0))
  {}
  IdSparseIndexedMapping &operator=(IdSparseIndexedMapping &&other)
  {
    if (this == &other)
      return *this;
    destroyAndDeallocate();
    elements = eastl::exchange(other.elements, nullptr);
    usedBits = eastl::exchange(other.usedBits, nullptr);
    capacityInWordsAndAllocator.first() = eastl::exchange(other.capacityInWordsAndAllocator.first(), 0);
    // TODO: handle stateful allocators properly
    capacityInWordsAndAllocator.second() = eastl::exchange(other.capacityInWordsAndAllocator.second(), Allocator{});
    size = eastl::exchange(other.size, 0);
    usedCount = eastl::exchange(other.usedCount, 0);
    return *this;
  }


  void reserve(size_t new_cap)
  {
    if (capacityInWordsAndAllocator.first() * WORD_BITS < new_cap)
      relocate((new_cap + WORD_BITS - 1) / WORD_BITS);
  }

  bool isMapped(EnumType key) const
  {
    const SizeType idx = eastl::to_underlying(key);
    return idx < size && isUsed(idx);
  }

  ValueType &operator[](EnumType key)
  {
    G_ASSERT(isMapped(key));
    return elements[eastl::to_underlying(key)].value;
  }

  const ValueType &operator[](EnumType key) const
  {
    G_ASSERT(isMapped(key));
    return elements[eastl::to_underlying(key)].value;
  }

  ValueType &get(EnumType key)
  {
    if (!isMapped(key))
      emplaceAt(key);
    return elements[eastl::to_underlying(key)].value;
  }

  template <class... Args>
  eastl::pair<EnumType, ValueType &> appendNew(Args &&...args)
  {
    uint32_t sw = 0;
    SizeType newIdx = size > 0 ? find<false, true>(0, sw) : 0;
    ensureSpaceFor(newIdx);
    new (eastl::addressof(elements[newIdx].value)) ValueType(eastl::forward<Args>(args)...);
    setUsed(newIdx, true);
    ++usedCount;
    return {static_cast<EnumType>(newIdx), elements[newIdx].value};
  }

  // Returns nullptr on failure.
  template <class... Args>
  ValueType *emplaceAt(EnumType at, Args &&...args)
  {
    const SizeType idx = eastl::to_underlying(at);

    ensureSpaceFor(idx);

    if (isUsed(idx))
      return nullptr;

    setUsed(idx, true);
    new (eastl::addressof(elements[idx].value)) ValueType(eastl::forward<Args>(args)...);
    ++usedCount;
    return &elements[idx].value;
  }

  struct EnumerateIterator;

  EnumerateIterator erase(EnumType key)
  {
    G_ASSERT_RETURN(isMapped(key), (EnumerateIterator{*this, size, 0}));

    const SizeType idx = eastl::to_underlying(key);
    elements[idx].value.~ValueType();

    setUsed(idx, false);
    --usedCount;

    // Data was mutated, so load the current subword for the search
    uint32_t sw = subwordData()[static_cast<uint32_t>(idx) / 32];
    auto result = bit_search<true, true>(subwordData(), static_cast<uint32_t>(size), static_cast<uint32_t>(idx), sw);
    return EnumerateIterator{*this, static_cast<SizeType>(result), sw};
  }

  struct ConstEnumerateIterator
  {
    using iterator_category = eastl::bidirectional_iterator_tag;
    using value_type = eastl::tuple<EnumType, const ValueType &>;
    using difference_type = ptrdiff_t;
    using pointer = const value_type *;
    using reference = const value_type &;

    ConstEnumerateIterator(const IdSparseIndexedMapping &m, SizeType i, uint32_t sw) : mapping{&m}, idx{i}, currentSubword{sw} {}

    ConstEnumerateIterator &operator++()
    {
      idx = static_cast<SizeType>(bit_search<true, true>(mapping->subwordData(), static_cast<uint32_t>(mapping->size),
        static_cast<uint32_t>(idx) + 1, currentSubword));
      return *this;
    }

    ConstEnumerateIterator operator++(int)
    {
      ConstEnumerateIterator temp = *this;
      ++(*this);
      return temp;
    }

    ConstEnumerateIterator &operator--()
    {
      idx = static_cast<SizeType>(bit_search<true, false>(mapping->subwordData(), static_cast<uint32_t>(mapping->size),
        static_cast<uint32_t>(idx) - 1, currentSubword));
      return *this;
    }

    ConstEnumerateIterator operator--(int)
    {
      ConstEnumerateIterator temp = *this;
      --(*this);
      return temp;
    }


    bool operator==(const ConstEnumerateIterator &other) const { return idx == other.idx; }

    value_type operator*() const { return {static_cast<EnumType>(idx), mapping->elements[idx].value}; }

  private:
    const IdSparseIndexedMapping *mapping;
    SizeType idx;
    uint32_t currentSubword;
  };

  struct ConstEnumerateView
  {
    ConstEnumerateView(const IdSparseIndexedMapping &m) : mapping{&m} {}

    ConstEnumerateIterator begin() const
    {
      uint32_t sw = 0;
      auto pos = mapping->begin(sw);
      return ConstEnumerateIterator{*mapping, pos, sw};
    }
    ConstEnumerateIterator end() const { return ConstEnumerateIterator{*mapping, mapping->end(), 0}; }

  private:
    const IdSparseIndexedMapping *mapping;
  };

  auto enumerate() const & { return ConstEnumerateView{*this}; }

  struct EnumerateIterator
  {
    using iterator_category = eastl::bidirectional_iterator_tag;
    using value_type = eastl::tuple<EnumType, ValueType &>;
    using difference_type = ptrdiff_t;
    using pointer = value_type *;
    using reference = value_type &;

    EnumerateIterator(IdSparseIndexedMapping &m, SizeType i, uint32_t sw) : mapping{&m}, idx{i}, currentSubword{sw} {}

    EnumerateIterator &operator++()
    {
      idx = static_cast<SizeType>(bit_search<true, true>(mapping->subwordData(), static_cast<uint32_t>(mapping->size),
        static_cast<uint32_t>(idx) + 1, currentSubword));
      return *this;
    }

    EnumerateIterator operator++(int)
    {
      EnumerateIterator temp = *this;
      ++(*this);
      return temp;
    }

    EnumerateIterator &operator--()
    {
      idx = static_cast<SizeType>(bit_search<true, false>(mapping->subwordData(), static_cast<uint32_t>(mapping->size),
        static_cast<uint32_t>(idx) - 1, currentSubword));
      return *this;
    }

    EnumerateIterator operator--(int)
    {
      EnumerateIterator temp = *this;
      --(*this);
      return temp;
    }

    bool operator==(const EnumerateIterator &other) const { return idx == other.idx; }

    value_type operator*() const { return {static_cast<EnumType>(idx), mapping->elements[idx].value}; }

  private:
    IdSparseIndexedMapping *mapping;
    SizeType idx;
    uint32_t currentSubword;
  };

  struct EnumerateView
  {
    EnumerateView(IdSparseIndexedMapping &m) : mapping{&m} {}

    EnumerateIterator begin()
    {
      uint32_t sw = 0;
      auto pos = mapping->begin(sw);
      return EnumerateIterator{*mapping, pos, sw};
    }
    EnumerateIterator end() { return EnumerateIterator{*mapping, mapping->end(), 0}; }

  private:
    IdSparseIndexedMapping *mapping;
  };

  auto enumerate() & { return EnumerateView{*this}; }

  template <class FlagsAllocator>
  void updateFrom(const IdSparseIndexedMapping &source, const IdIndexedFlags<EnumType, FlagsAllocator> &changes)
  {
    // Step 1: Iterate destination keys, remove anything not present in source
    auto enumView = enumerate();
    for (auto it = enumView.begin(); it != enumView.end();)
    {
      auto [key, _] = *it;
      if (!source.isMapped(key))
        it = erase(key);
      else
        ++it;
    }

    // Step 2: Iterate source keys, insert new items and copy changed items
    for (auto [key, value] : source.enumerate())
    {
      if (!isMapped(key))
        emplaceAt(key, value);
      else if (changes.test(key, false))
        operator[](key) = value;
    }
  }

  struct KeyIterator
  {
    using iterator_category = eastl::bidirectional_iterator_tag;
    using value_type = EnumType;
    using difference_type = ptrdiff_t;
    using pointer = const EnumType *;
    using reference = EnumType;

    KeyIterator(const IdSparseIndexedMapping &m, SizeType i, uint32_t sw) : mapping{&m}, idx{i}, currentSubword{sw} {}

    KeyIterator &operator++()
    {
      idx = static_cast<SizeType>(bit_search<true, true>(mapping->subwordData(), static_cast<uint32_t>(mapping->size),
        static_cast<uint32_t>(idx) + 1, currentSubword));
      return *this;
    }

    KeyIterator operator++(int)
    {
      KeyIterator temp = *this;
      ++(*this);
      return temp;
    }

    KeyIterator &operator--()
    {
      idx = static_cast<SizeType>(bit_search<true, false>(mapping->subwordData(), static_cast<uint32_t>(mapping->size),
        static_cast<uint32_t>(idx) - 1, currentSubword));
      return *this;
    }

    KeyIterator operator--(int)
    {
      KeyIterator temp = *this;
      --(*this);
      return temp;
    }

    bool operator==(const KeyIterator &other) const { return idx == other.idx; }

    EnumType operator*() const { return static_cast<EnumType>(idx); }

  private:
    const IdSparseIndexedMapping *mapping;
    SizeType idx;
    uint32_t currentSubword;
  };

  struct KeyView
  {
    KeyView(const IdSparseIndexedMapping &m) : mapping{&m} {}

    KeyIterator begin() const
    {
      uint32_t sw = 0;
      auto pos = mapping->begin(sw);
      return KeyIterator{*mapping, pos, sw};
    }
    KeyIterator end() const { return KeyIterator{*mapping, mapping->end(), 0}; }

  private:
    const IdSparseIndexedMapping *mapping;
  };

  auto keys() const { return KeyView{*this}; }

  struct ValueIterator
  {
    using iterator_category = eastl::bidirectional_iterator_tag;
    using value_type = const ValueType;
    using difference_type = ptrdiff_t;
    using pointer = const ValueType *;
    using reference = const ValueType &;

    ValueIterator(const IdSparseIndexedMapping &m, SizeType i, uint32_t sw) : mapping{&m}, idx{i}, currentSubword{sw} {}

    ValueIterator &operator++()
    {
      idx = static_cast<SizeType>(bit_search<true, true>(mapping->subwordData(), static_cast<uint32_t>(mapping->size),
        static_cast<uint32_t>(idx) + 1, currentSubword));
      return *this;
    }

    ValueIterator operator++(int)
    {
      ValueIterator temp = *this;
      ++(*this);
      return temp;
    }

    ValueIterator &operator--()
    {
      idx = static_cast<SizeType>(bit_search<true, false>(mapping->subwordData(), static_cast<uint32_t>(mapping->size),
        static_cast<uint32_t>(idx) - 1, currentSubword));
      return *this;
    }

    ValueIterator operator--(int)
    {
      ValueIterator temp = *this;
      --(*this);
      return temp;
    }

    bool operator==(const ValueIterator &other) const { return idx == other.idx; }

    const ValueType &operator*() const { return mapping->elements[idx].value; }
    const ValueType *operator->() const { return &mapping->elements[idx].value; }

  private:
    const IdSparseIndexedMapping *mapping;
    SizeType idx;
    uint32_t currentSubword;
  };

  struct ValueView
  {
    ValueView(const IdSparseIndexedMapping &m) : mapping{&m} {}

    ValueIterator begin() const
    {
      uint32_t sw = 0;
      auto pos = mapping->begin(sw);
      return ValueIterator{*mapping, pos, sw};
    }
    ValueIterator end() const { return ValueIterator{*mapping, mapping->end(), 0}; }

  private:
    const IdSparseIndexedMapping *mapping;
  };

  auto values() const & { return ValueView{*this}; }

  size_t totalKeys() const { return size; }

  size_t used() const { return usedCount; }

  EnumType frontKey() const
  {
    uint32_t sw = 0;
    return static_cast<EnumType>(begin(sw));
  }

  EnumType backKey() const
  {
    uint32_t sw = 0;
    return static_cast<EnumType>(prevUsed(size, sw));
  }

  ValueType &front() { return elements[eastl::to_underlying(frontKey())].value; }
  const ValueType &front() const { return elements[eastl::to_underlying(frontKey())].value; }

  ValueType &back() { return elements[eastl::to_underlying(backKey())].value; }
  const ValueType &back() const { return elements[eastl::to_underlying(backKey())].value; }

  void resize(SizeType new_size)
  {
    compact();
    while (size < new_size)
      appendNew();
    while (size > new_size)
    {
      if (isMapped(static_cast<EnumType>(size - 1)))
        erase(static_cast<EnumType>(size - 1));
      --size;
    }
  }


  bool empty() const { return usedCount == 0; }

  void clear()
  {
    // Don't actally deallocate the memory, same as std.
    destroy();
    memset(usedBits, 0, sizeInWords() * sizeof(WordType));
    size = 0;
    usedCount = 0;
  }

  template <class OtherValueType, typename OtherAllocator>
  bool usedKeysSameAs(const IdSparseIndexedMapping<EnumType, OtherValueType, OtherAllocator> &other) const
  {
    if (usedCount != other.usedCount)
      return false;

    // NOTE: unused words are always zero'd out.

    const auto ourWordCount = sizeInWords();
    const auto otherWordCount = other.sizeInWords();
    for (size_t i = 0; i < eastl::min(ourWordCount, otherWordCount); ++i)
    {
      if (!v_test_all_bits_ones(v_cmp_eqi(usedBits[i], other.usedBits[i])))
        return false;
    }

    if (ourWordCount < otherWordCount)
    {
      for (size_t i = ourWordCount; i < otherWordCount; ++i)
        if (!v_test_all_bits_zeros(other.usedBits[i]))
          return false;
    }
    else
    {
      for (size_t i = otherWordCount; i < ourWordCount; ++i)
        if (!v_test_all_bits_zeros(usedBits[i]))
          return false;
    }

    return true;
  }

  bool operator==(const IdSparseIndexedMapping &other) const
  {
    if (usedCount != other.usedCount)
      return false;

    const auto ours = enumerate();
    const auto theirs = other.enumerate();
    for (auto [ours, theirs] : zip(ours, theirs))
      if (eastl::get<0>(ours) != eastl::get<0>(theirs) || eastl::get<1>(ours) != eastl::get<1>(theirs))
        return false;

    return true;
  }

  // For debugging purposes
  eastl::string makeUsedKeysBitmaskString() const
  {
    eastl::string result;
    result.reserve(size);
    for (SizeType i = 0; i < size; ++i)
      result += isUsed(i) ? '1' : '0';
    return result;
  }

  ~IdSparseIndexedMapping() { destroyAndDeallocate(); }

private:
  union ElementStorage
  {
    ValueType value;

    ~ElementStorage() {}
  };

  size_t sizeInWords() const { return (size + WORD_BITS - 1) / WORD_BITS; }

  bool isUsed(SizeType idx) const
  {
    uint32_t subword;
    memcpy(&subword, &reinterpret_cast<uint32_t *>(usedBits)[idx / SUBWORD_BITS], sizeof(uint32_t));
    return (subword & (1u << (idx % SUBWORD_BITS))) != 0;
  }

  void setUsed(SizeType idx, bool used)
  {
    uint32_t subword;
    memcpy(&subword, &reinterpret_cast<uint32_t *>(usedBits)[idx / SUBWORD_BITS], sizeof(uint32_t));
    if (used)
      subword |= (1u << (idx % SUBWORD_BITS));
    else
      subword &= ~(1u << (idx % SUBWORD_BITS));
    memcpy(&reinterpret_cast<uint32_t *>(usedBits)[idx / SUBWORD_BITS], &subword, sizeof(uint32_t));
  }

  const uint32_t *subwordData() const { return reinterpret_cast<const uint32_t *>(usedBits); }
  uint32_t subwordCount() const
  {
    constexpr SizeType spw = WORD_BITS / (sizeof(uint32_t) * CHAR_BIT);
    return static_cast<uint32_t>(capacityInWordsAndAllocator.first() * spw);
  }

  // NOTE: start_idx is inclusive. If you want to exclude it, simply add or subtract 1.
  template <bool val, bool search_forward>
  SizeType find(SizeType start_idx, uint32_t &sw) const
  {
    auto result = bit_search<val, search_forward>(subwordData(), static_cast<uint32_t>(size), static_cast<uint32_t>(start_idx), sw);
    return static_cast<SizeType>(result);
  }

  SizeType nextUsed(SizeType idx, uint32_t &sw) const
  {
    G_ASSERT_RETURN(idx < size, size);
    return find<true, true>(idx + 1, sw);
  }
  SizeType prevUsed(SizeType idx, uint32_t &sw) const
  {
    G_ASSERT_RETURN(idx != 0, 0);
    return find<true, false>(idx - 1, sw);
  }

  void ensureSpaceFor(size_t idx)
  {
    if (idx < size)
      return;

    const size_t newSize = idx + 1;
    if (newSize >= capacityInWordsAndAllocator.first() * WORD_BITS)
      relocate(max<size_t>((newSize + WORD_BITS - 1) / WORD_BITS, capacityInWordsAndAllocator.first() * 2));

    size = newSize;
  }

  void alloc()
  {
    auto &alloc = capacityInWordsAndAllocator.second();
    auto cap = capacityInWordsAndAllocator.first();
    elements = (ElementStorage *)eastl::allocate_memory(alloc, cap * WORD_BITS * sizeof(ValueType), alignof(ValueType), 0);
    usedBits = (WordType *)eastl::allocate_memory(alloc, cap * sizeof(WordType), alignof(WordType), 0);
  }

  void destroy()
  {
    uint32_t sw = 0;
    for (SizeType i = begin(sw); i < size; i = nextUsed(i, sw))
      elements[i].value.~ValueType();
  }

  void destroyAndDeallocate()
  {
    destroy();
    capacityInWordsAndAllocator.second().deallocate((void *)elements, 0);
    capacityInWordsAndAllocator.second().deallocate((void *)usedBits, 0);
  }

  static bool tryResizeInplace(Allocator &a, void *p, size_t sz)
  {
    if constexpr (requires { a.resizeInplace(p, sz); })
      return a.resizeInplace(p, sz);
    else
      return false;
  }

  void relocate(size_t new_cap_in_words)
  {
    if (usedBits)
    {
      auto &a = capacityInWordsAndAllocator.second();
      auto oldCapInWords = capacityInWordsAndAllocator.first();
      if (tryResizeInplace(a, (void *)elements, new_cap_in_words * WORD_BITS * sizeof(ValueType)) &&
          tryResizeInplace(a, (void *)usedBits, new_cap_in_words * sizeof(WordType)))
      {
        memset(reinterpret_cast<uint8_t *>(usedBits) + oldCapInWords * sizeof(WordType), 0,
          (new_cap_in_words - oldCapInWords) * sizeof(WordType));
        capacityInWordsAndAllocator.first() = new_cap_in_words;
        return;
      }
    }

    auto oldElements = elements;
    auto oldUsed = usedBits;
    capacityInWordsAndAllocator.first() = new_cap_in_words;
    alloc();

    if (!oldUsed)
    {
      memset(usedBits, 0, new_cap_in_words * sizeof(WordType));
      return;
    }

    size_t bytesToCopy = sizeInWords() * sizeof(WordType);
    memcpy(usedBits, oldUsed, bytesToCopy);
    memset(reinterpret_cast<uint8_t *>(usedBits) + bytesToCopy, 0, new_cap_in_words * sizeof(WordType) - bytesToCopy);

    uint32_t sw = 0;
    for (SizeType i = begin(sw); i < size; i = nextUsed(i, sw))
    {
      new (eastl::addressof(elements[i].value)) ValueType(eastl::move(oldElements[i].value));
      oldElements[i].value.~ValueType();
    }

    capacityInWordsAndAllocator.second().deallocate((void *)oldElements, 0);
    capacityInWordsAndAllocator.second().deallocate((void *)oldUsed, 0);
  }

  void copyInitFrom(ElementStorage *elems, WordType *used)
  {
    if (!elems)
      return;

    alloc();
    const size_t bytesToCopy = sizeInWords() * sizeof(WordType);
    memcpy(usedBits, used, bytesToCopy);
    memset(reinterpret_cast<uint8_t *>(usedBits) + bytesToCopy, 0,
      capacityInWordsAndAllocator.first() * sizeof(WordType) - bytesToCopy);
    uint32_t sw = 0;
    for (SizeType i = begin(sw); i < size; i = nextUsed(i, sw))
      new (eastl::addressof(elements[i].value)) ValueType(elems[i].value);

    compact();
  }

  // "compact" the size on rarely called operations to speed up iteration
  void compact()
  {
    if (size > 0 && usedCount > 0)
    {
      uint32_t sw = 0;
      size = find<true, false>(size - 1, sw) + 1;
    }
    else if (usedCount == 0)
      size = 0;
  }

  SizeType begin(uint32_t &sw) const
  {
    if (!usedBits || size == 0)
      return size;
    return static_cast<SizeType>(bit_search<true, true>(subwordData(), static_cast<uint32_t>(size), 0, sw));
  }
  SizeType end() const { return size; }

  ElementStorage *elements = nullptr;
  WordType *usedBits = nullptr;
  eastl::compressed_pair<SizeType, Allocator> capacityInWordsAndAllocator;
  SizeType size = 0;
  SizeType usedCount = 0;
};
