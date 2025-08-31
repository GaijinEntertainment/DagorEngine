//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/type_traits.h>
#include <EASTL/memory.h>
#include <EASTL/variant.h>
#include <debug/dag_assert.h>


namespace dag
{

namespace detail
{

template <typename T>
inline T *cast_to(void *address)
{
  return static_cast<T *>(address);
}

template <typename T>
inline const T *cast_to_const(const void *address)
{
  return static_cast<const T *>(address);
}

template <typename T, typename...>
struct FirstTypeOf
{
  using Type = T;
};

template <typename... Ts, size_t... I>
__forceinline void destroy_as_ith_impl(eastl::index_sequence<I...>, size_t index, void *address)
{
  (
    [&]() {
      if (index == I)
        eastl::destroy_at(cast_to<Ts>(address));
    }(),
    ...);
}

template <typename... Ts>
__forceinline void destroy_as_ith(size_t index, void *address)
{
  destroy_as_ith_impl<Ts...>(eastl::make_index_sequence<sizeof...(Ts)>{}, index, address);
}

template <typename... Ts, size_t... I>
__forceinline void move_as_ith_impl(eastl::index_sequence<I...>, size_t index, void *from, void *to)
{
  (
    [&]() {
      if (index == I)
      {
        ::new (cast_to<Ts>(to)) Ts{*cast_to<Ts>(from)};
      }
    }(),
    ...);
}

template <typename V, typename... Ts, size_t... I>
__forceinline void visit_as_ith_impl(eastl::index_sequence<I...>, size_t index, void *address, V &&v)
{
  (
    [&]() {
      if (index == I)
      {
        v(*cast_to<Ts>(address));
      }
    }(),
    ...);
}

template <typename V, typename... Ts, size_t... I>
__forceinline void visit_as_ith_impl(eastl::index_sequence<I...>, size_t index, const void *address, V &&v)
{
  (
    [&]() {
      if (index == I)
      {
        v(*cast_to_const<Ts>(address));
      }
    }(),
    ...);
}

template <typename... Ts>
__forceinline void move_as_ith(size_t index, void *from, void *to)
{
  move_as_ith_impl<Ts...>(eastl::make_index_sequence<sizeof...(Ts)>{}, index, from, to);
}

template <typename V, typename... Ts>
__forceinline void visit_as_ith(size_t index, void *address, V &&v)
{
  return visit_as_ith_impl<V, Ts...>(eastl::make_index_sequence<sizeof...(Ts)>{}, index, address, eastl::move(v));
}

template <typename V, typename... Ts>
__forceinline void visit_as_ith(size_t index, const void *address, V &&v)
{
  return visit_as_ith_impl<V, Ts...>(eastl::make_index_sequence<sizeof...(Ts)>{}, index, address, eastl::move(v));
}

template <typename T, typename... Ts, size_t... Is>
constexpr size_t type_index_of_impl(eastl::index_sequence<Is...>)
{
  static_assert((eastl::is_same_v<T, Ts> || ...));
  return ([&]() {
    if constexpr (eastl::is_same_v<T, Ts>)
      return Is;
    else
      return 0;
  }() + ...);
}

template <typename T, typename... Ts>
constexpr size_t type_index_of()
{
  return type_index_of_impl<T, Ts...>(eastl::make_index_sequence<sizeof...(Ts)>{});
}

struct MaxSizeMonoid
{
  size_t value;

  constexpr friend MaxSizeMonoid operator+(MaxSizeMonoid fst, MaxSizeMonoid snd)
  {
    return {fst.value > snd.value ? fst.value : snd.value};
  }
};

template <size_t... Sizes>
constexpr size_t max_of()
{
  return (MaxSizeMonoid{Sizes} + ...).value;
}

template <typename... Ts>
class VariantElementStorage
{
  alignas(max_of<alignof(Ts)...>()) uint8_t storage[max_of<sizeof(Ts)...>()];

public:
  void *addressOf() { return storage; }
  const void *addressOf() const { return storage; }
};

template <size_t N>
auto select_index_type()
{
  if constexpr (N <= eastl::numeric_limits<uint8_t>::max())
    return uint8_t{};
  else if constexpr (N <= eastl::numeric_limits<uint16_t>::max())
    return uint16_t{};
  else if constexpr (N <= eastl::numeric_limits<uint32_t>::max())
    return uint32_t{};
  else
    return uint64_t{};
}

template <typename... Ts>
class VariantElementTypeStorage
{
  using IndexType = decltype(select_index_type<sizeof...(Ts)>());
  using DestructorType = void (*)(void *);
  IndexType index;

  template <typename U>
  static U &castTo(void *address)
  {
    return *cast_to<U>(address);
  }

  template <typename U>
  static const U &castTo(const void *address)
  {
    return *cast_to_const<U>(address);
  }
  template <typename U>
  static constexpr IndexType typeIndexOf()
  {
    return static_cast<IndexType>(type_index_of<U, Ts...>());
  }

  VariantElementTypeStorage(IndexType i) : index{i} {}

public:
  VariantElementTypeStorage() = default;
  VariantElementTypeStorage(const VariantElementTypeStorage &) = default;
  VariantElementTypeStorage &operator=(const VariantElementTypeStorage &) = default;
  template <typename U>
  static constexpr VariantElementTypeStorage typeOf()
  {
    return {typeIndexOf<U>()};
  }
  size_t getIndex() const { return index; }

  template <typename U>
  bool is() const
  {
    return index == typeIndexOf<U>();
  }

  template <typename U, typename... Us>
  void constructAt(void *address, Us &&...us)
  {
    using ConstructT = typename eastl::remove_cvref_t<U>;
    auto newIndex = typeIndexOf<ConstructT>();
    ::new ((void *)&castTo<ConstructT>(address)) ConstructT{eastl::forward<Us>(us)...};
    index = newIndex;
  }

  void destructAt(void *address) const { destroy_as_ith<Ts...>(index, address); }

  VariantElementTypeStorage moveTo(void *from, void *to) const
  {
    move_as_ith<Ts...>(index, from, to);
    return *this;
  }

  template <typename U>
  U *getIf(void *address) const
  {
    return is<U>() ? &castTo<U>(address) : nullptr;
  }

  template <typename U>
  const U *getIf(const void *address) const
  {
    return is<U>() ? &castTo<U>(address) : nullptr;
  }

  template <typename V>
  void visit(void *address, V &&v) const
  {
    return visit_as_ith<V, Ts...>(index, address, eastl::move(v));
  }

  template <typename V>
  void visit(const void *address, V &&v) const
  {
    return visit_as_ith<V, Ts...>(index, address, eastl::move(v));
  }

  friend bool operator==(const VariantElementTypeStorage &l, const VariantElementTypeStorage &r) { return l.index == r.index; }
  friend bool operator!=(const VariantElementTypeStorage &l, const VariantElementTypeStorage &r) { return !(l == r); }
};

} // namespace detail

/// @brief A type equivalent to array<variant<Ts...>, N> but with a more efficient storage layout
template <size_t N, typename... Ts>
class VariantArray
{
  using TypeStorage = detail::VariantElementTypeStorage<Ts...>;
  using ElementStorage = detail::VariantElementStorage<Ts...>;
  using FirstType = typename detail::FirstTypeOf<Ts...>::Type;

  TypeStorage types[N];
  ElementStorage values[N];

  // invokes c with type and value for each entry
  template <typename C>
  void forEach(C &&c)
  {
    for (size_t i = 0; i < size(); ++i)
    {
      c(types[i], values[i]);
    }
  }

  template <typename U>
  size_t findFirstOf(size_t offset) const
  {
    auto typeToLookFor = TypeStorage::template typeOf<U>();
    auto typesBegin = types + offset;
    auto typesEnd = types + N;
    auto typesAt = eastl::find(typesBegin, typesEnd, typeToLookFor);
    auto result = eastl::distance(types, typesAt);
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
    auto typesBegin = types + last_time_t + 1;
    auto typesEnd = types + N;
    auto typesAt = eastl::find(typesBegin, typesEnd, typeToLookFor);
    auto result = eastl::distance(types, typesAt);
    return result;
  }

public:
  // behave the same way as std variant, default create of the first type
  VariantArray() // -V730
  {
    forEach([](TypeStorage &type, ElementStorage &value) { type.template constructAt<FirstType>(value.addressOf()); });
  }

  ~VariantArray()
  {
    forEach([](TypeStorage &type, ElementStorage &value) { type.destructAt(value.addressOf()); });
  }

  constexpr size_t size() const { return N; }
  size_t typeIndex(size_t index) const { return types[index].getIndex(); }

  template <typename U, typename... Us>
  void emplace(size_t index, Us &&...us)
  {
    G_ASSERT(index < N);
    auto &type = types[index];
    auto &value = values[index];
    type.destructAt(value.addressOf());
    type.template constructAt<U>(value.addressOf(), eastl::forward<Us>(us)...);
  }

  template <typename U>
  void assign(size_t index, U &&u)
  {
    G_ASSERT(index < N);
    auto &type = types[index];
    auto &value = values[index];
    type.destructAt(value.addressOf());
    type.template constructAt<U>(value.addressOf(), eastl::move(u));
  }

  template <typename V>
  auto visit(size_t index, V &&v) const
  {
    G_ASSERT(index < N);
    auto &type = types[index];
    auto &value = values[index];
    type.visit(value.addressOf(), eastl::move(v));
  }

  template <typename U>
  bool holdsAlternative(size_t index) const
  {
    G_ASSERT(index < N);
    return types[index].template is<U>();
  }

  template <typename U>
  const U *getIf(size_t index) const
  {
    G_ASSERT(index < N);
    return types[index].template getIf<U>(values[index].addressOf());
  }

  template <typename U>
  U *getIf(size_t index)
  {
    G_ASSERT(index < N);
    return types[index].template getIf<U>(values[index].addressOf());
  }

  class ConstElementProxy;
  class ElementProxy;

  ConstElementProxy operator[](size_t index) const
  {
    G_ASSERT(index < N);
    return ConstElementProxy{*this, index};
  }
  ElementProxy operator[](size_t index)
  {
    G_ASSERT(index < N);
    return ElementProxy{*this, index};
  }

  class iterator;
  class const_iterator;

  iterator begin() { return iterator{*this, 0}; }
  iterator end() { return iterator{*this, N}; }

  const_iterator begin() const { return const_iterator{*this, 0}; }
  const_iterator end() const { return const_iterator{*this, N}; }

  const_iterator cbegin() const { return begin(); }
  const_iterator cend() const { return end(); }

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

template <size_t N, typename... Ts>
class VariantArray<N, Ts...>::ConstElementProxy
{
  const VariantArray &parent;
  size_t index;

  friend class VariantArray;
  ConstElementProxy(const VariantArray &p, size_t i) : parent{p}, index{i} {}

public:
  ConstElementProxy(const ConstElementProxy &) = default;

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

template <size_t N, typename... Ts>
class VariantArray<N, Ts...>::ElementProxy
{
  VariantArray &parent;
  size_t index;

  friend class VariantArray;
  ElementProxy(VariantArray &p, size_t i) : parent{p}, index{i} {}

public:
  ElementProxy(const ElementProxy &) = default;

  size_t typeIndex() const { return parent.typeIndex(index); }
  template <typename U, typename... Us>
  void emplace(Us &&...us) const
  {
    parent.emplace<U>(index, eastl::forward<Us>(us)...);
  }
  template <typename U>
  void assign(U &&u)
  {
    parent.assign(index, eastl::move(u));
  }

  template <typename U>
  ElementProxy &operator=(U &&u)
  {
    parent.assign(index, eastl::move(u));
    return *this;
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

template <size_t N, typename... Ts>
class VariantArray<N, Ts...>::iterator // satisfies LegacyRandomAccessIterator
{
  VariantArray *parent;
  size_t index;

  friend class VariantArray;
  iterator(VariantArray &p, size_t i) : parent{&p}, index{i} {}

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

template <size_t N, typename... Ts>
class VariantArray<N, Ts...>::const_iterator // satisfies LegacyRandomAccessIterator
{
  const VariantArray *parent;
  size_t index;

  friend class VariantArray;
  const_iterator(const VariantArray &p, size_t i) : parent{&p}, index{i} {}

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

template <size_t N, typename... Ts>
template <typename U>
class VariantArray<N, Ts...>::typed_iterator // satisfies LegacyForwardIterator
{
  VariantArray *parent;
  size_t index;

  friend class VariantArray;
  typed_iterator(VariantArray &p, size_t i) : parent{&p}, index{i} {}

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

template <size_t N, typename... Ts>
template <typename U>
class VariantArray<N, Ts...>::typed_const_iterator // satisfies LegacyForwardIterator
{
  const VariantArray *parent;
  size_t index;

  friend class VariantArray;
  typed_const_iterator(const VariantArray &p, size_t i) : parent{&p}, index{i} {}

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
