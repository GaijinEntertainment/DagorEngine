// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <cinttypes>
#include <EASTL/span.h>
#include <dag/dag_vector.h>
#include <memory/dag_framemem.h>
#include <3d/dag_resPtr.h>

#include <render/cascadeShadows.h>
#include <render/voxelShadows.h>
#include <render/gpuVisibilityTest.h>
#include <render/rendererFeatures.h>
#include <render/daFrameGraph/daFG.h>

#include <shaders/dag_postFxRenderer.h>

#include <render/cameraParams.h>
#include "rendinstShadowCullBboxesLoader.h"
#include <rendInst/visibilityDecl.h>
#include <render/variance.h>


struct ClusteredLights;

enum class ShadowsQuality : int
{
  SHADOWS_MIN,
  SHADOWS_LOW,
  SHADOWS_MEDIUM,
  SHADOWS_HIGH,
  SHADOWS_ULTRA_HIGH,
  SHADOWS_STATIC_ONLY
};

// Temporary solution to decouple this and worldRenderer
struct IShadowInfoProvider
{
  enum class DirToSunType
  {
    STATIC,
    CSM
  };
  virtual const CameraParams &getCurrentFrameCameraParams() const = 0;
  virtual Point3 getDirToSun(DirToSunType type) const = 0;
  virtual eastl::span<RiGenVisibility *> getRendinstShadowVisibilities() = 0;
  virtual bool hasRenderFeature(FeatureRenderFlags) const = 0;
  virtual BBox3 getStaticShadowsBBox() const = 0;
  virtual float getWaterLevel() const = 0;
  virtual float getCameraHeight() const = 0;
  virtual const DataBlock *getLevelSettings() const = 0;
  virtual int getShadowFramesCount() const = 0;
  virtual int getTemporalShadowFramesCount() const = 0;

  // Callbacks to actually render stuff. Should be replaced with FG
  // eventually.
  virtual void renderStaticSceneForShadowPass(
    int cascade, const Point3 &camera_pos, const TMatrix &view_itm, const Frustum &culling_frustum) = 0;
  virtual void renderDynamicsForShadowPass(int cascade, const TMatrix &itm, const Point3 &cam_pos) = 0;
};

class ShadowsManager final : public ICascadeShadowsClient
{
public:
  enum
  {
    CSM_MAX_CASCADES = 4,
    CSM_DYNAMIC_ONLY_CASCADE = 3,
  };

  ShadowsManager(IShadowInfoProvider &provider, ClusteredLights &lights);
  ~ShadowsManager();

  void renderGroundShadows(const Point3 &origin, int displacement_subdiv, const Frustum &culling_frustum, const TMatrix4 &view_proj);
  void renderShadowsOpaque(int shadow_cascade, bool only_dynamic, const TMatrix &itm, const Point3 &cam_pos);

  // also drops the riExtra collision data: pool ids are per level
  void closeAllStaticShadowsVisibility();

  void setShadowFrameIndex(const CameraParams &cur_frame_camera);
  void resetShadowFrame();
  void updateCsmData(const CameraParams &cur_frame_camera);
  void combineShadows(const dafg::multiplexing::Index multiplex_index, const CameraParams &cur_frame_camera);

  void staticShadowsSetWorldSize();
  void markWorldBBoxDirty();

  void initVisibilityNode();
  void initShadowsDownsampleNode();
  void initCombineShadowsNode();


  void initShadows();
  void closeShadows();
  void setNeedSsss(bool need_ssss);
  void changeSpotShadowBiasBasedOnQuality();

  void updateShadowsQFromSun();
  void updateShadowsQFromSunAndReinit();
  void initShadowsSettings();

  // force: the old data goes too, as for a new level
  void shadowsInvalidate(bool force);
  void shadowsInvalidate(const BBox3 *boxes, uint32_t cnt);
  void shadowsInvalidate(const BBox3 &box) { shadowsInvalidate(&box, 1); }
  bbox3f getActiveShadowVolume() const;

  bool hasRIShadowOcclusion() const;
  int gatherAndTryAddRiForShadowsVisibilityTest(mat44f_cref globtm_cull, float texel_size);
  void gatherBboxesForAnimcharShadowCull(const Point3 &cam_pos,
    dag::Vector<GpuVisibilityTestManager::TestObjectData, framemem_allocator> &out_bboxes,
    uint32_t usr_data) const;

  float getStaticShadowsBiggestTexelSize() const;

  Point4 getCascadeShadowAnchor(int cascade_no) override;
  void renderCascadeShadowDepth(int cascade_no, const Point2 &znzf) override;

  void testAnimcharTestBboxes(const Point3 &cam_pos, int frame_no);
  void testStaticShadowOcclusion();
  void resetShadowsVisibilityTesting();
  int getVisibilityTestAvailableSpace() const { return shadowsGpuOcclusionMgr.getAvailableSpace(); }
  // data on the covered pages, maybe stale
  bool validStaticShadows(const Point3 &pos, float cube_size) const;
  // valid, with nothing left to build
  bool fullyUpdatedStaticShadows(const Point3 &pos, float cube_size) const;
  uint32_t getStaticShadowsConvergedFrames() const;

  bool updateStaticShadowAround(const Point3 &pos); // Return true if some tp jobs were added
  void renderStaticShadows();
  // screen_mode > 0 and atlas_level >= 0 draw debug views
  void debugRenderStaticShadows(int screen_mode, int atlas_level);

  // not rendering, will be called from thread
  void prepareShadowsMatrices(
    const TMatrix &itm, const Driver3dPerspective &p, const TMatrix4 &proj_tm, const Frustum &frustum, float cascade0_dist);
  void initStaticShadow();
  void closeStaticShadow();

  void setAnimcharShadowCullBboxes(dag::Span<bbox3f> tested_bboxes, const Point3 &cam_pos);
  void invalidateAnimCharShadowOcclusionBox(const BBox3 &box);
  void invalidateAnimCharShadowOcclusion();

  // May return nullptr
  CascadeShadows *getCascadeShadows() { return csm; }

  void getCascadeShadowSparseUpdateParams(
    int /*cascade_no*/, const Frustum &, float &out_min_sparse_dist, int &out_min_sparse_frame) override;

  float getCsmShadowsMaxDist() const { return csmShadowsMaxDist; }

  ShadowsQuality getSettingsQuality() const { return settingsShadowQuality; }

  void afterDeviceReset();

  // Turns on at high and ultra settings, off otherwise, can be called if it's enabled already
  void conditionalEnableGPUObjsCSM();
  void disableGPUObjsCSM();


  rendinst::VisibilityRenderingFlags getCsmVisibilityRendering(int cascade) const;
  bool shouldRenderCsmRendinst(int cascade) const;
  bool shouldRenderCsmStatic(int cascade) const;
  bool anyCsmCascadeRendersStatic() const;

  void initVSM();
  void prepareVSM(const Point3 &camera_pos);
  void closeVSM();

  void prepareVoxelShadows(const Point3 &camera_pos);
  void debugRenderVoxelShadows();

public:
  float csmStartOffsetDistance = 0.0f;
  float staticShadowsAdditionalHeight = 0.f;

private:
  struct StaticShadowCallback;
  struct StaticShadows;

  // Lower shadow quality below this sunDir.y, to not render too many rendinsts into shadow (except on ultra)
  float shadowQualityLoweringThreshold = 0.25f;

  void initDynamicShadows();
  void initVoxelShadows();
  void initParallaxShadows();

  Point3 getThisFrameCameraPos() const { return shadowInfoProvider.getCurrentFrameCameraParams().viewItm.getcol(3); }

private:
  IShadowInfoProvider &shadowInfoProvider;
  ClusteredLights &clusteredLights;

  ShadowsQuality shadowsQuality = ShadowsQuality::SHADOWS_HIGH;
  ShadowsQuality settingsShadowQuality = ShadowsQuality::SHADOWS_HIGH;
  uint32_t shadowFrameBase = 0;

  CascadeShadows *csm = nullptr;
  CascadeShadows::ModeSettings csm_mode;
  float csmShadowsMaxDist = 50;
  bool csmShadowsDistanceFovScaling = true;
  dafg::NodeHandle csmExportResourcesNode;

  eastl::unique_ptr<VoxelShadows> voxelShadows;

  bool worldBBoxDirty = false;
  eastl::unique_ptr<StaticShadows> staticShadows;
  dafg::NodeHandle staticShadowRenderNode;

  static constexpr float VSM_SHADOW_DISTANCE = 20000;
  static constexpr float VSM_UPDATE_THRESHOLD_SQ = sqr(VSM_SHADOW_DISTANCE * 0.1);
  static constexpr int VSM_TEXTURE_SIZE = 1024;
  Variance vsm;

  PostFxRenderer shadowsDownsample;
  PostFxRenderer combine_shadows;

  // Last cascade is dynamic only even on high and ultra
  static constexpr int CSM_STATIC_CASCADE_COUNT = 3;

  GpuVisibilityTestManager shadowsGpuOcclusionMgr;
  WinCritSec shadowsGpuOcclusionMgrMutex;
  RiShadowCullBboxesLoaderJob riShadowCullBboxesLoader;

  dafg::NodeHandle staticShadowsVisibilityNodes[2];
  dafg::NodeHandle prepareDownsampledShadowsNode;
  dafg::NodeHandle combineShadowsResProviderNode;
  dafg::NodeHandle combineShadowsNode;
};

bool combined_shadows_use_additional_textures();
void combined_shadows_bind_additional_textures(dafg::Registry &registry);
