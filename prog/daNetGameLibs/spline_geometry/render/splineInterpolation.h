// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/componentTypes.h>
#include <memory/dag_framemem.h>

struct SplineGenSpline;

eastl::vector<SplineGenSpline, framemem_allocator> interpolate_points(
  const ecs::List<Point3> &points, const ecs::List<Point2> &radii, const ecs::List<Point3> &emissive_points, int stripes);