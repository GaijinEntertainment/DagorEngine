// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_carray.h>
#include <drv/3d/dag_driver.h>
#include "giFrameInfo.h"
#include <drv/3d/dag_tex3d.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_computeShaders.h>
#include <3d/dag_resizableTex.h>

struct RadianceCache;
struct VolumetricGI
{
  struct ZDistributionParams
  {
    uint32_t slices = 16;
    float zDist = 128, zStart = 0.25, zLogMul = 6; // the bigger zLogMul the more linear distribution is
    ZDistributionParams extend_distribution_for_dist(float dist) const;
  };
  void init(uint32_t tile_size, uint32_t sw, uint32_t sh, uint32_t max_sw, uint32_t max_sh, uint32_t probe_oct_size,
    uint32_t spatial_passes, float irradianceProbe0Size, uint32_t irradiance_clip_w, bool reproject,
    const ZDistributionParams &zparams);
  void calc(const TMatrix &viewItm, const TMatrix4 &proj, float zn, float zf, float quality);
  void setHistoryBlurTexelOfs(float texelOfs) { historyBlurTexelOfs = clamp(texelOfs, 0.f, 1.f); }
  // debug
  void drawDebug(int debug_type = 0);
  void afterReset();

  struct FroxelsRes
  {
    uint32_t w = 0, h = 0;
    bool operator==(const FroxelsRes &a) const { return w == a.w && h == a.h; }
    bool operator!=(const FroxelsRes &a) const { return !(*this == a); }
  };
  struct FrameInfo : public DaGIFrameInfo
  {
    FroxelsRes res;
  };

protected:
  void allocate(uint32_t tile_size, uint32_t max_sw, uint32_t max_sh, uint32_t probe_oct_size, uint32_t spatial_passes,
    float irradianceProbe0Size, uint32_t irradiance_clip_w, bool reproject, const ZDistributionParams &zparams);
  struct ZParams
  {
    float zDist = 0, zMul = 0, zAdd = 0, zExpMul = 0;
    bool operator==(const ZParams &a) const { return zDist == a.zDist && zAdd == a.zAdd && zMul == a.zMul && zExpMul == a.zExpMul; }
    bool operator!=(const ZParams &a) const { return !(*this == a); }
  } zParams;
  carray<ResizableTex, 2> gi_froxels_sph0, gi_froxels_sph1; // history textures
  carray<ResizableTex, 2> gi_froxels_radiance;              // temporal textures (can be buffers)!

  eastl::unique_ptr<ComputeShaderElement> calc_current_gi_froxels_cs, spatial_filter_gi_froxels_cs, calc_gi_froxels_irradiance_cs,
    calc_gi_froxels_irradiance_simple_cs;
  uint32_t maxW = 1, maxH = 1, d = 1, radianceRes = 3, spatialPasses = 0, tracedSlices = 0;
  float historyBlurTexelOfs = 0.125f;
  bool validHistory = false;
  uint32_t temporalFrame = 0;
  FrameInfo prevFrameInfo;
  FroxelsRes nextRes, curRes;

  DynamicShaderHelper drawDebugAllProbes;
  void initDebug();
};
