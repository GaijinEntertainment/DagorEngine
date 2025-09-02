// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <scene/dag_physMat.h>
#include <sceneRay/dag_sceneRayDecl.h>

class LandMeshManager;
class Point2;
struct CollisionObject;

namespace dacoll
{
LandMeshManager *get_lmesh();
PhysMat::MatID get_lmesh_mat_id();
const StaticSceneRayTracer *get_frt();
BuildableStaticSceneRayTracer *get_water_tracer();
dag::ConstSpan<unsigned char> get_pmid();
bool has_only_water2d();
void get_landmesh_mirroring(int &cells_x_pos, int &cells_x_neg, int &cells_z_pos, int &cells_z_neg);
float get_collision_object_collapse_threshold(const CollisionObject &co);
}; // namespace dacoll
