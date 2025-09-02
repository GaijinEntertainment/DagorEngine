//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/utility.h>

namespace dafg::detail
{

/**
 * \brief Determines which fields are specified in the dispatch request. Used for compile time checks.
 */
enum class DispatchRequestPolicy : uint32_t
{
  Default = 0,
  HasThreads = 1 << 1,
  HasGroups = 1 << 2,
  HasX = 1 << 3,
  HasY = 1 << 4,
  HasZ = 1 << 5,
  HasIndirect = 1 << 6,
  HasOffset = 1 << 7,
  HasMesh = 1 << 8,
  HasStride = 1 << 9,
  HasCount = 1 << 10,
  HasIndirectCount = 1 << 11,
  HasCountOffset = 1 << 12,
  HasMaxCount = 1 << 13,
};

inline constexpr DispatchRequestPolicy operator&(DispatchRequestPolicy fst, DispatchRequestPolicy snd)
{
  return static_cast<DispatchRequestPolicy>(eastl::to_underlying(fst) & eastl::to_underlying(snd));
}

inline constexpr DispatchRequestPolicy operator|(DispatchRequestPolicy fst, DispatchRequestPolicy snd)
{
  return static_cast<DispatchRequestPolicy>(eastl::to_underlying(fst) | eastl::to_underlying(snd));
}

} // namespace dafg::detail
