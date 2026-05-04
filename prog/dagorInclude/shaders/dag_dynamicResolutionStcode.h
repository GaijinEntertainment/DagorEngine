//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint2.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVar.h>
#include <drv/3d/dag_tex3d.h>

struct DynRes
{
  Point2 dynamicResolution = Point2::ONE;
  Point2 renderingResolution = Point2::ONE;

  inline Point2 CalcCurrentRate() const
  {
    return Point2(dynamicResolution.x / renderingResolution.x, dynamicResolution.y / renderingResolution.y);
  }
  inline IPoint2 CalcScaledResolution(int w, int h) const
  {
    Point2 rate = CalcCurrentRate();
    w = ceil(w * rate.x);
    h = ceil(h * rate.y);
    return IPoint2(w, h);
  }
};

inline void set_dynamic_resolution_stcode(Point2 current_dynamic_resolution, Point2 prev_dynamic_resolution,
  Point2 rendering_resolution)
{
  static int current_dynamic_resolutionVarId = get_shader_variable_id("current_dynamic_resolution");
  static int prev_dynamic_resolutionVarId = get_shader_variable_id("prev_dynamic_resolution");
  ShaderGlobal::set_float4(current_dynamic_resolutionVarId, current_dynamic_resolution, rendering_resolution);
  ShaderGlobal::set_float4(prev_dynamic_resolutionVarId, prev_dynamic_resolution, rendering_resolution);
}

inline IPoint2 calc_and_set_dynamic_resolution_stcode(int tw, int th, const DynRes &dyn_res, const DynRes &prev_dyn_res)
{
  IPoint2 sres = dyn_res.CalcScaledResolution(tw, th);
  set_dynamic_resolution_stcode(sres, prev_dyn_res.CalcScaledResolution(tw, th), Point2(tw, th));
  return sres;
}

inline IPoint2 calc_and_set_dynamic_resolution_stcode(Texture &texture, const DynRes &dyn_res, const DynRes &prev_dyn_res)
{
  TextureInfo ti;
  texture.getinfo(ti);
  return calc_and_set_dynamic_resolution_stcode(ti.w, ti.h, dyn_res, prev_dyn_res);
}
