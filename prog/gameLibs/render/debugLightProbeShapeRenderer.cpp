// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_render.h>
#include <debug/dag_debug3d.h>
#include <scene/dag_tiledScene.h>
#include <util/dag_string.h>
#include <math/dag_mathUtils.h>
#include <rendInst/rendInstExtra.h>
#include <math/dag_capsule.h>
#include <render/debugLightProbeShapeRenderer.h>
#include <render/indoorProbeScenes.h>

DebugLightProbeShapeRenderer::DebugLightProbeShapeRenderer(const IndoorProbeScenes *scenes) : scenes(scenes) {}

void draw_transformed_capsule_shape(mat44f_cref m, const BBox3 &box, E3DCOLOR color)
{
  alignas(16) TMatrix tm;
  v_mat_43ca_from_mat44(tm.m[0], m);

  Point3 boxSize(v_extract_x(v_length3_x(m.col0)), v_extract_x(v_length3_x(m.col1)), v_extract_x(v_length3_x(m.col2)));
  float width = min(boxSize.x, boxSize.y);
  float half_height = (boxSize.z - width) * 0.5f;
  Point3 a(0, 0, half_height / boxSize.z);
  Point3 b(0, 0, -half_height / boxSize.z);

  Capsule capsule(a, b, width / 2.0f);

  draw_cached_debug_capsule(capsule, color, tm);
  G_UNREFERENCED(box);
}

void draw_transformed_cylinder_shape(mat44f_cref m, const BBox3 &box, E3DCOLOR color)
{
  alignas(16) TMatrix tm;
  v_mat_43ca_from_mat44(tm.m[0], m);

  Point3 boxSize(v_extract_x(v_length3_x(m.col0)), v_extract_x(v_length3_x(m.col1)), v_extract_x(v_length3_x(m.col2)));
  float width = min(boxSize.x, min(boxSize.y, boxSize.z));
  float radius = width / 2.0f;

  Point3 a(0, 0, 0.5f);
  Point3 b(0, 0, -0.5f);

  Point3 tm_a = tm * a;
  Point3 tm_b = tm * b;

  draw_cached_debug_cylinder_w(tm_a, tm_b, radius, color);
  G_UNREFERENCED(box);
}

void draw_transfromed_box_shape(mat44f_cref m, const BBox3 &box, E3DCOLOR color)
{
  alignas(16) TMatrix tm;
  v_mat_43ca_from_mat44(tm.m[0], m);
  set_cached_debug_lines_wtm(tm);
  draw_cached_debug_box(box, color);
  set_cached_debug_lines_wtm(TMatrix::IDENT);
}

eastl::vector_map<const char *, eastl::function<void(mat44f_cref m, const BBox3 &box, E3DCOLOR color)>> map = {
  {"envi_probe_box", draw_transfromed_box_shape}, {"envi_probe_capsule", draw_transformed_capsule_shape},
  {"envi_probe_cylinder", draw_transformed_cylinder_shape}};

void DebugLightProbeShapeRenderer::render(const RiGenVisibility *visibility, const Point3 &view_pos)
{
  TIME_D3D_PROFILE(DebugLightProbeShapeRenderer)
  mat44f globtm;
  d3d::getglobtm(globtm);

  E3DCOLOR currentColor(0, 255, 0);
  BBox3 box(Point3(-0.5, -0.5, -0.5), Point3(0.5, 0.5, 0.5));

  const vec4f vpos_distscale = v_make_vec4f(view_pos.x, view_pos.y, view_pos.z, scene::defaultDisappearSq / (80 * 80));
  begin_draw_cached_debug_lines(false, false);
  for (int i = 0; i < scenes->sceneDatas.size(); i++)
  {
    auto name = scenes->sceneDatas[i].name;
    scenes->sceneDatas[i].scenePtr->template frustumCull<false, true, false>(globtm, vpos_distscale, 0, 0, nullptr,
      [&box, &currentColor, &name](scene::node_index, mat44f_cref m, vec4f) { map[name](m, box, currentColor); });
  }

  end_draw_cached_debug_lines();
  set_cached_debug_lines_wtm(TMatrix::IDENT);
  G_UNREFERENCED(visibility);
}