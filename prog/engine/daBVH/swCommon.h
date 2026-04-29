// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daBVH/dag_bvhBuild.h>

namespace build_bvh
{

__forceinline void v_write_float_to_half_up(uint16_t *__restrict m, vec4f v) { v_stui_half(m, v_float_to_half_up_lo(v)); }
__forceinline void v_write_float_to_half_down(uint16_t *__restrict m, vec4f v) { v_stui_half(m, v_float_to_half_down_lo(v)); }
__forceinline void v_write_float_to_half_round(uint16_t *__restrict m, vec4f v) { v_stui_half(m, v_float_to_half_rtne_lo(v)); }


inline void write_pair_halves(uint32_t *dest, vec4i v0, vec4i v1)
{
  vec4i tm = v_ori(v_andi(v0, v_splatsi(0xffff)), v_sll(v1, 16));
  alignas(16) uint32_t tmi[4];
  v_sti(tmi, tm);
  memcpy(dest, tmi, sizeof(uint32_t) * 3); // -V1086
}

} // namespace build_bvh