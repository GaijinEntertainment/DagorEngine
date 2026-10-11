// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <memory/dag_framemem.h>
#include <vecmath/dag_vecMath.h>
#include <dag/dag_vectorMap.h>
#include <math/random/dag_random.h>
#include <math/dag_mathUtils.h>
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_relocatableFixedVector.h>
#include <ska_hash_map/flat_hash_map2.hpp>

#include <daFracture/core/destrMesh.h>
#include <daFracture/core/meshCommon.h>
#include <daFracture/core/meshSlicing.h>
#include <daFracture/core/cutFaceFill.h>

#include "meshSliceImpl.h"
#include "shatterDriver.h"
#include "shatterFlavorPlane.h"
#include "shatterFlavorRough.h"


namespace frx
{

void mesh_slice(DestrContext &ctx, const DestrMesh &mesh, const MeshSliceParams &params) { mesh_slice_impl(ctx, mesh, params); }

void mesh_slice_hmap_plane(DestrContext &ctx, const DestrMesh &mesh, const MeshSliceParams &params)
{
  // kept only for testing purposes for now, to be removed
  struct HmapPlane
  {
    const float maxHeight = 0.5f;
    const float cellSize = 0.1f;
    const Point2 gridOrigin = Point2::ZERO;
    float sample(const Point2 &) { return maxHeight / 2.f; }
  } hmap;
  mesh_slice_impl(ctx, mesh, params, hmap);
}


void RoughShatterFlavorSettings::loadFromBlk(const DataBlock &blk)
{
  relCell = blk.getReal("relCell", relCell);
  minCell = blk.getReal("minCell", minCell);
  maxCell = blk.getReal("maxCell", maxCell);
  relAmpl = blk.getReal("relAmpl", relAmpl);
  ridgeCells = blk.getInt("ridgeCells", ridgeCells);
  ridgeRange = blk.getPoint2("ridgeRange", ridgeRange);
  ridgeJitter = blk.getReal("ridgeJitter", ridgeJitter);
  detailFrac = blk.getReal("detailFrac", detailFrac);
  detailCells = blk.getReal("detailCells", detailCells);
  fadeCells = blk.getReal("fadeCells", fadeCells);
  creaseFollowProb = blk.getReal("creaseFollowProb", creaseFollowProb);
  creaseBlendCells = blk.getReal("creaseBlendCells", creaseBlendCells);
  creaseMaxTiltDeg = blk.getReal("creaseMaxTiltDeg", creaseMaxTiltDeg);
  creaseTiltRnd = blk.getReal("creaseTiltRnd", creaseTiltRnd);
  creaseMaxOffcenter = blk.getReal("creaseMaxOffcenter", creaseMaxOffcenter);
}


template <size_t I = 0, class Variant>
static void load_flavor_from_blk(Variant &flavor, const DataBlock &blk)
{
  if constexpr (I < eastl::variant_size_v<Variant>)
  {
    using Settings = eastl::variant_alternative_t<I, Variant>;
    if (const DataBlock *fb = blk.getBlockByName(Settings::FLAVOR_NAME))
    {
      Settings *settings = eastl::get_if<Settings>(&flavor);
      (settings ? *settings : flavor.template emplace<Settings>()).loadFromBlk(*fb);
    }
    else
      load_flavor_from_blk<I + 1>(flavor, blk);
  }
  // no flavor block keeps the current flavor, so a fresh profile stays on the variant's first alternative (plane)
}


void ShatterMaterialProfile::loadFromBlk(const DataBlock &blk)
{
  minPieces = blk.getInt("minPieces", minPieces);
  maxPieces = blk.getInt("maxPieces", maxPieces);
  impactPointCuts = blk.getInt("impactPointCuts", impactPointCuts);
  relativeSizeWindow = blk.getPoint2("relativeSizeWindow", relativeSizeWindow);
  minCutRatio = blk.getReal("minCutRatio", minCutRatio);

  // mode lists are replaced as a whole when the block lists any, kept otherwise
  if (blk.blockExists("sizeMode"))
    sizeModes.clear();
  const bool hasCutPlaneModes = blk.blockExists("cutPlaneMode");
  if (hasCutPlaneModes)
    cutPlaneModes.clear();
  const int sizeModeNid = blk.getNameId("sizeMode");
  const int cutPlaneModeNid = blk.getNameId("cutPlaneMode");
  for (int i = 0, n = blk.blockCount(); i < n; i++)
  {
    const DataBlock &b = *blk.getBlock(i);
    if (b.getBlockNameId() == sizeModeNid)
    {
      SizeMode &m = sizeModes.push_back();
      m.weight = b.getReal("weight", m.weight);
      m.powerRange = b.getPoint2("powerRange", m.powerRange);
      m.sizeRange = b.getPoint2("sizeRange", m.sizeRange);
    }
    else if (b.getBlockNameId() == cutPlaneModeNid)
    {
      CutPlaneMode &m = cutPlaneModes.push_back();
      m.weight = b.getReal("weight", m.weight);
      const Point2 angleRangeDeg = b.getPoint2("angleRangeDeg", Point2::ZERO);
      m.cosineRange = Point2(cosf(angleRangeDeg.x * DEG_TO_RAD), cosf(angleRangeDeg.y * DEG_TO_RAD));
    }
  }

  if (const float jitterDeg = blk.getReal("cutPlaneJitterDeg", -1.f); !hasCutPlaneModes && jitterDeg >= 0.f)
  {
    cutPlaneModes.clear();
    CutPlaneMode &m = cutPlaneModes.push_back();
    m.cosineRange = Point2(1.f, cosf(jitterDeg * DEG_TO_RAD));
  }

  load_flavor_from_blk(flavor, blk);
}


#define SAVE_PARAM_IF_CHANGED(setter, field) \
  if (!base || field != base->field)         \
  blk.setter(#field, field)

void RoughShatterFlavorSettings::saveToBlk(DataBlock &blk, const RoughShatterFlavorSettings *base) const
{
  SAVE_PARAM_IF_CHANGED(setReal, relCell);
  SAVE_PARAM_IF_CHANGED(setReal, minCell);
  SAVE_PARAM_IF_CHANGED(setReal, maxCell);
  SAVE_PARAM_IF_CHANGED(setReal, relAmpl);
  SAVE_PARAM_IF_CHANGED(setInt, ridgeCells);
  SAVE_PARAM_IF_CHANGED(setPoint2, ridgeRange);
  SAVE_PARAM_IF_CHANGED(setReal, ridgeJitter);
  SAVE_PARAM_IF_CHANGED(setReal, detailFrac);
  SAVE_PARAM_IF_CHANGED(setReal, detailCells);
  SAVE_PARAM_IF_CHANGED(setReal, fadeCells);
  SAVE_PARAM_IF_CHANGED(setReal, creaseFollowProb);
  SAVE_PARAM_IF_CHANGED(setReal, creaseBlendCells);
  SAVE_PARAM_IF_CHANGED(setReal, creaseMaxTiltDeg);
  SAVE_PARAM_IF_CHANGED(setReal, creaseTiltRnd);
  SAVE_PARAM_IF_CHANGED(setReal, creaseMaxOffcenter);
}


void ShatterMaterialProfile::saveToBlk(DataBlock &blk, const ShatterMaterialProfile *base) const
{
  SAVE_PARAM_IF_CHANGED(setInt, minPieces);
  SAVE_PARAM_IF_CHANGED(setInt, maxPieces);
  SAVE_PARAM_IF_CHANGED(setInt, impactPointCuts);
  SAVE_PARAM_IF_CHANGED(setPoint2, relativeSizeWindow);
  SAVE_PARAM_IF_CHANGED(setReal, minCutRatio);

  if (!base || sizeModes != base->sizeModes)
    for (const SizeMode &m : sizeModes)
    {
      DataBlock &b = *blk.addNewBlock("sizeMode");
      b.setReal("weight", m.weight);
      b.setPoint2("powerRange", m.powerRange);
      b.setPoint2("sizeRange", m.sizeRange);
    }
  if (!base || cutPlaneModes != base->cutPlaneModes)
    for (const CutPlaneMode &m : cutPlaneModes)
    {
      DataBlock &b = *blk.addNewBlock("cutPlaneMode");
      b.setReal("weight", m.weight);
      b.setPoint2("angleRangeDeg",
        Point2(acosf(clamp(m.cosineRange.x, -1.f, 1.f)) * RAD_TO_DEG, acosf(clamp(m.cosineRange.y, -1.f, 1.f)) * RAD_TO_DEG));
    }

  eastl::visit(
    [&](const auto &flavor_params) {
      using Settings = eastl::decay_t<decltype(flavor_params)>;
      const Settings defaults{};
      const Settings *baseSettings = base ? eastl::get_if<Settings>(&base->flavor) : nullptr;
      const bool switchesFlavor = base && !baseSettings;
      DataBlock flavorBlk;
      flavor_params.saveToBlk(flavorBlk, switchesFlavor ? &defaults : baseSettings);
      if (!base || switchesFlavor || !flavorBlk.isEmpty())
        blk.addNewBlock(&flavorBlk, Settings::FLAVOR_NAME);
    },
    flavor);
}

#undef SAVE_PARAM_IF_CHANGED


void shatter_into_pieces(DestrContext &ctx, DestrSystem &sys, int start_piece_idx, uint16_t interior_mat,
  const ShatterImpactProfile &impact, const ShatterMaterialProfile &material, int &seed)
{
  TIME_PROFILE(shatter_into_pieces)
  FRAMEMEM_REGION;
  eastl::visit(
    [&](const auto &flavor_params) {
      using FlavorParams = eastl::decay_t<decltype(flavor_params)>;
      using FlavorImpl = typename FlavorParams::FlavorImpl;
      FlavorImpl flavor(flavor_params);
      ShatterDriver<FlavorImpl> driver(ctx, sys, interior_mat, impact, material, seed, flavor);
      [[maybe_unused]] const int cutsDone = driver.run(start_piece_idx);
      DA_PROFILE_TAG(shatter_into_pieces, FlavorParams::FLAVOR_NAME);
      DA_PROFILE_TAG(shatter_into_pieces, "cuts:%d", cutsDone)
    },
    material.flavor);
}

} // namespace frx
