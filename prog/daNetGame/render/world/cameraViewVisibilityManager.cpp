// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "cameraViewVisibilityManager.h"

#include <3d/dag_resPtr.h>
#include <daECS/core/entityManager.h>
#include <scene/dag_occlusion.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <util/dag_convar.h>

#include <ecs/render/renderPasses.h>
#include <rendInst/rendInstExtraRender.h>
#include <rendInst/visibility.h>

#include <render/world/cameraParams.h>
#include <render/world/dynModelRenderPass.h>
#include <render/world/global_vars.h>
#include <render/world/occlusionLandMeshManager.h>
#include <render/world/worldRendererQueries.h>

#include <EASTL/fixed_string.h>


extern ConVarB stop_occlusion;
extern ConVarB use_occlusion;
extern ConVarB use_hw_reprojection;
extern ConVarB sw_occlusion;
extern ConVarB sw_hmap_occlusion;
extern ConVarB sw_animchars_occlusion;
extern ConVarB occlusion_async_readback_gpu;

static ShaderVariableInfo cockpit_distanceVarId("cockpit_distance", true);

void CameraViewVisibilityMgr::init()
{
  using TmpName = eastl::fixed_string<char, 16, false>;
  TmpName hzbName{TmpName::CtorSprintf{}, "hzb_%d", cameraPrefix};
  occlusion.init(hzbName.c_str());

  uint32_t occlWidth, occlHeight;
  occlusion.getMaskedResolution(occlWidth, occlHeight);
  occlusionRasterizer = eastl::make_unique<ParallelOcclusionRasterizer>(occlWidth, occlHeight);

  occlusionLandMeshManager = eastl::make_unique<OcclusionLandMeshManager>();
  rendinstMainVisibility = rendinst::createRIGenVisibility(midmem);
}

void CameraViewVisibilityMgr::close()
{
  occlusionLandMeshManager = nullptr;
  occlusionRasterizer = nullptr;

  RiGenVisibility *rmv = eastl::exchange(rendinstMainVisibility, nullptr);
  rendinst::destroyRIGenVisibility(rmv);

  occlusion.close();
  riExtraRenderJob.vbase = nullptr;
}

void CameraViewVisibilityMgr::runOcclusionRasterizerCleaners()
{
  if (occlusionRasterizer)
    occlusionRasterizer->runCleaners();
}

void CameraViewVisibilityMgr::prepareLightProbeRIVisibilityAsync(const mat44f &globtm, const Point3 &view_pos)
{
  lightProbeVisibilityJob.start(globtm, view_pos);
}

void CameraViewVisibilityMgr::startOcclusionAndSwRaster(
  rendinst::RIOcclusionData &ri_occlusion_data, const CameraParams &cur_frame_cam, const LandMeshManager *lmesh_mgr)
{
  TMatrix4_vec4 viewTm = TMatrix4(cur_frame_cam.viewTm);

  startOcclusionFrame(cur_frame_cam.viewItm, reinterpret_cast<mat44f_cref>(viewTm),
    reinterpret_cast<mat44f_cref>(cur_frame_cam.noJitterProjTm), reinterpret_cast<mat44f_cref>(cur_frame_cam.noJitterGlobtm));
  startVisibilityReproject();
  startSwRasterization(ri_occlusion_data, (mat44f_cref)cur_frame_cam.noJitterGlobtm, (mat44f_cref)viewTm, cur_frame_cam.noJitterPersp,
    lmesh_mgr);
}

void CameraViewVisibilityMgr::waitVisibilityReproject() { threadpool::wait(&visibilityReprojectJob, 0, visibilityReprojectJob.prio); }

void CameraViewVisibilityMgr::startOcclusionFrame(const TMatrix &itm, const mat44f &viewTm, const mat44f &projTm, const mat44f &globtm)
{
  TIME_PROFILE_DEV(startOcclusionFrame);

  waitVisibilityReproject();
  const Point3_vec4 viewPos = itm.getcol(3);
  Occlusion *occl = getOcclusion();
  if (occl)
  {
    if (!stop_occlusion.get())
    {
      OcclusionData occlusionData = gather_occlusion_data();
      if (occlusionData)
      {
        const CockpitReprojectionMode cockpitMode =
          (flags & CameraViewVisibilityFlags::DontReprojectCockpitOcclusion) == CameraViewVisibilityFlags::None
            ? COCKPIT_REPROJECT_ANIMATED
            : COCKPIT_NO_REPROJECT;

        occl->startFrame(v_ld(&viewPos.x), viewTm, projTm, (mat44f &)globtm, occlusionData.closeGeometryBoundingRadius, cockpitMode,
          occlusionData.closeGeometryPrevToCurrFrameTransform, 0, 0);
      }
      else
        occl->startFrame(v_ld(&viewPos.x), viewTm, projTm, (mat44f &)globtm, 0, COCKPIT_NO_REPROJECT, mat44f{}, 0, 0);
      cockpit_distanceVarId.set_real(occlusionData ? occlusionData.closeGeometryBoundingRadius : 0);
    }
    else
      occl->resetStats();
  }
}

void CameraViewVisibilityMgr::startVisibilityReproject()
{
  Occlusion *occl = getOcclusion();
  if (occl && !stop_occlusion.get())
    if ((dgs_get_window_mode() != WindowMode::FULLSCREEN_EXCLUSIVE || ::dgs_app_active) && !d3d::device_lost(NULL) &&
        use_hw_reprojection.get())
    {
      visibilityJob.hasGpuDepth = true;
      if (occlusion_async_readback_gpu)
        occlusionReadbackGpuJob.start(&jobsCtx, occl);
      else
        occlusionReadbackGpuJob.performJob();
      return;
    }
  visibilityJob.hasGpuDepth = false;
}

void add_sw_rasterization_tasks(rendinst::RIOcclusionData &od,
  Occlusion &occlusion,
  mat44f_cref cullgtm,
  eastl::vector<ParallelOcclusionRasterizer::RasterizationTaskData> &tasks);

void gather_animchars_for_occlusion_rasterization(
  mat44f_cref globtm, const vec3f &origin, eastl::vector<ParallelOcclusionRasterizer::RasterizationTaskData> &tasks);

void CameraViewVisibilityMgr::startSwRasterization(rendinst::RIOcclusionData &ri_occlusion_data,
  mat44f_cref globtm,
  mat44f_cref view,
  const Driver3dPerspective &p,
  const LandMeshManager *lmesh_mgr)
{
  Occlusion *occl = getOcclusion();
  if (!occl || !sw_occlusion || !occlusionRasterizer)
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
  if (sw_hmap_occlusion.get() && occlusionLandMeshManager && lmesh_mgr && lmesh_mgr->getHmapHandler())
    occlusionLandMeshManager->makeRasterizeLandMeshTasks(*lmesh_mgr->getHmapHandler(), lmesh_mgr->getHolesManager(), *occl,
      rasterizationTasks);
  add_sw_rasterization_tasks(ri_occlusion_data, *occl, cullgtm, rasterizationTasks);
  if (sw_animchars_occlusion.get())
    gather_animchars_for_occlusion_rasterization(globtm, occl->getCurViewPos(), rasterizationTasks);
  occlusionRasterizer->rasterizeMeshes(eastl::move(rasterizationTasks), p.zn);
}

void CameraViewVisibilityMgr::startVisibility(const CameraParams &cur_frame_camera,
  const PartitionSphere &transparent_partition_sphere,
  const IPoint2 &rendering_resolution,
  const ShadowVisibilityContext &shadow_ctx)
{
  G_ASSERT_AND_DO(rendinstMainVisibility, init());
  Occlusion *occl = getOcclusion();
  lodsByDistanceJob.start(cur_frame_camera.viewItm.getcol(3));
  visibilityJob.start(&jobsCtx, occl, occlusionRasterizer.get(), rendinstMainVisibility, cur_frame_camera,
    transparent_partition_sphere, rendering_resolution, shadow_ctx);
}

void CameraViewVisibilityMgr::startGroundVisibility(LandMeshManager *lmesh_mgr,
  LandMeshRenderer *lmesh_renderer,
  const float hmap_camera_height,
  const float water_level,
  const int displacement_sub_div,
  const CameraParams &cur_frame_camera)
{
  Occlusion *occl = getOcclusion();

  // Norm prio to be executed before dafx jobs
  groundCullJob.start(&jobsCtx, &lmeshCullingData, lmesh_mgr, lmesh_renderer, occl, cur_frame_camera.noJitterFrustum,
    cur_frame_camera.viewItm.getcol(3), cur_frame_camera.noJitterProjTm, hmap_camera_height, water_level, displacement_sub_div,
    threadpool::PRIO_NORMAL);
}

void CameraViewVisibilityMgr::startGroundReflectionVisibility(LandMeshManager *lmesh_mgr,
  LandMeshRenderer *lmesh_renderer,
  const Frustum &frustum,
  const Point3 &viewPos,
  const TMatrix4 &viewProj,
  const float hmap_camera_height,
  const float water_level)
{
  groundReflectionCullJob.start(&jobsCtx, &lmeshReflectionCullingData, lmesh_mgr, lmesh_renderer, frustum, viewPos, viewProj,
    hmap_camera_height, water_level, threadpool::PRIO_LOW);
}

void CameraViewVisibilityMgr::startLightsCullingJob(const CameraParams &cur_frame_camera, const bool use_occlusion_culling)
{
  Occlusion *occl = use_occlusion_culling ? getOcclusion() : nullptr;
  lightsCullJob.start(occl, cur_frame_camera, threadpool::PRIO_NORMAL);
}

void CameraViewVisibilityMgr::waitVisibility(int cascade)
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
    G_ASSERT(rendinstMainVisibility);
    jobId = RENDER_MAIN_JOB;
    active = true; // we probably can help doing something
  }
  if (jobId >= 0)
  {
    TIME_PROFILE(wait_rendinst);
    threadpool::wait(&visibilityJob, active ? threadpool::PRIO_NORMAL : -1);
    static int wait_rendinst_desc = 0, wait_riextra_desc = 0;
    if (!wait_rendinst_desc)
      wait_rendinst_desc = DA_PROFILE_ADD_WAIT_DESC(__FILE__, __LINE__, "wait_rendinst");
    if (!wait_riextra_desc)
      wait_riextra_desc = DA_PROFILE_ADD_WAIT_DESC(__FILE__, __LINE__, "wait_riextra");
    threadpool::wait(&rendinstVisibilityJobs[jobId], wait_rendinst_desc, active ? rendinstVisibilityJobs[jobId].prio : -1);
    threadpool::wait(&rendinstExtraVisibilityJobs[jobId], wait_riextra_desc, active ? rendinstExtraVisibilityJobs[jobId].prio : -1);
  }
}

void CameraViewVisibilityMgr::waitMainVisibility()
{
#if TIME_PROFILER_ENABLED
  if (interlocked_acquire_load(visibilityJob.done))
    return;
  TIME_PROFILE(wait_main_visibility);
#endif
  threadpool::wait(&visibilityJob);
}

void CameraViewVisibilityMgr::waitGroundVisibility()
{
#if TIME_PROFILER_ENABLED
  if (interlocked_acquire_load(groundCullJob.done))
    return;
  TIME_PROFILE(wait_ground_visibility);
#endif
  threadpool::wait(&groundCullJob);
}

void CameraViewVisibilityMgr::waitGroundReflectionVisibility()
{
#if TIME_PROFILER_ENABLED
  if (interlocked_acquire_load(groundReflectionCullJob.done))
    return;
  TIME_PROFILE(wait_ground_reflection_visibility);
#endif
  threadpool::wait(&groundReflectionCullJob);
}

void CameraViewVisibilityMgr::waitLights()
{
  TIME_PROFILE(wait_lights);
  threadpool::wait(&visibilityJob, 0, threadpool::NUM_PRIO);
  threadpool::wait(&lightsCullJob);
}

void CameraViewVisibilityMgr::waitLightProbeVisibility()
{
  TIME_PROFILE(wait_light_probe_visibility);
  threadpool::wait(&lightProbeVisibilityJob);
}

void CameraViewVisibilityMgr::waitAllJobs()
{
  if (occlusionRasterizer)
    occlusionRasterizer->waitAllJobs();

  threadpool::wait(&visibilityJob, 0, threadpool::NUM_PRIO);
  threadpool::wait(&lodsByDistanceJob, 0, threadpool::NUM_PRIO);
  for (int jobId = 0; jobId < MAX_JOB_COUNT; ++jobId)
  {
    threadpool::wait(&rendinstVisibilityJobs[jobId], 0, threadpool::NUM_PRIO);
    threadpool::wait(&rendinstExtraVisibilityJobs[jobId], 0, threadpool::NUM_PRIO);
  }
  waitAsyncAnimcharMainRender();
  waitAsyncRIGenExtraOpaqueRender();
  waitLights();
  waitGroundVisibility();
  waitGroundReflectionVisibility();
  waitLightProbeVisibility();
}

void CameraViewVisibilityMgr::waitOcclusionAsyncReadbackGpu()
{
#if TIME_PROFILER_ENABLED
  if (interlocked_acquire_load(occlusionReadbackGpuJob.done))
    return;
  TIME_PROFILE(wait_occlusion_readback_gpu);
#endif
  threadpool::wait(&occlusionReadbackGpuJob);
}

Occlusion *CameraViewVisibilityMgr::getOcclusion() { return occlusion && use_occlusion.get() ? &occlusion : nullptr; }
const Occlusion *CameraViewVisibilityMgr::getOcclusion() const { return const_cast<CameraViewVisibilityMgr *>(this)->getOcclusion(); }

void CameraViewVisibilityMgr::prepareToStartAsyncRIGenExtraOpaqueRender(
  const int frame_stblk, const TexStreamingContext texCtx, const bool enable)
{
  riExtraRenderJob.prepare(*rendinstMainVisibility, frame_stblk, enable, texCtx);
}

void CameraViewVisibilityMgr::waitAsyncRIGenExtraOpaqueRenderVbFill() { riExtraRenderJob.waitVbFill(rendinstMainVisibility); }

rendinst::render::RiExtraRenderer *CameraViewVisibilityMgr::waitAsyncRIGenExtraOpaqueRender()
{
  return riExtraRenderJob.wait(rendinstMainVisibility);
}

void CameraViewVisibilityMgr::startAsyncAnimcharMainRender(
  const Frustum &fg_cam_blob_frustum, uint32_t hints, TexStreamingContext tex_ctx)
{
  G_ASSERT(g_entity_mgr->isConstrainedMTMode());

  animcharMainRenderJobCtx.jobs = animcharMainRenderJobs;

  ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Color));
  G_ASSERT(animcharMainRenderJobCtx.globalVarsState.empty() /* Expected to be cleared by previous `waitAsyncAnimcharMainRender` */);
  animcharMainRenderJobCtx.globalVarsState.set_allocator(framemem_ptr());
  copy_current_global_variables_states(animcharMainRenderJobCtx.globalVarsState);

  interlocked_relaxed_store(animcharMainRenderJobCtx.jobsLeft, AnimcharRenderAsyncFilter::ARF_IDX_COUNT);

  using TmpName = eastl::fixed_string<char, 16, false>;
  const Occlusion *occl = getOcclusion();
  for (int8_t i = 0; i < countof(animcharMainRenderJobs); ++i)
  {
    TmpName stateName{TmpName::CtorSprintf{}, "%s%d", cameraPrefix, i};

    animcharMainRenderJobs[i].start(&animcharMainRenderJobCtx, AnimcharRenderAsyncFilter{i}, stateName.c_str(), hints, occl,
      &fg_cam_blob_frustum, tex_ctx);
  }

  threadpool::wake_up_one();
}

void CameraViewVisibilityMgr::waitAsyncAnimcharMainRender()
{
  for (int j = AnimcharRenderAsyncFilter::ARF_IDX_COUNT - 1; j >= 0; j--)
  {
    AnimcharRenderMainJob &job = animcharMainRenderJobs[j];
    if (!interlocked_acquire_load(job.done))
    {
      TIME_PROFILE_DEV(wait_animchar_async_render_main);
      if (j == AnimcharRenderAsyncFilter::ARF_IDX_COUNT - 1) // Last
        threadpool::barrier_active_wait_for_job(&job, threadpool::PRIO_HIGH, animcharMainRenderJobCtx.lastJobTPQPos);
      for (int i = j; i >= 0; i--)
        threadpool::wait(&animcharMainRenderJobs[i]);
      break;
    }
  }
  G_ASSERT(!interlocked_relaxed_load(animcharMainRenderJobCtx.jobsLeft));
  G_ASSERT(
    animcharMainRenderJobCtx.globalVarsState.get_allocator() == framemem_ptr() || animcharMainRenderJobCtx.globalVarsState.empty());
  animcharMainRenderJobCtx.globalVarsState.clear(); // Free framemem allocated data
}

void CameraViewVisibilityMgr::updateLodsScaling(const float ri_dist_mul, const float impostors_dist_mul)
{
  G_UNUSED(impostors_dist_mul);
  rendinst::setRIGenVisibilityDistMul(rendinstMainVisibility, ri_dist_mul);
}