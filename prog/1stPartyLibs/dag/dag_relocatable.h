//
// Dagor Engine 6.5 - 1st party libs
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/type_traits.h>
#include <EASTL/utility.h>

namespace eastl
{
template <typename T>
struct default_delete;
template <typename T, typename D>
class unique_ptr;
} // namespace eastl

namespace dag
{

template <typename T, typename = void>
struct is_type_relocatable : public eastl::false_type
{};

template <typename T>
struct is_type_relocatable<T, typename eastl::enable_if_t<eastl::is_trivially_copyable_v<T>>> : public eastl::true_type
{};

template <typename T1, typename T2>
struct is_type_relocatable<eastl::pair<T1, T2>, typename eastl::disable_if_t<eastl::is_trivially_copyable_v<eastl::pair<T1, T2>>>>
{
  static constexpr bool value =
    eastl::is_trivially_copyable_v<eastl::pair<T1, T2>> || (is_type_relocatable<T1>::value && is_type_relocatable<T2>::value);
};

template <typename T>
struct is_type_relocatable<eastl::unique_ptr<T, eastl::default_delete<T>>, void> : public eastl::true_type
{};

template <typename T>
struct is_type_init_constructing : public eastl::true_type
{};

} // namespace dag

#define DAG_DECLARE_RELOCATABLE(C)                             \
  template <>                                                  \
  struct dag::is_type_relocatable<C> : public eastl::true_type \
  {}
