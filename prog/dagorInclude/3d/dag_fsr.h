//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/type_traits.h>

enum class Eye : uint32_t
{
  Left,
  Right,
  Count,
};
constexpr auto operator*(Eye eye) noexcept { return static_cast<eastl::underlying_type_t<Eye>>(eye); }

enum class FsrLutType : uint32_t
{
  Resolve,
  Distortion,
  Count,
};
constexpr auto operator*(FsrLutType lutType) noexcept { return static_cast<eastl::underlying_type_t<FsrLutType>>(lutType); }
