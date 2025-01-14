// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1 // fixme: move to jam

#include "render/renderer.h"
#include "private_worldRenderer.h"
#include <ecs/render/renderPasses.h>
#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstGenRender.h>
#include <rendInst/visibility.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraRender.h>
#include <scene/dag_occlusion.h>
#include <util/dag_threadPool.h>
#include <osApiWrappers/dag_miscApi.h>
#include <math/dag_frustum.h>
#include <perfMon/dag_statDrv.h>
#include <perfMon/dag_cpuFreq.h>
#include <util/dag_convar.h>
#include <landMesh/lmeshManager.h>
#include <render/world/occlusionLandMeshManager.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include "render/world/parallelOcclusionRasterizer.h"
#include "render/world/worldRendererQueries.h"
#include "depthAOAbove.h"

extern ConVarB stop_occlusion;
extern ConVarB use_occlusion;
CONSOLE_BOOL_VAL("occlusion", use_hw_reprojection, true);

CONSOLE_BOOL_VAL("render", sw_occlusion, true);
CONSOLE_BOOL_VAL("render", sw_hmap_occlusion, true);
CONSOLE_BOOL_VAL("render", sw_animchars_occlusion, true);
CONSOLE_BOOL_VAL("render", occlusion_async_readback_gpu, true);

extern void wait_static_shadows_cull_jobs();

static ShaderVariableInfo cockpit_distanceVarId("cockpit_distance", true);

static struct VisibilityReprojectJob final : public cpujobs::IJob
{
  static constexpr threadpool::JobPriority prio = threadpool::PRIO_NORMAL;
  void start()
  {
    G_FAST_ASSERT(current_occlusion);
    uint32_t _1;
    // It might be already set to not done by `occlusion_readback_gpu_job`
    threadpool::add(this, prio, _1, threadpool::AddFlags::IgnoreNotDone);
  }
  virtual void doJob() override
  {
    if (current_occlusion)
      current_occlusion->reprojectGPUFrame();
  }
} visibility_reproject_job;

static struct OcclusionReadbackGPUJob final : public cpujobs::IJob
{
  static constexpr threadpool::JobPriority prio = threadpool::PRIO_NORMAL;
  void start()
  {
    G_FAST_ASSERT(current_occlusion);
    threadpool::wait(this);
    if (sw_occlusion.get())
    {
      threadpool::wait(&visibility_reproject_job);
      interlocked_release_store(visibility_reproject_job.done, 0); // To be able wait one job instead of pair
    }
    threadpool::add(this, prio, false);
  }
  virtual void doJob() override
  {
    current_occlusion->readbackGPU();
    if (sw_occlusion.get())
      visibility_reproject_job.start();
  }
  void performJob()
  {
    doJob();
    done = 1;
  }
} occlusion_readback_gpu_job;

void wait_occlusion_async_readback_gpu()
{
#if TIME_PROFILER_ENABLED
  if (interlocked_acquire_load(occlusion_readback_gpu_job.done))
    return;
  TIME_PROFILE(wait_occlusion_readback_gpu);
#endif
  threadpool::wait(&occlusion_readback_gpu_job);
}

struct VisibilityContext
{
  ParallelOcclusionRasterizer *occlusionRasterizer;
  CascadeShadows *csm;
  RiGenVisibility **rendinstShadowsVisibility;
  RiGenVisibility *rendinstMainVisibility;
  ShadowsManager *shadowsManager;
};

static void prepare_visibility(bool hasGpuDepth, const Driver3dPerspective &p, VisibilityContext &ctx);

static struct VisibilityPrepareJob final : public cpujobs::IJob
{
  TMatrix itm;
  mat44f globtm, view, proj;
  Driver3dPerspective persp;
  PartitionSphere transparentPartitionSphere;
  bool hasGpuDepth = false;
  VisibilityContext ctx;
  eastl::optional<IPoint2> target = {};
  void start(const TMatrix &itm_,
    mat44f_cref globtm_,
    mat44f_cref view_,
    mat44f_cref proj_,
    const Driver3dPerspective &p,
    const PartitionSphere &transparentPartitionSphere_)
  {
    threadpool::wait(this, 0, threadpool::NUM_PRIO);
    itm = itm_;
    globtm = globtm_;
    view = view_;
    proj = proj_;
    persp = p;
    transparentPartitionSphere = transparentPartitionSphere_;
    if (get_world_renderer())
    {
      target = IPoint2(0, 0);
      get_world_renderer()->getRenderingResolution(target->x, target->y);
      ((WorldRenderer *)get_world_renderer())->fillVisibilityContext(ctx);
      threadpool::add(this, visibility_reproject_job.prio);
    }
  }
  void doJob() override
  {
    prepare_visibility(hasGpuDepth, persp, ctx);
    finalize();
  }
  void finalize();
  void startLightsCullingJob();
} visibility_job;

static struct GroundCullingJob final : public cpujobs::IJob
{
  Frustum frustum;
  Point3 viewPos = {0, 0, 0};
  void start(const Frustum &frustum_, const Point3 &viewPos_, threadpool::JobPriority prio)
  {
    threadpool::wait(this, 0, threadpool::NUM_PRIO);
    frustum = frustum_;
    viewPos = viewPos_;
    threadpool::add(this, prio, /*wake*/ false);
  }
  virtual void doJob() override
  {
    threadpool::wait(&visibility_job, 0, threadpool::NUM_PRIO);
    if (get_world_renderer())
      ((WorldRenderer *)get_world_renderer())->prepareGroundVisibilityMain(frustum, viewPos);
  }
} ground_cull_job;

static struct GroundReflectionCullingJob final : public cpujobs::IJob
{
  Frustum frustum;
  Point3 viewPos = {0, 0, 0};
  void start(const Frustum &frustum_, const Point3 &viewPos_, threadpool::JobPriority prio)
  {
    threadpool::wait(this, 0, threadpool::NUM_PRIO);
    frustum = frustum_;
    viewPos = viewPos_;
    threadpool::add(this, prio, /*wake*/ false);
  }
  virtual void doJob() override
  {
    threadpool::wait(&visibility_job, 0, threadpool::NUM_PRIO);
    if (get_world_renderer())
      ((WorldRenderer *)get_world_renderer())->prepareGroundVisibilityReflection(frustum, viewPos);
  }
} ground_reflection_cull_job;

enum
{
  RENDER_MAIN_JOB = 0,
  RENDER_CSM_JOB = 1,
  MAX_JOB_COUNT = 9
};
struct RendinstVisibilityPrepareJob final : public cpujobs::IJob
{
  Frustum frustum;
  Point3 viewPos = {0, 0, 0};
  RiGenVisibility *v = nullptr;
  bool shadow = false;
  threadpool::JobPriority prio;
  void start(const Frustum &frustum_, const Point3 &viewPos_, RiGenVisibility *v_, bool shadow_, threadpool::JobPriority prio_)
  {
    if (!v_)
      return;
    threadpool::wait(this, 0, threadpool::NUM_PRIO);
    frustum = frustum_;
    viewPos = viewPos_;
    v = v_;
    shadow = shadow_;
    prio = prio_;
    threadpool::add(this, prio, false);
  }
  virtual void doJob() override
  {
    // int64_t reft = ref_time_ticks();
    rendinst::prepareRIGenVisibility(frustum, viewPos, v, shadow, shadow ? nullptr : current_occlusion);
    // debug("rendinst in %dus", get_time_usec(reft));
  }
};

static carray<RendinstVisibilityPrepareJob, MAX_JOB_COUNT> rendinst_visibility_job;
struct RendinstExtraVisibilityPrepareJob final : public cpujobs::IJob
{
  mat44f culling;
  Point3 viewPos = Point3::ZERO;
  PartitionSphere transparentPartitionSphere;
  eastl::optional<IPoint2> target = {};
  RiGenVisibility *v = nullptr;
  bool shadow = false;
  threadpool::JobPriority prio;
  void start(mat44f_cref culling_,
    const Point3 &viewPos_,
    const PartitionSphere &transparentPartitionSphere_,
    RiGenVisibility *v_,
    bool shadow_,
    threadpool::JobPriority prio_,
    eastl::optional<IPoint2> target_ = {})
  {
    if (!v_)
      return;
    threadpool::wait(this, 0, threadpool::NUM_PRIO);
    culling = culling_;
    viewPos = viewPos_;
    transparentPartitionSphere = transparentPartitionSphere_;
    v = v_;
    shadow = shadow_;
    prio = prio_;
    target = target_;
    threadpool::add(this, prio, false);
  }
  void doJob() override;
};
extern ConVarT<bool, false> sort_transparent_riex_instances;
static carray<RendinstExtraVisibilityPrepareJob, MAX_JOB_COUNT> rendinst_extra_visibility_job;
void RendinstExtraVisibilityPrepareJob::doJob()
{
  // using occlusion for riExtra is correct, because inside we utilize the fact that it is for shadow
  rendinst::prepareRIGenExtraVisibility(culling, viewPos, *v, shadow, current_occlusion, target);
  if (this == &rendinst_extra_visibility_job[RENDER_MAIN_JOB])
  {
    if (sort_transparent_riex_instances.get())
    {
      if (transparentPartitionSphere.status == PartitionSphere::Status::CAMERA_INSIDE_SPHERE)
      {
        Point4 sphere = transparentPartitionSphere.sphere;
        sphere.w -= transparentPartitionSphere.maxRadiusError;
        rendinst::sortTransparentRiExtraInstancesByDistanceAndPartitionOutsideSphere(v, viewPos, sphere);
      }
      else if (transparentPartitionSphere.status == PartitionSphere::Status::CAMERA_OUTSIDE_SPHERE)
      {
        Point4 sphere = transparentPartitionSphere.sphere;
        sphere.w += transparentPartitionSphere.maxRadiusError;
        rendinst::sortTransparentRiExtraInstancesByDistanceAndPartitionInsideSphere(v, viewPos, sphere);
      }
      else
      {
        rendinst::sortTransparentRiExtraInstancesByDistance(v, viewPos);
      }
    }
    rendinst::render::startPreparedAsyncRIGenExtraOpaqueRender(*v, /*wake */ threadpool::get_current_worker_id() < 0);
  }
}
static struct LightsCullingJob final : public cpujobs::IJob
{
  mat44f globtm, view, proj;
  vec4f viewPos;
  float zn = 0.1;
  void start(mat44f_cref globtm_, mat44f_cref view_, mat44f_cref proj_, vec4f viewPos_, float zn_, threadpool::JobPriority prio)
  {
    threadpool::wait(this, 0, threadpool::NUM_PRIO);
    globtm = globtm_;
    view = view_;
    proj = proj_;
    viewPos = viewPos_;
    zn = zn_;
    threadpool::add(this, prio, /*wake*/ false);
  }
  virtual void doJob() override
  {
    // int64_t reft = ref_time_ticks();
    if (get_world_renderer())
      ((WorldRenderer *)get_world_renderer())->cullFrustumLights(viewPos, globtm, view, proj, zn);
    // debug("lights done %dus", get_time_usec(reft));
  }
} lights_cull_job;

static struct LightProbeVisibilityJob final : public cpujobs::IJob
{
  mat44f globtm;
  Point3 viewPos;

  void start(const mat44f &globtm_, const Point3 &view_pos)
  {
    threadpool::wait(this);
    globtm = globtm_;
    viewPos = view_pos;
    threadpool::add(this, threadpool::PRIO_LOW, false);
  }

  virtual void doJob() override
  {
    if (get_world_renderer())
      ((WorldRenderer *)get_world_renderer())->prepareLightProbeRIVisibility(globtm, viewPos);
  }
} light_probe_visibility_job;

void WorldRenderer::prepareLightProbeRIVisibilityAsync(const mat44f &globtm, const Point3 &view_pos)
{
  light_probe_visibility_job.start(globtm, view_pos);
}

void WorldRenderer::waitVisibility(int cascade)
{
  int jobId = -1;
  bool active = false;
  if (cascade >= RENDER_SHADOWS_CSM && cascade - RENDER_SHADOWS_CSM + RENDER_CSM_JOB <= MAX_JOB_COUNT)
  {
    jobId = cascade - RENDER_SHADOWS_CSM + RENDER_CSM_JOB;
    active = cascade == RENDER_SHADOWS_CSM ? false : true; // no active wait for first cascade. We need shadows asap,
                                                           // and we can't start doing something heavy
    // active = false;
  }
  else if (cascade == RENDER_MAIN)
  {
    jobId = RENDER_MAIN_JOB;
    active = true; // we probably can help doing something
  }
  if (jobId >= 0)
  {
    TIME_PROFILE(wait_rendinst);
    threadpool::wait(&visibility_job, active ? threadpool::PRIO_NORMAL : -1);
    static int wait_rendinst_desc = 0, wait_riextra_desc = 0;
    if (!wait_rendinst_desc)
      wait_rendinst_desc = DA_PROFILE_ADD_WAIT_DESC(__FILE__, __LINE__, "wait_rendinst");
    if (!wait_riextra_desc)
      wait_riextra_desc = DA_PROFILE_ADD_WAIT_DESC(__FILE__, __LINE__, "wait_riextra");
    threadpool::wait(&rendinst_visibility_job[jobId], wait_rendinst_desc, active ? rendinst_visibility_job[jobId].prio : -1);
    threadpool::wait(&rendinst_extra_visibility_job[jobId], wait_riextra_desc,
      active ? rendinst_extra_visibility_job[jobId].prio : -1);
  }
}

void WorldRenderer::waitMainVisibility()
{
#if TIME_PROFILER_ENABLED
  if (interlocked_acquire_load(visibility_job.done))
    return;
  TIME_PROFILE(wait_main_visibility);
#endif
  threadpool::wait(&visibility_job);
}

void WorldRenderer::waitGroundVisibility()
{
#if TIME_PROFILER_ENABLED
  if (interlocked_acquire_load(ground_cull_job.done))
    return;
  TIME_PROFILE(wait_ground_visibility);
#endif
  threadpool::wait(&ground_cull_job);
}

void WorldRenderer::waitGroundReflectionVisibility()
{
#if TIME_PROFILER_ENABLED
  if (interlocked_acquire_load(ground_reflection_cull_job.done))
    return;
  TIME_PROFILE(wait_ground_reflection_visibility);
#endif
  threadpool::wait(&ground_reflection_cull_job);
}

void WorldRenderer::waitLights()
{
  TIME_PROFILE(wait_lights);
  threadpool::wait(&visibility_job, 0, threadpool::NUM_PRIO);
  threadpool::wait(&lights_cull_job);
}

void WorldRenderer::waitLightProbeVisibility()
{
  TIME_PROFILE(wait_light_probe_visibility);
  threadpool::wait(&light_probe_visibility_job);
}

void WorldRenderer::waitAllJobs()
{
  threadpool::wait(&visibility_job, 0, threadpool::NUM_PRIO);
  for (int jobId = 0; jobId < MAX_JOB_COUNT; ++jobId)
  {
    threadpool::wait(&rendinst_visibility_job[jobId], 0, threadpool::NUM_PRIO);
    threadpool::wait(&rendinst_extra_visibility_job[jobId], 0, threadpool::NUM_PRIO);
  }
  rendinst::render::waitAsyncRIGenExtraOpaqueRender(rendinst_main_visibility);
  waitLights();
  waitGroundVisibility();
  waitGroundReflectionVisibility();
  wait_static_shadows_cull_jobs();
  waitLightProbeVisibility();
  if (depthAOAboveCtx)
    depthAOAboveCtx->waitCullJobs();
}

void WorldRenderer::toggleProbeReflectionQuality()
{
  G_ASSERT_RETURN(rendinst_cube_visibility, );
  // We can't safely 2+ lods because it is not appropriate for interiers
  hqProbesReflection = ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("HQProbeReflections", true);
  const int probesRILod = hqProbesReflection ? 1 : rendinst::render::MAX_LOD_COUNT_WITH_ALPHA;
  rendinst::setRIGenVisibilityMinLod(rendinst_cube_visibility, rendinst::render::MAX_LOD_COUNT_WITH_ALPHA, probesRILod);
}

void WorldRenderer::initRendinstVisibility()
{
  closeRendinstVisibility();
  const DataBlock *grCfg = ::dgs_get_settings()->getBlockByName("graphics");
  sw_occlusion.set(grCfg->getBool("sw_occlusion", sw_occlusion.get()));
  sw_hmap_occlusion.set(grCfg->getBool("sw_hmap_occlusion", sw_hmap_occlusion.get()));
  rendinst_cube_visibility = rendinst::createRIGenVisibility(midmem);
  toggleProbeReflectionQuality();
  rendinst_main_visibility = rendinst::createRIGenVisibility(midmem);
  for (int i = 0; i < rendinst_shadows_visibility.size(); ++i)
    rendinst_shadows_visibility[i] = rendinst::createRIGenVisibility(midmem);
  rendinstHmapPatchesVisibility = rendinst::createRIGenVisibility(midmem);
  rendinst_dynamic_shadow_visibility = rendinst::createRIGenVisibility(midmem);
}

void WorldRenderer::closeRendinstVisibility()
{
  RiGenVisibility *rmv = eastl::exchange(rendinst_main_visibility, nullptr);
  waitAllJobs();
  rendinst::destroyRIGenVisibility(rmv);
  if (auto rcv = eastl::exchange(rendinst_cube_visibility, nullptr))
    rendinst::destroyRIGenVisibility(rcv);
  for (auto &rsv : rendinst_shadows_visibility)
    rendinst::destroyRIGenVisibility(eastl::exchange(rsv, nullptr));
  rendinst::destroyRIGenVisibility(eastl::exchange(rendinstHmapPatchesVisibility, nullptr));
  rendinst::destroyRIGenVisibility(eastl::exchange(rendinst_dynamic_shadow_visibility, nullptr));
}

void WorldRenderer::startVisibility(const TMatrix &itm,
  mat44f_cref globtm,
  mat44f_cref view,
  mat44f_cref proj,
  const Driver3dPerspective &p,
  const PartitionSphere &transparent_partition_sphere)
{
  visibility_job.start(itm, globtm, view, proj, p, transparent_partition_sphere);
}

void WorldRenderer::waitVisibilityReproject() { threadpool::wait(&visibility_reproject_job, 0, visibility_reproject_job.prio); }

void WorldRenderer::startOcclusionFrame(
  Occlusion &occlusion, const TMatrix &itm, const mat44f &viewTm, const mat44f &projTm, const mat44f &globtm)
{
  TIME_PROFILE_DEV(startOcclusionFrame);

  waitVisibilityReproject();
  const Point3_vec4 viewPos = itm.getcol(3);
  current_occlusion = use_occlusion.get() ? &occlusion : NULL;
  if (current_occlusion)
  {
    if (!stop_occlusion.get())
    {
      OcclusionData occlusionData = gather_occlusion_data();
      if (occlusionData)
      {
        current_occlusion->startFrame(v_ld(&viewPos.x), viewTm, projTm, (mat44f &)globtm, occlusionData.closeGeometryBoundingRadius,
          COCKPIT_REPROJECT_ANIMATED, occlusionData.closeGeometryPrevToCurrFrameTransform, 0, 0);
      }
      else
        current_occlusion->startFrame(v_ld(&viewPos.x), viewTm, projTm, (mat44f &)globtm, 0, COCKPIT_NO_REPROJECT, mat44f{}, 0, 0);
      cockpit_distanceVarId.set_real(occlusionData ? occlusionData.closeGeometryBoundingRadius : 0);
    }
    else
      current_occlusion->resetStats();
  }
}

void WorldRenderer::startVisibilityReproject()
{
  if (current_occlusion && !stop_occlusion.get())
    if ((dgs_get_window_mode() != WindowMode::FULLSCREEN_EXCLUSIVE || ::dgs_app_active) && !d3d::device_lost(NULL) &&
        use_hw_reprojection.get())
    {
      visibility_job.hasGpuDepth = true;
      if (occlusion_async_readback_gpu)
        occlusion_readback_gpu_job.start();
      else
        occlusion_readback_gpu_job.performJob();
      return;
    }
  visibility_job.hasGpuDepth = false;
}

void add_sw_rasterization_tasks(rendinst::RIOcclusionData &od,
  Occlusion &occlusion,
  mat44f_cref cullgtm,
  eastl::vector<ParallelOcclusionRasterizer::RasterizationTaskData> &tasks);

void gather_animchars_for_occlusion_rasterization(
  mat44f_cref globtm, const vec3f &origin, eastl::vector<ParallelOcclusionRasterizer::RasterizationTaskData> &tasks);

void WorldRenderer::startSwRasterization(mat44f_cref globtm, mat44f_cref view, const Driver3dPerspective &p)
{
  if (!current_occlusion || !sw_occlusion || !occlusionRasterizer)
    return;
  TIME_PROFILE(sw_rasterization);
  mat44f cullgtm;
  if (p.zf > 100)
  {
    // shrink zfar plane. For very long distances reprojection works fine, we only need to correctly rasterize very small
    v_mat44_make_persp(cullgtm, p.wk, p.hk, p.zn, p.zn + 10.0f * p.hk);
    cullgtm.col2 = v_add(cullgtm.col2, v_make_vec4f(p.ox, p.oy, 0.f, 0.f));
    v_mat44_mul(cullgtm, cullgtm, view);
  }
  else
    cullgtm = globtm;
  auto &rasterizationTasks = occlusionRasterizer->acquireTasksNext();
  if (sw_hmap_occlusion.get() && occlusionLandMeshManager && lmeshMgr && lmeshMgr->getHmapHandler())
    occlusionLandMeshManager->makeRasterizeLandMeshTasks(*lmeshMgr->getHmapHandler(), lmeshMgr->getHolesManager(), *current_occlusion,
      rasterizationTasks);
  add_sw_rasterization_tasks(*riOcclusionData, *current_occlusion, cullgtm, rasterizationTasks);
  if (sw_animchars_occlusion.get())
    gather_animchars_for_occlusion_rasterization(globtm, current_occlusion->getCurViewPos(), rasterizationTasks);
  occlusionRasterizer->rasterizeMeshes(eastl::move(rasterizationTasks), p.zn);
}

void WorldRenderer::startOcclusionAndSwRaster(Occlusion &occl, const CameraParams &cur_frame_cam)
{
  TMatrix4_vec4 viewTm = TMatrix4(currentFrameCamera.viewTm);

  startOcclusionFrame(occl, cur_frame_cam.viewItm, reinterpret_cast<mat44f_cref>(viewTm),
    reinterpret_cast<mat44f_cref>(cur_frame_cam.noJitterProjTm), reinterpret_cast<mat44f_cref>(cur_frame_cam.noJitterGlobtm));
  startVisibilityReproject();
  startSwRasterization((mat44f_cref)cur_frame_cam.noJitterGlobtm, (mat44f_cref)viewTm, cur_frame_cam.noJitterPersp);
}

static void prepare_visibility(bool hasGpuDepth, const Driver3dPerspective &p, VisibilityContext &ctx)
{
  TIME_PROFILE_DEV(prepare_visibility);

  if (current_occlusion)
  {
    if (!hasGpuDepth)
      current_occlusion->clear();
    bool shouldFinalizeSwOcclusion = false;
    if (sw_occlusion.get() && ctx.occlusionRasterizer)
    {
      current_occlusion->startSWOcclusion(p.zn);
      shouldFinalizeSwOcclusion = ctx.occlusionRasterizer->mergeRasterizationResults(*current_occlusion) > 0;
      if (hasGpuDepth && !interlocked_acquire_load(visibility_reproject_job.done)) // very likely that is true
      {
        TIME_PROFILE_DEV(wait_occlusion_gpu_readback_and_reproject);
        threadpool::wait(&visibility_reproject_job, 0, visibility_reproject_job.prio);
      }
    }
    else if (hasGpuDepth)
      current_occlusion->reprojectGPUFrame();

    if (shouldFinalizeSwOcclusion)
    {
      TIME_PROFILE_DEV(finalize_sw_occlusion);
      current_occlusion->finalizeSWOcclusion();
    }

    current_occlusion->finalizeOcclusion();
  }
}

void VisibilityPrepareJob::finalize()
{
  TIME_PROFILE_DEV(finalize_prepare_visibility);
  // now occlusion is made
  const Frustum frustum(globtm);
  // Occlusion is good. start visibility jobs

  // main most important jobs
  if (ctx.csm)
  {
    for (int i = 0; i < ctx.csm->getNumCascadesToRender(); ++i) // upto 4*2 different jobs
    {
      if (ctx.shadowsManager->shouldRenderCsmRendinst(i))
      {
        rendinst::setRIGenVisibilityRendering(ctx.rendinstShadowsVisibility[i], ctx.shadowsManager->getCsmVisibilityRendering(i));
        rendinst_extra_visibility_job[i + RENDER_CSM_JOB].start((mat44f_cref)ctx.csm->getWorldCullingMatrix(i), itm.getcol(3),
          PartitionSphere{}, ctx.rendinstShadowsVisibility[i], true, threadpool::PRIO_NORMAL);
        if (ctx.shadowsManager->shouldRenderCsmStatic(i))
          rendinst_visibility_job[i + RENDER_CSM_JOB].start(ctx.csm->getFrustum(i), itm.getcol(3), ctx.rendinstShadowsVisibility[i],
            true, threadpool::PRIO_NORMAL);
      }
    }
  }

  // will be needed after CSM, but before lights
  rendinst_extra_visibility_job[RENDER_MAIN_JOB].start((mat44f &)globtm, itm.getcol(3), transparentPartitionSphere,
    ctx.rendinstMainVisibility, false, threadpool::PRIO_NORMAL, target);
  rendinst_visibility_job[RENDER_MAIN_JOB].start(frustum, itm.getcol(3), ctx.rendinstMainVisibility, false, threadpool::PRIO_NORMAL);
  rendinst::requestLodsByDistance(itm.getcol(3));

  // we can't start ground visibility here, landmesh design is totally broken (plenty of state inside). Fix it first
  // ground_cull_job.start(frustum, itm.getcol(3), threadpool::PRIO_LOW);

  threadpool::wake_up_all();
}


void WorldRenderer::fillVisibilityContext(VisibilityContext &ctx)
{
  CascadeShadows *csm = shadowsManager.getCascadeShadows();
  ctx.occlusionRasterizer = occlusionRasterizer.get();
  ctx.csm = csm;
  ctx.shadowsManager = &shadowsManager;
  ctx.rendinstShadowsVisibility = rendinst_shadows_visibility.data();
  ctx.rendinstMainVisibility = rendinst_main_visibility;
}

void WorldRenderer::startGroundVisibility(const Frustum &frustum, const TMatrix &itm)
{
  ground_cull_job.start(frustum, itm.getcol(3), threadpool::PRIO_NORMAL); // Norm prio to be executed before dafx jobs
}

void WorldRenderer::startGroundReflectionVisibility()
{
  if (!isWaterPlanarReflectionTerrainEnabled())
    return;

  // Non-oblique culling frustum is safe from nearly parallel planes.
  Frustum frustum(TMatrix4(waterPlanarReflectionViewTm) * currentFrameCamera.noJitterProjTm);
  Point3 viewPos = waterPlanarReflectionViewItm.getcol(3);
  ground_reflection_cull_job.start(frustum, viewPos, threadpool::PRIO_LOW);
}

inline void VisibilityPrepareJob::startLightsCullingJob()
{
  lights_cull_job.start(globtm, view, proj, v_ldu(itm.m[3]), persp.zn, threadpool::PRIO_LOW);
}

void WorldRenderer::startLightsCullingJob()
{
  // there is no dynamic lights in compatibility/forward, so no need to visibility test them
  if (hasFeature(FeatureRenderFlags::CLUSTERED_LIGHTS))
    visibility_job.startLightsCullingJob();
}
