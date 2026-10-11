// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_matricesAndPerspective.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <scene/dag_visibility.h>
#include <shaders/dag_shaders.h>

struct RenderView
{
  TMatrix view;
  TMatrix4 proj;
  TMatrix4 lodProj; // the heightmap tessellation metric: the CSM and light shadow passes keep the camera one
  Point3 position;
  mat44f culling;

  RenderView(const TMatrix &view_tm, const TMatrix4 &proj_tm, const Point3 &lod_pos) :
    view(view_tm), proj(proj_tm), lodProj(proj_tm), position(lod_pos)
  {
    d3d::calcglobtm(view, proj, reinterpret_cast<TMatrix4 &>(culling));
  }

  VisibilityFinder visibility() const
  {
    VisibilityFinder result;
    result.set(v_ldu_p3(&position.x), Frustum(culling), 0, 0, 1, 1, nullptr);
    return result;
  }

  void bind() const
  {
    d3d::settm(TM_WORLD, TMatrix::IDENT);
    d3d::settm(TM_VIEW, view);
    d3d::settm(TM_PROJ, &proj);
    ShaderGlobal::set_float4(get_shader_variable_id("world_view_pos", true), position.x, position.y, position.z, 1);
  }
};
