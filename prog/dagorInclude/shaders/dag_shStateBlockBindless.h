//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <stdint.h>


void update_bindless_state(uint32_t const_state_idx, int tex_level);
uint32_t get_material_offset(uint32_t const_state_idx);
uint32_t get_material_id(uint32_t const_state_idx);
bool is_packed_material(uint32_t const_state_idx);
