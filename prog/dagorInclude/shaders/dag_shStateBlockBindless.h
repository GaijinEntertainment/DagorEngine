//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <shaders/dag_shaderState.h>

void update_bindless_state(shaders::ConstStateIdx const_state_idx, int tex_level);
uint32_t get_material_offset(shaders::ConstStateIdx const_state_idx);
uint32_t get_material_id(shaders::ConstStateIdx const_state_idx)
#if defined(__GNUC__) || defined(__clang__)
  __attribute__((const))
#endif
  ;
bool is_packed_material(shaders::ConstStateIdx const_state_idx);
