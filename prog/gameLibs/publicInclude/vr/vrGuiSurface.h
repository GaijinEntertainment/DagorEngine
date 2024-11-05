//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_texMgr.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>


namespace vrgui
{

enum SurfaceCurvature
{
  VRGUI_SURFACE_DEFAULT,
  VRGUI_SURFACE_SPHERE,
  VRGUI_SURFACE_CYLINDER,
  VRGUI_SURFACE_PLANE,
};

bool init_surface(int ui_width, int ui_height, SurfaceCurvature type = VRGUI_SURFACE_DEFAULT);
void destroy_surface();
bool is_inited();
void get_ui_surface_params(int &ui_width, int &ui_height, SurfaceCurvature &type);

void render_to_surface(TEXTUREID tex_id);

Point3 map_to_surface(const Point2 &point);
bool intersect(const TMatrix &pointer, Point2 &point_in_gui, Point3 &intersection);
bool intersect(const Point3 &src, const Point3 &dir, Point2 &point_in_gui, Point3 &intersection);
float get_ui_depth();


} // namespace vrgui
