// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <scene/dag_physMat.h>

class LandMeshManager;
class Point2;
struct CollisionObject;

namespace dacoll
{
LandMeshManager *get_lmesh();
PhysMat::MatID get_lmesh_mat_id();
bool has_only_water2d();
void get_landmesh_mirroring(int &cells_x_pos, int &cells_x_neg, int &cells_z_pos, int &cells_z_neg);
float get_collision_object_collapse_threshold(const CollisionObject &co);

// v0.w of a cached triangle: TAG | matId, or untagged for a face that carries no material
inline constexpr int STATIC_CACHE_TRI_MAT_TAG = 0x40000000;
inline constexpr int STATIC_CACHE_TRI_MAT_MASK = 0xffff;
inline int static_cache_pack_tri_mat(int mat)
{
  return mat >= 0 ? (STATIC_CACHE_TRI_MAT_TAG | (mat & STATIC_CACHE_TRI_MAT_MASK)) : (int)0x80000000;
}
}; // namespace dacoll
