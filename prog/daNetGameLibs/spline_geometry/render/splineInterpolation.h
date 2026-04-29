// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/componentTypes.h>
#include <memory/dag_framemem.h>

struct SplineGenSpline;

eastl::vector<SplineGenSpline, framemem_allocator> interpolate_points(const ecs::List<Point3> &points,
  const ecs::List<Point2> &radii,
  const ecs::List<Point3> &emissive_points,
  const ecs::List<Point2> &spline_gen_geometry__shape_positions,
  const ecs::List<IPoint2> &spline_gen_geometry__cached_shape_ofs_data,
  Point3 first_normal,
  Point3 first_bitangent,
  bool use_last_point_to_orient_spline,
  int stripes);