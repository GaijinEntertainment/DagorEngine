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

__forceinline void set_tex(int stage, uint reg, unsigned id) { internalCbTable.setTex(stage, reg, id); }
__forceinline void set_buf(int stage, uint reg, unsigned id) { internalCbTable.setBuf(stage, reg, id); }
__forceinline void set_const_buf(int stage, uint reg, unsigned id) { internalCbTable.setConstBuf(stage, reg, id); }
__forceinline void set_sampler(int stage, uint reg, void *handle_ptr) { internalCbTable.setSampler(stage, reg, handle_ptr); }
__forceinline void set_rwtex(int stage, uint reg, unsigned id) { internalCbTable.setRwtex(stage, reg, id); }
__forceinline void set_rwbuf(int stage, uint reg, unsigned id) { internalCbTable.setRwbuf(stage, reg, id); }
__forceinline void set_tlas(int stage, uint reg, void *ptr) { internalCbTable.setTlas(stage, reg, ptr); }
__forceinline void get_globtm(float4x4 *out) { internalCbTable.getGlobTm(out); }
__forceinline void get_projtm(float4x4 *out) { internalCbTable.getProjTm(out); }
__forceinline void get_viewprojtm(float4x4 *out) { internalCbTable.getViewProjTm(out); }
__forceinline void get_lview(int component, float4 *out) { internalCbTable.getLocalViewComponent(component, out); }
__forceinline void get_lworld(int component, float4 *out) { internalCbTable.getLocalWorldComponent(component, out); }
__forceinline real get_shader_global_time_phase(float period, float offset)
{
  return internalCbTable.getShaderGlobalTimePhase(period, offset);
}
__forceinline float4 get_tex_dim(unsigned int id, int mip) { return internalCbTable.getTexDim(id, mip); }
__forceinline int get_buf_size(unsigned int id) { return internalCbTable.getBufSize(id); }
__forceinline float4 get_viewport() { return internalCbTable.getViewport(); }
__forceinline int exists_tex(unsigned int id) { return internalCbTable.texExists(id); }
__forceinline int exists_buf(unsigned int id) { return internalCbTable.bufExists(id); }
__forceinline uint64_t request_sampler(int smp_id, float4 border_color, int anisotropic_max, int mipmap_bias)
{
  return internalCbTable.requestSampler(smp_id, border_color, anisotropic_max, mipmap_bias);
}
__forceinline void set_const(int stage, unsigned int id, float4 *val, int cnt) { internalCbTable.setConst(stage, id, val, cnt); }

__forceinline void *get_shadervar_ptr_from_dump(int var_id) { return internalCbTable.getShadervarPtrFromDump(var_id); }

} // namespace stcode::cpp
