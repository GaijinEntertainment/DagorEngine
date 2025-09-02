//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/utility.h>

namespace dafg::detail
{

/**
 * \brief Determines which fields are specified in the draw request. Used for compile time checks.
 */
enum class DrawRequestPolicy : uint32_t
{
  Default = 0,
  HasStartVertex = 0b0000000001,
  HasStartIndex = 0b0000000010,
  HasBaseVertex = 0b0000000100,
  HasPrimitiveCount = 0b0000001000,
  HasStartInstance = 0b0000010000,
  HasInstanceCount = 0b0000100000,
  HasIndirect = 0b0001000000,
  HasOffset = 0b0010000000,
  HasStride = 0b0100000000,
  HasCount = 0b1000000000,
};

inline constexpr DrawRequestPolicy operator&(DrawRequestPolicy fst, DrawRequestPolicy snd)
{
  return static_cast<DrawRequestPolicy>(eastl::to_underlying(fst) & eastl::to_underlying(snd));
}

inline constexpr DrawRequestPolicy operator|(DrawRequestPolicy fst, DrawRequestPolicy snd)
{
  return static_cast<DrawRequestPolicy>(eastl::to_underlying(fst) | eastl::to_underlying(snd));
}

} // namespace dafg::detail
