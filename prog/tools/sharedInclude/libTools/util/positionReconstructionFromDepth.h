//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_resId.h>

class Point2;
class Point3;
class TMatrix;
class TMatrix4;

namespace position_reconstruction_from_depth
{

float linearize_z(float raw_z, Point2 decode_depth);
float linearize_z(float raw_z, float near_z, float far_z);

Point3 reconstruct_ws_position_from_raw_depth(const TMatrix &view_tm, const TMatrix4 &proj_tm, Point2 sample_pos, float raw_z);
Point3 reconstruct_ws_position_from_linear_depth(const TMatrix &view_tm, const TMatrix4 &proj_tm, Point2 sample_pos, float linear_z);

// potentially very slow, should only be used in tools
// depth texture type must be R32 (float)
bool get_position_from_depth(const TMatrix &view_tm, const TMatrix4 &proj_tm, TEXTUREID depth_texture_id, const Point2 &sample_pos,
  Point3 &out_world_pos);

} // namespace position_reconstruction_from_depth
