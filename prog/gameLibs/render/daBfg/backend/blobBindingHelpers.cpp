// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "blobBindingHelpers.h"

#include <vecmath/dag_vecMath_common.h>
#include <shaders/dag_shaderVar.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_dxmath.h>
#include <vecmath/dag_vecMath_common.h>

bool dabfg::set_color3_helper(int var, const Color3 &val) { return ShaderGlobal::set_color4(var, val); }
bool dabfg::set_point2_helper(int var, const Point2 &val) { return ShaderGlobal::set_color4(var, val); }
bool dabfg::set_point3_helper(int var, const Point3 &val) { return ShaderGlobal::set_color4(var, val); }
bool dabfg::set_vec4f_helper(int var, vec4f val)
{
  alignas(16) Point4 p4;
  v_st(&p4.x, val);
  return ShaderGlobal::set_color4(var, p4.x, p4.y, p4.z, p4.w);
}
bool dabfg::set_bool_helper(int var, bool val) { return ShaderGlobal::set_int(var, static_cast<int>(val)); }

Color3 dabfg::get_color3_helper(int var)
{
  const auto value = ShaderGlobal::get_color4(var);
  return {value.r, value.g, value.b};
}
Point2 dabfg::get_point2_helper(int var)
{
  const auto value = ShaderGlobal::get_color4(var);
  return {value.r, value.g};
}
Point3 dabfg::get_point3_helper(int var)
{
  const auto value = ShaderGlobal::get_color4(var);
  return {value.r, value.g, value.b};
}
Point4 dabfg::get_point4_helper(int var)
{
  const auto value = ShaderGlobal::get_color4(var);
  return {value.r, value.g, value.b, value.a};
}
bool dabfg::get_bool_helper(int var)
{
  const auto value = ShaderGlobal::get_int(var);
  G_ASSERT(value == 0 || value == 1);
  return static_cast<bool>(value);
}
E3DCOLOR dabfg::get_e3dcolor_helper(int var)
{
  const auto value = ShaderGlobal::get_color4(var);
  return E3DCOLOR_MAKE(int(value.r * 255), int(value.g * 255), int(value.b * 255), int(value.a * 255));
}
XMFLOAT4 dabfg::get_xmfloat4_helper(int var)
{
  const auto value = ShaderGlobal::get_color4(var);
  return {value.r, value.g, value.b, value.a};
}
XMFLOAT4X4 dabfg::get_xmfloat4x4_helper(int var)
{
  const auto value = ShaderGlobal::get_float4x4(var);
  XMFLOAT4X4 result;
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      result.m[i][j] = value.m[i][j];
  return result;
}
XMMATRIX dabfg::get_xmmatrix_helper(int var)
{
  const auto value = ShaderGlobal::get_float4x4(var);
  XMMATRIX result;
#if defined(_XM_NO_INTRINSICS_)
  G_STATIC_ASSERT(sizeof(result) == sizeof(value));
  memcpy(result.r, value.row, sizeof(result.r));
#else
  for (int i = 0; i < 4; ++i)
    result.r[i] = v_ldu(&value.row[i].x);
#endif
  return result;
}

bool dabfg::ShaderVarBindingValidationHelper<float, float>::validate(const float &a, const float &b) { return abs(b - a) == 0; }

bool dabfg::ShaderVarBindingValidationHelper<XMVECTOR, DirectX::XMFLOAT4>::validate(const XMVECTOR &a, const DirectX::XMFLOAT4 &b)
{
#if defined(_XM_NO_INTRINSICS_)
  return memcmp(&a.vector4_f32[0], &b.x, sizeof(b)) == 0;
#else
  XMVECTOR v_b = v_make_vec4f(b.x, b.y, b.z, b.w);
  return v_signmask(v_cmp_eq(a, v_b)) == 0b1111;
#endif
}
#if defined(_XM_NO_INTRINSICS_)
bool dabfg::ShaderVarBindingValidationHelper<vec4f, DirectX::XMFLOAT4>::validate(const vec4f &a, const DirectX::XMFLOAT4 &b)
{
  vec4f v_b = v_make_vec4f(b.x, b.y, b.z, b.w);
  return v_signmask(v_cmp_eq(a, v_b)) == 0b1111;
}
#endif

bool dabfg::ShaderVarBindingValidationHelper<Point4, Point4>::validate(const Point4 &a, const Point4 &b)
{
  return lengthSq(b - a) == 0;
}

bool dabfg::ShaderVarBindingValidationHelper<Point3, Point3>::validate(const Point3 &a, const Point3 &b)
{
  return lengthSq(b - a) == 0;
}


bool dabfg::ShaderVarBindingValidationHelper<Point2, Point2>::validate(const Point2 &a, const Point2 &b)
{
  return lengthSq(b - a) == 0;
}

bool dabfg::ShaderVarBindingValidationHelper<Color4, Color4>::validate(const Color4 &a, const Color4 &b)
{
  return lengthSq(b - a) == 0;
}

bool dabfg::ShaderVarBindingValidationHelper<Color3, Color3>::validate(const Color3 &a, const Color3 &b)
{
  return lengthSq(b - a) == 0;
}

bool dabfg::ShaderVarBindingValidationHelper<TMatrix4, TMatrix4>::validate(const TMatrix4 &a, const TMatrix4 &b)
{
  for (int i = 0; i < 4; ++i)
    if (!ShaderVarBindingValidationHelper<Point4, Point4>::validate(a.row[i], b.row[i]))
      return false;
  return true;
}

bool dabfg::ShaderVarBindingValidationHelper<DirectX::XMFLOAT4X4, DirectX::XMFLOAT4X4>::validate(const DirectX::XMFLOAT4X4 &a,
  const DirectX::XMFLOAT4X4 &b)
{
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      if (!ShaderVarBindingValidationHelper<float, float>::validate(a(i, j), b(i, j)))
        return false;
  return true;
}

bool dabfg::ShaderVarBindingValidationHelper<DirectX::XMFLOAT4, DirectX::XMFLOAT4>::validate(const DirectX::XMFLOAT4 &a,
  const DirectX::XMFLOAT4 &b)
{
  return ShaderVarBindingValidationHelper<Point4, Point4>::validate(Point4(a.x, a.y, a.z, a.w), Point4(b.x, b.y, b.z, b.w));
}


bool dabfg::ShaderVarBindingValidationHelper<DirectX::XMMATRIX, DirectX::XMMATRIX>::validate(const DirectX::XMMATRIX &a,
  const DirectX::XMMATRIX &b)
{
#if defined(_XM_NO_INTRINSICS_)
  return memcmp(a.r, b.r, sizeof(a.r)) == 0;
#else
  for (int i = 0; i < 4; ++i)
    if (v_signmask(v_cmp_eq(a.r[i], b.r[i])) != 0b1111)
      return false;
  return true;
#endif
}
