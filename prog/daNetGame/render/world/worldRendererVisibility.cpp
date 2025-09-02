// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1 // fixme: move to jam

#include "render/renderer.h"
#include "private_worldRenderer.h"
#include <rendInst/gpuObjects.h>
#include <rendInst/rendInstGenRender.h>
#include <rendInst/visibility.h>
#include <osApiWrappers/dag_miscApi.h>
#include <math/dag_frustum.h>
#include <perfMon/dag_statDrv.h>
#include <perfMon/dag_cpuFreq.h>
#include <util/dag_convar.h>
#include <landMesh/lmeshManager.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include "depthAOAbove.h"
#include <render/world/cameraInCamera.h>
#include <shaders/dag_shaderBlock.h>


extern ShaderBlockIdHolder globalFrameBlockId;
extern ConVarB async_riex_opaque;

extern ConVarB stop_occlusion;
extern ConVarB use_occlusion;

CONSOLE_BOOL_VAL("occlusion", use_hw_reprojection, true);

CONSOLE_BOOL_VAL("render", sw_occlusion, true);
CONSOLE_BOOL_VAL("render", sw_hmap_occlusion, true);
CONSOLE_BOOL_VAL("render", sw_animchars_occlusion, true);
CONSOLE_BOOL_VAL("render", occlusion_async_readback_gpu, true);

extern void wait_static_shadows_cull_jobs();

void WorldRenderer::prepareLightProbeRIVisibilityAsync(const mat44f &globtm, const Point3 &view_pos)
{
  mainCameraVisibilityMgr.prepareLightProbeRIVisibilityAsync(globtm, view_pos);
}

void WorldRenderer::waitAllJobs()
{
  threadpool::wake_up_all();
  mainCameraVisibilityMgr.waitAllJobs();
  camcamVisibilityMgr.waitAllJobs();
  wait_static_shadows_cull_jobs();
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
  mainCameraVisibilityMgr.init();
  if (hasFeature(CAMERA_IN_CAMERA))
    camcamVisibilityMgr.init();
  for (int i = 0; i < rendinst_shadows_visibility.size(); ++i)
    rendinst_shadows_visibility[i] = rendinst::createRIGenVisibility(midmem);
  rendinstHmapPatchesVisibility = rendinst::createRIGenVisibility(midmem);
  rendinst_dynamic_shadow_visibility = rendinst::createRIGenVisibility(midmem);
}

void WorldRenderer::closeRendinstVisibility()
{
  waitAllJobs();
  mainCameraVisibilityMgr.close();
  camcamVisibilityMgr.close();
  if (auto rcv = eastl::exchange(rendinst_cube_visibility, nullptr))
    rendinst::destroyRIGenVisibility(rcv);
  for (auto &rsv : rendinst_shadows_visibility)
    rendinst::destroyRIGenVisibility(eastl::exchange(rsv, nullptr));
  rendinst::destroyRIGenVisibility(eastl::exchange(rendinstHmapPatchesVisibility, nullptr));
  rendinst::destroyRIGenVisibility(eastl::exchange(rendinst_dynamic_shadow_visibility, nullptr));
}

void WorldRenderer::startVisibility()
{
  ShadowVisibilityContext shadowCtx;
  shadowCtx.csm = shadowsManager.getCascadeShadows();
  shadowCtx.shadowsManager = &shadowsManager;
  shadowCtx.rendinstShadowsVisibility = rendinst_shadows_visibility.data();

  IPoint2 renderingResolution(0, 0);
  getRenderingResolution(renderingResolution.x, renderingResolution.y);

  const bool asyncRiJobEnabled = async_riex_opaque.get() && !rendinst::gpuobjects::has_pending();
  mainCameraVisibilityMgr.prepareToStartAsyncRIGenExtraOpaqueRender(globalFrameBlockId, currentTexCtx, asyncRiJobEnabled);
  mainCameraVisibilityMgr.startVisibility(currentFrameCamera, transparentPartitionSphere, renderingResolution, shadowCtx);

  if (camera_in_camera::is_lens_render_active())
  {
    camcamVisibilityMgr.prepareToStartAsyncRIGenExtraOpaqueRender(globalFrameBlockId, currentTexCtx, asyncRiJobEnabled);
    camcamVisibilityMgr.startVisibility(*camcamParams, transparentPartitionSphere, renderingResolution, ShadowVisibilityContext{});
  }
}

void WorldRenderer::startOcclusionAndSwRaster()
{
  mainCameraVisibilityMgr.startOcclusionAndSwRaster(*riOcclusionData, currentFrameCamera, lmeshMgr);

  if (camera_in_camera::is_lens_render_active())
    camcamVisibilityMgr.startOcclusionAndSwRaster(*riOcclusionData, *camcamParams, lmeshMgr);
}

void WorldRenderer::startGroundVisibility()
{
  mainCameraVisibilityMgr.startGroundVisibility(lmeshMgr, lmeshRenderer, cameraHeight, waterLevel, displacementSubDiv,
    currentFrameCamera);

  if (camera_in_camera::is_lens_render_active())
    camcamVisibilityMgr.startGroundVisibility(lmeshMgr, lmeshRenderer, cameraHeight, waterLevel, displacementSubDiv, *camcamParams);
}

void WorldRenderer::startGroundReflectionVisibility()
{
  if (!isWaterPlanarReflectionTerrainEnabled())
    return;

  // Non-oblique culling frustum is safe from nearly parallel planes.
  Frustum frustum(TMatrix4(waterPlanarReflectionViewTm) * currentFrameCamera.noJitterProjTm);
  Point3 viewPos = waterPlanarReflectionViewItm.getcol(3);
  mainCameraVisibilityMgr.startGroundReflectionVisibility(lmeshMgr, lmeshRenderer, frustum, viewPos, currentFrameCamera.noJitterProjTm,
    cameraHeight, waterLevel);

  if (camera_in_camera::is_lens_render_active())
  {
    Frustum frustum(TMatrix4(waterPlanarReflectionViewTm) * camcamParams->noJitterProjTm);
    camcamVisibilityMgr.startGroundReflectionVisibility(lmeshMgr, lmeshRenderer, frustum, viewPos, camcamParams->noJitterProjTm,
      cameraHeight, waterLevel);
  }
}

void WorldRenderer::startLightsCullingJob()
{
  // there is no dynamic lights in compatibility/forward, so no need to visibility test them
  if (hasFeature(FeatureRenderFlags::CLUSTERED_LIGHTS))
  {
    const bool useOcclusion = !camera_in_camera::is_lens_render_active();
    mainCameraVisibilityMgr.startLightsCullingJob(currentFrameCamera, useOcclusion);
  }
}

void WorldRenderer::waitOcclusionAsyncReadbackGpu()
{
  mainCameraVisibilityMgr.waitOcclusionAsyncReadbackGpu();
  camcamVisibilityMgr.waitOcclusionAsyncReadbackGpu();
}

void wait_occlusion_async_readback_gpu()
{
  auto wr = static_cast<WorldRenderer *>(get_world_renderer());
  if (wr)
    wr->waitOcclusionAsyncReadbackGpu();
}