//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_generationRefId.h>

namespace shaders
{
struct RenderState;
class RenderStateIdDummy
{};
using RenderStateId = GenerationRefId<8, RenderStateIdDummy>; // weak reference

class DriverRenderStateIdDummy
{};
using DriverRenderStateId = GenerationRefId<8, DriverRenderStateIdDummy>; // weak reference
} // namespace shaders
