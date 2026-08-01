//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <cstdint>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_plane3.h>
#include <generic/dag_relocatableFixedVector.h>

class DataBlock;

namespace frx
{

struct DestrMesh;
struct DestrContext;

void mesh_slice(DestrContext &ctx, const DestrMesh &mesh, DestrMesh *up_mesh, DestrMesh *down_mesh, Plane3 plane, uint16_t cut_mat_id,
  float push_dist = 0.f);


struct ShatterImpactProfile
{
  // formula: power = cvt(pow(cvt(radius, radiusRange.x, radiusRange.y, 0, 1), fallowPow), 0, 1, powerRange.x, powerRange.y)
  Point3 pos;
  Point2 radiusRange = Point2::ZERO;
  Point2 powerRange = Point2::ZERO;
  float falloffPow = 1.f;
};

struct ShatterMaterialProfile
{
  struct SizeMode
  {
    float weight = 1.f;
    Point2 powerRange = Point2::ZERO;
    Point2 sizeRange = Point2::ZERO;
  };
  dag::RelocatableFixedVector<SizeMode, 2> sizeModes;

  struct CutPlaneMode
  {
    float weight = 1.f;
    Point2 cosineRange = Point2::ZERO;
  };
  dag::RelocatableFixedVector<CutPlaneMode, 1> cutPlaneModes;

  int minPieces = 3, maxPieces = 25;
  int impactPointCuts = 3;
  Point2 relativeSizeWindow = Point2(0.8f, 1.7f);
  float minCutRatio = 0.125f;

  void loadFromBlk(const DataBlock &blk);
};

void shatter_into_pieces(DestrContext &ctx, DestrSystem &sys, int start_piece_idx, uint16_t interior_mat,
  const ShatterImpactProfile &impact, const ShatterMaterialProfile &material, int &seed);

} // namespace frx
