//
// Dagor Engine 6.5 - 1st party libs
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/type_traits.h>
#include <EASTL/utility.h>

namespace dag
{

template <typename T, typename = void>
struct is_type_relocatable : public eastl::false_type
{};

template <typename T>
struct is_type_relocatable<T, typename eastl::enable_if_t<eastl::is_trivially_copyable_v<T>>> : public eastl::true_type
{};

template <typename T1, typename T2>
struct is_type_relocatable<eastl::pair<T1, T2>,
  typename eastl::enable_if_t<is_type_relocatable<T1>::value && is_type_relocatable<T2>::value>> : public eastl::true_type
{};

template <typename T>
struct is_type_init_constructing : public eastl::true_type
{};

} // namespace dag

#define DAG_DECLARE_RELOCATABLE(C)                             \
  template <>                                                  \
  struct dag::is_type_relocatable<C> : public eastl::true_type \
  {}
