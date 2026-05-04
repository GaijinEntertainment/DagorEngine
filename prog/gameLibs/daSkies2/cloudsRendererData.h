// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daSkies2/daSkies.h>
#include <resourcePool/resourcePool.h>
#include <shaders/dag_dynamicResolutionStcode.h>

#include "shaders/clouds2/cloud_settings.hlsli"

struct CloudsRendererData
{
  IPoint2 cloudTexRes = IPoint2::ZERO;
  eastl::optional<DynRes> prevDynRes;

  bool useBlurredClouds = false;
  CloudsResolution cloudResolution = CloudsResolution::Default;

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
  UniqueBuf clouds_sub_view_close_layer_is_outside; // todo: must be initialized only when needed.
  UniqueBuf cloudsIndirectBuffer;

  bool useCompute = true, taaUseCompute = false;
  bool lowresCloseClouds = true;

  bool lastFrameInfiniteSkies = false;

  void clearTemporalData(uint32_t gen);
  void close();
  void setVars(const bool is_main_view);
  void init(const IPoint2 &resolution, const char *name_prefix, bool can_be_in_clouds, CloudsResolution clouds_resolution,
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