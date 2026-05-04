//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/type_traits.h>


namespace dag
{

[[nodiscard]] constexpr bool is_constant_evaluated() noexcept
{
  // MSVC, Clang, and GCC all use the same builtin name
#if defined(__GNUC__) || defined(__clang__) || defined(_MSC_VER)
  return __builtin_is_constant_evaluated();
#else
  return true; // Force constexpr (i.e non intrin) code path
#endif
}

} // namespace dag
