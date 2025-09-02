// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "visibilityJobs.h"

#include <rendInst/rendInstExtraRender.h>
#include <rendInst/rendInstRenderJob.h>
#include <rendInst/visibility.h>

#include <scene/dag_occlusion.h>
#include <util/dag_convar.h>
#include <render/occlusion/parallelOcclusionRasterizer.h>

#define INSIDE_RENDERER 1
#include <render/world/private_worldRenderer.h>


extern ConVarB sw_occlusion;
extern ConVarB sort_transparent_riex_instances;

void VisibilityReprojectJob::start(Occlusion *occl)
{
  G_FAST_ASSERT(occl);
  occlusion = occl;
  uint32_t _1;
  // It might be already set to not done by `OcclusionReadbackGPUJob`
  threadpool::add(this, prio, _1, threadpool::AddFlags::IgnoreNotDone);
}

void VisibilityReprojectJob::doJob()
{
  if (occlusion && *occlusion)
    occlusion->reprojectGPUFrame();
}

void OcclusionReadbackGPUJob::start(const VisibilityJobsContext *jc, Occlusion *occl)
{
  G_FAST_ASSERT(occl);
  threadpool::wait(this);
  jobs = jc;
  occlusion = occl;
  if (sw_occlusion.get())
  {
    threadpool::wait(jobs->visibilityReprojectJob);
    interlocked_release_store(jobs->visibilityReprojectJob->done, 0); // To be able wait one job instead of pair
  }
  // Note: would be wake-up either by wake_up_all in `animchar_before_render_es` or `bvh_start_before_render_jobs_es`
  threadpool::add(this, prio, false);
}

void OcclusionReadbackGPUJob::doJob()
{
  occlusion->readbackGPU();
  if (sw_occlusion.get())
    jobs->visibilityReprojectJob->start(occlusion);
}

void OcclusionReadbackGPUJob::performJob()
{
  doJob();
  done = 1;
}

void RendinstVisibilityPrepareJob::start(
  Occlusion *occl, const Frustum &frustum_, const Point3 &viewPos_, RiGenVisibility *v_, bool shadow_, threadpool::JobPriority prio_)
{
  if (!v_)
    return;
  threadpool::wait(this, 0, threadpool::NUM_PRIO);
  occlusion = occl;
  frustum = frustum_;
  viewPos = viewPos_;
  v = v_;
  shadow = shadow_;
  prio = prio_;
  threadpool::add(this, prio, false);
}

void RendinstVisibilityPrepareJob::doJob()
{
  // int64_t reft = ref_time_ticks();
  rendinst::prepareRIGenVisibility(frustum, viewPos, v, shadow, shadow ? nullptr : occlusion);
  // debug("rendinst in %dus", get_time_usec(reft));
}

void RendinstExtraVisibilityPrepareJob::start(const VisibilityJobsContext *jc,
  Occlusion *occl,
  mat44f_cref culling_,
  const Point3 &viewPos_,
  const PartitionSphere &transparentPartitionSphere_,
  RiGenVisibility *v_,
  bool shadow_,
  threadpool::JobPriority prio_,
  eastl::optional<IPoint2> target_,
  bool is_main_job)
{
  if (!v_)
    return;
  jobs = jc;
  isMainJob = is_main_job;
  occlusion = occl;
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

void RendinstExtraVisibilityPrepareJob::doJob()
{
  // using occlusion for riExtra is correct, because inside we utilize the fact that it is for shadow
  rendinst::prepareRIGenExtraVisibility(culling, viewPos, *v, shadow, occlusion, target);
  if (isMainJob)
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

    const bool wake = threadpool::get_current_worker_id() < 0;
    jobs->riExtraRenderJob->start(*v, wake);
  }
}

void VisibilityPrepareJob::start(const VisibilityJobsContext *jc,
  Occlusion *occl,
  ParallelOcclusionRasterizer *occlusion_rasterizer,
  RiGenVisibility *rendinst_main_visibility,
  const CameraParams &cur_frame_camera,
  const PartitionSphere &transparentPartitionSphere_,
  const IPoint2 &rendering_resolution,
  const ShadowVisibilityContext &shadow_ctx)
{
  threadpool::wait(this, 0, threadpool::NUM_PRIO);
  TMatrix4_vec4 viewTm = TMatrix4(cur_frame_camera.viewTm);

  jobs = jc;
  occlusion = occl;
  rendinstMainVisibility = rendinst_main_visibility;
  occlusionRasterizer = occlusion_rasterizer;
  itm = cur_frame_camera.viewItm;
  globtm = (mat44f_cref)cur_frame_camera.noJitterGlobtm;
  view = (mat44f_cref)viewTm;
  proj = (mat44f_cref)cur_frame_camera.noJitterProjTm;
  persp = cur_frame_camera.noJitterPersp;
  transparentPartitionSphere = transparentPartitionSphere_;
  target = rendering_resolution;
  shadowCtx = shadow_ctx;
  threadpool::add(this, VisibilityReprojectJob::prio);
}

void VisibilityPrepareJob::doJob()
{
  prepareVisibility();
  finalize();
}

void VisibilityPrepareJob::prepareVisibility()
{
  TIME_PROFILE_DEV(prepare_visibility);

  if (occlusion && *occlusion)
  {
    if (!hasGpuDepth)
      occlusion->clear();
    bool shouldFinalizeSwOcclusion = false;
    if (sw_occlusion.get() && occlusionRasterizer)
    {
      occlusion->startSWOcclusion(persp.zn);
      shouldFinalizeSwOcclusion = occlusionRasterizer->mergeRasterizationResults(*occlusion) > 0;
      if (hasGpuDepth && !interlocked_acquire_load(jobs->visibilityReprojectJob->done)) // very likely that is true
      {
        TIME_PROFILE_DEV(wait_occlusion_gpu_readback_and_reproject);
        threadpool::wait(jobs->visibilityReprojectJob, 0, jobs->visibilityReprojectJob->prio);
      }
    }
    else if (hasGpuDepth)
      occlusion->reprojectGPUFrame();

    if (shouldFinalizeSwOcclusion)
    {
      TIME_PROFILE_DEV(finalize_sw_occlusion);
      occlusion->finalizeSWOcclusion();
    }

    occlusion->finalizeOcclusion();
  }
}

void VisibilityPrepareJob::finalize()
{
  TIME_PROFILE_DEV(finalize_prepare_visibility);
  // now occlusion is made
  const Frustum frustum(globtm);
  // Occlusion is good. start visibility jobs

  // main most important jobs
  if (shadowCtx.csm)
  {
    for (int i = 0; i < shadowCtx.csm->getNumCascadesToRender(); ++i) // upto 4*2 different jobs
    {
      if (shadowCtx.shadowsManager->shouldRenderCsmRendinst(i))
      {
        rendinst::setRIGenVisibilityRendering(shadowCtx.rendinstShadowsVisibility[i],
          shadowCtx.shadowsManager->getCsmVisibilityRendering(i));
        jobs->rendinstExtraVisibilityJobs[i + RENDER_CSM_JOB].start(jobs, occlusion,
          (mat44f_cref)shadowCtx.csm->getWorldCullingMatrix(i), itm.getcol(3), PartitionSphere{},
          shadowCtx.rendinstShadowsVisibility[i], true, threadpool::PRIO_NORMAL);
        if (shadowCtx.shadowsManager->shouldRenderCsmStatic(i))
          jobs->rendinstVisibilityJobs[i + RENDER_CSM_JOB].start(occlusion, shadowCtx.csm->getFrustum(i), itm.getcol(3),
            shadowCtx.rendinstShadowsVisibility[i], true, threadpool::PRIO_NORMAL);
      }
    }
  }

  // will be needed after CSM, but before lights
  G_ASSERT(rendinstMainVisibility);
  jobs->rendinstExtraVisibilityJobs[RENDER_MAIN_JOB].start(jobs, occlusion, (mat44f &)globtm, itm.getcol(3),
    transparentPartitionSphere, rendinstMainVisibility, false, threadpool::PRIO_NORMAL, target, RI_EXTRA_VIS_MAIN_JOB);
  jobs->rendinstVisibilityJobs[RENDER_MAIN_JOB].start(occlusion, frustum, itm.getcol(3), rendinstMainVisibility, false,
    threadpool::PRIO_NORMAL);

  threadpool::wake_up_all();

  // we can't start ground visibility here, landmesh design is totally broken (plenty of state inside). Fix it first
  // ground_cull_job.start(frustum, itm.getcol(3), threadpool::PRIO_LOW);
}

void LodsByDistanceJob::start(const Point3 &camera_pos)
{
  threadpool::wait(this, 0, threadpool::NUM_PRIO);
  cameraPos = camera_pos;
  threadpool::add(this, LodsByDistanceJob::prio);
}

void LodsByDistanceJob::doJob() { rendinst::requestRiExtraLodsByDistance(cameraPos); }

void GroundCullingJob::start(const VisibilityJobsContext *jc,
  LandMeshCullingData *culling_data,
  LandMeshManager *lmesh_mgr,
  LandMeshRenderer *lmesh_renderer,
  Occlusion *occl,
  const Frustum &frustum_,
  const Point3 &viewPos_,
  const TMatrix4 &viewProj_,
  const float hmap_camera_height,
  const float water_level,
  const int displacement_sub_div,
  threadpool::JobPriority prio)
{
  threadpool::wait(this, 0, threadpool::NUM_PRIO);
  if (!lmesh_mgr)
    return;

  jobs = jc;
  lmeshCullingState.copyLandmeshState(*lmesh_mgr, *lmesh_renderer);
  lmeshCullingData = culling_data;
  lmeshMgr = lmesh_mgr;
  lmeshRenderer = lmesh_renderer;
  occlusion = occl;
  frustum = frustum_;
  viewPos = viewPos_;
  viewProj = viewProj_;
  hmapCameraHeight = hmap_camera_height;
  waterLevel = water_level;
  displacementSubDiv = displacement_sub_div;
  threadpool::add(this, prio, /*wake*/ false);
}

void GroundCullingJob::doJob()
{
  threadpool::wait(jobs->visibilityJob, 0, threadpool::NUM_PRIO);
  lmeshCullingState.frustumCulling(*lmeshMgr, *lmeshCullingData, NULL, 0,
    HeightmapFrustumCullingInfo{viewPos, hmapCameraHeight, waterLevel, frustum, occlusion, 0, displacementSubDiv, 1, viewProj});
}

void GroundReflectionCullingJob::start(const VisibilityJobsContext *jc,
  LandMeshCullingData *culling_data,
  LandMeshManager *lmesh_mgr,
  LandMeshRenderer *lmesh_renderer,
  const Frustum &frustum_,
  const Point3 &viewPos_,
  const TMatrix4 &viewProj_,
  const float hmap_camera_height,
  const float water_level,
  threadpool::JobPriority prio)
{
  threadpool::wait(this, 0, threadpool::NUM_PRIO);
  if (!lmesh_mgr)
    return;

  jobs = jc;
  lmeshCullingState.copyLandmeshState(*lmesh_mgr, *lmesh_renderer);
  lmeshCullingData = culling_data;
  lmeshMgr = lmesh_mgr;
  lmeshRenderer = lmesh_renderer;
  frustum = frustum_;
  viewPos = viewPos_;
  hmapCameraHeight = hmap_camera_height;
  viewProj = viewProj_;
  waterLevel = water_level;
  threadpool::add(this, prio, /*wake*/ false);
}

void GroundReflectionCullingJob::doJob()
{
  threadpool::wait(jobs->visibilityJob, 0, threadpool::NUM_PRIO);
  lmeshCullingState.frustumCulling(*lmeshMgr, *lmeshCullingData, NULL, 0,
    HeightmapFrustumCullingInfo{
      viewPos, hmapCameraHeight, waterLevel, frustum, nullptr, 0, 0, 1, viewProj, 0.5, HeightmapFrustumCullingInfo::FASTEST});
}

void LightsCullingJob::start(Occlusion *occlusion_, const CameraParams &cur_frame_camera, threadpool::JobPriority prio)
{
  threadpool::wait(this, 0, threadpool::NUM_PRIO);
  TMatrix4_vec4 viewTm = TMatrix4(cur_frame_camera.viewTm);

  occlusion = occlusion_;
  globtm = (mat44f_cref)cur_frame_camera.noJitterGlobtm;
  view = (mat44f_cref)viewTm;
  proj = (mat44f_cref)cur_frame_camera.noJitterProjTm;
  viewPos = v_ldu(cur_frame_camera.viewItm.m[3]);
  zn = cur_frame_camera.noJitterPersp.zn;
  threadpool::add(this, prio, /*wake*/ false);
}

void LightsCullingJob::doJob()
{
  // int64_t reft = ref_time_ticks();
  if (get_world_renderer())
    ((WorldRenderer *)get_world_renderer())->cullFrustumLights(occlusion, viewPos, globtm, view, proj, zn);
  // debug("lights done %dus", get_time_usec(reft));
}

void LightProbeVisibilityJob::start(const mat44f &globtm_, const Point3 &view_pos)
{
  threadpool::wait(this);
  globtm = globtm_;
  viewPos = view_pos;
  threadpool::add(this, threadpool::PRIO_LOW, false);
}

void LightProbeVisibilityJob::doJob()
{
  if (get_world_renderer())
    ((WorldRenderer *)get_world_renderer())->prepareLightProbeRIVisibility(globtm, viewPos);
}