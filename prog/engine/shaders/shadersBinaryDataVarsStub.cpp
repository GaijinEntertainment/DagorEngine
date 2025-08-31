// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_shaderVar.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_color.h>
#include <math/dag_dxmath.h>
#include "mapBinarySearch.h"

unsigned shaderbindump::blockStateWord = 0;
bool shaderbindump::autoBlockStateWordChange = false;
char ShaderVariableInfo::zero[64]; // static variables are zero by default
shaderbindump::ShaderBlock *shaderbindump::nullBlock[shaderbindump::MAX_BLOCK_LAYERS] = {0};

bool ShaderGlobal::set_texture(int, TEXTUREID) { return true; }
bool ShaderGlobal::set_texture(int, const ManagedTex &) { return true; }
bool ShaderGlobal::set_texture(const ShaderVariableInfo &, const ManagedTex &) { return true; }
bool ShaderGlobal::set_buffer(const ShaderVariableInfo &, const ManagedBuf &) { return true; }
bool ShaderGlobal::set_buffer(int, D3DRESID) { return true; }
bool ShaderGlobal::set_buffer(int, const ManagedBuf &) { return true; }
bool ShaderGlobal::set_tlas(int, RaytraceTopAccelerationStructure *) { return true; }
bool ShaderGlobal::set_sampler(int, d3d::SamplerHandle) { return true; }

bool ShaderGlobal::set_color4(int, const Point2 &, const Point2 &) { return true; }
bool ShaderGlobal::set_color4(int, const Point3 &, float) { return true; }
bool ShaderGlobal::set_color4(int, const Point4 &) { return true; }
bool ShaderGlobal::set_color4(int, const Color3 &, float) { return true; }
bool ShaderGlobal::set_color4(int, const Color4 &) { return true; }
bool ShaderGlobal::set_color4(int, E3DCOLOR) { return true; }
bool ShaderGlobal::set_color4(int, real, real, real, real) { return true; }
bool ShaderGlobal::set_color4(int, const XMFLOAT2 &, const XMFLOAT2 &) { return true; }
bool ShaderGlobal::set_color4(int, const XMFLOAT4 &) { return true; }
bool ShaderGlobal::set_color4(int, FXMVECTOR) { return true; }
bool ShaderGlobal::set_color4_array(int, const Point4 *, int) { return true; }
bool ShaderGlobal::set_int(int, int v) { return true; }
bool ShaderGlobal::set_int4(int, const IPoint4 &) { return true; }
bool ShaderGlobal::set_float4x4(int, const TMatrix4 &) { return true; }
bool ShaderGlobal::set_float4x4(int, const XMFLOAT4X4 &) { return true; }
bool ShaderGlobal::set_float4x4(int variable_id, FXMMATRIX) { return true; }
bool ShaderGlobal::set_real(int, real) { return true; }

int ShaderGlobal::get_int_fast(int) { return 0; }
real ShaderGlobal::get_real_fast(int) { return 0.f; }
Color4 ShaderGlobal::get_color4_fast(int) { return ZERO<Color4>(); }
Color4 ShaderGlobal::get_color4(int) { return ZERO<Color4>(); }
TMatrix4 ShaderGlobal::get_float4x4(int) { return ZERO<TMatrix4>(); }
IPoint4 ShaderGlobal::get_int4(int) { return IPoint4::ZERO; }
TEXTUREID ShaderGlobal::get_tex_fast(int) { return BAD_TEXTUREID; }
D3DRESID ShaderGlobal::get_buf_fast(int) { return BAD_D3DRESID; }
d3d::SamplerHandle ShaderGlobal::get_sampler(int) { return d3d::INVALID_SAMPLER_HANDLE; }
int ShaderGlobal::get_var_type(int) { return -1; }
int ShaderGlobal::get_interval_current_value(int) { return -1; }
bool ShaderGlobal::is_var_assumed(int) { return false; }
int ShaderGlobal::get_interval_assumed_value(int) { return -1; }
bool ShaderGlobal::has_associated_interval(int) { return false; }
bool ShaderGlobal::is_glob_interval_presented(Interval) { return false; }
dag::ConstSpan<float> ShaderGlobal::get_interval_ranges(int) { return {}; }
dag::Vector<String> ShaderGlobal::get_subinterval_names(Interval) { return {}; }
ShaderGlobal::Subinterval ShaderGlobal::get_variant(Interval) { return Subinterval(nullptr, 0); }
void ShaderGlobal::set_variant(Interval, Subinterval) {}

bool VariableMap::isGlobVariablePresent(int) { return false; }
bool VariableMap::isVariablePresent(int) { return false; }
bool VariableMap::isVariablePresent(const ShaderVariableInfo &) { return false; }
int VariableMap::getVariableId(const char *, bool) { return -1; }
dag::Vector<String> VariableMap::getPresentedGlobalVariableNames() { return {}; }
int VariableMap::getVariablesCount(bool) { return 0; }
const char *VariableMap::getVariableName(int, bool) { return nullptr; }
void ShaderVariableInfo::nanError() const {}
int VariableMap::getGlobalVariablesCount() { return 0; }
const char *VariableMap::getGlobalVariableName(int) { return nullptr; }

void ShaderVariableInfo::set_texture_ool(TEXTUREID) const {}
void ShaderVariableInfo::set_buffer_ool(D3DRESID) const {}
void ShaderVariableInfo::set_int_var_interval(int) const {}
void ShaderVariableInfo::set_float_var_interval(float) const {}
void ShaderVariableInfo::resolve() {}
void ShaderVariableInfo::setEnabled(bool) {}

int mapbinarysearch::bfindStrId(dag::ConstSpan<StrHolder>, const char *) { return -1; }
void ScriptedShadersBinDumpOwner::initVarIndexMaps(size_t) {}

template <>
int shaderbindump::VariantTable::qfindIntervalVariant(unsigned) const
{
  return FIND_NOTFOUND;
}
template <>
int shaderbindump::VariantTable::qfindDirectVariant(unsigned) const
{
  return FIND_NOTFOUND;
}
template <>
const shaderbindump::ShaderClass *ScriptedShadersBinDump::findShaderClass(const char *) const
{
  return nullptr;
}
template <>
int shaderbindump::VarList::findVar(int) const
{
  return -1;
}
template <>
int shaderbindump::ShaderCode::findVar(int) const
{
  return -1;
}
template <bool is_interval>
ShaderGlobal::detail::StringToHash<is_interval>::StringToHash(const char *, size_t) : hash(0)
{}

template struct ShaderGlobal::detail::StringToHash<false>;
template struct ShaderGlobal::detail::StringToHash<true>;

namespace shaderbindump
{
void dump_shader_var_names() {}
} // namespace shaderbindump