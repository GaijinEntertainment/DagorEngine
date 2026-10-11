//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <cstdint>
#include <EASTL/variant.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_plane3.h>
#include <generic/dag_relocatableFixedVector.h>

#include "cutFaceFill.h"


class DataBlock;

namespace frx
{

struct DestrMesh;
struct DestrSystem;
struct DestrContext;

struct MeshSliceParams
{
  DestrMesh *upMesh = nullptr, *downMesh = nullptr;
  PlaneBasis cutPlane;
  int16_t cutMatId = -1;
  float pushDist = 0.f;
};

void mesh_slice(DestrContext &ctx, const DestrMesh &mesh, const MeshSliceParams &params);
void mesh_slice_hmap_plane(DestrContext &ctx, const DestrMesh &mesh, const MeshSliceParams &params);


struct ShatterImpactProfile
{
  // formula: power = cvt(pow(cvt(radius, radiusRange.x, radiusRange.y, 0, 1), fallowPow), 0, 1, powerRange.x, powerRange.y)
  Point3 pos;
  Point2 radiusRange = Point2::ZERO;
  Point2 powerRange = Point2::ZERO;
  float falloffPow = 1.f;
};

struct PlaneSliceFlavor;
struct PlaneShatterFlavorSettings
{
  using FlavorImpl = PlaneSliceFlavor;
  static constexpr const char *FLAVOR_NAME = "planeFlavor";

  // no specific settings, used as default flavor

  void loadFromBlk(const DataBlock &) {}
  void saveToBlk(DataBlock &, const PlaneShatterFlavorSettings * = nullptr) const {}
};

// Feature parameters for the organic (heightmap-cut) flavor -- surface look only; the structural knobs stay
// in ShatterMaterialProfile. Defaults reproduce the original heightmap-shatter prototype. Sizes that set a
// feature scale are RELATIVE TO THE PIECE's long-axis diameter (minCell/maxCell are absolute world clamps);
// everything else is in grid cells or degrees.
struct RoughShatterFlavor;
struct RoughShatterFlavorSettings
{
  using FlavorImpl = RoughShatterFlavor;
  static constexpr const char *FLAVOR_NAME = "roughFlavor";

  // heightfield scale
  float relCell = 0.035f;                 // grid pitch L as a fraction of the piece's long-axis diameter
  float minCell = 0.03f, maxCell = 0.35f; // absolute world clamps on L
  float relAmpl = 2.5f;                   // maxHeight = relAmpl * L

  // break-line ridge (along the main axis u)
  int ridgeCells = 4;                      // grid cells between break lines (kinks stay grid-aligned)
  Point2 ridgeRange = Point2(0.06f, 0.9f); // knots alternate near x (valley) / y (peak), fraction of maxHeight
  float ridgeJitter = 0.35f;               // random deviation of each knot inward from its extreme
  float detailFrac = 0.35f;                // secondary-noise amplitude, fraction of maxHeight
  float detailCells = 2.f;                 // detail value-noise lattice pitch, in cells
  float fadeCells = 1.5f;                  // detail fades to 0 within this many cells of a kink

  // inherited-crease branching
  float creaseFollowProb = 1.f;    // chance to branch a cut off an inherited concave crease
  float creaseBlendCells = 3.f;    // hmap blends to neutral within this many cells of the crease
  float creaseMaxTiltDeg = 20.f;   // max branch tilt off the crease's inward normal
  float creaseTiltRnd = 0.3f;      // per-candidate angle jitter, fraction of the tilt limit
  float creaseMaxOffcenter = 0.1f; // reject a crease whose best cut misses hint.center by more than
                                   // this fraction of the PIECE size (would drift off the target)

  void loadFromBlk(const DataBlock &blk);
  void saveToBlk(DataBlock &blk, const RoughShatterFlavorSettings *base = nullptr) const;
};

struct ShatterMaterialProfile
{
  struct SizeMode
  {
    float weight = 1.f;
    Point2 powerRange = Point2::ZERO;
    Point2 sizeRange = Point2::ZERO;

    bool operator==(const SizeMode &) const = default;
  };
  dag::RelocatableFixedVector<SizeMode, 2> sizeModes;

  struct CutPlaneMode
  {
    float weight = 1.f;
    Point2 cosineRange = Point2::ZERO;

    bool operator==(const CutPlaneMode &) const = default;
  };
  dag::RelocatableFixedVector<CutPlaneMode, 1> cutPlaneModes;

  int minPieces = 3, maxPieces = 25;
  int impactPointCuts = 3;
  Point2 relativeSizeWindow = Point2(0.8f, 1.7f);
  float minCutRatio = 0.125f;

  eastl::variant<PlaneShatterFlavorSettings, RoughShatterFlavorSettings> flavor;

  void loadFromBlk(const DataBlock &blk);
  void saveToBlk(DataBlock &blk, const ShatterMaterialProfile *base = nullptr) const;
};

// Plane-cut shatter (flat cut faces).
void shatter_into_pieces(DestrContext &ctx, DestrSystem &sys, int start_piece_idx, uint16_t interior_mat,
  const ShatterImpactProfile &impact, const ShatterMaterialProfile &material, int &seed);

} // namespace frx
