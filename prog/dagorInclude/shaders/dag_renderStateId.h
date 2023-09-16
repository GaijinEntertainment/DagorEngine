//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_renderStateId.h>
#include <util/dag_generationRefId.h>

namespace shaders
{
namespace render_states
{
RenderStateId create(const RenderState &state);
void set(RenderStateId);
const RenderState &get(RenderStateId id);
uint32_t get_count();
} // namespace render_states
} // namespace shaders
