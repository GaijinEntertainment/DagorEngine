//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_renderStateId.h>
#include <util/dag_generationRefId.h>

namespace shaders
{
namespace render_states
{

/**
 * \brief Create a render state.
 *
 * \param state The render state description.
 * \return A render state handle.
 */
RenderStateId create(const RenderState &state);

/**
 * \brief Set render state.
 *
 * \param id Render state handle.
 */
void set(RenderStateId);

/**
 * \brief Get render state description.
 *
 * \param id Render state handle.
 * \return A render state description.
 */
const RenderState &get(RenderStateId id);

/**
 * \brief Get the number of created unique render states.
 *
 * \return Uniqie render states count.
 */
uint32_t get_count();
} // namespace render_states
} // namespace shaders
