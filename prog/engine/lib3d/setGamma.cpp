// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_render.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_platform.h>
#include <util/dag_stdint.h>
#include <math/dag_mathBase.h>
#include <shaders/dag_shaders.h>

static real cur_gamma = 1;

void set_gamma(real p)
{
  cur_gamma = p;
  d3d::setgamma(p);
}

void set_gamma_shadervar(real p)
{
  cur_gamma = p;
  static int shader_gammaVarId = get_shader_variable_id("shader_gamma");
  ShaderGlobal::set_real(shader_gammaVarId, p);
}

real get_current_gamma() { return cur_gamma; }
