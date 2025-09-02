//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <shaders/dag_bindumpReloadListener.h>
#include <shaders/dag_shaderVarType.h>

#include <3d/dag_texMgr.h>
#include <stdint.h>
#include <math/dag_check_nan.h>
#include <vecmath/dag_vecMathDecl.h>

class IPoint4;
class TMatrix4;
struct Color4;
class Point2;
class Point3;
class Point4;

struct RaytraceTopAccelerationStructure;

struct ShaderVariableInfo : public IShaderBindumpReloadListener
{
#define CHECK_SET_VAR_TYPE(tp)    \
  if (varType != tp)              \
  {                               \
    type_error(tp, __FUNCTION__); \
    return false;                 \
  }
  bool set_int(int v) const
  {
    CHECK_SET_VAR_TYPE(SHVT_INT);
    *(int *)data = v;
    if (iid >= 0)
      set_int_var_interval(v);
    return true;
  }
  bool set_real(float v) const
  {
    CHECK_SET_VAR_TYPE(SHVT_REAL);
#if _TARGET_PC
    if (check_nan(v))
    {
      nanError();
      return false;
    }
#endif
    *(float *)data = v;
    if (iid >= 0)
      set_float_var_interval(v);
    return true;
  }
  bool set_color4(const Color4 &v) const
  {
    CHECK_SET_VAR_TYPE(SHVT_COLOR4);
#if _TARGET_PC
    if (check_nan(((float *)&v)[0] + ((float *)&v)[1] + ((float *)&v)[2] + ((float *)&v)[3])) // without including Color4
    {
      nanError();
      return false;
    }
#endif
    memcpy(data, &v, 16); // sizeof(Color4) without including Color4
    return true;
  }
  bool set_color4(float r, float g, float b, float a) const
  {
    CHECK_SET_VAR_TYPE(SHVT_COLOR4);
#if _TARGET_PC
    if (check_nan(r + g + b + a))
    {
      nanError();
      return false;
    }
#endif
    ((float *)data)[0] = r;
    ((float *)data)[1] = g;
    ((float *)data)[2] = b;
    ((float *)data)[3] = a;
    return true;
  }
  bool set_color4_array(const Color4 *v, int count) const
  {
    CHECK_SET_VAR_TYPE(SHVT_COLOR4);
    memcpy(data, v, 16 * uint32_t(count)); // sizeof(Color4) without including Color4
    return true;
  }
  bool set_int4(const IPoint4 &v) const
  {
    CHECK_SET_VAR_TYPE(SHVT_INT4);
    memcpy(data, &v, 16); // sizeof(IPoint4) without including IPoint4
    return true;
  }
  bool set_int4(int x, int y, int z, int w) const
  {
    CHECK_SET_VAR_TYPE(SHVT_INT4);
    ((int *)data)[0] = x;
    ((int *)data)[1] = y;
    ((int *)data)[2] = z;
    ((int *)data)[3] = w;
    return true;
  }
  bool set_int4_array(const IPoint4 *v, int count) const
  {
    CHECK_SET_VAR_TYPE(SHVT_INT4);
    memcpy(data, v, 16 * uint32_t(count)); // sizeof(IPoint4) without including IPoint4
    return true;
  }
  bool set_texture(TEXTUREID v) const
  {
    if (varType != SHVT_TEXTURE)
      return false;
    set_texture_ool(v);
    return true;
  }
  bool set_buffer(D3DRESID v) const
  {
    CHECK_SET_VAR_TYPE(SHVT_BUFFER);
    set_buffer_ool(v);
    return true;
  }
  bool set_tlas(RaytraceTopAccelerationStructure *v) const
  {
    CHECK_SET_VAR_TYPE(SHVT_TLAS);
    (RaytraceTopAccelerationStructure *&)*data = v;
    return true;
  }
  bool set_sampler(d3d::SamplerHandle v) const
  {
    CHECK_SET_VAR_TYPE(SHVT_SAMPLER);
    (d3d::SamplerHandle &)*data = v;
    return true;
  }
  bool set_float4x4(const TMatrix4 &v) const
  {
    CHECK_SET_VAR_TYPE(SHVT_FLOAT4X4);
    memcpy(data, &v, sizeof(float) * 16); // sizeof(TMatrix4) without including
    return true;
  }
#undef CHECK_SET_VAR_TYPE

#define CHECK_GET_VAR_TYPE(tp, ret) \
  if (varType != tp)                \
  {                                 \
    type_error(tp, __FUNCTION__);   \
    return ret;                     \
  }
  int get_int() const
  {
    CHECK_GET_VAR_TYPE(SHVT_INT, 0);
    return *(const int *)data;
  }
  float get_real() const
  {
    CHECK_GET_VAR_TYPE(SHVT_REAL, 0);
    return *(const float *)data;
  }
  const Color4 &get_color4() const
  {
    CHECK_GET_VAR_TYPE(SHVT_COLOR4, *(const Color4 *)zero);
    return *(const Color4 *)data;
  }
  const IPoint4 &get_int4() const
  {
    CHECK_GET_VAR_TYPE(SHVT_INT4, *(const IPoint4 *)zero);
    return *(const IPoint4 *)data;
  }
  TEXTUREID get_texture() const
  {
    CHECK_GET_VAR_TYPE(SHVT_TEXTURE, BAD_TEXTUREID);
    return *(const TEXTUREID *)data; // offset verified in cpp
  }
  D3DRESID get_buffer() const
  {
    CHECK_GET_VAR_TYPE(SHVT_BUFFER, BAD_D3DRESID);
    return *(const D3DRESID *)data; // offset verified in cpp
  }
  RaytraceTopAccelerationStructure *get_tlas() const
  {
    CHECK_GET_VAR_TYPE(SHVT_TLAS, nullptr);
    return *(RaytraceTopAccelerationStructure *const *)data;
  }
  const TMatrix4 &get_float4x4() const
  {
    CHECK_GET_VAR_TYPE(SHVT_FLOAT4X4, *(const TMatrix4 *)zero);
    return *(const TMatrix4 *)data;
  }
#undef CHECK_GET_VAR_TYPE
  void type_error(int required, const char *fun) const
  {
    (void)required;
    (void)fun;
    if (varType != -1)
    {
      G_ASSERTF(0, "invalid var_id %d type varType = %d, required %d, not %s", var_id, varType, required, fun);
    }
  }


  int get_type() const { return varType; }
  int get_var_id() const { return var_id; }
  explicit operator int() const { return get_var_id(); }
  explicit operator bool() const { return data != nullptr; }
  bool operator==(int i) const { return var_id == i; }
  bool operator!=(int i) const { return var_id != i; }
  bool operator>=(int i) const { return var_id >= i; }
  bool operator<(int i) const { return var_id < i; }
  bool operator<=(int i) const { return var_id <= i; }
  ShaderVariableInfo(int v_id = -1) : var_id(v_id) { resolve(); }
  explicit ShaderVariableInfo(const char *name, bool optional_ = false, const char *tag_ = nullptr) :
    IShaderBindumpReloadListener(tag_), var_id(-1), optional(optional_), data(const_cast<char *>(name))
  {
    resolve();
  }
  ShaderVariableInfo &operator=(int var_id);
  ShaderVariableInfo &operator=(const ShaderVariableInfo &) = delete;
  ShaderVariableInfo(const ShaderVariableInfo &) = delete;
  const char *getName() const;

protected:
  void set_texture_ool(TEXTUREID texture_id) const;
  void set_buffer_ool(D3DRESID buf_id) const;
  void nanError() const;
  void set_int_var_interval(int v) const;
  void set_float_var_interval(float v) const;

  void resolve() override;
  void setEnabled(bool enabled_) override;

  int var_id = -1;
  int16_t iid = -1;
  int8_t varType = -1;
  bool optional = true;
  bool enabled = true;
  char *data = nullptr;
  static char zero[64];
  friend struct ScriptedShadersBinDumpOwner;
};

namespace ShaderGlobal
{
inline int get_var_type(const ShaderVariableInfo &v) { return v.get_type(); }

inline int get_int(const ShaderVariableInfo &v) { return v.get_int(); }
inline float get_real(const ShaderVariableInfo &v) { return v.get_real(); }
inline TEXTUREID get_texture(const ShaderVariableInfo &v) { return v.get_texture(); }
inline D3DRESID get_buffer(const ShaderVariableInfo &v) { return v.get_buffer(); }
inline const Color4 &get_color4(const ShaderVariableInfo &v) { return v.get_color4(); }
inline const IPoint4 &get_int4(const ShaderVariableInfo &v) { return v.get_int4(); }
inline const TMatrix4 &get_float4x4(const ShaderVariableInfo &v) { return v.get_float4x4(); }

inline bool set_int(const ShaderVariableInfo &v, int val) { return v.set_int(val); }
inline bool set_real(const ShaderVariableInfo &v, float val) { return v.set_real(val); }
inline bool set_color4(const ShaderVariableInfo &v, const Color4 &val) { return v.set_color4(val); }
inline bool set_color4_array(const ShaderVariableInfo &v, const Color4 *val, int count) { return v.set_color4_array(val, count); }
inline bool set_color4_array(const ShaderVariableInfo &v, const Point4 *val, int count)
{
  return v.set_color4_array((const Color4 *)val, count); // -V1027
}
inline bool set_int4(const ShaderVariableInfo &v, const IPoint4 &val) { return v.set_int4(val); }
inline bool set_int4(const ShaderVariableInfo &v, const vec4i &val) { return v.set_int4_array((const IPoint4 *)&val, 1); } // -V1027
inline bool set_int4_array(const ShaderVariableInfo &v, const IPoint4 *val, int count) { return v.set_int4_array(val, count); }
inline bool set_texture(const ShaderVariableInfo &v, TEXTUREID val) { return v.set_texture(val); }
inline bool set_buffer(const ShaderVariableInfo &v, D3DRESID val) { return v.set_buffer(val); }
inline bool set_sampler(const ShaderVariableInfo &v, d3d::SamplerHandle val) { return v.set_sampler(val); }
inline bool set_float4x4(const ShaderVariableInfo &v, const TMatrix4 &val) { return v.set_float4x4(val); }
inline bool set_color4(const ShaderVariableInfo &v, float r, float g, float b = 0, float a = 0) { return v.set_color4(r, g, b, a); }
inline bool set_color4(const ShaderVariableInfo &v, const Point4 &c) { return v.set_color4((const Color4 &)c); }          // -V1027
inline bool set_color4(const ShaderVariableInfo &v, const vec4f &c) { return v.set_color4_array((const Color4 *)&c, 1); } // -V1027
inline bool set_color4(const ShaderVariableInfo &v, const Point3 &rgb, float a = 0)
{
  return v.set_color4(((float *)&rgb)[0], ((float *)&rgb)[1], ((float *)&rgb)[2], a);
}
inline bool set_color4(const ShaderVariableInfo &v, const Point2 &rg, float b = 0, float a = 0)
{
  return v.set_color4(((float *)&rg)[0], ((float *)&rg)[1], b, a);
}
inline bool set_int4(const ShaderVariableInfo &v, int x, int y, int z, int w) { return v.set_int4(x, y, z, w); }
} // namespace ShaderGlobal
