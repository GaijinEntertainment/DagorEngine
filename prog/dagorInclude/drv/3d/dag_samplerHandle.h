//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>

namespace d3d
{
// Have always 64bits - this allows the drivers to use pointers, packed data structures of API objects to be returned
enum class SamplerHandle : uint64_t
{
  Invalid = 0x0,
};
inline const SamplerHandle INVALID_SAMPLER_HANDLE = SamplerHandle::Invalid;
} // namespace d3d
