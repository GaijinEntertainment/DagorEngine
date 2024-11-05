// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/pointLod/range.h>
#include <EASTL/array.h>
#include <vecmath/dag_vecMathDecl.h>
#include <math/dag_frustum.h>
#include <math/integer/dag_IPoint2.h>

namespace plod
{

float get_range2_for_density(float density, IPoint2 resolution, mat44f_cref globtm, Point3 camera_pos)
{
  Frustum frustum(globtm);
  vec3f pos = v_make_vec3f(camera_pos.x, camera_pos.y, camera_pos.z);
  eastl::array<vec3f, 8> frustumPoints;
  frustum.generateAllPointFrustm(frustumPoints.data());

  const auto nearPlaneW = v_extract_x(v_length3_x(v_sub(frustumPoints[0], frustumPoints[2])));
  const auto nearPlaneH = v_extract_x(v_length3_x(v_sub(frustumPoints[0], frustumPoints[4])));
  const auto nearPlaneArea = nearPlaneW * nearPlaneH;

  const auto rtDensity = static_cast<float>(resolution.x) * static_cast<float>(resolution.y) / nearPlaneArea;

  const auto zNearDist = v_extract_x(v_plane_dist_x(frustum.camPlanes[Frustum::NEARPLANE], pos));
  const auto zNearDist2 = zNearDist * zNearDist;
  const auto zFarDist = v_extract_x(v_plane_dist_x(frustum.camPlanes[Frustum::NEARPLANE], frustumPoints[1]));
  const auto zFarDist2 = zFarDist * zFarDist;
  const auto frustumEdgeLength2 = v_extract_x(v_length3_sq_x(v_sub(frustumPoints[0], frustumPoints[1])));
  const auto cos2inv = frustumEdgeLength2 / zFarDist2;

  return rtDensity / density * zNearDist2 * cos2inv;
}

int get_density_power2_scale(float dist2, float min_dist2) { return static_cast<int>(std::ceil(std::log2(dist2 / min_dist2))) - 1; }

} // namespace plod
