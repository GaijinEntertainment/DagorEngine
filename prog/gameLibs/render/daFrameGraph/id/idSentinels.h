// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/type_traits.h>


namespace dafg
{

template <class T, class = eastl::enable_if_t<eastl::is_enum_v<T> && eastl::is_unsigned_v<eastl::underlying_type_t<T>>>>
constexpr T MINUS_ONE_SENTINEL_FOR = static_cast<T>(static_cast<eastl::underlying_type_t<T>>(-1));

template <class T, class = eastl::enable_if_t<eastl::is_enum_v<T> && eastl::is_unsigned_v<eastl::underlying_type_t<T>>>>
constexpr T MINUS_TWO_SENTINEL_FOR = static_cast<T>(static_cast<eastl::underlying_type_t<T>>(-2));

} // namespace dafg
