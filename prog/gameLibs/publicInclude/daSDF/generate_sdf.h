//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/integer/dag_IPoint3.h>
#include <math/integer/dag_IPoint2.h>
#include <generic/dag_staticTab.h>
#include <math/dag_point2.h>
#include <daSDF/sparseSDFMip.h>
#include <math/dag_hlsl_floatx.h>
#include <daSDF/objects_sdf.hlsli>
#include <sceneRay/dag_sceneRayDecl.h>

class MippedMeshSDF
{
public:
  BBox3 localBounds;
  uint8_t mipCountTwoSided = 0;
  uint8_t mipCount() const { return mipCountTwoSided & 0x3; }
  bool mostlyTwoSided() const { return bool(mipCountTwoSided >> 1); }
  carray<SparseSDFMip, SDF_NUM_MIPS> mipInfo;

  dag::Vector<uint8_t> compressedMips;
};

class BBox3;

void generate_sdf(StaticSceneRayTracer &tr, MippedMeshSDF &OutData,
  float voxel_density = 3.f, // 5 voxels per local unit (meter) - voxel size is 0.2m
  int per_mesh_max_res = 128);
void init_sdf_generate();
void close_sdf_generate();
