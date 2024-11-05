// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>
#include <math/dag_color.h>
#include <shaders/dag_shaderVar.h>


namespace rendinst::render
{

extern int baked_impostor_multisampleVarId;
extern int vertical_impostor_slicesVarId;
extern int dynamicImpostorViewXVarId;
extern int dynamicImpostorViewYVarId;

} // namespace rendinst::render

extern int impostorLastMipSize;

void render_impostors_ofs(int count, int start_ofs, int vectors_cnt);

void initImpostorsGlobals();
void termImpostorsGlobals();

__forceinline void get_up_left(Point3 &up, Point3 &left, const TMatrix &itm)
{
  Point3 forward = itm.getcol(2);
  if (abs(forward.y) < 0.999)
  {
    left = normalize(cross(Point3(0, 1, 0), forward));
    up = cross(forward, left);
  }
  else
  {
    left = itm.getcol(0);
    up = itm.getcol(1);
  }
}

__forceinline void set_up_left_to_shader(const TMatrix &itm)
{
  Point3 up, left;

  get_up_left(up, left, itm);

  ShaderGlobal::set_color4_fast(rendinst::render::dynamicImpostorViewXVarId, Color4(left.x, left.y, left.z, 0));
  ShaderGlobal::set_color4_fast(rendinst::render::dynamicImpostorViewYVarId, Color4(up.x, up.y, up.z, 0));
}
