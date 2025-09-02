//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/utility.h>


namespace dafg::detail
{
// Determines which functionality is included in the request class and
// avoid logic stupid errors at compile time.
enum class ResourceRequestPolicy : uint32_t
{
  None = 0,
  Readonly = 0b00000001,
  Optional = 0b00000010,
  History = 0b00000100,
  HasUsageStage = 0b00001000,
  HasUsageType = 0b00010000,
  HasClearValue = 0b00100000,
};

inline constexpr ResourceRequestPolicy operator&(ResourceRequestPolicy fst, ResourceRequestPolicy snd)
{
  return static_cast<ResourceRequestPolicy>(eastl::to_underlying(fst) & eastl::to_underlying(snd));
}

inline constexpr ResourceRequestPolicy operator|(ResourceRequestPolicy fst, ResourceRequestPolicy snd)
{
  return static_cast<ResourceRequestPolicy>(eastl::to_underlying(fst) | eastl::to_underlying(snd));
}

} // namespace dafg::detail
