// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daSkies2/daSkies.h>
#include <resourcePool/resourcePool.h>
#include <util/dag_string.h>

#include "shaders/clouds2/cloud_settings.hlsli"

struct CloudsRendererData
{
  IPoint2 cloudTexRes = IPoint2::ZERO;
  String texPrefix; // managed-texture names must be unique per view data

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
  // checkerboard trace: packed quarter-res target holding exactly this frame's
  // traced quincunx lattice (packed texel k = full-res texel 2k+sub of this frame)
  UniqueTex cloudsCheckerColor;
  // todo: reflection never needs clouds_color_close

  UniqueTex clouds_color_close;
  UniqueTex clouds_tile_distance, clouds_tile_distance_tmp;
  UniqueBuf clouds_close_layer_is_outside;
  UniqueBuf clouds_sub_view_close_layer_is_outside; // todo: must be initialized only when needed.
  UniqueBuf cloudsIndirectBuffer;

  bool useCompute = true, taaUseCompute = true;
  bool lowresCloseClouds = true;

  bool lastFrameInfiniteSkies = false;
  // written by renderCloudsPrepare, read by the same frame's renderCloudsApply; reset in close()
  bool closeLayerWasActive = false;

  void clearTemporalData(uint32_t gen);
  void ensureCheckerColor(bool wanted);
  void close();
  void setVars(const bool is_main_view);
  void init(const IPoint2 &resolution, const char *name_prefix, bool can_be_in_clouds, CloudsResolution clouds_resolution,
    bool use_blurred_clouds);

  void update(const DPoint3 &dir, const DPoint3 &origin);

  bool frameValid = false;

  unsigned lastPrepareMs = 0;             // real frame dt source for the checker weight scale
  DPoint3 prevCamPosForClamp{0., 0., 0.}; // camera speed source for the clamp collapse
  float prevLayerDist = 1e9f;             // layer distance last frame (ghosts look back one frame)
  float clampCollapseSmooth = 0;          // rises instantly, decays slowly

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