//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <dasModules/dasModulesCommon.h>
#include <dasModules/dasDagorResPtr.h>
#include <dasModules/aotDagorDriver3d.h>

#include <osApiWrappers/dag_miscApi.h>
#include <vecmath/dag_vecMathDecl.h>

#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaderBlock.h>
#include <ecs/render/shaderVar.h>
#include <3d/dag_resPtr.h>


enum class SHVT
{
  INT = SHVT_INT,
  REAL = SHVT_REAL,
  COLOR4 = SHVT_COLOR4,
  TEXTURE = SHVT_TEXTURE,
  BUFFER = SHVT_BUFFER
};
DAS_BIND_ENUM_CAST(SHVT);

MAKE_TYPE_FACTORY(ShaderVar, ShaderVar);
MAKE_TYPE_FACTORY(ShaderMaterial, ShaderMaterial);

using ShaderMaterialPtr = Ptr<ShaderMaterial>;
MAKE_TYPE_FACTORY(ShaderMaterialPtr, ShaderMaterialPtr);

namespace bind_dascript
{
inline SHVT my_get_var_type(int variable_id) { return (SHVT)ShaderGlobal::get_var_type(variable_id); }
inline bool set_color4(int v, const Color4 &col) { return ShaderGlobal::set_color4(v, col); }
inline bool set_color4_e3d(int v, E3DCOLOR col) { return ShaderGlobal::set_color4(v, col); }
inline bool set_color4_p4(int v, const Point4 &col) { return ShaderGlobal::set_color4(v, col); }
inline bool set_color4_p3(int v, const Point3 &col) { return ShaderGlobal::set_color4(v, col); }
inline bool set_color4_p3f(int v, const Point3 &col, float a) { return ShaderGlobal::set_color4(v, col, a); }
inline bool set_color4_f4(int v, float r, float g, float b, float a) { return ShaderGlobal::set_color4(v, r, g, b, a); }
inline bool set_color4_f3(int v, float r, float g, float b) { return ShaderGlobal::set_color4(v, r, g, b); }
inline bool set_color4_f2(int v, float r, float g) { return ShaderGlobal::set_color4(v, r, g); }
inline bool set_texture(int v, TEXTUREID id) { return ShaderGlobal::set_texture(v, id); }
inline bool set_texture(int v, ManagedTexView tex) { return ShaderGlobal::set_texture(v, tex); }
inline bool set_buffer(int v, D3DRESID id) { return ShaderGlobal::set_buffer(v, id); }
inline bool set_buffer(int v, ManagedBufView buf) { return ShaderGlobal::set_buffer(v, buf); }

inline void exec_with_shader_blocks_scope_reset(const das::TBlock<void> &block, das::Context *context, das::LineInfoArg *at)
{
  ScopeResetShaderBlocks resetBlocks;
  context->invoke(block, nullptr, nullptr, at);
}

inline ShaderMaterial *ShaderMaterialPtr_get(ShaderMaterialPtr &mat_ptr) { return mat_ptr.get(); }
} // namespace bind_dascript
