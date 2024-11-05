//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "dag_variantArray.h"
#include <EASTL/unique_ptr.h>


namespace dag
{

/// @brief A type equivalent to vector<variant<Ts...>, N> but with a more efficient storage layout
template <typename... Ts>
class VariantVector
{
  using TypeStorage = detail::VariantElementTypeStorage<Ts...>;
  using ElementStorage = detail::VariantElementStorage<Ts...>;
  using FirstType = typename detail::FirstTypeOf<Ts...>::Type;

  eastl::unique_ptr<uint8_t[]> storage;
  size_t count = 0;
  size_t maxCount = 0;

  // cache lines are usually 64 byte so align storage for type id to this
  static constexpr size_t block_align_bytes = 64;

  static size_t calculate_count(size_t wanted_count)
  {
    constexpr size_t values_per_line = block_align_bytes / sizeof(TypeStorage);
    return (wanted_count + values_per_line - 1) & ~(values_per_line - 1);
  }

  static TypeStorage *get_type_store(uint8_t *bytes, size_t max_count)
  {
    G_UNUSED(max_count);
    return reinterpret_cast<TypeStorage *>(bytes);
  }
  static const TypeStorage *get_type_store(const uint8_t *bytes, size_t max_count)
  {
    G_UNUSED(max_count);
    return reinterpret_cast<const TypeStorage *>(bytes);
  }
  static ElementStorage *get_value_store(uint8_t *bytes, size_t max_count)
  {
    return reinterpret_cast<ElementStorage *>(get_type_store(bytes, max_count) + max_count);
  }
  static const ElementStorage *get_value_store(const uint8_t *bytes, size_t max_count)
  {
    return reinterpret_cast<ElementStorage *>(get_type_store(bytes, max_count) + max_count);
  }

  static size_t calculate_total_size(size_t count) { return (sizeof(TypeStorage) + sizeof(ElementStorage)) * count; }

  TypeStorage *getTypes() { return get_type_store(storage.get(), maxCount); }
  const TypeStorage *getTypes() const { return get_type_store(storage.get(), maxCount); }
  ElementStorage *getValues() { return get_value_store(storage.get(), maxCount); }
  const ElementStorage *getValues() const { return get_value_store(storage.get(), maxCount); }

  // invokes c with type and value for each entry
  template <typename C>
  void forEach(C &&c)
  {
    auto t = getTypes();
    auto v = getValues();
    for (size_t i = 0; i < size(); ++i)
    {
      c(t[i], v[i]);
    }
  }

  void reserveInternal(size_t new_size)
  {
    G_ASSERT(new_size >= count);
    // recalculate size so values segment is aligned
    auto newCount = calculate_count(new_size);
    auto newStore = eastl::make_unique<uint8_t[]>(calculate_total_size(newCount));

    auto sourceTypes = getTypes();
    auto sourceValues = getValues();
    auto destinationTypes = get_type_store(newStore.get(), newCount);
    auto destinationValues = get_value_store(newStore.get(), newCount);

    for (size_t i = 0; i < count; ++i)
    {
      destinationTypes[i] = sourceTypes[i].moveTo(sourceValues[i].addressOf(), destinationValues[i].addressOf());
    }
    eastl::swap(storage, newStore);
    maxCount = newCount;
  }

  void pushGrow()
  {
    if (maxCount > count)
    {
      return;
    }
    reserveInternal(maxCount + (maxCount + 2) / 2);
  }

  template <typename U>
  size_t findFirstOf(size_t offset) const
  {
    auto typeToLookFor = TypeStorage::template typeOf<U>();
    auto typesBegin = getTypes() + offset;
    auto typesEnd = getTypes() + count;
    auto typesAt = eastl::find(typesBegin, typesEnd, typeToLookFor);
    auto result = eastl::distance(getTypes(), typesAt);
    return result;
  }

  // usage is for ++ of iterators, offset is the last index, so we want to start at last_time_t + 1
  template <typename U>
  size_t findNextOf(size_t last_time_t) const
  {
    if (last_time_t + 1 >= size())
    {
      return size();
    }
    auto typeToLookFor = TypeStorage::template typeOf<U>();
    auto typesBegin = getTypes() + last_time_t + 1;
    auto typesEnd = getTypes() + count;
    auto typesAt = eastl::find(typesBegin, typesEnd, typeToLookFor);
    auto result = eastl::distance(getTypes(), typesAt);
    return result;
  }

public:
  VariantVector() = default;

  VariantVector(const VariantVector &) = delete;
  VariantVector &operator=(const VariantVector &) = delete;

  VariantVector(VariantVector &&) = default;
  VariantVector &operator=(VariantVector &&) = default;

  ~VariantVector()
  {
    forEach([](TypeStorage &type, ElementStorage &value) { type.destructAt(value.addressOf()); });
  }

  bool empty() const { return 0 == size(); }
  size_t size() const { return count; }
  size_t capacity() const { return maxCount; }
  size_t typeIndex(size_t index) const { return getTypes()[index].getIndex(); }

  template <typename U, typename... Us>
  void assign(size_t index, Us &&...us)
  {
    G_ASSERT(index < size());
    auto &type = getTypes()[index];
    auto &value = getValues()[index];
    type.destructAt(value.addressOf());
    type.template constructAt<U>(value.addressOf(), eastl::forward<Us>(us)...);
  }

  template <typename U>
  void assign(size_t index, U &&u)
  {
    auto &type = getTypes()[index];
    auto &value = getValues()[index];
    type.destructAt(value.addressOf());
    type.template constructAt<U>(value.addressOf(), eastl::move(u));
  }

  template <typename V>
  auto visit(size_t index, V &&v) const
  {
    G_ASSERT(index < size());
    auto &type = getTypes()[index];
    auto &value = getValues()[index];
    type.visit(value.addressOf(), eastl::move(v));
  }

  template <typename U>
  bool holdsAlternative(size_t index) const
  {
    G_ASSERT(index < size());
    return getTypes()[index].template is<U>();
  }

  template <typename U>
  const U *getIf(size_t index) const
  {
    G_ASSERT(index < size());
    return getTypes()[index].template getIf<U>(getTypes()[index].addressOf());
  }

  template <typename U>
  U *getIf(size_t index)
  {
    G_ASSERT(index < size());
    return getTypes()[index].template getIf<U>(getValues()[index].addressOf());
  }

  class ConstElementProxy;
  class ElementProxy;

  ConstElementProxy operator[](size_t index) const
  {
    G_ASSERT(index < size());
    return ConstElementProxy{*this, index};
  }
  ElementProxy operator[](size_t index)
  {
    G_ASSERT(index < size());
    return ElementProxy{*this, index};
  }

  class iterator;
  class const_iterator;

  iterator begin() { return iterator{*this, 0}; }
  iterator end() { return iterator{*this, size()}; }

  const_iterator begin() const { return const_iterator{*this, 0}; }
  const_iterator end() const { return const_iterator{*this, size()}; }

  const_iterator cbegin() const { return begin(); }
  const_iterator cend() const { return end(); }

  template <typename U>
  void push_back(const U &u)
  {
    pushGrow();
    auto &type = getTypes()[count];
    auto &value = getValues()[count];
    type.template constructAt<U>(value.addressOf(), u);
    ++count;
  }
  template <typename U, typename... Us>
  void emplace_back(Us &&...us)
  {
    pushGrow();
    auto &type = getTypes()[count];
    auto &value = getValues()[count];
    type.template constructAt<U>(value.addressOf(), eastl::forward<Us>(us)...);
    ++count;
  }
  void pop_back()
  {
    G_ASSERT(size() > 0);
    auto &type = getTypes()[--count];
    auto &value = getValues()[count];
    type.destructAt(value.addressOf());
  }
  ElementProxy back()
  {
    G_ASSERT(size() > 0);
    return {*this, size() - 1};
  }
  ElementProxy front()
  {
    G_ASSERT(size() > 0);
    return {*this, 0};
  }
  void reserve(size_t new_size)
  {
    if (new_size <= maxCount)
    {
      return;
    }
    reserveInternal(new_size);
  }
  template <typename U>
  void resize(size_t new_size, const U &u)
  {
    reserve(new_size);
    while (count < new_size)
    {
      push_back(u);
    }
  }
  void shrink_to_fit()
  {
    if (maxCount == count)
    {
      return;
    }
    reserveInternal(count);
  }

  template <typename U>
  class typed_iterator;
  template <typename U>
  class typed_const_iterator;

  template <typename U>
  friend class typed_iterator;
  template <typename U>
  friend class typed_const_iterator;

  template <typename U>
  typed_iterator<U> beginOf()
  {
    return {*this, findFirstOf<U>(0)};
  }
  template <typename U>
  typed_iterator<U> endOf()
  {
    return {*this, size()};
  }

  template <typename U>
  typed_const_iterator<U> beginOf() const
  {
    return {*this, findFirstOf<U>(0)};
  }
  template <typename U>
  typed_const_iterator<U> endOf() const
  {
    return {*this, size()};
  }

  template <typename U>
  typed_const_iterator<U> cbeginOf() const
  {
    return {*this, findFirstOf<U>(0)};
  }
  template <typename U>
  typed_const_iterator<U> cendOf() const
  {
    return {*this, size()};
  }

  template <typename U>
  typed_iterator<U> beginOfStartingAt(size_t offset)
  {
    return {*this, findFirstOf<U>(offset)};
  }
  template <typename U>
  typed_const_iterator<U> beginOfStartingAt(size_t offset) const
  {
    return {*this, findFirstOf<U>(offset)};
  }
};


template <typename... Ts>
class VariantVector<Ts...>::ConstElementProxy
{
  const VariantVector &parent;
  size_t index;

  friend class VariantVector;
  ConstElementProxy(const VariantVector &p, size_t i) : parent{p}, index{i} {}

public:
  ConstElementProxy(const ConstElementProxy &) = default;
  ConstElementProxy &operator=(const ConstElementProxy &) = default;

  size_t typeIndex() const { return parent.typeIndex(index); }

  template <typename V>
  auto visit(V &&v) const
  {
    return parent.visit(index, eastl::forward<V>(v));
  }

  template <typename U>
  bool holdsAlternative() const
  {
    return parent.holdsAlternative<U>(index);
  }

  template <typename U>
  const U *getIf() const
  {
    return parent.getIf<U>(index);
  }

  template <typename U>
  friend bool operator==(const ConstElementProxy &lhs, const U &rhs)
  {
    auto ptr = lhs.template getIf<U>();
    if (!ptr)
      return false;
    return *ptr == rhs;
  }

  template <typename U>
  friend bool operator==(const U &lhs, const ConstElementProxy &rhs)
  {
    return rhs == lhs;
  }
  template <typename U>
  friend bool operator!=(const ConstElementProxy &lhs, const U &rhs)
  {
    return !(lhs == rhs);
  }
  template <typename U>
  friend bool operator!=(const U &lhs, const ConstElementProxy &rhs)
  {
    return !(rhs == lhs);
  }
};

template <typename... Ts>
class VariantVector<Ts...>::ElementProxy
{
  VariantVector &parent;
  size_t index;

  friend class VariantVector;
  ElementProxy(VariantVector &p, size_t i) : parent{p}, index{i} {}

public:
  ElementProxy(const ElementProxy &) = default;
  ElementProxy &operator=(const ElementProxy &) = default;

  size_t typeIndex() const { return parent.typeIndex(index); }
  template <typename U, typename... Us>
  void emplace(Us &&...us) const
  {
    parent.assign<U>(index, eastl::forward<Us>(us)...);
  }

  template <typename U>
  void assign(U &&u)
  {
    parent.assign(index, eastl::move(u));
  }

  template <typename V>
  auto visit(V &&v) const
  {
    return parent.visit(index, eastl::forward<V>(v));
  }

  template <typename U>
  bool holdsAlternative() const
  {
    return parent.holdsAlternative<U>(index);
  }

  template <typename U>
  U *getIf() const
  {
    return parent.getIf<U>(index);
  }

  template <typename U>
  ElementProxy &operator=(U &&u)
  {
    assign(eastl::move(u));
    return *this;
  }

  // TODO: copypasta can probably be avoided here
  template <typename U>
  friend bool operator==(const ElementProxy &lhs, const U &rhs)
  {
    auto ptr = lhs.template getIf<U>();
    if (!ptr)
      return false;
    return *ptr == rhs;
  }

  template <typename U>
  friend bool operator==(const U &lhs, const ElementProxy &rhs)
  {
    return rhs == lhs;
  }
  template <typename U>
  friend bool operator!=(const ElementProxy &lhs, const U &rhs)
  {
    return !(lhs == rhs);
  }
  template <typename U>
  friend bool operator!=(const U &lhs, const ElementProxy &rhs)
  {
    return !(rhs == lhs);
  }
};

template <typename... Ts>
class VariantVector<Ts...>::iterator // satisfies LegacyRandomAccessIterator
{
  VariantVector *parent;
  size_t index;

  friend class VariantVector;
  iterator(VariantVector &p, size_t i) : parent{&p}, index{i} {}

public:
  iterator(const iterator &) = default;
  iterator &operator=(const iterator &) = default;

  using value_type = ElementProxy;
  using difference_type = ptrdiff_t;
  using reference = ElementProxy;
  using pointer = void;
  using iterator_category = eastl::random_access_iterator_tag;

  ElementProxy operator*() { return ElementProxy{*parent, index}; }
  ElementProxy operator[](size_t i) const { return ElementProxy{*parent, index + i}; }
  iterator &operator++()
  {
    ++index;
    return *this;
  }
  iterator operator++(int)
  {
    auto copy = *this;
    ++index;
    return copy;
  }
  iterator &operator--()
  {
    --index;
    return *this;
  }
  iterator operator--(int)
  {
    auto copy = *this;
    --index;
    return copy;
  }

  iterator &operator+=(size_t r)
  {
    index += r;
    return *this;
  }

  iterator &operator-=(size_t r)
  {
    index -= r;
    return *this;
  }

  friend iterator operator+(const iterator &l, size_t r) { return {*l.parent, l.index + r}; }
  friend iterator operator-(const iterator &l, size_t r) { return {*l.parent, l.index - r}; }
  friend iterator operator+(size_t l, const iterator &r) { return {*r.parent, r.index + l}; }
  friend iterator operator-(size_t l, const iterator &r) { return {*r.parent, r.index - l}; }
  friend difference_type operator-(const iterator &l, const iterator &r) { return l.index - r.index; }

  bool operator==(const iterator &other) const { return index == other.index; }
  bool operator!=(const iterator &other) const { return index != other.index; }
  bool operator<(const iterator &other) const { return index < other.index; }
  bool operator>(const iterator &other) const { return index > other.index; }
  bool operator<=(const iterator &other) const { return index <= other.index; }
  bool operator>=(const iterator &other) const { return index >= other.index; }
};

template <typename... Ts>
class VariantVector<Ts...>::const_iterator // satisfies LegacyRandomAccessIterator
{
  const VariantVector *parent;
  size_t index;

  friend class VariantVector;
  const_iterator(const VariantVector &p, size_t i) : parent{&p}, index{i} {}

public:
  const_iterator(const const_iterator &) = default;
  const_iterator &operator=(const const_iterator &) = default;

  using value_type = ConstElementProxy;
  using difference_type = ptrdiff_t;
  using reference = ConstElementProxy;
  using pointer = void;
  using iterator_category = eastl::random_access_iterator_tag;

  ConstElementProxy operator*() { return ConstElementProxy{*parent, index}; }
  ConstElementProxy operator[](size_t i) const { return ConstElementProxy{*parent, index + i}; }
  const_iterator &operator++()
  {
    ++index;
    return *this;
  }
  const_iterator operator++(int)
  {
    auto copy = *this;
    ++index;
    return copy;
  }
  const_iterator &operator--()
  {
    --index;
    return *this;
  }
  const_iterator operator--(int)
  {
    auto copy = *this;
    --index;
    return copy;
  }

  const_iterator &operator+=(size_t r)
  {
    index += r;
    return *this;
  }

  const_iterator &operator-=(size_t r)
  {
    index -= r;
    return *this;
  }

  friend const_iterator operator+(const const_iterator &l, size_t r) { return {*l.parent, l.index + r}; }
  friend const_iterator operator-(const const_iterator &l, size_t r) { return {*l.parent, l.index - r}; }
  friend const_iterator operator+(size_t l, const const_iterator &r) { return {*r.parent, r.index + l}; }
  friend const_iterator operator-(size_t l, const const_iterator &r) { return {*r.parent, r.index - l}; }
  friend difference_type operator-(const const_iterator &l, const const_iterator &r) { return l.index - r.index; }

  bool operator==(const const_iterator &other) const { return index == other.index; }
  bool operator!=(const const_iterator &other) const { return index != other.index; }
  bool operator<(const const_iterator &other) const { return index < other.index; }
  bool operator>(const const_iterator &other) const { return index > other.index; }
  bool operator<=(const const_iterator &other) const { return index <= other.index; }
  bool operator>=(const const_iterator &other) const { return index >= other.index; }
};

template <typename... Ts>
template <typename U>
class VariantVector<Ts...>::typed_iterator // satisfies LegacyForwardIterator
{
  VariantVector *parent;
  size_t index;

  friend class VariantVector;
  typed_iterator(VariantVector &p, size_t i) : parent{&p}, index{i} {}

public:
  typed_iterator(const typed_iterator &) = default;
  typed_iterator &operator=(const typed_iterator &) = default;

  using value_type = U;
  using difference_type = ptrdiff_t;
  using reference = U &;
  using pointer = U *;
  using iterator_category = eastl::forward_iterator_tag;

  U &operator*() { return *parent->getIf<U>(index); }
  U *operator->() { return parent->getIf<U>(index); }
  typed_iterator &operator++()
  {
    index = parent->findNextOf<U>(index);
    return *this;
  }
  typed_iterator operator++(int)
  {
    auto copy = *this;
    index = parent->findNextOf<U>(index);
    return copy;
  }
  auto asUntyped() const { return parent->begin() + index; }

  bool operator==(const typed_iterator &other) const { return index == other.index; }
  bool operator!=(const typed_iterator &other) const { return index != other.index; }
};

template <typename... Ts>
template <typename U>
class VariantVector<Ts...>::typed_const_iterator // satisfies LegacyForwardIterator
{
  const VariantVector *parent;
  size_t index;

  friend class VariantVector;
  typed_const_iterator(const VariantVector &p, size_t i) : parent{&p}, index{i} {}

public:
  typed_const_iterator(const typed_const_iterator &) = default;
  typed_const_iterator &operator=(const typed_const_iterator &) = default;

  using value_type = U;
  using difference_type = ptrdiff_t;
  using reference = const U &;
  using pointer = const U *;
  using iterator_category = eastl::forward_iterator_tag;

  const U &operator*() { return *parent->getIf<U>(index); }
  const U *operator->() { return parent->getIf<U>(index); }
  typed_const_iterator &operator++()
  {
    index = parent->findNextOf<U>(index);
    return *this;
  }
  typed_const_iterator operator++(int)
  {
    auto copy = *this;
    index = parent->findNextOf<U>(index);
    return copy;
  }
  auto asUntyped() const { return parent->cbegin() + index; }

  bool operator==(const typed_const_iterator &other) const { return index == other.index; }
  bool operator!=(const typed_const_iterator &other) const { return index != other.index; }
};

} // namespace dag
