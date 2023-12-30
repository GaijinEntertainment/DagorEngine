//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_texMgr.h>
#include <3d/dag_sampler.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <math/integer/dag_IPoint4.h>
#include <math/dag_Point2.h>
#include <math/dag_dxmath_forward.h>

struct Color4;
class DataBlock;
struct RoDataBlock;
struct ShaderVariableInfo;

namespace resptr_detail
{
template <typename ResType>
class ManagedRes;
}

using ManagedTex = resptr_detail::ManagedRes<BaseTexture>;
using ManagedBuf = resptr_detail::ManagedRes<Sbuffer>;

namespace VariableMap
{
static constexpr int BAD_ID = -1;

// get variable name by ID - return NULL when var_id is out of range
const char *getVariableName(int var_id, bool sec_dump_for_exp = false);

//! get varID by for name; returns BAD_ID if fail (but generally will always return valid ID, so isVariablePresent() should also be
//! used)
int getVariableId(const char *variable_name, bool sec_dump_for_exp = false);

// returns the size of an array for a variable name
int getVariableArraySize(const char *name);

// returns total number of variable identifiers (var_id will be valid from 0 to count-1)
int getVariablesCount(bool sec_dump_for_exp = false);

// returns total number of global variables
int getGlobalVariablesCount();

// returns name of global variable specified by ordinal index
const char *getGlobalVariableName(int globvar_idx);

//! returns true when variable for specified var_id is present in shaders dump
bool isVariablePresent(int var_id);
//! returns true when variable for specified var_id is present in shaders dump
bool isVariablePresent(const ShaderVariableInfo &var_id);
//! returns true when global variable for specified var_id is present in shaders dump
bool isGlobVariablePresent(int var_id);
//! returns true when global variable for specified var_id is present in shaders dump
bool isGlobVariablePresent(const ShaderVariableInfo &var_id);

// Return list of all global shader variables names which are presented in current shaders dump.
// Consider that some shadervars are present in dump but never used by ShaderGlobal:: or VariableMap::
// public API. To iterate over all used variables you can use raw IDs from 0 to getGlobalVariablesCount()
dag::Vector<String> getPresentedGlobalVariableNames();

//! returns generation (i.e. is shader dump was reloaded)
uint32_t generation();
}; // namespace VariableMap


namespace ShaderGlobal
{
namespace detail
{
template <bool is_interval>
struct StringToHash
{
  uint32_t hash;
  explicit constexpr StringToHash(uint32_t h) : hash(h) {}
  StringToHash(const char *name, size_t len);

  bool operator==(const StringToHash &rhs) const { return hash == rhs.hash; }
  bool operator!=(const StringToHash &rhs) const { return hash != rhs.hash; }
};
} // namespace detail

using Interval = detail::StringToHash<true>;
using Subinterval = detail::StringToHash<false>;
using Variant = eastl::pair<Interval, Subinterval>;

template <uint32_t h>
static constexpr Interval interval = Interval(h);

template <uint32_t h>
static constexpr Subinterval subinterval = Subinterval(h);

template <uint32_t interv_h, uint32_t subinterv_h>
static constexpr Variant variant = Variant(interval<interv_h>, subinterval<subinterv_h>);

void set_variant(Interval interv, Subinterval subinterv);
inline void set_variant(Variant var) { set_variant(var.first, var.second); }
Subinterval get_variant(Interval interv);
bool is_glob_interval_presented(Interval interv);
dag::Vector<String> get_subinterval_names(Interval interv);

// returns SHVT_INT, SHVT_REAL, SHVT_COLOR4 or SHVT_TEXTURE
int get_var_type(int variable_id);
bool is_var_assumed(int variable_id);
int get_interval_assumed_value(int variable_id);
bool has_associated_interval(int variable_id);
dag::ConstSpan<float> get_interval_ranges(int variable_id);
dag::ConstSpan<float> get_interval_ranges(Interval interv);
// set global shader variables using variable_id;
bool set_int(int variable_id, int v);
bool set_real(int variable_id, float v);
bool set_color4(int variable_id, const Point2 &rg, const Point2 &ba = Point2(0, 0));
bool set_color4(int variable_id, const Point3 &rgb, float a = 0.f);
bool set_color4(int variable_id, const Point4 &v);
bool set_color4(int variable_id, const Color4 &v);
bool set_color4(int variable_id, const Color3 &rgb, float a = 0.f);
bool set_color4(int variable_id, float r, float g, float b = 0.f, float a = 0.f);
bool set_color4_array(int variable_id, const Color4 *data, int count);
bool set_color4_array(int variable_id, const Point4 *data, int count);
bool set_color4(int variable_id, E3DCOLOR c);
bool set_color4(int variable_id, const XMFLOAT2 &rg, const XMFLOAT2 &ba);
bool set_color4(int variable_id, const XMFLOAT3 &rgb, float a = 0.f);
bool set_color4(int variable_id, const XMFLOAT4 &v);
bool set_color4(int variable_id, FXMVECTOR v);
bool set_float4x4(int variable_id, const TMatrix4 &mat);
bool set_float4x4(int variable_id, const XMFLOAT4X4 &mat);
bool set_float4x4(int variable_id, FXMMATRIX mat);
bool set_int4(int variable_id, const IPoint4 &v);
bool set_int4_array(int variable_id, const IPoint4 *data, int count);
bool set_texture(int variable_id, TEXTUREID texture_id);
bool set_texture(int variable_id, const ManagedTex &texture);
bool set_sampler(int variable_id, d3d::SamplerHandle handle);
bool set_buffer(int variable_id, D3DRESID buffer_id);
bool set_buffer(int variable_id, const ManagedBuf &buffer);
bool set_texture(const ShaderVariableInfo &, const ManagedTex &texture);
bool set_buffer(const ShaderVariableInfo &, const ManagedBuf &buffer);

// sets all texture global vars to BAD_TEXTUREID and issue release_managed_tex() on them
void reset_textures(bool removed_tex_only = false);

// resets references to texture from all global shader vars
void reset_from_vars(TEXTUREID id);
// resets references to texture from all global shader vars and than calls release_managed_tex(id) and clears id
inline void reset_from_vars_and_release_managed_tex(TEXTUREID &id)
{
  if (id == BAD_TEXTUREID)
    return;
  reset_from_vars(id);
  release_managed_tex(id);
  id = BAD_TEXTUREID;
}
template <class T>
inline void reset_from_vars_and_release_managed_tex_verified(TEXTUREID &id, T &check_tex)
{
  if (id == BAD_TEXTUREID)
    return;
  reset_from_vars(id);
  release_managed_tex_verified<T>(id, check_tex);
}

// returns global shader variables using glob_var_id; tolerant to glob_var_id=-1 (return 0);
// with DAGOR_DBGLEVEL>0 uses G_ASSERT to check that glob_var_id is in range and to check type
int get_int_fast(int glob_var_id);
float get_real_fast(int glob_var_id);
Color4 get_color4_fast(int glob_var_id);
TEXTUREID get_tex_fast(int glob_var_id);
D3DRESID get_buf_fast(int glob_var_id);

inline bool get_int_by_name(const char *name, int &val)
{
  int id = VariableMap::getVariableId(name);
#if DAGOR_DBGLEVEL > 0
  if (!VariableMap::isVariablePresent(id)) // we check only var validity here (not checking that var is global) to allow asserts on
                                           // wrong calls
#else
  if (!VariableMap::isGlobVariablePresent(id))
#endif
    return false;
  val = get_int_fast(id);
  return true;
}

inline bool get_real_by_name(const char *name, float &val)
{
  int id = VariableMap::getVariableId(name);
#if DAGOR_DBGLEVEL > 0
  if (!VariableMap::isVariablePresent(id)) // we check only var validity here (not checking that var is global) to allow asserts on
                                           // wrong calls
#else
  if (!VariableMap::isGlobVariablePresent(id))
#endif
    return false;
  val = get_real_fast(id);
  return true;
}

bool get_color4_by_name(const char *name, Color4 &val);

// reads list of global var values from DataBlock and sets them to shadr global variables
void set_vars_from_blk(const DataBlock &blk, bool loadTexures = false, bool allowAddTextures = false);
void set_vars_from_blk(const RoDataBlock &blk);

// reset variables set by the set_vars_from_blk to their initial value (before setting any block, except textures).
void reset_vars_from_blk();


// obsolete functions (compatibility)

inline int get_glob_var_id(int var_id) { return var_id; }
inline void set_int_fast(int glob_var_id, int v) { set_int(glob_var_id, v); }
inline void set_real_fast(int glob_var_id, float v) { set_real(glob_var_id, v); }
inline void set_color4_fast(int glob_var_id, const Color4 &v) { set_color4(glob_var_id, v); }
inline void set_color4_fast(int glob_var_id, float r, float g, float b, float a) { set_color4(glob_var_id, r, g, b, a); }
inline void set_texture_fast(int glob_var_id, TEXTUREID tex_id) { set_texture(glob_var_id, tex_id); }

inline int get_int(int glob_var_id) { return get_int_fast(glob_var_id); }
inline float get_real(int glob_var_id) { return get_real_fast(glob_var_id); }
Color4 get_color4(int glob_var_id);
TMatrix4 get_float4x4(int glob_var_id);
IPoint4 get_int4(int glob_var_id);
inline TEXTUREID get_tex(int glob_var_id) { return get_tex_fast(glob_var_id); }
inline D3DRESID get_buf(int glob_var_id) { return get_buf_fast(glob_var_id); }


}; // namespace ShaderGlobal
