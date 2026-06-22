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
__forceinline float get_shader_global_time_phase(float period, float offset)
{
  return internalCbTable.getShaderGlobalTimePhase(period, offset);
}
__forceinline float4 get_tex_dim(const void *texptr, int mip) { return internalCbTable.getTexDim(texptr, mip); }
__forceinline int get_buf_size(const void *bufptr) { return internalCbTable.getBufSize(bufptr); }
__forceinline float4 get_viewport() { return internalCbTable.getViewport(); }
__forceinline int exists_tex(const void *texptr) { return internalCbTable.texExists(texptr); }
__forceinline int exists_buf(const void *bufptr) { return internalCbTable.bufExists(bufptr); }
__forceinline uint64_t request_sampler(int smp_id, float4 border_color, float anisotropic_max, float mipmap_bias)
{
  return internalCbTable.requestSampler(smp_id, border_color, anisotropic_max, mipmap_bias);
}
__forceinline void set_const(int stage, unsigned int id, float4 *val, int cnt) { internalCbTable.setConst(stage, id, val, cnt); }

__forceinline void get_shadervar_ptrs_from_dump(const ShadervarPtrInitInfo *out_data_infos, uint32_t ptr_count)
{
  return internalCbTable.getShadervarPtrsFromDump(out_data_infos, ptr_count);
}

__forceinline void reg_bindless(void *texptr, int reg, void *ctx) { internalCbTable.regBindless(texptr, reg, ctx); }

__forceinline void create_state(const uint *vs_tex, uint32_t vs_tex_range_packed, const uint *ps_tex, uint32_t ps_tex_range_packed,
  const void *consts, int const_cnt, bool multidraw_cbuf, void *ctx)
{
  internalCbTable.createState(vs_tex, vs_tex_range_packed, ps_tex, ps_tex_range_packed, consts, const_cnt, multidraw_cbuf, ctx);
}

__forceinline uint acquire_tex(void *texptr) { return internalCbTable.acquireTex(texptr); }

__forceinline void fatal(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  internalCbTable.fatal(fmt, args);
  va_end(args);
}

__forceinline float4 rb_get_f4(int32_t gid) { return internalCbTable.rbGetF4(gid); }
__forceinline float rb_get_real(int32_t gid) { return internalCbTable.rbGetReal(gid); }
__forceinline int32_t rb_get_int(int32_t gid) { return internalCbTable.rbGetInt(gid); }
__forceinline void rb_get_mat44(int32_t gid, float4x4 *out) { internalCbTable.rbGetMat44(gid, out); }
__forceinline void *rb_get_tex(int32_t gid) { return internalCbTable.rbGetTex(gid); }
__forceinline void *rb_get_buf(int32_t gid) { return internalCbTable.rbGetBuf(gid); }
__forceinline int4 rb_get_ivec(int32_t gid) { return internalCbTable.rbGetIvec(gid); }
__forceinline void rb_flush_tex(int32_t stage, int32_t slot, void *tex) { internalCbTable.rbFlushTex(stage, slot, tex); }
__forceinline void rb_flush_buf(int32_t stage, int32_t slot, void *buf) { internalCbTable.rbFlushBuf(stage, slot, buf); }
__forceinline uint32_t rb_alloc_bindless(void *tex) { return internalCbTable.rbAllocBindless(tex); }
__forceinline void rb_flush_bindless_tex(uint32_t bi, void *tex) { internalCbTable.rbFlushBindlessTex(bi, tex); }
__forceinline uint64_t rb_get_sampler(int32_t gid) { return internalCbTable.rbGetSampler(gid); }
__forceinline void *rb_get_tlas(int32_t gid) { return internalCbTable.rbGetTlas(gid); }
__forceinline void rb_flush_cbuf(int32_t stage, int32_t slot, void *buf) { internalCbTable.rbFlushCb(stage, slot, buf); }
__forceinline void rb_flush_sampler(int32_t stage, int32_t slot, uint64_t handle)
{
  internalCbTable.rbFlushSampler(stage, slot, handle);
}
__forceinline uint32_t rb_alloc_bindless_sampler(uint64_t handle) { return internalCbTable.rbAllocBindlessSampler(handle); }
__forceinline void rb_flush_bindless_sampler(uint32_t bi, uint64_t handle) { internalCbTable.rbFlushBindlessSampler(bi, handle); }
__forceinline void rb_flush_tlas(int32_t stage, int32_t slot, void *tlas) { internalCbTable.rbFlushTlas(stage, slot, tlas); }
__forceinline void rb_flush_rwtex(int32_t stage, int32_t slot, void *tex) { internalCbTable.rbFlushRwtex(stage, slot, tex); }
__forceinline void rb_flush_rwbuf(int32_t stage, int32_t slot, void *buf) { internalCbTable.rbFlushRwbuf(stage, slot, buf); }

} // namespace stcode::cpp
