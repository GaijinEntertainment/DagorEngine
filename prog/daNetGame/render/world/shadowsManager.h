// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <cinttypes>
#include <mutex>
#include <EASTL/span.h>
#include <dag/dag_vector.h>
#include <memory/dag_framemem.h>
#include <3d/dag_resPtr.h>

#include <render/cascadeShadows.h>
#include <render/toroidalStaticShadows.h>
#include <render/gpuVisibilityTest.h>
#include <render/rendererFeatures.h>
#include <render/daBfg/bfg.h>

#include <shaders/dag_postFxRenderer.h>

#include "cameraParams.h"
#include "rendinstShadowCullBboxesLoader.h"
#include <rendInst/visibilityDecl.h>


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
  virtual bool isForward() const = 0;
  virtual BBox3 getStaticShadowsBBox() const = 0;
  virtual float getWaterLevel() const = 0;
  virtual float getCameraHeight() const = 0;
  virtual const DataBlock *getLevelSettings() const = 0;
  virtual int getShadowFramesCount() const = 0;
  virtual int getTemporalShadowFramesCount() const = 0;
  virtual bool isTimeDynamic() const = 0;

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
    MAX_NUM_STATIC_SHADOWS_CASCADES = 2,
    MAX_NUM_STATIC_SHADOWS_VISIBILITY_JOBS = 16,
  };

  ShadowsManager(IShadowInfoProvider &provider, ClusteredLights &lights);

  void renderGroundShadows(const Point3 &origin, int displacement_subdiv, const Frustum &culling_frustum);
  void renderStaticShadows(
    const mat44f &culling_view_proj, const TMatrix4 &shadow_glob_tm, const TMatrix &view_itm, int cascade, int region);
  void renderShadowsOpaque(int shadow_cascade, bool only_dynamic, const TMatrix &itm, const Point3 &cam_pos);

  RiGenVisibility *initOneStaticShadowsVisibility();
  void closeAllStaticShadowsVisibility();

  void setShadowFrameIndex();
  void updateCsmData();
  void combineShadows();

  void staticShadowsSetWorldSize();
  void restoreShadowSampler();

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

  void shadowsInvalidate(bool force);
  void shadowsInvalidate(const BBox3 *boxes, uint32_t cnt);
  void shadowsInvalidate(const BBox3 &box) { shadowsInvalidate(&box, 1); }
  void shadowsAddInvalidBBox(const BBox3 &box);
  void shadowsInvalidateGatheredBBoxes();
  bbox3f getActiveShadowVolume() const;

  bool hasRIShadowOcclusion() const;
  int gatherAndTryAddRiForShadowsVisibilityTest(mat44f_cref globtm_cull, const ViewTransformData &transform);
  bbox3f gatherBboxesForAnimcharShadowCull(const Point3 &cam_pos,
    dag::Vector<GpuVisibilityTestManager::TestObjectData, framemem_allocator> &out_bboxes,
    uint32_t usr_data) const;

  float getStaticShadowsSmallestTexelSize() const;
  float getStaticShadowsBiggestTexelSize() const;
  int getStaticShadowsCascadesCount() const;
  void getStaticShadowsBoxViews(const BBox3 &box, ViewTransformData *out_views, int &out_cnt) const;
  bool getStaticShadowsCascadeWorldBoxViews(int cascade_id, ViewTransformData *out_views, int &out_cnt) const;

  Point4 getCascadeShadowAnchor(int cascade_no) override;
  void renderCascadeShadowDepth(int cascade_no, const Point2 &znzf) override;

  void testAnimcharTestBboxes(const Point3 &cam_pos, int frame_no);
  void processShadowsVisibility(const Point3 &cam_pos);
  void resetShadowsVisibilityTesting();
  int getVisibilityTestAvailableSpace() const { return shadowsGpuOcclusionMgr.getAvailableSpace(); }
  BaseTexture *getStaticShadowsTex();
  // Sphere is inside static shadows
  bool insideStaticShadows(const Point3 &pos, float radius) const;
  // Sphere is inside valid static shadows.
  // Valid means something was ever rendered to it
  bool validStaticShadows(const Point3 &pos, float radius) const;
  // Sphere is inside valid static shadows.
  // Valid means something was ever rendered to it and
  // it wasnt invalidated after that
  bool fullyUpdatedStaticShadows(const Point3 &pos, float radius) const;

  bool updateStaticShadowAround(const Point3 &pos, bool update_only_last_cascade); // Return true if some tp jobs were added
  void renderStaticShadows();

  // not rendering, will be called from thread
  void prepareShadowsMatrices(
    const TMatrix &itm, const Driver3dPerspective &p, const TMatrix4 &proj_tm, const Frustum &frustum, float cascade0_dist);
  shaders::OverrideState getStaticShadowsOverrideState() const;
  void initStaticShadow();
  void closeStaticShadow();

  void setAnimcharShadowCullBboxes(dag::Span<bbox3f> tested_bboxes, const Point3 &cam_pos);
  void invalidateAnimCharShadowOcclusionBox(const BBox3 &box);
  void invalidateAnimCharShadowOcclusion();

  // May return nullptr
  CascadeShadows *getCascadeShadows() { return csm; }

  void getCascadeShadowSparseUpdateParams(
    int /*cascade_no*/, const Frustum &, float &out_min_sparse_dist, int &out_min_sparse_frame) override;

  const ToroidalStaticShadows *getStaticShadows() const { return staticShadows.get(); }
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

public:
  float csmStartOffsetDistance = 0.0f;
  float staticShadowsAdditionalHeight = 0.f;

private:
  struct StaticShadowCallback;

  // Lower shadow quality below this sunDir.y, to not render too many rendinsts into shadow (except on ultra)
  float shadowQualityLoweringThreshold = 0.25f;

  void initDynamicShadows();

  Point3 getThisFrameCameraPos() const { return shadowInfoProvider.getCurrentFrameCameraParams().viewItm.getcol(3); }

private:
  IShadowInfoProvider &shadowInfoProvider;
  ClusteredLights &clusteredLights;

  ShadowsQuality shadowsQuality = ShadowsQuality::SHADOWS_HIGH;
  ShadowsQuality settingsShadowQuality = ShadowsQuality::SHADOWS_HIGH;

  CascadeShadows *csm = nullptr;
  CascadeShadows::ModeSettings csm_mode;
  float csmShadowsMaxDist = 50;

  float staticShadowMaxUpdateAmount = 0.1;
  bool staticShadowUniformUpdate = false;
  eastl::unique_ptr<ToroidalStaticShadows> staticShadows;
  dabfg::NodeHandle staticShadowRenderNode;
  bool staticShadowsSetShaderVars = false;
  shaders::UniqueOverrideStateId staticShadowsOverride;
  shaders::UniqueOverrideStateId staticShadowsOverrideFlipCull;
  bool isTimeDynamic;

  PostFxRenderer shadowsDownsample;
  PostFxRenderer combine_shadows;

  struct
  {
    dag::Vector<BBox3> list;
    uint64_t lastApplyTicks = 0;
    std::mutex lock;
  } staticShadowInvalidBboxes;

  // Last cascade is dynamic only even on high and ultra
  static constexpr int CSM_STATIC_CASCADE_COUNT = 3;

  GpuVisibilityTestManager shadowsGpuOcclusionMgr;
  WinCritSec shadowsGpuOcclusionMgrMutex;
  RiShadowCullBboxesLoaderJob riShadowCullBboxesLoader;

  dabfg::NodeHandle processVisibilityNode;
  dabfg::NodeHandle prepareDownsampledShadowsNode;
  dabfg::NodeHandle combineShadowsNode;
};

bool combined_shadows_use_additional_textures();
void combined_shadows_bind_additional_textures(dabfg::Registry &registry);
