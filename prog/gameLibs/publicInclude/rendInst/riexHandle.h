//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>

namespace rendinst
{
// 16.32u for ri_idx/instance
typedef uint64_t riex_handle_t;
inline constexpr riex_handle_t RIEX_HANDLE_NULL = ~riex_handle_t(0);
inline constexpr uint32_t ri_instance_type_shift = 32, ri_instance_inst_mask = ~0u;

constexpr inline uint32_t riex_max_type() { return (1u << (48 - ri_instance_type_shift)) - 1; }
constexpr inline uint32_t riex_max_inst() { return ri_instance_inst_mask; }

inline uint32_t handle_to_ri_type(riex_handle_t h) { return h >> ri_instance_type_shift; }
inline uint32_t handle_to_ri_inst(riex_handle_t h) { return h & ri_instance_inst_mask; }
inline riex_handle_t make_handle(uint32_t ri_type, uint32_t ri_inst)
{
  return (uint64_t(ri_type) << ri_instance_type_shift) | ri_inst;
}
} // namespace rendinst
