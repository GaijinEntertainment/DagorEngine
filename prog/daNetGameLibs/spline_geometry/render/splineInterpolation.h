// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/componentTypes.h>
#include <memory/dag_framemem.h>

struct SplineGenSpline;

eastl::vector<SplineGenSpline, framemem_allocator> interpolate_points(const ecs::List<Point3> &points, int stripes);