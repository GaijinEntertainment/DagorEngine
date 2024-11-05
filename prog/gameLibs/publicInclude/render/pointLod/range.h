//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>
#include <vecmath/dag_vecMathDecl.h>

namespace plod
{

// Returns minimum squared range for the given view and render target, which satisfy point cloud density restriction,
// so point cloud instance will have no holes in it when rendered.
float get_range2_for_density(float density, IPoint2 resolution, mat44f_cref globtm, Point3 camera_pos);

// Returns the power of 2 for point cloud density scale at distance to safely reduce the number of drawn points.
int get_density_power2_scale(float dist2, float min_dist2);

} // namespace plod
