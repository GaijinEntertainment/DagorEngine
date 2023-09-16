//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/type_traits.h>
#include <string.h>

template <typename To, typename From>
__forceinline To bitwise_cast(const From &from)
{
  static_assert(sizeof(To) == sizeof(From), "bitwise_cast: types of different sizes provided");
  static_assert(eastl::is_pointer<To>::value == eastl::is_pointer<From>::value,
    "bitwise_cast: can't cast between pointer and non-pointer types");
  static_assert(sizeof(From) <= 16, "bitwise_cast: can't cast types of size > 16");
  To result;
  memcpy(&result, &from, sizeof(From));
  return result;
}
