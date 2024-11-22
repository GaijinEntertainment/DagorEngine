//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/vector.h>
#include <generic/dag_span.h>
#include <math/dag_Point3.h>
#include <math/dag_Point2.h>
#include <math/dag_TMatrix.h>

namespace sndsys
{
int add_geometry(int max_polygons, int max_vertices);

void remove_geometry(int geometry_id);

void remove_all_geometry();

void add_polygons(int geometry_id, dag::ConstSpan<Point3> vertices, int num_verts_per_poly, float direct_occlusion,
  float reverb_occlusion, bool doublesided);

void add_polygon(int geometry_id, const Point3 &a, const Point3 &b, const Point3 &c, float direct_occlusion, float reverb_occlusion,
  bool doublesided);

void set_geometry_position(int geometry_id, const Point3 &position);
Point3 get_geometry_position(int geometry_id);

int get_geometry_count();
int get_geometry_id(int idx);

void save_geometry_to_file(const char *filename);
bool load_geometry_from_file(const char *filename);

// debug
const eastl::vector<Point3> *get_geometry_faces(int geometry_id);
Point2 get_geometry_occlusion(const Point3 &source, const Point3 &listener); // output: float2(direct, reverb)
} // namespace sndsys
