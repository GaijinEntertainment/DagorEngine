// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/dstReadbackLights.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaders.h>
#include <shaders/find_max_depth_2d.hlsli>

#define SHADER_VARS_LIST VAR(proj_values)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
SHADER_VARS_LIST
#undef VAR

DistanceReadbackLights::DistanceReadbackLights(ShadowSystem *shadowSystem, SpotLightsManager *spotLights, const char *name_suffix) :
  lightShadows(shadowSystem), spotLights(spotLights), lastNonOptId(-1), processing(false)
{
  findMaxDepth2D.reset(new_compute_shader("find_max_depth_2d"));
  if (!findMaxDepth2D)
  {
    logerr("missing shader find_max_depth_2d");
    return;
  }
  String uniqueName(32, "find_max_depth_2d%s", name_suffix);
  resultRingBuffer.init(sizeof(float), 4, 1, uniqueName.c_str(), SBCF_UA_STRUCTURED_READBACK, 0, false);
}

void DistanceReadbackLights::update(eastl::fixed_function<sizeof(void *) * 2, RenderStaticCallback> render_static)
{
  if (!processing)
    dispatchQuery(render_static);
  if (processing)
    completeQuery();
}

void DistanceReadbackLights::dispatchQuery(eastl::fixed_function<sizeof(void *) * 2, RenderStaticCallback> render_static)
{
  if (!findMaxDepth2D)
    return;
  TIME_D3D_PROFILE(find_light_max_depth)
  if (!spotLights->tryGetNonOptimizedLightId(lastNonOptId))
    return;

  mat44f view, proj, viewItm;
  const SpotLight &light = spotLights->getLight(lastNonOptId);
  float prevValue = light.pos_radius.w;
  if (prevValue <= 0.0 || !light.requiresCullRadiusOptimization) // This shouldn't be true, but it can (or can be explicitly disabled)
  {
    // Perspective matrix is wrong, skip this light
    spotLights->setLightOptimized(lastNonOptId);
    return;
  }

  int frame = 0;
  Sbuffer *resultBuffer = (Sbuffer *)resultRingBuffer.getNewTarget(frame);
  if (!resultBuffer)
    return;

  spotLights->getLightView(lastNonOptId, viewItm);
  v_mat44_orthonormal_inverse43_to44(view, viewItm);
  spotLights->getLightPersp(lastNonOptId, proj);
  // May be forward perspective projection instead of reverse (that used now) would be better

  {
    TIME_D3D_PROFILE(render_temp_shadow)
    lightShadows->startRenderTempShadow();
    d3d::settm(TM_VIEW, view);
    d3d::settm(TM_PROJ, proj);
    alignas(16) TMatrix viewItmS;
    v_mat_43ca_from_mat44(viewItmS[0], viewItm);

    mat44f globTm;
    v_mat44_mul(globTm, proj, view);

    // Don't use shadow render extensions for distance readback.
    constexpr int NO_INDEX = -1;

    render_static(globTm, proj, viewItmS, NO_INDEX, NO_INDEX, DynamicShadowRenderGPUObjects::NO);
    lightShadows->endRenderTempShadow();
    d3d::set_render_target();
  }

  TextureInfo tinfo;
  Texture *shadowTex = lightShadows->getTempShadowTexture();
  shadowTex->getinfo(tinfo, 0);

  {
    TIME_D3D_PROFILE(find_max_depth)

    float wk = v_extract_x(proj.col0);
    float hk = v_extract_y(proj.col1);
    float q = v_extract_z(proj.col2);
    float mzkQ = v_extract_z(proj.col3); // -znQ or -zfQ

    ShaderGlobal::set_color4(proj_valuesVarId, Color4(1.0 / wk, 1.0 / hk, 1.0 / mzkQ, -q / mzkQ));

    d3d::zero_rwbufi(resultBuffer);
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), resultBuffer);
    d3d::resource_barrier({resultBuffer, RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
    d3d::resource_barrier({shadowTex, RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
    d3d::set_tex(STAGE_CS, 0, shadowTex);

    findMaxDepth2D->dispatch((tinfo.w + FMD2D_WG_DIM_W - 1) / FMD2D_WG_DIM_W, (tinfo.h + FMD2D_WG_DIM_H - 1) / FMD2D_WG_DIM_H, 1);
  }

  resultRingBuffer.startCPUCopy();
  processing = true;
}

void DistanceReadbackLights::completeQuery()
{
  if (!findMaxDepth2D)
    return;
  int stride;
  uint32_t frame;
  float *data = reinterpret_cast<float *>(resultRingBuffer.lock(stride, frame, true));
  if (!data)
    return;

  float minDist = sqrtf(*data);
  resultRingBuffer.unlock();

  // if light wasn't removed & shadow data is populated
  if (spotLights->isLightNonOptimized(lastNonOptId) && (minDist != 0.0f))
  {
    float prevValue = spotLights->getLight(lastNonOptId).pos_radius.w;

    bool shouldBeOptimized = (prevValue > minDist);
    if (shouldBeOptimized)
    {
      const int maxLogs = 100;
      auto it = lightLogCount.find(lastNonOptId);
      int logCount = it != lightLogCount.end() ? it->second : 0;
      if (logCount < maxLogs)
      {
        lightLogCount[lastNonOptId] = ++logCount;
        debug("Optimized light id: %d; radius: %lf; prev: %lf", lastNonOptId, minDist, prevValue);
      }
    }
    spotLights->setLightCullingRadius(lastNonOptId, shouldBeOptimized ? minDist : -1.0f);
    spotLights->setLightOptimized(lastNonOptId);
  }
  processing = false;
}