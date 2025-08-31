// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <landMesh/lmeshCulling.h>

#include <drv/3d/dag_decl.h>
#include <math/dag_Point3.h>
#include <math/dag_frustum.h>
#include <math/integer/dag_IPoint2.h>
#include <util/dag_threadPool.h>
#include <vecmath/dag_vecMath.h>

#include <EASTL/optional.h>

#include <render/world/partitionSphere.h>


class CascadeShadows;
class Occlusion;
class ParallelOcclusionRasterizer;
class ShadowsManager;
struct RiGenVisibility;
struct CameraParams;

struct ShadowVisibilityContext
{
  CascadeShadows *csm = nullptr;
  RiGenVisibility **rendinstShadowsVisibility = nullptr;
  ShadowsManager *shadowsManager = nullptr;
};

struct VisibilityJobsContext
{
  struct VisibilityPrepareJob *visibilityJob;
  struct LodsByDistanceJob *lodsByDistanceJob;
  struct VisibilityReprojectJob *visibilityReprojectJob;
  struct RendinstVisibilityPrepareJob *rendinstVisibilityJobs;
  struct RendinstExtraVisibilityPrepareJob *rendinstExtraVisibilityJobs;
  struct RenderRiExtraJob *riExtraRenderJob;
};

struct VisibilityReprojectJob final : public cpujobs::IJob
{
  static constexpr threadpool::JobPriority prio = threadpool::PRIO_NORMAL;
  Occlusion *occlusion;

  void start(Occlusion *occl);
  const char *getJobName(bool &) const override { return "VisibilityReprojectJob"; }
  void doJob() override;
};

struct OcclusionReadbackGPUJob final : public cpujobs::IJob
{
  static constexpr threadpool::JobPriority prio = threadpool::PRIO_NORMAL;
  const VisibilityJobsContext *jobs = nullptr;
  Occlusion *occlusion = nullptr;

  void start(const VisibilityJobsContext *jobs, Occlusion *occl);
  const char *getJobName(bool &) const override { return "OcclusionReadbackGPUJob"; }
  virtual void doJob() override;
  void performJob();
};

enum
{
  RENDER_MAIN_JOB = 0,
  RENDER_CSM_JOB = 1,
  MAX_JOB_COUNT = 9
};
struct RendinstVisibilityPrepareJob final : public cpujobs::IJob
{
  Occlusion *occlusion;
  Frustum frustum;
  Point3 viewPos = {0, 0, 0};
  RiGenVisibility *v = nullptr;
  bool shadow = false;
  threadpool::JobPriority prio;

  void start(Occlusion *occl,
    const Frustum &frustum_,
    const Point3 &viewPos_,
    RiGenVisibility *v_,
    bool shadow_,
    threadpool::JobPriority prio_);

  const char *getJobName(bool &) const override { return "RendinstVisibilityPrepareJob"; }
  void doJob() override;
};

constexpr bool RI_EXTRA_VIS_MAIN_JOB = true;
struct RenderRiExtraJob;
struct RendinstExtraVisibilityPrepareJob final : public cpujobs::IJob
{
  const VisibilityJobsContext *jobs;
  bool isMainJob = false;
  Occlusion *occlusion;
  mat44f culling;
  Point3 viewPos = Point3::ZERO;
  PartitionSphere transparentPartitionSphere;
  eastl::optional<IPoint2> target = {};
  RiGenVisibility *v = nullptr;
  bool shadow = false;
  threadpool::JobPriority prio;

  void start(const VisibilityJobsContext *jobs,
    Occlusion *occl,
    mat44f_cref culling_,
    const Point3 &viewPos_,
    const PartitionSphere &transparentPartitionSphere_,
    RiGenVisibility *v_,
    bool shadow_,
    threadpool::JobPriority prio_,
    eastl::optional<IPoint2> target_ = {},
    bool is_main_job = false);
  const char *getJobName(bool &) const override { return "RendinstExtraVisibilityPrepareJob"; }
  void doJob() override;
};

struct VisibilityPrepareJob final : public cpujobs::IJob
{
  const VisibilityJobsContext *jobs = nullptr;
  Occlusion *occlusion = nullptr;
  ParallelOcclusionRasterizer *occlusionRasterizer = nullptr;
  RiGenVisibility *rendinstMainVisibility = nullptr;
  TMatrix itm;
  mat44f globtm, view, proj;
  Driver3dPerspective persp;
  PartitionSphere transparentPartitionSphere;
  bool hasGpuDepth = false;
  ShadowVisibilityContext shadowCtx;
  eastl::optional<IPoint2> target = {};

  void start(const VisibilityJobsContext *jobs,
    Occlusion *occl,
    ParallelOcclusionRasterizer *occlusion_rasterizer,
    RiGenVisibility *rendinst_main_visibility,
    const CameraParams &cur_frame_camera,
    const PartitionSphere &transparentPartitionSphere_,
    const IPoint2 &rendering_resolution,
    const ShadowVisibilityContext &shadow_ctx);
  const char *getJobName(bool &) const override { return "VisibilityPrepareJob"; }
  void doJob() override;
  void prepareVisibility();
  void finalize();
};

struct LodsByDistanceJob final : public cpujobs::IJob
{
  static constexpr threadpool::JobPriority prio = threadpool::PRIO_LOW;
  Point3 cameraPos;

  void start(const Point3 &camera_pos);
  const char *getJobName(bool &) const override { return "LodsByDistanceJob"; }
  void doJob() override;
};

struct GroundCullingJob final : public cpujobs::IJob
{
  const VisibilityJobsContext *jobs = nullptr;
  LandMeshCullingState lmeshCullingState;
  LandMeshCullingData *lmeshCullingData = nullptr;
  LandMeshManager *lmeshMgr = nullptr;
  LandMeshRenderer *lmeshRenderer = nullptr;
  Occlusion *occlusion = nullptr;
  Frustum frustum;
  TMatrix4 viewProj = TMatrix4::IDENT;
  Point3 viewPos = {0, 0, 0};
  float hmapCameraHeight = 0.0f;
  float waterLevel = 0.0f;
  int displacementSubDiv = 1;

  void start(const VisibilityJobsContext *jobs,
    LandMeshCullingData *culling_data,
    LandMeshManager *lmesh_mgr,
    LandMeshRenderer *lmesh_renderer,
    Occlusion *occlusion,
    const Frustum &frustum_,
    const Point3 &viewPos_,
    const TMatrix4 &viewProj_,
    const float hmap_camera_height,
    const float water_level,
    const int displacement_sub_div,
    threadpool::JobPriority prio);
  const char *getJobName(bool &) const override { return "GroundCullingJob"; }
  void doJob() override;
};

struct GroundReflectionCullingJob final : public cpujobs::IJob
{
  const VisibilityJobsContext *jobs = nullptr;
  LandMeshCullingState lmeshCullingState;
  LandMeshCullingData *lmeshCullingData = nullptr;
  LandMeshManager *lmeshMgr = nullptr;
  LandMeshRenderer *lmeshRenderer = nullptr;
  Frustum frustum;
  TMatrix4 viewProj = TMatrix4::IDENT;
  Point3 viewPos = {0, 0, 0};
  float hmapCameraHeight = 0.0f;
  float waterLevel = 0.0f;

  void start(const VisibilityJobsContext *jobs,
    LandMeshCullingData *culling_data,
    LandMeshManager *lmesh_mgr,
    LandMeshRenderer *lmesh_renderer,
    const Frustum &frustum_,
    const Point3 &viewPos_,
    const TMatrix4 &viewProj_,
    const float hmap_camera_height,
    const float water_level,
    threadpool::JobPriority prio);
  const char *getJobName(bool &) const override { return "GroundReflectionCullingJob"; }
  virtual void doJob() override;
};

struct LightsCullingJob final : public cpujobs::IJob
{
  Occlusion *occlusion;
  mat44f globtm, view, proj;
  vec4f viewPos;
  float zn = 0.1;

  void start(Occlusion *occlusion, const CameraParams &cur_frame_camera, threadpool::JobPriority prio);
  const char *getJobName(bool &) const override { return "LightsCullingJob"; }
  void doJob() override;
};

struct LightProbeVisibilityJob final : public cpujobs::IJob
{
  mat44f globtm;
  Point3 viewPos;

  void start(const mat44f &globtm_, const Point3 &view_pos);
  const char *getJobName(bool &) const override { return "LightProbeVisibilityJob"; }
  virtual void doJob() override;
};
