//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/entityManager.h>
#include <shaders/dag_shaders.h>
#include <math/dag_color.h>

/*
usage:
"my_shader_var:ShaderVar"{_tags:t="render"} // get_shader_variable_id("my_shader_var", false);

supports init component with "_value" prefix:
my_shader_var_value:c=r,g,b,a(int)   //ShaderGlobal::set_color4(varId, my_shader_var_value);
my_shader_var_value:i=int            //ShaderGlobal::set_int(varId, my_shader_var_value);
my_shader_var_value:r=float          //ShaderGlobal::set_real(varId, my_shader_var_value);
my_shader_var_value:p4=r,g,b,a(float)//ShaderGlobal::set_color4(varId, my_shader_var_value);

*/

struct ShaderVar
{
private:
  int varId;

public:
  ShaderVar() : varId(-1) {}
  ShaderVar(const char *var_name, bool optional = false) : varId(get_shader_variable_id(var_name, optional)) {}
  operator int() const { return varId; }
  int get_var_id() const { return varId; }
  // returns SHVT_INT, SHVT_REAL, SHVT_COLOR4 or SHVT_TEXTURE
  int get_var_type() const { return ShaderGlobal::get_var_type(varId); }
  bool set_int(int value) const { return ShaderGlobal::set_int(varId, value); }
  bool set_real(float value) const { return ShaderGlobal::set_real(varId, value); }
  bool set_color4(const Point2 &rg, const Point2 &ba = Point2(0, 0)) { return ShaderGlobal::set_color4(varId, rg, ba); }
  bool set_color4(const Point3 &rgb, float a = 0.f) const { return ShaderGlobal::set_color4(varId, rgb, a); }
  bool set_color4(const Point4 &value) const { return ShaderGlobal::set_color4(varId, value); }
  bool set_color4(const Color4 &value) const { return ShaderGlobal::set_color4(varId, value); }
  bool set_color4(float r, float g, float b, float a = 0.f) const { return ShaderGlobal::set_color4(varId, r, g, b, a); }
  bool set_float4x4(const TMatrix4 &mat) const { return ShaderGlobal::set_float4x4(varId, mat); }
  bool set_int4(const IPoint4 &value) const { return ShaderGlobal::set_int4(varId, value); }
  bool set_texture(TEXTUREID texture_id) const { return ShaderGlobal::set_texture(varId, texture_id); }
  bool set_texture(const ManagedTex &texture) const { return ShaderGlobal::set_texture(varId, texture); }
  bool set_buffer(D3DRESID buffer_id) const { return ShaderGlobal::set_buffer(varId, buffer_id); }
  bool set_buffer(const ManagedBuf &buffer) const { return ShaderGlobal::set_buffer(varId, buffer); }
  bool set_color4(const E3DCOLOR &value) const { return ShaderGlobal::set_color4(varId, color4(value)); }
};

ECS_DECLARE_RELOCATABLE_TYPE(ShaderVar);
