//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/type_traits.h>

namespace dag
{

template <class _To, class _From>
  requires(sizeof(_To) == sizeof(_From)) && eastl::is_trivially_copyable_v<_To> && eastl::is_trivially_copyable_v<_From>
[[nodiscard]] constexpr _To bit_cast(const _From &_Val) noexcept
{
  return __builtin_bit_cast(_To, _Val);
}

} // namespace dag

template <typename To, typename From>
__forceinline To bitwise_cast(const From &from) // To consider: may be just make it alias to `dag::bit_cast`?
{
  static_assert(sizeof(To) == sizeof(From), "bitwise_cast: types of different sizes provided");
  static_assert(eastl::is_pointer<To>::value == eastl::is_pointer<From>::value,
    "bitwise_cast: can't cast between pointer and non-pointer types");
  static_assert(sizeof(From) <= 16, "bitwise_cast: can't cast types of size > 16");
  return dag::bit_cast<To>(from);
}
