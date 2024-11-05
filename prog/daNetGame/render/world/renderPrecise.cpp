// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "renderPrecise.h"
#include "global_vars.h"
#include <shaders/dag_shaderVariableInfo.h>
#include <math/dag_Point4.h>
#include <drv/3d/dag_consts_base.h>
#include <drv/3d/dag_matricesAndPerspective.h>


RenderPrecise::RenderPrecise(const TMatrix &view_tm, const Point3 &view_pos) : savedViewTm(view_tm)
{
  TMatrix offsetedView = view_tm;
  offsetedView.setcol(3, Point3::ZERO);
  d3d::settm(TM_VIEW, offsetedView);

  ShaderGlobal::set_color4(camera_base_offsetVarId, -view_pos);
}

RenderPrecise::~RenderPrecise()
{
  ShaderGlobal::set_color4(camera_base_offsetVarId, Point4::ZERO);
  d3d::settm(TM_VIEW, savedViewTm);
}
