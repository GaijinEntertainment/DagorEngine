//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_globDef.h>
#include <EASTL/memory.h>

namespace dag
{

namespace detail
{

template <class T, class... TArgs>
constexpr T *construct_at(T *loc, TArgs &&...args)
  requires(!eastl::is_unbounded_array_v<T> && requires(void *p, TArgs &&...args) { ::new (p) T(std::forward<TArgs>(args)...); })
{
  ::new (loc) T(std::forward<TArgs>(args)...);
  return loc;
}

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

inline constexpr size_t lcm(size_t a, size_t b)
{
  const size_t prod = a * b;
  while (b != 0)
  {
    const size_t mx = max(a, b);
    const size_t mn = min(a, b);
    a = mx - mn;
    b = mn;
  }
  return prod / a;
}


template <class T>
struct IsExpectedSpecImpl : eastl::false_type
{};

template <class T>
inline constexpr bool is_expected_spec_v = IsExpectedSpecImpl<eastl::remove_cvref_t<T>>::value;

} // namespace detail

struct unexpect_t
{};
inline constexpr unexpect_t unexpect{};

template <typename E>
class Unexpected
{
public:
  constexpr Unexpected(const Unexpected &other)
    requires std::is_copy_constructible_v<E>
  = default;
  constexpr Unexpected(Unexpected &&other)
    requires std::is_move_constructible_v<E>
  = default;

  template <class... TArgs>
  constexpr explicit Unexpected(eastl::in_place_t, TArgs &&...args)
    requires eastl::is_constructible_v<E, TArgs...>
    : mError(eastl::forward<TArgs>(args)...)
  {}
  template <class U, class... TArgs>
  constexpr explicit Unexpected(eastl::in_place_t, std::initializer_list<U> il, TArgs &&...args)
    requires eastl::is_constructible_v<E, std::initializer_list<U>, TArgs...>
    : mError(il, eastl::forward<TArgs>(args)...)
  {}

  template <typename TErr = E>
  constexpr explicit Unexpected(TErr &&err)
    requires eastl::is_constructible_v<E, TErr>
    : mError(eastl::forward<TErr>(err))
  {}

  constexpr const E &error() const & { return mError; }
  constexpr E &error() & { return mError; }
  constexpr const E &&error() const && { return eastl::move(mError); }
  constexpr E &&error() && { return eastl::move(mError); }

  constexpr void swap(Unexpected &other)
    requires eastl::is_swappable_v<E>
  {
    eastl::swap(mError, other.mError);
  }
  friend constexpr void swap(Unexpected &u1, Unexpected &u2)
    requires eastl::is_swappable_v<E>
  {
    u1.swap(u2);
  }

  template <class E2>
  friend constexpr bool operator==(const Unexpected &x, const Unexpected<E2> &y)
    requires detail::is_eq_comparable_v<E, E2>
  {
    return x.error() == y.error();
  }

private:
  E mError;
};

template <typename TErr>
Unexpected(TErr &&) -> Unexpected<eastl::remove_reference_t<TErr>>;

template <typename T, typename E>
class [[nodiscard]] Expected
{
public:
  using value_type = T;
  using error_type = E;
  using unexpected_type = dag::Unexpected<E>;

  constexpr Expected()
    requires eastl::is_default_constructible_v<T>
    : mHasValue(true)
  {
    detail::construct_at(reinterpret_cast<T *>(mStorage));
  }

  template <typename U = T>
  constexpr Expected(U &&val)
    requires eastl::is_convertible_v<U, T> && eastl::is_move_constructible_v<T>
    : mHasValue(true)
  {
    detail::construct_at(reinterpret_cast<T *>(mStorage), eastl::move(val));
  }

  template <typename G = E>
  constexpr Expected(const Unexpected<G> &unexpected)
    requires eastl::is_constructible_v<E, const G &>
    : mHasValue(false)
  {
    detail::construct_at(reinterpret_cast<E *>(mStorage), unexpected.error());
  }
  template <typename G = E>
  constexpr Expected(Unexpected<G> &&unexpected)
    requires eastl::is_constructible_v<E, G>
    : mHasValue(false)
  {
    detail::construct_at(reinterpret_cast<E *>(mStorage), eastl::move(unexpected.error()));
  }

  template <typename U, typename G>
  constexpr Expected(const Expected<U, G> &other)
    requires eastl::is_constructible_v<T, const U &> && eastl::is_constructible_v<E, const G &>
    : mHasValue(other.has_value())
  {
    if (mHasValue)
      detail::construct_at(reinterpret_cast<T *>(mStorage), other.value());
    else
      detail::construct_at(reinterpret_cast<E *>(mStorage), other.error());
  }

  template <typename U, typename G>
  constexpr Expected(Expected<U, G> &&other)
    requires eastl::is_constructible_v<T, U> && eastl::is_constructible_v<E, G>
    : mHasValue(other.has_value())
  {
    if (mHasValue)
      detail::construct_at(reinterpret_cast<T *>(mStorage), eastl::move(other.value()));
    else
      detail::construct_at(reinterpret_cast<E *>(mStorage), eastl::move(other.error()));
  }

  template <class... TArgs>
  constexpr explicit Expected(eastl::in_place_t, TArgs &&...args)
    requires eastl::is_constructible_v<T, TArgs...>
    : mHasValue(true)
  {
    detail::construct_at(reinterpret_cast<T *>(mStorage), eastl::forward<TArgs>(args)...);
  }
  template <class U, class... TArgs>
  constexpr explicit Expected(eastl::in_place_t, std::initializer_list<U> il, TArgs &&...args)
    requires eastl::is_constructible_v<T, std::initializer_list<U>, TArgs...>
    : mHasValue(true)
  {
    detail::construct_at(reinterpret_cast<T *>(mStorage), il, eastl::forward<TArgs>(args)...);
  }

  template <class... TArgs>
  constexpr explicit Expected(unexpect_t, TArgs &&...args)
    requires eastl::is_constructible_v<E, TArgs...>
    : mHasValue(false)
  {
    detail::construct_at(reinterpret_cast<E *>(mStorage), eastl::forward<TArgs>(args)...);
  }
  template <class U, class... TArgs>
  constexpr explicit Expected(unexpect_t, std::initializer_list<U> il, TArgs &&...args)
    requires eastl::is_constructible_v<E, std::initializer_list<U>, TArgs...>
    : mHasValue(false)
  {
    detail::construct_at(reinterpret_cast<E *>(mStorage), il, eastl::forward<TArgs>(args)...);
  }

  constexpr ~Expected()
  {
    if (mHasValue)
      eastl::destroy_at(reinterpret_cast<T *>(mStorage));
    else
      eastl::destroy_at(reinterpret_cast<E *>(mStorage));
  }

  template <typename U = T>
  constexpr Expected &operator=(U &&val)
    requires eastl::is_assignable_v<T, U> && eastl::is_constructible_v<T, U>
  {
    if (mHasValue)
    {
      *reinterpret_cast<T *>(mStorage) = eastl::move(val);
    }
    else
    {
      mHasValue = true;
      eastl::destroy_at(reinterpret_cast<E *>(mStorage));
      detail::construct_at(reinterpret_cast<T *>(mStorage), eastl::move(val));
    }
    return *this;
  }

  template <typename G = E>
  constexpr Expected &operator=(const Unexpected<G> &unexpected)
    requires eastl::is_assignable_v<E, const G &> && eastl::is_constructible_v<E, const G &>
  {
    if (mHasValue)
    {
      mHasValue = false;
      eastl::destroy_at(reinterpret_cast<T *>(mStorage));
      detail::construct_at(reinterpret_cast<E *>(mStorage), unexpected.error());
    }
    else
    {
      *reinterpret_cast<E *>(mStorage) = unexpected.error();
    }
    return *this;
  }
  template <typename G = E>
  constexpr Expected &operator=(Unexpected<G> &&unexpected)
    requires eastl::is_assignable_v<E, G> && eastl::is_constructible_v<E, G>
  {
    if (mHasValue)
    {
      mHasValue = false;
      eastl::destroy_at(reinterpret_cast<T *>(mStorage));
      detail::construct_at(reinterpret_cast<E *>(mStorage), eastl::move(unexpected.error()));
    }
    else
    {
      *reinterpret_cast<E *>(mStorage) = eastl::move(unexpected.error());
    }
    return *this;
  }

  constexpr const T *operator->() const { return mHasValue ? reinterpret_cast<const T *>(mStorage) : nullptr; }
  constexpr T *operator->() { return mHasValue ? reinterpret_cast<T *>(mStorage) : nullptr; }

  constexpr bool has_value() const { return mHasValue; }
  constexpr operator bool() const { return mHasValue; }

  constexpr T &value() & { return *reinterpret_cast<T *>(mStorage); }
  constexpr const T &value() const & { return *reinterpret_cast<const T *>(mStorage); }
  constexpr T &&value() && { return eastl::move(*reinterpret_cast<T *>(mStorage)); }
  constexpr const T &&value() const && { return eastl::move(*reinterpret_cast<const T *>(mStorage)); }

  constexpr const T &operator*() const & { return value(); }
  constexpr T &operator*() & { return value(); }
  constexpr const T &&operator*() const && { return value(); }
  constexpr T &&operator*() && { return value(); }

  constexpr E &error() & { return *reinterpret_cast<E *>(mStorage); }
  constexpr const E &error() const & { return *reinterpret_cast<const E *>(mStorage); }
  constexpr E &&error() && { return eastl::move(*reinterpret_cast<E *>(mStorage)); }
  constexpr const E &&error() const && { return eastl::move(*reinterpret_cast<const E *>(mStorage)); }

  template <class... TArgs>
  constexpr T &emplace(TArgs &&...args)
    requires eastl::is_constructible_v<T, TArgs...>
  {
    if (mHasValue)
      eastl::destroy_at(reinterpret_cast<T *>(mStorage));
    else
      eastl::destroy_at(reinterpret_cast<E *>(mStorage));
    detail::construct_at(reinterpret_cast<T *>(mStorage), eastl::forward<TArgs>(args)...);
    mHasValue = true;
    return value();
  }

  template <class U, class... TArgs>
  constexpr T &emplace(std::initializer_list<U> il, TArgs &&...args)
    requires eastl::is_constructible_v<T, std::initializer_list<U>, TArgs...>
  {
    if (mHasValue)
      eastl::destroy_at(reinterpret_cast<T *>(mStorage));
    else
      eastl::destroy_at(reinterpret_cast<E *>(mStorage));
    detail::construct_at(reinterpret_cast<T *>(mStorage), il, eastl::forward<TArgs>(args)...);
    mHasValue = true;
    return value();
  }

  template <class U = T>
  constexpr T value_or(U &&default_val) const &
    requires eastl::is_convertible_v<U, T>
  {
    return has_value() ? value() : T{eastl::forward<U>(default_val)};
  }
  template <class U = T>
  constexpr T value_or(U &&default_val) &&
    requires eastl::is_convertible_v<U, T>
  {
    return has_value() ? eastl::move(value()) : T{eastl::forward<U>(default_val)};
  }
  template <class G = E>
  constexpr E error_or(G &&default_val) const &
    requires eastl::is_convertible_v<G, E>
  {
    return has_value() ? E{eastl::forward<G>(default_val)} : error();
  }
  template <class G = E>
  constexpr E error_or(G &&default_val) &&
    requires eastl::is_convertible_v<G, E>
  {
    return has_value() ? E{eastl::forward<G>(default_val)} : eastl::move(error());
  }

  template <class F>
    requires detail::is_expected_spec_v<eastl::invoke_result_t<F, T &>> &&                                      //
             eastl::is_same_v<typename eastl::remove_cvref_t<eastl::invoke_result_t<F, T &>>::error_type, E> && //
             eastl::is_copy_constructible_v<E>
  constexpr eastl::remove_cvref_t<eastl::invoke_result_t<F, T &>> and_then(F &&f) &
  {
    if (has_value())
      return f(*reinterpret_cast<T *>(mStorage));
    else
      return dag::Unexpected{error()};
  }
  template <class F>
    requires detail::is_expected_spec_v<eastl::invoke_result_t<F, const T &>> &&                                      //
             eastl::is_same_v<typename eastl::remove_cvref_t<eastl::invoke_result_t<F, const T &>>::error_type, E> && //
             eastl::is_copy_constructible_v<E>
  constexpr eastl::remove_cvref_t<eastl::invoke_result_t<F, const T &>> and_then(F &&f) const &
  {
    if (has_value())
      return f(*reinterpret_cast<const T *>(mStorage));
    else
      return dag::Unexpected{error()};
  }
  template <class F>
    requires detail::is_expected_spec_v<eastl::invoke_result_t<F, T &&>> &&                                      //
             eastl::is_same_v<typename eastl::remove_cvref_t<eastl::invoke_result_t<F, T &&>>::error_type, E> && //
             eastl::is_move_constructible_v<E>
  constexpr eastl::remove_cvref_t<eastl::invoke_result_t<F, T &&>> and_then(F &&f) &&
  {
    if (has_value())
      return f(eastl::move(*reinterpret_cast<T *>(mStorage)));
    else
      return dag::Unexpected{eastl::move(error())};
  }
  template <class F>
    requires detail::is_expected_spec_v<eastl::invoke_result_t<F, const T &&>> &&                                      //
             eastl::is_same_v<typename eastl::remove_cvref_t<eastl::invoke_result_t<F, const T &&>>::error_type, E> && //
             eastl::is_move_constructible_v<E>
  constexpr eastl::remove_cvref_t<eastl::invoke_result_t<F, const T &&>> and_then(F &&f) const &&
  {
    if (has_value())
      return f(eastl::move(*reinterpret_cast<const T *>(mStorage)));
    else
      return dag::Unexpected{eastl::move(error())};
  }

  template <class F>
    requires eastl::is_copy_constructible_v<E>
  constexpr Expected<eastl::remove_cv_t<eastl::invoke_result_t<F, T &>>, E> transform(F &&f) &
  {
    if (has_value())
    {
      if constexpr (eastl::is_same_v<eastl::remove_cv_t<eastl::invoke_result_t<F, T &>>, void>)
      {
        f(*reinterpret_cast<T *>(mStorage));
        return {};
      }
      else
      {
        return f(*reinterpret_cast<T *>(mStorage));
      }
    }
    else
    {
      return dag::Unexpected{error()};
    }
  }
  template <class F>
    requires eastl::is_copy_constructible_v<E>
  constexpr Expected<eastl::remove_cv_t<eastl::invoke_result_t<F, const T &>>, E> transform(F &&f) const &
  {
    if (has_value())
    {
      if constexpr (eastl::is_same_v<eastl::remove_cv_t<eastl::invoke_result_t<F, const T &>>, void>)
      {
        f(*reinterpret_cast<const T *>(mStorage));
        return {};
      }
      else
      {
        return f(*reinterpret_cast<const T *>(mStorage));
      }
    }
    else
    {
      return dag::Unexpected{error()};
    }
  }
  template <class F>
    requires eastl::is_move_constructible_v<E>
  constexpr Expected<eastl::remove_cv_t<eastl::invoke_result_t<F, T &&>>, E> transform(F &&f) &&
  {
    if (has_value())
    {
      if constexpr (eastl::is_same_v<eastl::remove_cv_t<eastl::invoke_result_t<F, T &&>>, void>)
      {
        f(eastl::move(*reinterpret_cast<T *>(mStorage)));
        return {};
      }
      else
      {
        return f(eastl::move(*reinterpret_cast<T *>(mStorage)));
      }
    }
    else
    {
      return dag::Unexpected{eastl::move(error())};
    }
  }
  template <class F>
    requires eastl::is_move_constructible_v<E>
  constexpr Expected<eastl::remove_cv_t<eastl::invoke_result_t<F, const T &&>>, E> transform(F &&f) const &&
  {
    if (has_value())
    {
      if constexpr (eastl::is_same_v<eastl::remove_cv_t<eastl::invoke_result_t<F, const T &&>>, void>)
      {
        f(eastl::move(*reinterpret_cast<const T *>(mStorage)));
        return {};
      }
      else
      {
        return f(eastl::move(*reinterpret_cast<const T *>(mStorage)));
      }
    }
    else
    {
      return dag::Unexpected{eastl::move(error())};
    }
  }

  template <class F>
    requires detail::is_expected_spec_v<eastl::invoke_result_t<F, E &>> &&                                      //
             eastl::is_same_v<typename eastl::remove_cvref_t<eastl::invoke_result_t<F, E &>>::value_type, T> && //
             eastl::is_copy_constructible_v<T>
  constexpr eastl::remove_cvref_t<eastl::invoke_result_t<F, E &>> or_else(F &&f) &
  {
    if (has_value())
      return value();
    else
      return f(*reinterpret_cast<E *>(mStorage));
  }
  template <class F>
    requires detail::is_expected_spec_v<eastl::invoke_result_t<F, const E &>> &&                                      //
             eastl::is_same_v<typename eastl::remove_cvref_t<eastl::invoke_result_t<F, const E &>>::value_type, T> && //
             eastl::is_copy_constructible_v<T>
  constexpr eastl::remove_cvref_t<eastl::invoke_result_t<F, const E &>> or_else(F &&f) const &
  {
    if (has_value())
      return value();
    else
      return f(*reinterpret_cast<const E *>(mStorage));
  }
  template <class F>
    requires detail::is_expected_spec_v<eastl::invoke_result_t<F, E &&>> &&                                      //
             eastl::is_same_v<typename eastl::remove_cvref_t<eastl::invoke_result_t<F, E &&>>::value_type, T> && //
             eastl::is_move_constructible_v<T>
  constexpr eastl::remove_cvref_t<eastl::invoke_result_t<F, E &&>> or_else(F &&f) &&
  {
    if (has_value())
      return eastl::move(value());
    else
      return f(eastl::move(*reinterpret_cast<E *>(mStorage)));
  }
  template <class F>
    requires detail::is_expected_spec_v<eastl::invoke_result_t<F, const E &&>> &&                                      //
             eastl::is_same_v<typename eastl::remove_cvref_t<eastl::invoke_result_t<F, const E &&>>::value_type, T> && //
             eastl::is_move_constructible_v<T>
  constexpr eastl::remove_cvref_t<eastl::invoke_result_t<F, const E &&>> or_else(F &&f) const &&
  {
    if (has_value())
      return eastl::move(value());
    else
      return f(eastl::move(*reinterpret_cast<const E *>(mStorage)));
  }

  template <class F>
    requires eastl::is_copy_constructible_v<E>
  constexpr Expected<T, eastl::remove_cv_t<eastl::invoke_result_t<F, E &>>> transform_error(F &&f) &
  {
    if (has_value())
      return value();
    else
      return dag::Unexpected{f(*reinterpret_cast<E *>(mStorage))};
  }
  template <class F>
    requires eastl::is_copy_constructible_v<E>
  constexpr Expected<T, eastl::remove_cv_t<eastl::invoke_result_t<F, const E &>>> transform_error(F &&f) &&
  {
    if (has_value())
      return value();
    else
      return dag::Unexpected{f(*reinterpret_cast<const E *>(mStorage))};
  }
  template <class F>
    requires eastl::is_move_constructible_v<E>
  constexpr Expected<T, eastl::remove_cv_t<eastl::invoke_result_t<F, E &&>>> transform_error(F &&f) const &
  {
    if (has_value())
      return eastl::move(value());
    else
      return dag::Unexpected{f(eastl::move(*reinterpret_cast<E *>(mStorage)))};
  }
  template <class F>
    requires eastl::is_copy_constructible_v<E>
  constexpr Expected<T, eastl::remove_cv_t<eastl::invoke_result_t<F, const E &&>>> transform_error(F &&f) const &&
  {
    if (has_value())
      return eastl::move(value());
    else
      return dag::Unexpected{f(eastl::move(*reinterpret_cast<const E *>(mStorage)))};
  }

  constexpr void swap(Expected &other)
    requires eastl::is_swappable_v<T> && eastl::is_swappable_v<E>
  {
    if (mHasValue && other.has_value())
    {
      eastl::swap(value(), other.value());
    }
    else if (!mHasValue && !other.has_value())
    {
      eastl::swap(error(), other.error());
    }
    else if (mHasValue)
    {
      T tmp = eastl::move(value());
      *this = eastl::move(other);
      other = eastl::move(tmp);
    }
    else
    {
      E tmp = eastl::move(error());
      *this = eastl::move(other);
      other = eastl::move(Unexpected<E>{eastl::move(tmp)});
    }
  }
  friend constexpr void swap(Expected &e1, Expected &e2)
    requires eastl::is_swappable_v<T> && eastl::is_swappable_v<E>
  {
    e1.swap(e2);
  }

  template <class T2, class E2>
  friend constexpr bool operator==(const Expected &x, const Expected<T2, E2> &y)
    requires detail::is_eq_comparable_v<T, T2> && detail::is_eq_comparable_v<E, E2>
  {
    return x.has_value() == y.has_value() && ((x.has_value() && (*x == *y)) || (!x.has_value() && (x.error() == y.error())));
  }
  template <class T2>
  friend constexpr bool operator==(const Expected &x, const T2 &y)
    requires detail::is_eq_comparable_v<T, T2>
  {
    return x.has_value() && *x == y;
  }
  template <class E2>
  friend constexpr bool operator==(const Expected &x, const Unexpected<E2> &y)
    requires detail::is_eq_comparable_v<E, E2>
  {
    return !x.has_value() && x.error() == y.error();
  }

private:
  alignas(detail::lcm(alignof(T), alignof(E))) unsigned char mStorage[max(sizeof(T), sizeof(E))]{};
  bool mHasValue = false;
};

template <typename T, typename E>
  requires eastl::is_void_v<T>
class [[nodiscard]] Expected<T, E>
{
public:
  using value_type = void;
  using error_type = E;
  using unexpected_type = dag::Unexpected<E>;

  constexpr Expected() : mHasValue(true) {}

  template <typename G = E>
  constexpr Expected(const Unexpected<G> &unexpected)
    requires eastl::is_constructible_v<E, const G &>
    : mHasValue(false)
  {
    detail::construct_at(reinterpret_cast<E *>(mStorage), unexpected.error());
  }
  template <typename G = E>
  constexpr Expected(Unexpected<G> &&unexpected)
    requires eastl::is_constructible_v<E, G>
    : mHasValue(false)
  {
    detail::construct_at(reinterpret_cast<E *>(mStorage), eastl::move(unexpected.error()));
  }

  template <typename U, typename G>
  constexpr Expected(const Expected<U, G> &other)
    requires eastl::is_void_v<U> && eastl::is_constructible_v<E, const G &>
    : mHasValue(other.has_value())
  {
    if (!mHasValue)
      detail::construct_at(reinterpret_cast<E *>(mStorage), other.error());
  }

  template <typename U, typename G>
  constexpr Expected(Expected<U, G> &&other)
    requires eastl::is_void_v<U> && eastl::is_constructible_v<E, G>
    : mHasValue(other.has_value())
  {
    if (!mHasValue)
      detail::construct_at(reinterpret_cast<E *>(mStorage), eastl::move(other.error()));
  }

  template <class... TArgs>
  constexpr explicit Expected(unexpect_t, TArgs &&...args)
    requires eastl::is_constructible_v<E, TArgs...>
    : mHasValue(false)
  {
    detail::construct_at(reinterpret_cast<E *>(mStorage), eastl::forward<TArgs>(args)...);
  }
  template <class U, class... TArgs>
  constexpr explicit Expected(unexpect_t, std::initializer_list<U> il, TArgs &&...args)
    requires eastl::is_constructible_v<E, std::initializer_list<U>, TArgs...>
    : mHasValue(false)
  {
    detail::construct_at(reinterpret_cast<E *>(mStorage), il, eastl::forward<TArgs>(args)...);
  }

  constexpr ~Expected()
  {
    if (!mHasValue)
      eastl::destroy_at(reinterpret_cast<E *>(mStorage));
  }

  template <typename G = E>
  constexpr Expected &operator=(const Unexpected<G> &unexpected)
    requires eastl::is_assignable_v<E, const G &> && eastl::is_constructible_v<E, const G &>
  {
    if (mHasValue)
    {
      mHasValue = false;
      detail::construct_at(reinterpret_cast<E *>(mStorage), unexpected.error());
    }
    else
    {
      *reinterpret_cast<E *>(mStorage) = unexpected.error();
    }
    return *this;
  }
  template <typename G = E>
  constexpr Expected &operator=(Unexpected<G> &&unexpected)
    requires eastl::is_assignable_v<E, G> && eastl::is_constructible_v<E, G>
  {
    if (mHasValue)
    {
      mHasValue = false;
      detail::construct_at(reinterpret_cast<E *>(mStorage), eastl::move(unexpected.error()));
    }
    else
    {
      *reinterpret_cast<E *>(mStorage) = eastl::move(unexpected.error());
    }
    return *this;
  }

  constexpr bool has_value() const { return mHasValue; }
  constexpr operator bool() const { return mHasValue; }

  constexpr E &error() & { return *reinterpret_cast<E *>(mStorage); }
  constexpr const E &error() const & { return *reinterpret_cast<const E *>(mStorage); }
  constexpr E &&error() && { return eastl::move(*reinterpret_cast<E *>(mStorage)); }
  constexpr const E &&error() const && { return eastl::move(*reinterpret_cast<const E *>(mStorage)); }

  constexpr void emplace()
  {
    if (!mHasValue)
      eastl::destroy_at(reinterpret_cast<E *>(mStorage));
    mHasValue = true;
  }

  template <class G = E>
  constexpr E error_or(G &&default_val) const &
    requires eastl::is_convertible_v<G, E>
  {
    return has_value() ? E{eastl::forward<G>(default_val)} : error();
  }
  template <class G = E>
  constexpr E error_or(G &&default_val) &&
    requires eastl::is_convertible_v<G, E>
  {
    return has_value() ? E{eastl::forward<G>(default_val)} : eastl::move(error());
  }

  template <class F>
    requires detail::is_expected_spec_v<eastl::invoke_result_t<F>> &&                                      //
             eastl::is_same_v<typename eastl::remove_cvref_t<eastl::invoke_result_t<F>>::error_type, E> && //
             eastl::is_copy_constructible_v<E>
  constexpr eastl::remove_cvref_t<eastl::invoke_result_t<F>> and_then(F &&f) &
  {
    if (has_value())
      return f();
    else
      return dag::Unexpected{error()};
  }
  template <class F>
    requires detail::is_expected_spec_v<eastl::invoke_result_t<F>> &&                                      //
             eastl::is_same_v<typename eastl::remove_cvref_t<eastl::invoke_result_t<F>>::error_type, E> && //
             eastl::is_copy_constructible_v<E>
  constexpr eastl::remove_cvref_t<eastl::invoke_result_t<F>> and_then(F &&f) const &
  {
    if (has_value())
      return f();
    else
      return dag::Unexpected{error()};
  }
  template <class F>
    requires detail::is_expected_spec_v<eastl::invoke_result_t<F>> &&                                      //
             eastl::is_same_v<typename eastl::remove_cvref_t<eastl::invoke_result_t<F>>::error_type, E> && //
             eastl::is_move_constructible_v<E>
  constexpr eastl::remove_cvref_t<eastl::invoke_result_t<F>> and_then(F &&f) &&
  {
    if (has_value())
      return f();
    else
      return dag::Unexpected{eastl::move(error())};
  }
  template <class F>
    requires detail::is_expected_spec_v<eastl::invoke_result_t<F>> &&                                      //
             eastl::is_same_v<typename eastl::remove_cvref_t<eastl::invoke_result_t<F>>::error_type, E> && //
             eastl::is_move_constructible_v<E>
  constexpr eastl::remove_cvref_t<eastl::invoke_result_t<F>> and_then(F &&f) const &&
  {
    if (has_value())
      return f();
    else
      return dag::Unexpected{eastl::move(error())};
  }

  template <class F>
    requires eastl::is_copy_constructible_v<E>
  constexpr Expected<eastl::remove_cv_t<eastl::invoke_result_t<F>>, E> transform(F &&f) &
  {
    if (has_value())
    {
      if constexpr (eastl::is_same_v<eastl::remove_cv_t<eastl::invoke_result_t<F>>, void>)
      {
        f();
        return {};
      }
      else
      {
        return f();
      }
    }
    else
    {
      return dag::Unexpected{error()};
    }
  }
  template <class F>
    requires eastl::is_copy_constructible_v<E>
  constexpr Expected<eastl::remove_cv_t<eastl::invoke_result_t<F>>, E> transform(F &&f) const &
  {
    if (has_value())
    {
      if constexpr (eastl::is_same_v<eastl::remove_cv_t<eastl::invoke_result_t<F>>, void>)
      {
        f();
        return {};
      }
      else
      {
        return f();
      }
    }
    else
    {
      return dag::Unexpected{error()};
    }
  }
  template <class F>
    requires eastl::is_move_constructible_v<E>
  constexpr Expected<eastl::remove_cv_t<eastl::invoke_result_t<F>>, E> transform(F &&f) &&
  {
    if (has_value())
    {
      if constexpr (eastl::is_same_v<eastl::remove_cv_t<eastl::invoke_result_t<F>>, void>)
      {
        f();
        return {};
      }
      else
      {
        return f();
      }
    }
    else
    {
      return dag::Unexpected{eastl::move(error())};
    }
  }
  template <class F>
    requires eastl::is_move_constructible_v<E>
  constexpr Expected<eastl::remove_cv_t<eastl::invoke_result_t<F>>, E> transform(F &&f) const &&
  {
    if (has_value())
    {
      if constexpr (eastl::is_same_v<eastl::remove_cv_t<eastl::invoke_result_t<F>>, void>)
      {
        f();
        return {};
      }
      else
      {
        return f();
      }
    }
    else
    {
      return dag::Unexpected{eastl::move(error())};
    }
  }

  template <class F>
    requires detail::is_expected_spec_v<eastl::invoke_result_t<F, E &>> && //
             eastl::is_void_v<typename eastl::remove_cvref_t<eastl::invoke_result_t<F, E &>>::value_type>
  constexpr eastl::remove_cvref_t<eastl::invoke_result_t<F, E &>> or_else(F &&f) &
  {
    if (has_value())
      return {};
    else
      return f(*reinterpret_cast<E *>(mStorage));
  }
  template <class F>
    requires detail::is_expected_spec_v<eastl::invoke_result_t<F, const E &>> && //
             eastl::is_void_v<typename eastl::remove_cvref_t<eastl::invoke_result_t<F, const E &>>::value_type>
  constexpr eastl::remove_cvref_t<eastl::invoke_result_t<F, const E &>> or_else(F &&f) const &
  {
    if (has_value())
      return {};
    else
      return f(*reinterpret_cast<const E *>(mStorage));
  }
  template <class F>
    requires detail::is_expected_spec_v<eastl::invoke_result_t<F, E &&>> && //
             eastl::is_void_v<typename eastl::remove_cvref_t<eastl::invoke_result_t<F, E &&>>::value_type>
  constexpr eastl::remove_cvref_t<eastl::invoke_result_t<F, E &&>> or_else(F &&f) &&
  {
    if (has_value())
      return {};
    else
      return f(eastl::move(*reinterpret_cast<E *>(mStorage)));
  }
  template <class F>
    requires detail::is_expected_spec_v<eastl::invoke_result_t<F, const E &&>> && //
             eastl::is_void_v<typename eastl::remove_cvref_t<eastl::invoke_result_t<F, const E &&>>::value_type>
  constexpr eastl::remove_cvref_t<eastl::invoke_result_t<F, const E &&>> or_else(F &&f) const &&
  {
    if (has_value())
      return {};
    else
      return f(eastl::move(*reinterpret_cast<const E *>(mStorage)));
  }

  template <class F>
    requires eastl::is_copy_constructible_v<E>
  constexpr Expected<void, eastl::remove_cv_t<eastl::invoke_result_t<F, E &>>> transform_error(F &&f) &
  {
    if (has_value())
      return {};
    else
      return dag::Unexpected{f(*reinterpret_cast<E *>(mStorage))};
  }
  template <class F>
    requires eastl::is_copy_constructible_v<E>
  constexpr Expected<void, eastl::remove_cv_t<eastl::invoke_result_t<F, const E &>>> transform_error(F &&f) &&
  {
    if (has_value())
      return {};
    else
      return dag::Unexpected{f(*reinterpret_cast<const E *>(mStorage))};
  }
  template <class F>
    requires eastl::is_move_constructible_v<E>
  constexpr Expected<void, eastl::remove_cv_t<eastl::invoke_result_t<F, E &&>>> transform_error(F &&f) const &
  {
    if (has_value())
      return {};
    else
      return dag::Unexpected{f(eastl::move(*reinterpret_cast<E *>(mStorage)))};
  }
  template <class F>
    requires eastl::is_copy_constructible_v<E>
  constexpr Expected<void, eastl::remove_cv_t<eastl::invoke_result_t<F, const E &&>>> transform_error(F &&f) const &&
  {
    if (has_value())
      return {};
    else
      return dag::Unexpected{f(eastl::move(*reinterpret_cast<const E *>(mStorage)))};
  }

  constexpr void swap(Expected &other)
    requires eastl::is_swappable_v<E>
  {
    if (mHasValue && other.has_value())
    {
      return;
    }
    else if (!mHasValue && !other.has_value())
    {
      eastl::swap(error(), other.error());
    }
    else if (mHasValue)
    {
      *this = eastl::move(other);
      other = {};
    }
    else
    {
      other = eastl::move(error());
      *this = {};
    }
  }
  friend constexpr void swap(Expected &e1, Expected &e2)
    requires eastl::is_swappable_v<E>
  {
    e1.swap(e2);
  }

  template <class E2>
  friend constexpr bool operator==(const Expected &x, const Expected<void, E2> &y)
    requires detail::is_eq_comparable_v<E, E2>
  {
    return x.has_value() == y.has_value() && (x.has_value() || (x.error() == y.error()));
  }
  template <class E2>
  friend constexpr bool operator==(const Expected &x, const Unexpected<E2> &y)
    requires detail::is_eq_comparable_v<E, E2>
  {
    return !x.has_value() && x.error() == y.error();
  }

private:
  alignas(E) unsigned char mStorage[sizeof(E)]{};
  bool mHasValue = false;
};

namespace detail
{

template <class T, class E>
struct IsExpectedSpecImpl<Expected<T, E>> : eastl::true_type
{};

} // namespace detail

} // namespace dag
