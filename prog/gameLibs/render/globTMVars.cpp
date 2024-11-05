// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_shaders.h>
#include <math/dag_TMatrix4.h>

#define VIEW_VARS_LIST \
  VAR(globtm_psf_0)    \
  VAR(globtm_psf_1)    \
  VAR(globtm_psf_2)    \
  VAR(globtm_psf_3)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
VIEW_VARS_LIST
#undef VAR

void set_globtm_to_shader(const TMatrix4 &globtm_)
{
  TMatrix4 globtm = globtm_.transpose();
  ShaderGlobal::set_color4(globtm_psf_0VarId, Color4(globtm[0]));
  ShaderGlobal::set_color4(globtm_psf_1VarId, Color4(globtm[1]));
  ShaderGlobal::set_color4(globtm_psf_2VarId, Color4(globtm[2]));
  ShaderGlobal::set_color4(globtm_psf_3VarId, Color4(globtm[3]));
}