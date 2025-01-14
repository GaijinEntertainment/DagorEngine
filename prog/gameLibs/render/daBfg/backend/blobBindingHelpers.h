// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_color.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_dxmath_forward.h>
#include <vecmath/dag_vecMathDecl.h>

namespace dabfg
{

bool set_color3_helper(int var, const Color3 &val);
bool set_point2_helper(int var, const Point2 &val);
bool set_point3_helper(int var, const Point3 &val);
bool set_vec4f_helper(int var, vec4f val);
bool set_bool_helper(int var, bool val);

Color3 get_color3_helper(int var);
Point2 get_point2_helper(int var);
Point3 get_point3_helper(int var);
Point4 get_point4_helper(int var);
bool get_bool_helper(int var);
E3DCOLOR get_e3dcolor_helper(int var);
DirectX::XMFLOAT4 get_xmfloat4_helper(int var);
DirectX::XMFLOAT4X4 get_xmfloat4x4_helper(int var);
DirectX::XMMATRIX get_xmmatrix_helper(int var);

template <typename T1, typename T2>
struct ShaderVarBindingValidationHelper
{
  static bool validate(const T1 &a, const T2 &b);
};

template <typename T>
struct ShaderVarBindingValidationHelper<T, T>
{
  static bool validate(const T &a, const T &b) { return memcmp(&a, &b, sizeof(T)) == 0; }
};


template <>
struct ShaderVarBindingValidationHelper<float, float>
{
  static bool validate(const float &a, const float &b);
};

template <>
struct ShaderVarBindingValidationHelper<XMVECTOR, DirectX::XMFLOAT4>
{
  static bool validate(const XMVECTOR &a, const DirectX::XMFLOAT4 &b);
};

#if defined(_XM_NO_INTRINSICS_)
template <>
struct ShaderVarBindingValidationHelper<vec4f, DirectX::XMFLOAT4>
{
  static bool validate(const vec4f &a, const DirectX::XMFLOAT4 &b);
};
#endif

template <>
struct ShaderVarBindingValidationHelper<Point4, Point4>
{
  static bool validate(const Point4 &a, const Point4 &b);
};

template <>
struct ShaderVarBindingValidationHelper<Point3, Point3>
{
  static bool validate(const Point3 &a, const Point3 &b);
};

template <>
struct ShaderVarBindingValidationHelper<Point2, Point2>
{
  static bool validate(const Point2 &a, const Point2 &b);
};

template <>
struct ShaderVarBindingValidationHelper<Color4, Color4>
{
  static bool validate(const Color4 &a, const Color4 &b);
};

template <>
struct ShaderVarBindingValidationHelper<Color3, Color3>
{
  static bool validate(const Color3 &a, const Color3 &b);
};

template <>
struct ShaderVarBindingValidationHelper<TMatrix4, TMatrix4>
{
  static bool validate(const TMatrix4 &a, const TMatrix4 &b);
};

template <>
struct ShaderVarBindingValidationHelper<DirectX::XMFLOAT4X4, DirectX::XMFLOAT4X4>
{
  static bool validate(const DirectX::XMFLOAT4X4 &a, const DirectX::XMFLOAT4X4 &b);
};

template <>
struct ShaderVarBindingValidationHelper<DirectX::XMFLOAT4, DirectX::XMFLOAT4>
{
  static bool validate(const DirectX::XMFLOAT4 &a, const DirectX::XMFLOAT4 &b);
};


template <>
struct ShaderVarBindingValidationHelper<DirectX::XMMATRIX, DirectX::XMMATRIX>
{
  static bool validate(const DirectX::XMMATRIX &a, const DirectX::XMMATRIX &b);
};

#define SHV_BIND_BLOB_LIST                                                                                                           \
  SHV_CASE(SHVT_INT)                                                                                                                 \
  TAG_CASE(int, static_cast<bool (*)(int, int)>(&ShaderGlobal::set_int), static_cast<int (*)(int)>(&ShaderGlobal::get_int))          \
  TAG_CASE(bool, static_cast<bool (*)(int, bool)>(&set_bool_helper), static_cast<bool (*)(int)>(&get_bool_helper))                   \
  SHV_CASE_END(SHVT_INT)                                                                                                             \
  SHV_CASE(SHVT_REAL)                                                                                                                \
  TAG_CASE(float, static_cast<bool (*)(int, float)>(&ShaderGlobal::set_real), static_cast<float (*)(int)>(&ShaderGlobal::get_real))  \
  SHV_CASE_END(SHVT_REAL)                                                                                                            \
  SHV_CASE(SHVT_COLOR4)                                                                                                              \
  TAG_CASE(Color3, static_cast<bool (*)(int, const Color3 &)>(&set_color3_helper), static_cast<Color3 (*)(int)>(&get_color3_helper)) \
  TAG_CASE(Color4, static_cast<bool (*)(int, const Color4 &)>(&ShaderGlobal::set_color4),                                            \
    static_cast<Color4 (*)(int)>(&ShaderGlobal::get_color4))                                                                         \
  TAG_CASE(Point2, static_cast<bool (*)(int, const Point2 &)>(&set_point2_helper), static_cast<Point2 (*)(int)>(&get_point2_helper)) \
  TAG_CASE(Point3, static_cast<bool (*)(int, const Point3 &)>(&set_point3_helper), static_cast<Point3 (*)(int)>(&get_point3_helper)) \
  TAG_CASE(Point4, static_cast<bool (*)(int, const Point4 &)>(&ShaderGlobal::set_color4),                                            \
    static_cast<Point4 (*)(int)>(&get_point4_helper))                                                                                \
  TAG_CASE(E3DCOLOR, static_cast<bool (*)(int, E3DCOLOR)>(&ShaderGlobal::set_color4),                                                \
    static_cast<E3DCOLOR (*)(int)>(&get_e3dcolor_helper))                                                                            \
  TAG_CASE(XMFLOAT4, static_cast<bool (*)(int, const XMFLOAT4 &)>(&ShaderGlobal::set_color4),                                        \
    static_cast<XMFLOAT4 (*)(int)>(&get_xmfloat4_helper))                                                                            \
  TAG_CASE(XMVECTOR, static_cast<bool (*)(int, FXMVECTOR)>(&ShaderGlobal::set_color4),                                               \
    static_cast<XMFLOAT4 (*)(int)>(&get_xmfloat4_helper))                                                                            \
  TAG_CASE(vec4f, static_cast<bool (*)(int, vec4f)>(&set_vec4f_helper), static_cast<XMFLOAT4 (*)(int)>(&get_xmfloat4_helper))        \
  SHV_CASE_END(SHVT_COLOR4)                                                                                                          \
  SHV_CASE(SHVT_INT4)                                                                                                                \
  TAG_CASE(IPoint4, static_cast<bool (*)(int, const IPoint4 &)>(&ShaderGlobal::set_int4),                                            \
    static_cast<IPoint4 (*)(int)>(&ShaderGlobal::get_int4))                                                                          \
  SHV_CASE_END(SHVT_INT4)                                                                                                            \
  SHV_CASE(SHVT_FLOAT4X4)                                                                                                            \
  TAG_CASE(TMatrix4, static_cast<bool (*)(int, const TMatrix4 &)>(&ShaderGlobal::set_float4x4),                                      \
    static_cast<TMatrix4 (*)(int)>(&ShaderGlobal::get_float4x4))                                                                     \
  TAG_CASE(XMFLOAT4X4, static_cast<bool (*)(int, const XMFLOAT4X4 &)>(&ShaderGlobal::set_float4x4),                                  \
    static_cast<XMFLOAT4X4 (*)(int)>(&get_xmfloat4x4_helper))                                                                        \
  TAG_CASE(FXMMATRIX, static_cast<bool (*)(int, FXMMATRIX)>(&ShaderGlobal::set_float4x4),                                            \
    static_cast<XMMATRIX (*)(int)>(&get_xmmatrix_helper))                                                                            \
  SHV_CASE_END(SHVT_FLOAT4X4)                                                                                                        \
  SHV_CASE(SHVT_SAMPLER)                                                                                                             \
  TAG_CASE(d3d::SamplerHandle, static_cast<bool (*)(int, d3d::SamplerHandle)>(&ShaderGlobal::set_sampler),                           \
    static_cast<d3d::SamplerHandle (*)(int)>(&ShaderGlobal::get_sampler))                                                            \
  SHV_CASE_END(SHVT_SAMPLER)
} // namespace dabfg
