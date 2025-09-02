//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/variant.h>

namespace dag
{

// @TODO (at some point): void specialization, monadic operations, full sfinae

namespace detail
{

template <typename L, typename R, class = void>
struct IsEqComparable : eastl::false_type
{};

template <typename L, typename R>
using Comparability = decltype(eastl::declval<L>() == eastl::declval<R>());

template <typename L, typename R>
struct IsEqComparable<L, R, eastl::void_t<Comparability<L, R>>> : eastl::true_type
{};

template <typename L, typename R>
static constexpr bool is_eq_comparable_v = IsEqComparable<L, R>::value;

} // namespace detail

struct unexpect_t
{};
static constexpr unexpect_t unexpect{};

template <typename E>
class Unexpected
{
public:
  constexpr Unexpected(const Unexpected &other) = default;
  constexpr Unexpected(Unexpected &&other) = default;

  template <class... TArgs, eastl::enable_if_t<eastl::is_constructible_v<E, TArgs...>, bool> = true>
  constexpr explicit Unexpected(eastl::in_place_t, TArgs &&...args) : m_error(eastl::forward<TArgs>(args)...)
  {}
  template <class U, class... TArgs, eastl::enable_if_t<eastl::is_constructible_v<E, std::initializer_list<U>, TArgs...>, bool> = true>
  constexpr explicit Unexpected(eastl::in_place_t, std::initializer_list<U> il, TArgs &&...args) :
    m_error(il, eastl::forward<TArgs>(args)...)
  {}

  template <typename TErr, eastl::enable_if_t<eastl::is_convertible_v<TErr, E>, bool> = true>
  constexpr explicit Unexpected(TErr &&err) : m_error(eastl::forward<TErr>(err))
  {}

  constexpr const E &error() const & { return m_error; }
  constexpr E &error() & { return m_error; }
  constexpr const E &&error() const && { return eastl::move(m_error); }
  constexpr E &&error() && { return eastl::move(m_error); }

  template <eastl::enable_if_t<eastl::is_swappable_v<E>, bool> = true>
  constexpr void swap(Unexpected &other)
  {
    eastl::swap(m_error, other.m_error);
  }
  template <eastl::enable_if_t<eastl::is_swappable_v<E>, bool> = true>
  friend constexpr void swap(Unexpected &u1, Unexpected &u2)
  {
    u1.swap(u2);
  }

  template <class E2, eastl::enable_if_t<detail::is_eq_comparable_v<E, E2>, bool> = true>
  friend constexpr bool operator==(const Unexpected &x, const Unexpected<E2> &y)
  {
    return x.error() == y.error();
  }

private:
  E m_error;
};

template <typename TErr>
Unexpected(TErr &&) -> Unexpected<TErr>;

template <typename T, typename E, eastl::enable_if_t<!eastl::is_same_v<T, void>, bool> = false>
class Expected
{
public:
  constexpr Expected() = default;
  constexpr Expected(const Expected &other) = default;
  constexpr Expected(Expected &&other) = default;

  template <typename U = T, eastl::enable_if_t<eastl::is_convertible_v<U, T>, bool> = true>
  constexpr Expected(U &&val) : m_var(eastl::move(val))
  {}

  template <typename G = E, eastl::enable_if_t<eastl::is_convertible_v<const G &, E>, bool> = true>
  constexpr Expected(const Unexpected<G> &unexpected) : m_var(unexpected.error())
  {}
  template <typename G = E, eastl::enable_if_t<eastl::is_convertible_v<G, E>, bool> = true>
  constexpr Expected(Unexpected<G> &&unexpected) : m_var(eastl::move(unexpected.error()))
  {}

  template <typename U, typename G>
  constexpr Expected(const Expected<U, G> &other) : m_var(other.has_value() ? Expected{*other} : Expected{other.error()})
  {}
  template <typename U, typename G>
  constexpr Expected(Expected<U, G> &&other) :
    m_var(other.has_value() ? Expected{eastl::move(*other)} : Expected{eastl::move(other.error())})
  {}

  template <class... TArgs, eastl::enable_if_t<eastl::is_constructible_v<T, TArgs...>, bool> = true>
  constexpr explicit Expected(eastl::in_place_t, TArgs &&...args) : m_var(T{eastl::forward<TArgs>(args)...})
  {}
  template <class U, class... TArgs, eastl::enable_if_t<eastl::is_constructible_v<T, std::initializer_list<U>, TArgs...>, bool> = true>
  constexpr explicit Expected(eastl::in_place_t, std::initializer_list<U> il, TArgs &&...args) :
    m_var(T{il, eastl::forward<TArgs>(args)...})
  {}

  template <class... TArgs, eastl::enable_if_t<eastl::is_constructible_v<E, TArgs...>, bool> = true>
  constexpr explicit Expected(unexpect_t, TArgs &&...args) : m_var(E{eastl::forward<TArgs>(args)...})
  {}
  template <class U, class... TArgs, eastl::enable_if_t<eastl::is_constructible_v<E, std::initializer_list<U>, TArgs...>, bool> = true>
  constexpr explicit Expected(unexpect_t, std::initializer_list<U> il, TArgs &&...args) : m_var(E{il, eastl::forward<TArgs>(args)...})
  {}

  constexpr Expected &operator=(const Expected &other) = default;
  constexpr Expected &operator=(Expected &&other) = default;

  template <typename U = T, eastl::enable_if_t<eastl::is_convertible_v<U, T>, bool> = true>
  constexpr Expected &operator=(U &&val)
  {
    m_var = eastl::move(val);
    return *this;
  }

  template <typename G = E, eastl::enable_if_t<eastl::is_convertible_v<const G &, E>, bool> = true>
  constexpr Expected &operator=(const Unexpected<G> &unexpected)
  {
    m_var = unexpected.error();
    return *this;
  }
  template <typename G = E, eastl::enable_if_t<eastl::is_convertible_v<G, E>, bool> = true>
  constexpr Expected &operator=(Unexpected<G> &&unexpected)
  {
    m_var = eastl::move(unexpected.error());
    return *this;
  }

  constexpr const T *operator->() const { return eastl::get_if<T>(&m_var); }
  constexpr T *operator->() { return eastl::get_if<T>(&m_var); }

  constexpr bool has_value() const { return eastl::get_if<T>(&m_var) != nullptr; }
  constexpr operator bool() const { return has_value(); }

  constexpr T &value() & { return eastl::get<T>(m_var); }
  constexpr const T &value() const & { return eastl::get<T>(m_var); }
  constexpr T &&value() && { return eastl::get<T>(eastl::move(m_var)); }
  constexpr const T &&value() const && { return eastl::get<T>(eastl::move(m_var)); }

  constexpr const T &operator*() const & { return value(); }
  constexpr T &operator*() & { return value(); }
  constexpr const T &&operator*() const && { return value(); }
  constexpr T &&operator*() && { return value(); }

  constexpr const E &error() const & { return eastl::get<E>(m_var); }
  constexpr E &error() & { return eastl::get<E>(m_var); }
  constexpr const E &&error() const && { return eastl::get<E>(eastl::move(m_var)); }
  constexpr E &&error() && { return eastl::get<E>(eastl::move(m_var)); }

  template <class... TArgs>
  constexpr T &emplace(TArgs &&...args)
  {
    m_var = T{eastl::forward<TArgs>(args)...};
    return eastl::get<T>(m_var);
  }

  template <class U = T>
  constexpr T value_or(U &&default_val) const &
  {
    return has_value() ? **this : T{eastl::forward<U>(default_val)};
  }
  template <class U = T>
  constexpr T value_or(U &&default_val) &&
  {
    return has_value() ? eastl::move(**this) : T{eastl::forward<U>(default_val)};
  }
  template <class G = E>
  constexpr E error_or(G &&default_val) const &
  {
    return has_value() ? E{eastl::forward<G>(default_val)} : error();
  }
  template <class G = E>
  constexpr E error_or(G &&default_val) &&
  {
    return has_value() ? E{eastl::forward<G>(default_val)} : eastl::move(error());
  }

  template <eastl::enable_if_t<eastl::is_swappable_v<eastl::variant<T, E>>, bool> = true>
  constexpr void swap(Expected &other)
  {
    eastl::swap(m_var, other.m_var);
  }
  template <eastl::enable_if_t<eastl::is_swappable_v<eastl::variant<T, E>>, bool> = true>
  friend constexpr void swap(Expected &e1, Expected &e2)
  {
    e1.swap(e2);
  }

  template <class T2, class E2,
    eastl::enable_if_t<detail::is_eq_comparable_v<T, T2> && detail::is_eq_comparable_v<E, E2>, bool> = true>
  friend constexpr bool operator==(const Expected &x, const Expected<T2, E2> &y)
  {
    return x.has_value() == y.has_value() && ((x.has_value() && (*x == *y)) || (!x.has_value() && (x.error() == y.error())));
  }
  template <class T2, eastl::enable_if_t<detail::is_eq_comparable_v<T, T2>, bool> = true>
  friend constexpr bool operator==(const Expected &x, const T2 &y)
  {
    return x.has_value() && *x == y;
  }
  template <class E2, eastl::enable_if_t<detail::is_eq_comparable_v<E, E2>, bool> = true>
  friend constexpr bool operator==(const Expected &x, const Unexpected<E2> &y)
  {
    return !x.has_value() && x.error() == y.error();
  }

private:
  eastl::variant<T, E> m_var;
};

} // namespace dag