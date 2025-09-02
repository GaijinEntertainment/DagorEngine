// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#ifndef __GENERATED_STCODE_FILE
#error This file can only be included in generated stcode
#endif

#include "scalarTypes.h"

#include "callbackTable.h"

namespace stcode::cpp
{

extern CallbackTable internalCbTable;

__forceinline void set_tex(int stage, uint reg, const void *texptr) { internalCbTable.setTex(stage, reg, texptr); }
__forceinline void set_buf(int stage, uint reg, const void *bufptr) { internalCbTable.setBuf(stage, reg, bufptr); }
__forceinline void set_const_buf(int stage, uint reg, const void *bufptr) { internalCbTable.setConstBuf(stage, reg, bufptr); }
__forceinline void set_sampler(int stage, uint reg, const void *handle_ptr) { internalCbTable.setSampler(stage, reg, handle_ptr); }
__forceinline void set_rwtex(int stage, uint reg, const void *texptr) { internalCbTable.setRwtex(stage, reg, texptr); }
__forceinline void set_rwbuf(int stage, uint reg, const void *bufptr) { internalCbTable.setRwbuf(stage, reg, bufptr); }
__forceinline void set_tlas(int stage, uint reg, const void *ptr) { internalCbTable.setTlas(stage, reg, ptr); }
__forceinline void get_globtm(float4x4 *out) { internalCbTable.getGlobTm(out); }
__forceinline void get_projtm(float4x4 *out) { internalCbTable.getProjTm(out); }
__forceinline void get_viewprojtm(float4x4 *out) { internalCbTable.getViewProjTm(out); }
__forceinline void get_lview(int component, float4 *out) { internalCbTable.getLocalViewComponent(component, out); }
__forceinline void get_lworld(int component, float4 *out) { internalCbTable.getLocalWorldComponent(component, out); }
__forceinline real get_shader_global_time_phase(float period, float offset)
{
  return internalCbTable.getShaderGlobalTimePhase(period, offset);
}
__forceinline float4 get_tex_dim(const void *texptr, int mip) { return internalCbTable.getTexDim(texptr, mip); }
__forceinline int get_buf_size(const void *bufptr) { return internalCbTable.getBufSize(bufptr); }
__forceinline float4 get_viewport() { return internalCbTable.getViewport(); }
__forceinline int exists_tex(const void *texptr) { return internalCbTable.texExists(texptr); }
__forceinline int exists_buf(const void *bufptr) { return internalCbTable.bufExists(bufptr); }
__forceinline uint64_t request_sampler(int smp_id, float4 border_color, int anisotropic_max, int mipmap_bias)
{
  return internalCbTable.requestSampler(smp_id, border_color, anisotropic_max, mipmap_bias);
}
__forceinline void set_const(int stage, unsigned int id, float4 *val, int cnt) { internalCbTable.setConst(stage, id, val, cnt); }

__forceinline void get_shadervar_ptrs_from_dump(const ShadervarPtrInitInfo *out_data_infos, uint32_t ptr_count)
{
  return internalCbTable.getShadervarPtrsFromDump(out_data_infos, ptr_count);
}

__forceinline void reg_bindless(void *texptr, int reg, void *ctx) { internalCbTable.regBindless(texptr, reg, ctx); }

__forceinline void create_state(const uint *vs_tex, uint16_t vs_tex_range_packed, const uint *ps_tex, uint16_t ps_tex_range_packed,
  const void *consts, int const_cnt, bool multidraw_cbuf, void *ctx)
{
  internalCbTable.createState(vs_tex, vs_tex_range_packed, ps_tex, ps_tex_range_packed, consts, const_cnt, multidraw_cbuf, ctx);
}

__forceinline uint acquire_tex(void *texptr) { return internalCbTable.acquireTex(texptr); }

} // namespace stcode::cpp
