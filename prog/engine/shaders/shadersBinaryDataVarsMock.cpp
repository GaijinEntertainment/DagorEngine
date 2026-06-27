// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_shaderVar.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_color.h>
#include <math/dag_dxmath.h>
#include <util/dag_oaHashNameMap.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <EASTL/any.h>
#include <EASTL/string.h>
#include <EASTL/utility.h>

#include "mapBinarySearch.h"

unsigned shaderbindump::blockStateWord = 0;
bool shaderbindump::autoBlockStateWordChange = false;
char ShaderVariableInfo::zero[64]; // static variables are zero by default
shaderbindump::ShaderBlock *shaderbindump::nullBlock[shaderbindump::MAX_BLOCK_LAYERS] = {0};

namespace shader_vars_mock
{
ska::flat_hash_map<eastl::string, eastl::any> shader_vars;

static OAHashNameMap<false> shader_var_names;

int get_value_type(eastl::any *value);

static Color4 to_color4(const Point2 &rg, const Point2 &ba) { return Color4{rg.x, rg.y, ba.x, ba.y}; }
static Color4 to_color4(const Point3 &rgb, float a) { return Color4{rgb.x, rgb.y, rgb.z, a}; }
static Color4 to_color4(const Point4 &value) { return Color4{value.x, value.y, value.z, value.w}; }
static Color4 to_color4(const Color3 &rgb, float a) { return Color4{rgb.r, rgb.g, rgb.b, a}; }
static Color4 to_color4(const XMFLOAT2 &rg, const XMFLOAT2 &ba) { return Color4{rg.x, rg.y, ba.x, ba.y}; }
static Color4 to_color4(const XMFLOAT3 &rgb, float a) { return Color4{rgb.x, rgb.y, rgb.z, a}; }
static Color4 to_color4(const XMFLOAT4 &value) { return Color4{value.x, value.y, value.z, value.w}; }

static TMatrix4 to_tmatrix4(const XMFLOAT4X4 &value);

static TMatrix4 to_tmatrix4(FXMMATRIX value)
{
  XMFLOAT4X4 storedValue;
  XMStoreFloat4x4(&storedValue, value);
  return to_tmatrix4(storedValue);
}

static TMatrix4 to_tmatrix4(const XMFLOAT4X4 &value)
{
  TMatrix4 result;
  for (int row = 0; row < 4; ++row)
    for (int column = 0; column < 4; ++column)
      result.m[row][column] = value.m[row][column];
  return result;
}

static void normalize_value(eastl::any &value)
{
  if (auto *v = eastl::any_cast<bool>(&value))
    value = static_cast<int>(*v);
  else if (auto *v = eastl::any_cast<Color3>(&value))
    value = to_color4(*v, 0.f);
  else if (auto *v = eastl::any_cast<Point2>(&value))
    value = to_color4(*v, Point2::ZERO);
  else if (auto *v = eastl::any_cast<Point3>(&value))
    value = to_color4(*v, 0.f);
  else if (auto *v = eastl::any_cast<Point4>(&value))
    value = to_color4(*v);
  else if (auto *v = eastl::any_cast<E3DCOLOR>(&value))
    value = Color4{*v};
  else if (auto *v = eastl::any_cast<XMFLOAT2>(&value))
    value = to_color4(*v, XMFLOAT2{0.f, 0.f});
  else if (auto *v = eastl::any_cast<XMFLOAT3>(&value))
    value = to_color4(*v, 0.f);
  else if (auto *v = eastl::any_cast<XMFLOAT4>(&value))
    value = to_color4(*v);
  else if (auto *v = eastl::any_cast<XMFLOAT4X4>(&value))
    value = to_tmatrix4(*v);
  else if (auto *v = eastl::any_cast<XMMATRIX>(&value))
    value = to_tmatrix4(*v);
}

void reset()
{
  shader_vars.clear();
  shader_var_names.reset();
}

void set(const char *name, eastl::any value)
{
  G_ASSERT_RETURN(name, );
  normalize_value(value);
  G_ASSERTF_RETURN(get_value_type(&value) >= 0, , "Unsupported global shader variable %s", name);
  const int varId = shader_var_names.addNameId(name);
  shader_vars[shader_var_names.getName(varId)] = eastl::move(value);
}

int get_id(const char *name)
{
  G_ASSERT_RETURN(name, VariableMap::BAD_ID);

  const int varId = shader_var_names.addNameId(name);
  return varId;
}

const char *get_name(int var_id) { return shader_var_names.getName(var_id); }

eastl::any *get_value(int var_id)
{
  if (const char *name = get_name(var_id))
  {
    auto it = shader_vars.find(name);
    if (it != shader_vars.end())
      return &it->second;
  }
  return nullptr;
}

int get_value_type(eastl::any *value)
{
  if (!value)
    return -1;
  if (eastl::any_cast<int>(value))
    return SHVT_INT;
  if (eastl::any_cast<float>(value))
    return SHVT_REAL;
  if (eastl::any_cast<Color4>(value))
    return SHVT_COLOR4;
  if (eastl::any_cast<IPoint4>(value))
    return SHVT_INT4;
  if (eastl::any_cast<TMatrix4>(value))
    return SHVT_FLOAT4X4;
  if (eastl::any_cast<d3d::SamplerHandle>(value))
    return SHVT_SAMPLER;
  if (eastl::any_cast<BaseTexture *>(value))
    return SHVT_TEXTURE;
  if (eastl::any_cast<Sbuffer *>(value))
    return SHVT_BUFFER;
  return -1;
}

template <class T>
T get_or_default(int var_id, const T &default_value = T{})
{
  eastl::any *value = get_value(var_id);
  if (!value)
    return default_value;
  if (T *typedValue = eastl::any_cast<T>(value))
    return *typedValue;
  return default_value;
}

template <class T>
void set_value(int var_id, int expected_type, T value)
{
  const char *name = get_name(var_id);
  G_ASSERTF_RETURN(name, , "Invalid shader variable id %d", var_id);

  auto it = shader_vars.find(name);
  G_ASSERTF_RETURN(it != shader_vars.end(), , "Global shader variable %s not found", name);
  G_ASSERTF_RETURN(get_value_type(&it->second) == expected_type, , "Global shader variable %s has wrong type", name);
  it->second = eastl::move(value);
}

int get_var_type(int var_id) { return get_value_type(get_value(var_id)); }

bool is_present(int var_id) { return get_value(var_id) != nullptr; }

const char *get_present_name(int present_idx)
{
  for (int i = 0, presentIdx = 0; i < shader_var_names.nameCount(); ++i)
  {
    const char *name = shader_var_names.getName(i);
    if (shader_vars.find(name) == shader_vars.end())
      continue;
    if (presentIdx == present_idx)
      return name;
    ++presentIdx;
  }
  return nullptr;
}
} // namespace shader_vars_mock

#if DAGOR_DBGLEVEL > 0
bool validate_var_const_state(int) { return true; }
#endif

bool ShaderGlobal::set_texture(int variable_id, TEXTUREID)
{
  G_ASSERTF(false, "Not implemented");
  return false;
}
bool ShaderGlobal::set_texture_unsafe(int variable_id, BaseTexture *texture)
{
  shader_vars_mock::set_value(variable_id, SHVT_TEXTURE, texture);
  return true;
}
bool ShaderGlobal::set_texture(int, const ManagedTex &)
{
  G_ASSERTF(false, "Not implemented");
  return false;
}
bool ShaderGlobal::set_texture(const ShaderVariableInfo &, const ManagedTex &)
{
  G_ASSERTF(false, "Not implemented");
  return false;
}
bool ShaderGlobal::set_buffer(const ShaderVariableInfo &, const ManagedBuf &)
{
  G_ASSERTF(false, "Not implemented");
  return false;
}
bool ShaderGlobal::set_buffer(int variable_id, D3DRESID)
{
  G_ASSERTF(false, "Not implemented");
  return false;
}
bool ShaderGlobal::set_buffer_unsafe(int variable_id, Sbuffer *buffer)
{
  shader_vars_mock::set_value(variable_id, SHVT_BUFFER, buffer);
  return true;
}
bool ShaderGlobal::set_buffer(int, const ManagedBuf &)
{
  G_ASSERTF(false, "Not implemented");
  return false;
}
bool ShaderGlobal::set_tlas(int, RaytraceTopAccelerationStructure *)
{
  G_ASSERTF(false, "Not implemented");
  return false;
}
bool ShaderGlobal::set_sampler(int variable_id, d3d::SamplerHandle sampler)
{
  shader_vars_mock::set_value(variable_id, SHVT_SAMPLER, sampler);
  return true;
}

bool ShaderGlobal::set_float4(int variable_id, const Point2 &rg, const Point2 &ba)
{
  shader_vars_mock::set_value(variable_id, SHVT_COLOR4, shader_vars_mock::to_color4(rg, ba));
  return true;
}
bool ShaderGlobal::set_float4(int variable_id, const Point3 &rgb, float a)
{
  shader_vars_mock::set_value(variable_id, SHVT_COLOR4, shader_vars_mock::to_color4(rgb, a));
  return true;
}
bool ShaderGlobal::set_float4(int variable_id, const Point4 &value)
{
  shader_vars_mock::set_value(variable_id, SHVT_COLOR4, shader_vars_mock::to_color4(value));
  return true;
}
bool ShaderGlobal::set_float4(int variable_id, const Color3 &rgb, float a)
{
  shader_vars_mock::set_value(variable_id, SHVT_COLOR4, shader_vars_mock::to_color4(rgb, a));
  return true;
}
bool ShaderGlobal::set_float4(int variable_id, const Color4 &value)
{
  shader_vars_mock::set_value(variable_id, SHVT_COLOR4, value);
  return true;
}
bool ShaderGlobal::set_float4(int variable_id, E3DCOLOR value)
{
  shader_vars_mock::set_value(variable_id, SHVT_COLOR4, Color4{value});
  return true;
}
bool ShaderGlobal::set_float4(int variable_id, real r, real g, real b, real a)
{
  shader_vars_mock::set_value(variable_id, SHVT_COLOR4, Color4{r, g, b, a});
  return true;
}
bool ShaderGlobal::set_float4(int variable_id, const XMFLOAT2 &rg, const XMFLOAT2 &ba)
{
  shader_vars_mock::set_value(variable_id, SHVT_COLOR4, shader_vars_mock::to_color4(rg, ba));
  return true;
}
bool ShaderGlobal::set_float4(int variable_id, const XMFLOAT3 &rgb, float a)
{
  shader_vars_mock::set_value(variable_id, SHVT_COLOR4, shader_vars_mock::to_color4(rgb, a));
  return true;
}
bool ShaderGlobal::set_float4(int variable_id, const XMFLOAT4 &value)
{
  shader_vars_mock::set_value(variable_id, SHVT_COLOR4, shader_vars_mock::to_color4(value));
  return true;
}
bool ShaderGlobal::set_float4(int variable_id, FXMVECTOR value)
{
  XMFLOAT4 storedValue;
  XMStoreFloat4(&storedValue, value);
  shader_vars_mock::set_value(variable_id, SHVT_COLOR4, shader_vars_mock::to_color4(storedValue));
  return true;
}
bool ShaderGlobal::set_float4_array(int, const Color4 *, int)
{
  G_ASSERTF(false, "Not implemented");
  return false;
}
bool ShaderGlobal::set_float4_array(int, const Point4 *, int)
{
  G_ASSERTF(false, "Not implemented");
  return false;
}
bool ShaderGlobal::set_int(int variable_id, int value)
{
  shader_vars_mock::set_value(variable_id, SHVT_INT, value);
  return true;
}
bool ShaderGlobal::set_int4(int variable_id, const IPoint4 &value)
{
  shader_vars_mock::set_value(variable_id, SHVT_INT4, value);
  return true;
}
bool ShaderGlobal::set_int4_array(int, const IPoint4 *, int)
{
  G_ASSERTF(false, "Not implemented");
  return false;
}
bool ShaderGlobal::set_float4x4(int variable_id, const TMatrix4 &value)
{
  shader_vars_mock::set_value(variable_id, SHVT_FLOAT4X4, value);
  return true;
}
bool ShaderGlobal::set_float4x4(int variable_id, const XMFLOAT4X4 &value)
{
  shader_vars_mock::set_value(variable_id, SHVT_FLOAT4X4, shader_vars_mock::to_tmatrix4(value));
  return true;
}
bool ShaderGlobal::set_float4x4(int variable_id, FXMMATRIX value)
{
  XMFLOAT4X4 storedValue;
  XMStoreFloat4x4(&storedValue, value);
  shader_vars_mock::set_value(variable_id, SHVT_FLOAT4X4, shader_vars_mock::to_tmatrix4(storedValue));
  return true;
}
bool ShaderGlobal::set_float(int variable_id, real value)
{
  shader_vars_mock::set_value(variable_id, SHVT_REAL, value);
  return true;
}

int ShaderGlobal::get_int_fast(int variable_id) { return shader_vars_mock::get_or_default<int>(variable_id); }
real ShaderGlobal::get_float(int variable_id) { return shader_vars_mock::get_or_default<real>(variable_id); }
Color4 ShaderGlobal::get_float4(int variable_id) { return shader_vars_mock::get_or_default<Color4>(variable_id); }
TMatrix4 ShaderGlobal::get_float4x4(int variable_id) { return shader_vars_mock::get_or_default<TMatrix4>(variable_id); }
IPoint4 ShaderGlobal::get_int4(int variable_id) { return shader_vars_mock::get_or_default<IPoint4>(variable_id, IPoint4::ZERO); }
TEXTUREID ShaderGlobal::get_tex_fast(int)
{
  G_ASSERTF(false, "Not implemented");
  return BAD_TEXTUREID;
}
D3DRESID ShaderGlobal::get_buf_fast(int)
{
  G_ASSERTF(false, "Not implemented");
  return BAD_D3DRESID;
}
BaseTexture *ShaderGlobal::get_tex_ptr_fast(int variable_id) { return shader_vars_mock::get_or_default<BaseTexture *>(variable_id); }
Sbuffer *ShaderGlobal::get_buf_ptr_fast(int variable_id) { return shader_vars_mock::get_or_default<Sbuffer *>(variable_id); }
d3d::SamplerHandle ShaderGlobal::get_sampler(int variable_id)
{
  return shader_vars_mock::get_or_default<d3d::SamplerHandle>(variable_id, d3d::INVALID_SAMPLER_HANDLE);
}
int ShaderGlobal::get_var_type(int variable_id) { return shader_vars_mock::get_var_type(variable_id); }
int ShaderGlobal::get_interval_current_value(int)
{
  G_ASSERTF(false, "Not implemented");
  return -1;
}
bool ShaderGlobal::is_var_assumed(int)
{
  G_ASSERTF(false, "Not implemented");
  return false;
}
int ShaderGlobal::get_interval_assumed_value(int)
{
  G_ASSERTF(false, "Not implemented");
  return -1;
}
bool ShaderGlobal::has_associated_interval(int)
{
  G_ASSERTF(false, "Not implemented");
  return false;
}
bool ShaderGlobal::is_glob_interval_presented(Interval)
{
  G_ASSERTF(false, "Not implemented");
  return false;
}
dag::ConstSpan<float> ShaderGlobal::get_interval_ranges(int)
{
  G_ASSERTF(false, "Not implemented");
  return {};
}
dag::ConstSpan<float> ShaderGlobal::get_interval_ranges(Interval)
{
  G_ASSERTF(false, "Not implemented");
  return {};
}
dag::Vector<String> ShaderGlobal::get_subinterval_names(Interval)
{
  G_ASSERTF(false, "Not implemented");
  return {};
}
size_t ShaderGlobal::get_subinterval_count(Interval)
{
  G_ASSERTF(false, "Not implemented");
  return 0;
}
ShaderGlobal::Subinterval ShaderGlobal::get_variant(Interval)
{
  G_ASSERTF(false, "Not implemented");
  return Subinterval(nullptr, 0);
}
void ShaderGlobal::set_variant(Interval, Subinterval) { G_ASSERTF(false, "Not implemented"); }

#if DAGOR_DBGLEVEL > 0
bool ShaderGlobal::is_resource_used_as_unmanaged_pointer(D3dResource *resource, bool) { return false; }
#endif

bool VariableMap::isGlobVariablePresent(int variable_id) { return shader_vars_mock::is_present(variable_id); }
bool VariableMap::isGlobVariablePresent(const ShaderVariableInfo &)
{
  G_ASSERTF(false, "Not implemented");
  return false;
}
bool VariableMap::isVariablePresent(int variable_id) { return shader_vars_mock::is_present(variable_id); }
bool VariableMap::isVariablePresent(const ShaderVariableInfo &)
{
  G_ASSERTF(false, "Not implemented");
  return false;
}
int VariableMap::getVariableId(const char *name, bool) { return shader_vars_mock::get_id(name); }
int VariableMap::getVariableArraySize(const char *)
{
  G_ASSERTF(false, "Not implemented");
  return 0;
}
dag::Vector<String> VariableMap::getPresentedGlobalVariableNames()
{
  dag::Vector<String> result;
  result.reserve(shader_vars_mock::shader_vars.size());
  for (int i = 0; i < shader_vars_mock::shader_var_names.nameCount(); ++i)
  {
    const char *name = shader_vars_mock::shader_var_names.getName(i);
    if (shader_vars_mock::shader_vars.find(name) != shader_vars_mock::shader_vars.end())
      result.push_back(String{name});
  }
  return result;
}
int VariableMap::getVariablesCount(bool)
{
  G_ASSERTF(false, "Not implemented");
  return 0;
}
const char *VariableMap::getVariableName(int variable_id, bool) { return shader_vars_mock::get_name(variable_id); }
uint32_t VariableMap::generation() { return 0; }
void ShaderVariableInfo::nanError() const {}
int VariableMap::getGlobalVariablesCount() { return shader_vars_mock::shader_vars.size(); }
const char *VariableMap::getGlobalVariableName(int globvar_idx) { return shader_vars_mock::get_present_name(globvar_idx); }

void ShaderVariableInfo::set_texture_ool(TEXTUREID) const { G_ASSERTF(false, "Not implemented"); }
void ShaderVariableInfo::set_buffer_ool(D3DRESID) const { G_ASSERTF(false, "Not implemented"); }
void ShaderVariableInfo::set_texture_ool(BaseTexture *) const { G_ASSERTF(false, "Not implemented"); }
void ShaderVariableInfo::set_buffer_ool(Sbuffer *) const { G_ASSERTF(false, "Not implemented"); }
void ShaderVariableInfo::set_int_var_interval(int) const { G_ASSERTF(false, "Not implemented"); }
void ShaderVariableInfo::set_float_var_interval(float) const { G_ASSERTF(false, "Not implemented"); }
void ShaderVariableInfo::resolve() {}

int mapbinarysearch::bfindStrId(dag::ConstSpan<StrHolder>, const char *)
{
  G_ASSERTF(false, "Not implemented");
  return -1;
}
void ScriptedShadersGlobalData::initVarIndexMaps(size_t) { G_ASSERTF(false, "Not implemented"); }
void ScriptedShadersGlobalData::preRequestStaticSamplers(ScriptedShadersBinDumpOwner const &) { G_ASSERTF(false, "Not implemented"); }

template <>
int shaderbindump::VariantTable::qfindIntervalVariant(unsigned) const
{
  G_ASSERTF(false, "Not implemented");
  return FIND_NOTFOUND;
}
template <>
int shaderbindump::VariantTable::qfindDirectVariant(unsigned) const
{
  G_ASSERTF(false, "Not implemented");
  return FIND_NOTFOUND;
}
template <>
const shaderbindump::ShaderClass *ScriptedShadersBinDump::findShaderClass(const char *) const
{
  G_ASSERTF(false, "Not implemented");
  return nullptr;
}
template <>
int shaderbindump::VarList::findVar(int) const
{
  G_ASSERTF(false, "Not implemented");
  return -1;
}
template <>
int shaderbindump::ShaderCode::findVar(int) const
{
  G_ASSERTF(false, "Not implemented");
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
