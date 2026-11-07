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
#include <debug/dag_assert.h>
#include <id/idIndexedFlags.h>
#include <id/bitSearch.h>


// Basically an Id -> Value map that is supposed to have
// very few holes in it.
template <class EnumType, class ValueType, typename Allocator = EASTLAllocatorType, bool HIDEABLE = false>
class IdSparseIndexedMapping
{
  using SizeType = eastl::underlying_type_t<EnumType>;
  using WordType = vec4f;
  static constexpr size_t WORD_BITS = sizeof(WordType) * CHAR_BIT;
  static constexpr size_t SUBWORD_BITS = sizeof(float) * CHAR_BIT;

  template <class OtherEnumType, class OtherValueType, typename OtherAllocator, bool OTHER_HIDEABLE>
  friend class IdSparseIndexedMapping;

public:
  using index_type = EnumType;
  using value_type = ValueType;

  IdSparseIndexedMapping(const Allocator &allocator = Allocator()) : capacityInWordsAndAllocator(0, allocator) {}

  IdSparseIndexedMapping(const IdSparseIndexedMapping &other) :
    capacityInWordsAndAllocator(other.capacityInWordsAndAllocator),
    size(other.size),
    usedCount(other.usedCount),
    hiddenCount(other.hiddenCount)
  {
    copyInitFrom(other);
  }
  IdSparseIndexedMapping &operator=(const IdSparseIndexedMapping &other)
  {
    if (this == &other)
      return *this;

    // TODO: don't re-allocate if we have enough capacity
    destroyAndDeallocate();

    elements = nullptr;
    usedBits = nullptr;
    storedBits = nullptr;
    capacityInWordsAndAllocator = other.capacityInWordsAndAllocator;
    size = other.size;
    usedCount = other.usedCount;
    hiddenCount = other.hiddenCount;
    copyInitFrom(other);
    return *this;
  }

  IdSparseIndexedMapping(IdSparseIndexedMapping &&other) :
    elements(eastl::exchange(other.elements, nullptr)),
    usedBits(eastl::exchange(other.usedBits, nullptr)),
    storedBits(eastl::exchange(other.storedBits, nullptr)),
    capacityInWordsAndAllocator(eastl::exchange(other.capacityInWordsAndAllocator.first(), 0),
      // TODO: handle stateful allocators properly
      eastl::exchange(other.capacityInWordsAndAllocator.second(), Allocator{})),
    size(eastl::exchange(other.size, 0)),
    usedCount(eastl::exchange(other.usedCount, 0)),
    hiddenCount(eastl::exchange(other.hiddenCount, 0))
  {}
  IdSparseIndexedMapping &operator=(IdSparseIndexedMapping &&other)
  {
    if (this == &other)
      return *this;
    destroyAndDeallocate();
    elements = eastl::exchange(other.elements, nullptr);
    usedBits = eastl::exchange(other.usedBits, nullptr);
    storedBits = eastl::exchange(other.storedBits, nullptr);
    capacityInWordsAndAllocator.first() = eastl::exchange(other.capacityInWordsAndAllocator.first(), 0);
    // TODO: handle stateful allocators properly
    capacityInWordsAndAllocator.second() = eastl::exchange(other.capacityInWordsAndAllocator.second(), Allocator{});
    size = eastl::exchange(other.size, 0);
    usedCount = eastl::exchange(other.usedCount, 0);
    hiddenCount = eastl::exchange(other.hiddenCount, 0);
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
    requires(!HIDEABLE)
  {
    if (!isMapped(key))
      emplaceAt(key);
    return elements[eastl::to_underlying(key)].value;
  }

  template <class... Args>
  eastl::pair<EnumType, ValueType &> appendNew(Args &&...args)
  {
    uint32_t sw = 0;
    SizeType newIdx = size > 0 ? find<false, true>(storedSubwordData(), 0, sw) : 0;
    ensureSpaceFor(newIdx);
    new (eastl::addressof(elements[newIdx].value)) ValueType(eastl::forward<Args>(args)...);
    setStored(newIdx, true);
    ++usedCount;
    return {static_cast<EnumType>(newIdx), elements[newIdx].value};
  }

  // Returns nullptr on failure.
  template <class... Args>
  ValueType *emplaceAt(EnumType at, Args &&...args)
  {
    const SizeType idx = eastl::to_underlying(at);

    ensureSpaceFor(idx);

    if (isStored(idx))
      return nullptr;

    setStored(idx, true);
    new (eastl::addressof(elements[idx].value)) ValueType(eastl::forward<Args>(args)...);
    ++usedCount;
    return &elements[idx].value;
  }

  struct EnumerateIterator;

  EnumerateIterator erase(EnumType key)
  {
    const SizeType idx = eastl::to_underlying(key);
    G_ASSERT_RETURN(idx < size && isStored(idx), (EnumerateIterator{*this, subwordData(), size, 0}));

    elements[idx].value.~ValueType();

    if (isUsed(idx))
      --usedCount;
    else
      --hiddenCount;
    setStored(idx, false);

    // Data was mutated, so load the current subword for the search
    uint32_t sw = subwordData()[static_cast<uint32_t>(idx) / 32];
    auto result = bit_search<true, true>(subwordData(), static_cast<uint32_t>(size), static_cast<uint32_t>(idx), sw);
    return EnumerateIterator{*this, subwordData(), static_cast<SizeType>(result), sw};
  }

  void hide(EnumType key)
    requires HIDEABLE
  {
    G_ASSERT_RETURN(isMapped(key), );
    setUsed(eastl::to_underlying(key), false);
    --usedCount;
    ++hiddenCount;
  }

  struct ConstEnumerateIterator
  {
    using iterator_category = eastl::bidirectional_iterator_tag;
    using value_type = eastl::tuple<EnumType, const ValueType &>;
    using difference_type = ptrdiff_t;
    using pointer = const value_type *;
    using reference = const value_type &;

    ConstEnumerateIterator(const IdSparseIndexedMapping &m, const uint32_t *b, SizeType i, uint32_t sw) :
      mapping{&m}, bits{b}, idx{i}, currentSubword{sw}
    {}

    ConstEnumerateIterator &operator++()
    {
      idx = static_cast<SizeType>(
        bit_search<true, true>(bits, static_cast<uint32_t>(mapping->size), static_cast<uint32_t>(idx) + 1, currentSubword));
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
      idx = static_cast<SizeType>(
        bit_search<true, false>(bits, static_cast<uint32_t>(mapping->size), static_cast<uint32_t>(idx) - 1, currentSubword));
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
    const uint32_t *bits;
    SizeType idx;
    uint32_t currentSubword;
  };

  struct ConstEnumerateView
  {
    ConstEnumerateView(const IdSparseIndexedMapping &m, const uint32_t *b) : mapping{&m}, bits{b} {}

    ConstEnumerateIterator begin() const
    {
      uint32_t sw = 0;
      auto pos = mapping->begin(bits, sw);
      return ConstEnumerateIterator{*mapping, bits, pos, sw};
    }
    ConstEnumerateIterator end() const { return ConstEnumerateIterator{*mapping, bits, mapping->end(), 0}; }

  private:
    const IdSparseIndexedMapping *mapping;
    const uint32_t *bits;
  };

  auto enumerate() const & { return ConstEnumerateView{*this, subwordData()}; }

  struct EnumerateIterator
  {
    using iterator_category = eastl::bidirectional_iterator_tag;
    using value_type = eastl::tuple<EnumType, ValueType &>;
    using difference_type = ptrdiff_t;
    using pointer = value_type *;
    using reference = value_type &;

    EnumerateIterator(IdSparseIndexedMapping &m, const uint32_t *b, SizeType i, uint32_t sw) :
      mapping{&m}, bits{b}, idx{i}, currentSubword{sw}
    {}

    EnumerateIterator &operator++()
    {
      idx = static_cast<SizeType>(
        bit_search<true, true>(bits, static_cast<uint32_t>(mapping->size), static_cast<uint32_t>(idx) + 1, currentSubword));
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
      idx = static_cast<SizeType>(
        bit_search<true, false>(bits, static_cast<uint32_t>(mapping->size), static_cast<uint32_t>(idx) - 1, currentSubword));
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
    const uint32_t *bits;
    SizeType idx;
    uint32_t currentSubword;
  };

  struct EnumerateView
  {
    EnumerateView(IdSparseIndexedMapping &m, const uint32_t *b) : mapping{&m}, bits{b} {}

    EnumerateIterator begin()
    {
      uint32_t sw = 0;
      auto pos = mapping->begin(bits, sw);
      return EnumerateIterator{*mapping, bits, pos, sw};
    }
    EnumerateIterator end() { return EnumerateIterator{*mapping, bits, mapping->end(), 0}; }

  private:
    IdSparseIndexedMapping *mapping;
    const uint32_t *bits;
  };

  auto enumerate() & { return EnumerateView{*this, subwordData()}; }

  template <class FlagsAllocator>
  void updateFrom(const IdSparseIndexedMapping &source, const IdIndexedFlags<EnumType, FlagsAllocator> &changes)
    requires(!HIDEABLE)
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

    KeyIterator(const IdSparseIndexedMapping &m, const uint32_t *b, SizeType i, uint32_t sw) :
      mapping{&m}, bits{b}, idx{i}, currentSubword{sw}
    {}

    KeyIterator &operator++()
    {
      idx = static_cast<SizeType>(
        bit_search<true, true>(bits, static_cast<uint32_t>(mapping->size), static_cast<uint32_t>(idx) + 1, currentSubword));
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
      idx = static_cast<SizeType>(
        bit_search<true, false>(bits, static_cast<uint32_t>(mapping->size), static_cast<uint32_t>(idx) - 1, currentSubword));
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
    const uint32_t *bits;
    SizeType idx;
    uint32_t currentSubword;
  };

  struct KeyView
  {
    KeyView(const IdSparseIndexedMapping &m, const uint32_t *b) : mapping{&m}, bits{b} {}

    KeyIterator begin() const
    {
      uint32_t sw = 0;
      auto pos = mapping->begin(bits, sw);
      return KeyIterator{*mapping, bits, pos, sw};
    }
    KeyIterator end() const { return KeyIterator{*mapping, bits, mapping->end(), 0}; }

  private:
    const IdSparseIndexedMapping *mapping;
    const uint32_t *bits;
  };

  auto keys() const { return KeyView{*this, subwordData()}; }

  struct ValueIterator
  {
    using iterator_category = eastl::bidirectional_iterator_tag;
    using value_type = const ValueType;
    using difference_type = ptrdiff_t;
    using pointer = const ValueType *;
    using reference = const ValueType &;

    ValueIterator(const IdSparseIndexedMapping &m, const uint32_t *b, SizeType i, uint32_t sw) :
      mapping{&m}, bits{b}, idx{i}, currentSubword{sw}
    {}

    ValueIterator &operator++()
    {
      idx = static_cast<SizeType>(
        bit_search<true, true>(bits, static_cast<uint32_t>(mapping->size), static_cast<uint32_t>(idx) + 1, currentSubword));
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
      idx = static_cast<SizeType>(
        bit_search<true, false>(bits, static_cast<uint32_t>(mapping->size), static_cast<uint32_t>(idx) - 1, currentSubword));
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
    const uint32_t *bits;
    SizeType idx;
    uint32_t currentSubword;
  };

  struct ValueView
  {
    ValueView(const IdSparseIndexedMapping &m, const uint32_t *b) : mapping{&m}, bits{b} {}

    ValueIterator begin() const
    {
      uint32_t sw = 0;
      auto pos = mapping->begin(bits, sw);
      return ValueIterator{*mapping, bits, pos, sw};
    }
    ValueIterator end() const { return ValueIterator{*mapping, bits, mapping->end(), 0}; }

  private:
    const IdSparseIndexedMapping *mapping;
    const uint32_t *bits;
  };

  auto values() const & { return ValueView{*this, subwordData()}; }

  template <class Mapping>
  class AllView
  {
    using Ref = eastl::conditional_t<eastl::is_const_v<Mapping>, const ValueType &, ValueType &>;

  public:
    explicit AllView(Mapping &m) : mapping{&m} {}

    bool isMapped(EnumType key) const
    {
      const SizeType idx = eastl::to_underlying(key);
      return idx < mapping->size && mapping->isStored(idx);
    }

    Ref operator[](EnumType key) const
    {
      G_ASSERT(isMapped(key));
      return mapping->elements[eastl::to_underlying(key)].value;
    }

    auto keys() const { return KeyView{*mapping, mapping->storedSubwordData()}; }
    auto values() const { return ValueView{*mapping, mapping->storedSubwordData()}; }
    auto enumerate() const { return ConstEnumerateView{*mapping, mapping->storedSubwordData()}; }

    EnumType frontKey() const
    {
      uint32_t sw = 0;
      return static_cast<EnumType>(mapping->begin(mapping->storedSubwordData(), sw));
    }

    EnumType backKey() const
    {
      uint32_t sw = 0;
      return static_cast<EnumType>(mapping->prevUsed(mapping->storedSubwordData(), mapping->size, sw));
    }

    EnumType getNextUsed(EnumType key) const
    {
      G_ASSERT(isMapped(key));
      const uint32_t *bits = mapping->storedSubwordData();
      uint32_t sw = bits[eastl::to_underlying(key) / detail::WORD_BITS];
      return static_cast<EnumType>(mapping->nextUsed(bits, eastl::to_underlying(key), sw));
    }

    size_t totalKeys() const { return mapping->size; }
    size_t used() const { return mapping->storedCount(); }
    bool empty() const { return mapping->storedCount() == 0; }

  private:
    Mapping *mapping;
  };

  auto all() const { return AllView<const IdSparseIndexedMapping>{*this}; }
  auto all() { return AllView<IdSparseIndexedMapping>{*this}; }

  size_t totalKeys() const { return size; }

  size_t used() const { return usedCount; }

  EnumType frontKey() const
  {
    uint32_t sw = 0;
    return static_cast<EnumType>(begin(subwordData(), sw));
  }

  EnumType backKey() const
  {
    uint32_t sw = 0;
    return static_cast<EnumType>(prevUsed(subwordData(), size, sw));
  }

  ValueType &front() { return elements[eastl::to_underlying(frontKey())].value; }
  const ValueType &front() const { return elements[eastl::to_underlying(frontKey())].value; }

  ValueType &back() { return elements[eastl::to_underlying(backKey())].value; }
  const ValueType &back() const { return elements[eastl::to_underlying(backKey())].value; }

  EnumType getNextUsed(EnumType key) const
  {
    G_ASSERT(isStored(eastl::to_underlying(key)));
    uint32_t sw = subwordData()[eastl::to_underlying(key) / detail::WORD_BITS];
    return static_cast<EnumType>(nextUsed(subwordData(), eastl::to_underlying(key), sw));
  }

  void resize(SizeType new_size)
    requires(!HIDEABLE)
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
    if constexpr (HIDEABLE)
      memset(storedBits, 0, sizeInWords() * sizeof(WordType));
    size = 0;
    usedCount = 0;
    hiddenCount = 0;
  }

  template <class OtherValueType, typename OtherAllocator, bool OTHER_HIDEABLE>
  bool usedKeysSameAs(const IdSparseIndexedMapping<EnumType, OtherValueType, OtherAllocator, OTHER_HIDEABLE> &other) const
  {
    if (usedCount != other.usedCount)
      return false;

    if (!sameWords(usedBits, sizeInWords(), other.usedBits, other.sizeInWords()))
      return false;

    if constexpr (HIDEABLE && OTHER_HIDEABLE)
      return hiddenCount == other.hiddenCount && sameWords(storedBits, sizeInWords(), other.storedBits, other.sizeInWords());

    return true;
  }

  bool operator==(const IdSparseIndexedMapping &other) const
  {
    if (usedCount != other.usedCount || hiddenCount != other.hiddenCount)
      return false;

    if constexpr (HIDEABLE)
      return sameWords(usedBits, sizeInWords(), other.usedBits, other.sizeInWords()) &&
             sameElements(all().enumerate(), other.all().enumerate());

    return sameElements(enumerate(), other.enumerate());
  }

  // For debugging purposes
  eastl::string makeUsedKeysBitmaskString() const
  {
    eastl::string result;
    result.reserve(size);
    for (SizeType i = 0; i < size; ++i)
      result += isUsed(i) ? '1' : (isStored(i) ? 'h' : '0');
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

  static bool testBit(const WordType *bits, SizeType idx)
  {
    uint32_t subword;
    memcpy(&subword, &reinterpret_cast<const uint32_t *>(bits)[idx / SUBWORD_BITS], sizeof(uint32_t));
    return (subword & (1u << (idx % SUBWORD_BITS))) != 0;
  }

  static void setBit(WordType *bits, SizeType idx, bool value)
  {
    uint32_t subword;
    memcpy(&subword, &reinterpret_cast<uint32_t *>(bits)[idx / SUBWORD_BITS], sizeof(uint32_t));
    if (value)
      subword |= (1u << (idx % SUBWORD_BITS));
    else
      subword &= ~(1u << (idx % SUBWORD_BITS));
    memcpy(&reinterpret_cast<uint32_t *>(bits)[idx / SUBWORD_BITS], &subword, sizeof(uint32_t));
  }

  // NOTE: unused words are always zero'd out.
  static bool sameWords(const WordType *ours, size_t our_count, const WordType *theirs, size_t their_count)
  {
    for (size_t i = 0; i < eastl::min(our_count, their_count); ++i)
      if (!v_test_all_bits_ones(v_cmp_eqi(ours[i], theirs[i])))
        return false;

    for (size_t i = their_count; i < our_count; ++i)
      if (!v_test_all_bits_zeros(ours[i]))
        return false;
    for (size_t i = our_count; i < their_count; ++i)
      if (!v_test_all_bits_zeros(theirs[i]))
        return false;

    return true;
  }

  template <class View>
  static bool sameElements(const View &ours, const View &theirs)
  {
    auto a = ours.begin();
    auto b = theirs.begin();
    for (; a != ours.end() && b != theirs.end(); ++a, ++b)
    {
      const auto [aKey, aValue] = *a;
      const auto [bKey, bValue] = *b;
      if (aKey != bKey || aValue != bValue)
        return false;
    }
    return a == ours.end() && b == theirs.end();
  }

  bool isUsed(SizeType idx) const { return testBit(usedBits, idx); }
  void setUsed(SizeType idx, bool used) { setBit(usedBits, idx, used); }

  bool isStored(SizeType idx) const
  {
    if constexpr (HIDEABLE)
      return testBit(storedBits, idx);
    else
      return isUsed(idx);
  }

  void setStored(SizeType idx, bool stored)
  {
    setUsed(idx, stored);
    if constexpr (HIDEABLE)
      setBit(storedBits, idx, stored);
  }

  SizeType storedCount() const { return usedCount + hiddenCount; }

  const uint32_t *subwordData() const { return reinterpret_cast<const uint32_t *>(usedBits); }
  const uint32_t *storedSubwordData() const
  {
    if constexpr (HIDEABLE)
      return reinterpret_cast<const uint32_t *>(storedBits);
    else
      return subwordData();
  }
  uint32_t subwordCount() const
  {
    constexpr SizeType spw = WORD_BITS / (sizeof(uint32_t) * CHAR_BIT);
    return static_cast<uint32_t>(capacityInWordsAndAllocator.first() * spw);
  }

  // NOTE: start_idx is inclusive. If you want to exclude it, simply add or subtract 1.
  template <bool val, bool search_forward>
  SizeType find(const uint32_t *bits, SizeType start_idx, uint32_t &sw) const
  {
    auto result = bit_search<val, search_forward>(bits, static_cast<uint32_t>(size), static_cast<uint32_t>(start_idx), sw);
    return static_cast<SizeType>(result);
  }

  SizeType nextUsed(const uint32_t *bits, SizeType idx, uint32_t &sw) const
  {
    G_ASSERT_RETURN(idx < size, size);
    return find<true, true>(bits, idx + 1, sw);
  }
  SizeType prevUsed(const uint32_t *bits, SizeType idx, uint32_t &sw) const
  {
    G_ASSERT_RETURN(idx != 0, 0);
    return find<true, false>(bits, idx - 1, sw);
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
    if constexpr (HIDEABLE)
      storedBits = (WordType *)eastl::allocate_memory(alloc, cap * sizeof(WordType), alignof(WordType), 0);
  }

  void deallocate(ElementStorage *elems, WordType *used, WordType *stored)
  {
    capacityInWordsAndAllocator.second().deallocate((void *)elems, 0);
    capacityInWordsAndAllocator.second().deallocate((void *)used, 0);
    if constexpr (HIDEABLE)
      capacityInWordsAndAllocator.second().deallocate((void *)stored, 0);
  }

  void destroy()
  {
    uint32_t sw = 0;
    const uint32_t *bits = storedSubwordData();
    for (SizeType i = begin(bits, sw); i < size; i = nextUsed(bits, i, sw))
      elements[i].value.~ValueType();
  }

  void destroyAndDeallocate()
  {
    destroy();
    deallocate(elements, usedBits, storedBits);
  }

  static bool tryResizeInplace(Allocator &a, void *p, size_t sz)
  {
    if constexpr (requires { a.resizeInplace(p, sz); })
      return a.resizeInplace(p, sz);
    else
      return false;
  }

  static void copyWords(WordType *dst, const WordType *src, size_t src_bytes, size_t dst_bytes)
  {
    memcpy(dst, src, src_bytes);
    memset(reinterpret_cast<uint8_t *>(dst) + src_bytes, 0, dst_bytes - src_bytes);
  }

  void relocate(size_t new_cap_in_words)
  {
    if (usedBits)
    {
      auto &a = capacityInWordsAndAllocator.second();
      auto oldCapInWords = capacityInWordsAndAllocator.first();
      if (tryResizeInplace(a, (void *)elements, new_cap_in_words * WORD_BITS * sizeof(ValueType)) &&
          tryResizeInplace(a, (void *)usedBits, new_cap_in_words * sizeof(WordType)) &&
          (!HIDEABLE || tryResizeInplace(a, (void *)storedBits, new_cap_in_words * sizeof(WordType))))
      {
        memset(reinterpret_cast<uint8_t *>(usedBits) + oldCapInWords * sizeof(WordType), 0,
          (new_cap_in_words - oldCapInWords) * sizeof(WordType));
        if constexpr (HIDEABLE)
          memset(reinterpret_cast<uint8_t *>(storedBits) + oldCapInWords * sizeof(WordType), 0,
            (new_cap_in_words - oldCapInWords) * sizeof(WordType));
        capacityInWordsAndAllocator.first() = new_cap_in_words;
        return;
      }
    }

    auto oldElements = elements;
    auto oldUsed = usedBits;
    auto oldStored = storedBits;
    capacityInWordsAndAllocator.first() = new_cap_in_words;
    alloc();

    if (!oldUsed)
    {
      memset(usedBits, 0, new_cap_in_words * sizeof(WordType));
      if constexpr (HIDEABLE)
        memset(storedBits, 0, new_cap_in_words * sizeof(WordType));
      return;
    }

    const size_t bytesToCopy = sizeInWords() * sizeof(WordType);
    copyWords(usedBits, oldUsed, bytesToCopy, new_cap_in_words * sizeof(WordType));
    if constexpr (HIDEABLE)
      copyWords(storedBits, oldStored, bytesToCopy, new_cap_in_words * sizeof(WordType));

    uint32_t sw = 0;
    const uint32_t *bits = storedSubwordData();
    for (SizeType i = begin(bits, sw); i < size; i = nextUsed(bits, i, sw))
    {
      new (eastl::addressof(elements[i].value)) ValueType(eastl::move(oldElements[i].value));
      oldElements[i].value.~ValueType();
    }

    deallocate(oldElements, oldUsed, oldStored);
  }

  void copyInitFrom(const IdSparseIndexedMapping &other)
  {
    if (!other.elements)
      return;

    alloc();
    const size_t bytesToCopy = sizeInWords() * sizeof(WordType);
    const size_t capacityBytes = capacityInWordsAndAllocator.first() * sizeof(WordType);
    copyWords(usedBits, other.usedBits, bytesToCopy, capacityBytes);
    if constexpr (HIDEABLE)
      copyWords(storedBits, other.storedBits, bytesToCopy, capacityBytes);
    uint32_t sw = 0;
    const uint32_t *bits = storedSubwordData();
    for (SizeType i = begin(bits, sw); i < size; i = nextUsed(bits, i, sw))
      new (eastl::addressof(elements[i].value)) ValueType(other.elements[i].value);

    compact();
  }

  // "compact" the size on rarely called operations to speed up iteration
  void compact()
  {
    if (size > 0 && storedCount() > 0)
    {
      uint32_t sw = 0;
      size = find<true, false>(storedSubwordData(), size - 1, sw) + 1;
    }
    else if (storedCount() == 0)
      size = 0;
  }

  SizeType begin(const uint32_t *bits, uint32_t &sw) const
  {
    if (!usedBits || size == 0)
      return size;
    return static_cast<SizeType>(bit_search<true, true>(bits, static_cast<uint32_t>(size), 0, sw));
  }
  SizeType end() const { return size; }

  ElementStorage *elements = nullptr;
  WordType *usedBits = nullptr;
  WordType *storedBits = nullptr;
  eastl::compressed_pair<SizeType, Allocator> capacityInWordsAndAllocator;
  SizeType size = 0;
  SizeType usedCount = 0;
  SizeType hiddenCount = 0;
};

template <class EnumType, class ValueType, typename Allocator = EASTLAllocatorType>
using IdHideableSparseIndexedMapping = IdSparseIndexedMapping<EnumType, ValueType, Allocator, true>;
