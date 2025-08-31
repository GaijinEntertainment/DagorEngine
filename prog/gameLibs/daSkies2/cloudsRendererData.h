// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daSkies2/daSkies.h>
#include <resourcePool/resourcePool.h>

#include "shaders/clouds2/cloud_settings.hlsli"

struct CloudsRendererData
{
  int w = 0, h = 0;
  bool useBlurredClouds = false;

  RTargetPool::Ptr cloudsColorPoolRT;
  RTargetPool::Ptr cloudsColorBlurPoolRT;
  RTargetPool::Ptr cloudsWeightPoolRT;
  RTarget::Ptr prevCloudsColor, nextCloudsColor;
  RTarget::Ptr cloudsTextureColor;
  RTarget::Ptr cloudsBlurTextureColor;
  RTarget::Ptr prevCloudsWeight;
  RTarget::Ptr cloudsTextureWeight;
  UniqueTex cloudsTextureDepth;
  // todo: reflection never needs clouds_color_close

  UniqueTex clouds_color_close;
  UniqueTex clouds_tile_distance, clouds_tile_distance_tmp;
  UniqueBuf clouds_close_layer_is_outside;
  UniqueBuf cloudsIndirectBuffer;

  bool useCompute = true, taaUseCompute = false;
  bool lowresCloseClouds = true;

  void clearTemporalData(uint32_t gen);
  void close();
  void setVars();
  void init(int width, int height, const char *name_prefix, bool can_be_in_clouds, CloudsResolution clouds_resolution,
    bool use_blurred_clouds);

  void update(const DPoint3 &dir, const DPoint3 &origin);

  bool frameValid = false;

  TMatrix4 prevGlobTm = TMatrix4::IDENT, prevProjTm = TMatrix4::IDENT;
  Point4 prevViewVecLT = {0, 0, 0, 0};
  Point4 prevViewVecRT = {0, 0, 0, 0};
  Point4 prevViewVecLB = {0, 0, 0, 0};
  Point4 prevViewVecRB = {0, 0, 0, 0};
  DPoint3 prevWorldPos{0., 0., 0.};
  unsigned int frameNo = 0;
  double currentCloudsOffset = 0.;

private:
  void initTiledDist(const char *prefix); // only needed when it is not 100% cloudy. todo: calc pixels count allocation
  static void clear_black(ManagedTex &t);

  static constexpr uint32_t tileX = CLOUDS_TILE_W, tileY = CLOUDS_TILE_H;
  DPoint3 cloudsCameraOrigin{0., 0., 0.};
  uint32_t resetGen = 0;
};