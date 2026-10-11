// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_hlsl_floatx.h>
#include "../shaders/rtss_common.hlsli"

// constraint: a layout change needs a new scheduler
struct RtssSchedulerLayout
{
  int levels = 4;       // 1..12
  int windowPages = 16; // power of two, 4..32
  int slotTexels = 64;  // power of two, 32..128
  int borderTexels = 1; // 1..4
  float texel0 = 0.05f; // meters, 0.001..1000
  float maxCasterReach = 2000.f, minSunY = 0.f;
  float maxAnisotropy = 1.f; // 1..8; above 1, light space y is scaled for square ground texels at a low sun

  int pageTexels() const { return slotTexels - 2 * borderTexels; }
  bool operator==(const RtssSchedulerLayout &) const = default;
};

struct RtssSchedulerBudget
{
  float toleranceDeg0 = 0.1f, toleranceDegMax = 1.6f, fastSweepDeg = 5.f;
  float minSweepSeconds = 0.1f, maxSweepSeconds = 8.f, staticSweepSeconds = 0.25f;
  float maxCameraSpeed = 343.f; // m/s, the speed of sound; a faster camera teleports
  // in cost units: pages times pageCost, and rasterCallCost for each raster call;
  // the burst pays the REQUIRED pages of the coarsest level, the update all others, and first its missing ones past the burst
  int updatePagesPerFrame = 24, validityBurstPagesPerFrame = 256;
  int batchFrames = 1;   // the update budget is a balance of up to this many frames, which one frame may spend and the next repay
  float seamBand = 0.1f; // outer fraction of a level cube that dithers to the next level or fades out
  // a raster call draws a box of pages of one level, of at most rasterBoxPagesPerJob pages for each job, that fits a scratch
  // ceil(sqrt(rasterBatchPages)) slots wide and as high, or half as high with rasterTranspose; the job that opens a box pays
  // rasterCallCost, as a call costs its culls
  int rasterBatchPages = 16, rasterBoxPagesPerJob = 2; // 1..256, 1..64
  float rasterCallCost = 0.f;
  bool rasterTranspose = true; // the scratch is half as high, and a box that fits it only turned by 90 deg draws turned
};

enum class RtssRaster : uint8_t
{
  None,   // the project tracer builds the page
  All,    // the raster producer builds the page
  Augment // the callback draws a subset of the casters, then the project tracer builds the page from the raster hit
};

struct RtssLevelRecipe
{
  RtssRaster raster = RtssRaster::None;
  float pageCost = 1.f; // budget units of one page, 0.1..100
};

struct RtssSchedulerRecipes
{
  RtssLevelRecipe recipes[RTSS_MAX_LEVELS]; // a raster change of a level invalidates that level
};

struct RtssSchedulerSettings : RtssSchedulerLayout, RtssSchedulerBudget, RtssSchedulerRecipes
{};
