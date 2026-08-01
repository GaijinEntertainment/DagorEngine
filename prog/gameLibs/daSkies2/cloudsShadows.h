// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/computeShaderFallback/voltexRenderer.h>
#include <shaders/dag_computeShaders.h>
#include <EASTL/unique_ptr.h>
#include "shaders/clouds2/cloud_settings.hlsli"
#include "clouds2Common.h"

class CloudsShadows
{
public:
  void init();
  void invalidate()
  {
    resetGen = 0;
    bsmNeedFull = true;
  }
  // instant BSM re-render on the next frame (app-driven reset, major settings change)
  void invalidateBSM() { bsmNeedFull = true; }
  void setBSMLog2AmortizeFrames(int log2_frames);
  // camera teleport catch-up for callers that learn the frame's origin after render():
  // an out-of-window map re-renders now instead of next frame
  void ensureBSMWindow(const Point2 &camera_xz, const Point3 &light_dir);

  CloudsChangeFlags render(const Point3 &main_light_dir, const Point2 &camera_xz); // todo: use main_light_dir for next temporality

  bool isConverged() const;

  UniqueTexWithShaderVar cloudsShadowsVol;

private:
  void validate();

  void addRecalcAll();
  void forceFullUpdate(const Point3 &main_light_dir, bool update_ambient);

  void initTemporal();
  bool updateTemporal();
  void updateTemporalStep(int x, int y, int z);

  UniqueTexWithShaderVar cloudsShadowsTemp;

  // Beer shadow map: per texel (front depth, mean extinction, max optical depth)
  // along the sun axis - one representation valid for any receiver altitude,
  // unlike the ground-only 2D map it replaced or ESM (format and units:
  // clouds_bsm.dshl). One 512^2 map over a 300km sun-slid camera window,
  // filtered with occupancy-aware depth dilation.
  // Texels index sun rays by their layer-BOTTOM plane crossing: rays are
  // parallel, so any anchor plane carries the same ray family, and the bottom
  // one keeps below-layer receivers (terrain, water, objects - the dominant
  // class) in-window at any sun elevation, where a top anchor needs the window
  // top_alt/tan(elevation) up-sun - unreachable at grazing sun. In/above-layer
  // receivers drift out down-sun instead; out-of-window lookups fall back to
  // the statistical coverage-mean shell transmittance - past ~100km of slant
  // the sun-disk penumbra averages per-gap structure out, so the mean is the
  // physically right answer there.
  // The bake amortizes over 2^clouds.bsm_log2_amortize_frames frames
  // (0 = instant on events): frame k of N traces Bayer slot k of every 8x8 quad
  // (one rect of N on the no-compute path) with the state frozen at attempt
  // start, a filter pass finishes the attempt, then the next N frames blend it
  // into the live map with linear 1/(N-k) weights - equal per-frame
  // contributions, the last write is exact, a window re-center rebases history
  // by a texel-snapped uv shift instead of popping
  struct BSMState
  {
    Point2 origin = {0, 0};
    Point3 sun = {0, 0, 0};
    Point2 cloudsOfs = {0, 0};
    Point2 holeOfs = {0, 0}; // the hole displaces the sampled field like cloudsOfs does
  };
  void initBSM();
  void closeBSM();
  void updateBSM(const Point2 &camera_xz, const Point3 &light_dir);
  void renderBSMFull(const BSMState &state);
  void setBSMGenVars(const BSMState &state);
  void bsmTraceFrame(int frame, int frames_num);
  void bsmFilterInto(const UniqueTex &target);
  void publishBSM(const BSMState &state, const UniqueTex &tex);
  Point2 computeBSMOrigin(const Point2 &camera_xz, const Point3 &light_dir) const;
  bool bsmStatesDiffer(const BSMState &a, const BSMState &b) const;
  UniqueTex cloudsBSM[2];     // live ping-pong: blend output
  UniqueTex cloudsBSMPub;     // THE published texture, copied from the live ping-pong:
                              // a stable id whose content never changes mid-frame -
                              // sparse temporal consumers (GI voxel relight) flicker
                              // when the published id alternates per frame
  UniqueTex cloudsBSMFilt[2]; // finished-attempt ping-pong: build target / blend source
  UniqueBuf cloudsBSMRaw;     // traced curve params, f16 triplet, 8 bytes per texel
  UniqueTex cloudsBSMRawTex;  // same payload as an RT for the no-compute trace path
  eastl::unique_ptr<ComputeShaderElement> gen_clouds_bsm_cs;
  PostFxRenderer gen_clouds_bsm_ps; // no-compute fallback: traces one rect of N per frame
  PostFxRenderer clouds_bsm_filter_ps;
  PostFxRenderer clouds_bsm_blend_ps;
  BSMState bsmLive, bsmBuild, bsmPending;
  Point3 bsmBlendSunFrom = {0, 0, 0};
  bool bsmLiveValid = false, bsmPendingValid = false, bsmNeedFull = false, bsmBuildActive = false;
  int bsmCycleFrame = 0, bsmLiveIdx = 0, bsmFiltIdx = 0, bsmLog2AmortizeFrames = 4;
  // textures, raw store, dispatch and texel snapping all derive from this;
  // must stay divisible by 8 (Bayer quads / rect split)
  int bsmResolution = 512;

  VoltexRenderer gen_cloud_shadows_volume_partial, copy_cloud_shadows_volume;
  VoltexRenderer genCloudShadowsVolume;
  Point3 lastLightDir = {0, 0, 0};
  uint32_t resetGen = 0;
  bool updateAmbient = false;

  enum
  {
    CLOUDS_MAX_SHADOW_TEMPORAL_XZ = 8,
    CLOUDS_MAX_SHADOW_TEMPORAL_Y = 8,
    CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD = CLOUDS_MAX_SHADOW_TEMPORAL_XZ * CLOUD_SHADOWS_WARP_SIZE_XZ,
    CLOUDS_MAX_SHADOW_TEMPORAL_Y_WD = CLOUDS_MAX_SHADOW_TEMPORAL_Y * CLOUD_SHADOWS_WARP_SIZE_Y,
    CLOUDS_MAX_SHADOW_TEMPORAL_XZ_STEPS =
      (CLOUD_SHADOWS_VOLUME_RES_XZ + CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD - 1) / CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD,
    CLOUDS_MAX_SHADOW_TEMPORAL_Y_STEPS =
      (CLOUD_SHADOWS_VOLUME_RES_Y + CLOUDS_MAX_SHADOW_TEMPORAL_Y_WD - 1) / CLOUDS_MAX_SHADOW_TEMPORAL_Y_WD,
  };

  enum
  {
    TEMPORAL_STEPS = CLOUDS_MAX_SHADOW_TEMPORAL_XZ_STEPS * CLOUDS_MAX_SHADOW_TEMPORAL_XZ_STEPS * CLOUDS_MAX_SHADOW_TEMPORAL_Y_STEPS,
    TEMPORAL_STEPS_LERP = 1, // can be used for smoother update light
    ALL_TEMPORAL_STEPS_LERP = TEMPORAL_STEPS * TEMPORAL_STEPS_LERP,
    TEMPORAL_STEP_FORCED = ~0u,
  };
  uint32_t temporalStep = 0, temporalStepFinal = 0;
};
