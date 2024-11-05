// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_Point4.h>

class LandMeshManager *getLmeshMgr();

// By using TEXFMT_L16 we store 4x4 regions in one texel.
constexpr int TEX_CELL_RES = 4;

// .x = X scale, .y = Y scale, .z = -X scaled offset, .w = -Y scaled offset
Point4 getScaleOffset(int texture_size);

// .x = min height approx, .y = max height approx
Point2 getApproxHeightmapMinMax(const BBox2 &box);
Point2 getApproxHeightmapMinMax(const BBox3 &box);

float getSamplingStepDist(float length, int tex_size);

bool hasGroundHoleManager();
