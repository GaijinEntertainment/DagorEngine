// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <rendInst/rendInstRenderJob.h>
#include <render/occlusion/parallelOcclusionRasterizer.h>
#include <render/occlusion/occlusionMaskApplier.h>

#include <render/world/animCharRenderJob.h>
#include <render/world/cameraParams.h>
#include <render/world/occlusionLandMeshManager.h>
#include <render/world/visibilityJobs.h>


struct RiGenVisibility;
class ParallelOcclusionRasterizer;
class OcclusionLandMeshManager;
struct CameraParams;
class LandMeshManager;

namespace rendinst
{
struct RIOcclusionData;
}
struct OcclusionEllipse;

class CameraViewVisibilityMgr
{
public:
  constexpr static bool SUB_VIEW = true;
  CameraViewVisibilityMgr(const int ri_extra_vb_ctx_id, const char *cam_prefix, const bool is_sub_view = false) :
    riExtraRenderJob(ri_extra_vb_ctx_id), cameraPrefix(cam_prefix), isSubView(is_sub_view)
  {}

  ~CameraViewVisibilityMgr() { close(); }
  CameraViewVisibilityMgr &operator=(const CameraViewVisibilityMgr &) = delete;

  void init();

  void close();

  void runOcclusionRasterizerCleaners();

  void prepareLightProbeRIVisibilityAsync(const mat44f &globtm, const Point3 &view_pos);

  void startOcclusionAndSwRaster(rendinst::RIOcclusionData &ri_occlusion_data,
    const CameraParams &cur_frame_cam,
    const LandMeshManager *lmesh_mgr,
    mat44f_cref cockpit_proj);

  void waitVisibilityReproject();

  void startOcclusionFrame(
    const TMatrix &itm, const mat44f &viewTm, const mat44f &projTm, const mat44f &globtm, mat44f_cref cockpit_proj);

  void startVisibilityReproject();

  // start async visibility reproject job
  void startSwRasterization(rendinst::RIOcclusionData &ri_occlusion_data,
    mat44f_cref globtm,
    mat44f_cref view,
    const Driver3dPerspective &p,
    const LandMeshManager *lmesh_mgr);

  void startVisibility(const CameraParams &cur_frame_cam,
    const PartitionSphere &transparent_partition_sphere,
    const IPoint2 &rendering_resolution,
    const ShadowVisibilityContext &shadow_ctx);

  void startGroundVisibility(LandMeshManager *lmesh_mgr,
    LandMeshRenderer *lmesh_renderer,
    const float water_level,
    const int displacement_sub_div,
    const float displacement_radius,
    const CameraParams &cur_frame_camera);

  void startGroundReflectionVisibility(LandMeshManager *lmesh_mgr,
    LandMeshRenderer *lmesh_renderer,
    const Frustum &frustum,
    const Point3 &viewPos,
    const TMatrix4 &viewProj,
    const float water_level);

  void startLightsCullingJob(const CameraParams &cur_frame_camera, const bool use_occlusion_culling = true);

  void waitVisibility(int cascade);
  void waitMainVisibility();
  void waitGroundVisibility();
  void waitGroundReflectionVisibility();
  void waitLights();
  void waitLightProbeVisibility();
  void waitAllJobs();
  void waitOcclusionAsyncReadbackGpu();

  RiGenVisibility *getRiMainVisibility() const { return rendinstMainVisibility; }
  Occlusion *getOcclusion();
  const Occlusion *getOcclusion() const;

  void prepareToStartAsyncRIGenExtraOpaqueRender(const int frame_stblk, const TexStreamingContext texCtx, const bool enable);
  void waitAsyncRIGenExtraOpaqueRenderVbFill();
  rendinst::render::RiExtraRenderer *waitAsyncRIGenExtraOpaqueRender();

  const LandMeshCullingData &getLandMeshCullingData() const { return lmeshCullingData; }
  const LandMeshCullingData &getLandMeshReflectionCullingData() const { return lmeshReflectionCullingData; }

  void startAsyncAnimcharMainRender(const Frustum &fg_blob_frustum,
    uint32_t hints,
    TexStreamingContext tex_ctx,
    const TMatrix4 &view_tm,
    const TMatrix4 &proj_tm,
    const TMatrix4 &prev_view_tm,
    const TMatrix4 &prev_proj_tm);
  void waitAsyncAnimcharMainRender();
  dynrend::ContextId getAsyncAnimcharMainRenderCtx() { return animcharMainRenderJobs[0].dynCtx; }

  void updateLodsScaling(const float ri_dist_mul, const float impostors_dist_mul);

  void setCockpitReprojectionMode(CockpitReprojectionMode mode) { cockpitMode = mode; }

private:
  bool isSubView = false;
  CockpitReprojectionMode cockpitMode = COCKPIT_REPROJECT_ANIMATED;
  const char *cameraPrefix = nullptr;

  Occlusion occlusion;
  OcclusionMaskApplier occlusionMaskApplier;
  eastl::unique_ptr<ParallelOcclusionRasterizer> occlusionRasterizer;
  eastl::unique_ptr<OcclusionLandMeshManager> occlusionLandMeshManager;

  LandMeshCullingData lmeshCullingData;
  LandMeshCullingData lmeshReflectionCullingData;

  RiGenVisibility *rendinstMainVisibility = nullptr;

  RenderRiExtraJob riExtraRenderJob;

  VisibilityReprojectJob visibilityReprojectJob;
  OcclusionReadbackGPUJob occlusionReadbackGpuJob;
  carray<RendinstVisibilityPrepareJob, MAX_JOB_COUNT> rendinstVisibilityJobs;
  carray<RendinstExtraVisibilityPrepareJob, MAX_JOB_COUNT> rendinstExtraVisibilityJobs;
  VisibilityPrepareJob visibilityJob;
  LodsByDistanceJob lodsByDistanceJob;
  GroundCullingJob groundCullJob;
  GroundReflectionCullingJob groundReflectionCullJob;
  LightsCullingJob lightsCullJob;
  LightProbeVisibilityJob lightProbeVisibilityJob;

  const VisibilityJobsContext jobsCtx{&visibilityJob, &lodsByDistanceJob, &visibilityReprojectJob, &rendinstVisibilityJobs[0],
    &rendinstExtraVisibilityJobs[0], &riExtraRenderJob};

  AnimcharRenderMainJobCtx animcharMainRenderJobCtx;
  AnimcharRenderMainJob animcharMainRenderJobs[AnimcharRenderAsyncFilter::ARF_IDX_COUNT];
};