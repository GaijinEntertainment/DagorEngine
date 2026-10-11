//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <shaders/dag_shaderState.h>

// low bits of ConstStateIdx hold the global state id, high bits the material id (packed stcode + buffer)
inline constexpr uint32_t BINDLESS_GLOBAL_STATE_BITS = 16;

void update_bindless_state(shaders::ConstStateIdx const_state_idx, int tex_level);
uint32_t get_material_offset(shaders::ConstStateIdx const_state_idx);
inline uint32_t get_material_id(shaders::ConstStateIdx const_state_idx)
{
  return static_cast<uint32_t>(const_state_idx) >> BINDLESS_GLOBAL_STATE_BITS;
}
bool is_packed_material(shaders::ConstStateIdx const_state_idx);
