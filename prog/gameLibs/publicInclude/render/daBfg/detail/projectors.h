//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/functional.h>

namespace dabfg::detail
{

using TypeErasedProjector = const void *(*)(const void *);

template <class T>
constexpr bool is_const_lvalue_reference = eastl::is_lvalue_reference_v<T> && eastl::is_const_v<eastl::remove_reference_t<T>>;

template <auto projector, class T>
TypeErasedProjector erase_projector_type()
{
  return [](const void *blob) -> const void * { return &eastl::invoke(projector, *static_cast<const T *>(blob)); };
};

inline constexpr TypeErasedProjector identity_projector = +[](const void *blob) -> const void * { return blob; };

template <class F>
struct DecomposeProjector;
template <class R, class A>
struct DecomposeProjector<R (*)(A)>
{
  static_assert(is_const_lvalue_reference<A>, "A projector must take a const lvalue reference!");
  static_assert(is_const_lvalue_reference<R>, "A projector must return a const lvalue reference!");
  using Projectee = eastl::remove_cvref_t<A>;
  using Projected = eastl::remove_cvref_t<R>;
};
template <class S, class F>
struct DecomposeProjector<F(S::*)>
{
  using Projectee = eastl::remove_cvref_t<S>;
  using Projected = eastl::remove_cvref_t<F>;
};

template <auto projector>
using ProjecteeType = typename DecomposeProjector<decltype(projector)>::Projectee;
template <auto projector>
using ProjectedType = typename DecomposeProjector<decltype(projector)>::Projected;

} // namespace dabfg::detail
