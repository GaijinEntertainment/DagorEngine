// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <sceneRay/dag_sceneRayDecl.h>

class MeshData;
class TMatrix;

void calculatePRT(MeshData &meshData, const TMatrix &toWorld, StaticSceneRayTracer *theRayTracer, int rays_per_point,
  float points_per_sq_meter, int maxPointsPerFace, float maxDist, int prt1, int prt2, int prt3); // channels
