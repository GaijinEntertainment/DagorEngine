//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_generationRefId.h>

namespace shaders
{
struct RenderState;

enum class RenderStateId : uint32_t
{
  Invalid = 0
};

class DriverRenderStateIdDummy
{};
using DriverRenderStateId = GenerationRefId<8, DriverRenderStateIdDummy>; // weak reference
} // namespace shaders
