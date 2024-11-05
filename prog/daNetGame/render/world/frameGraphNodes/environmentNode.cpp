// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/world/defaultVrsSettings.h>
#include <render/deferredRenderer.h>
#include <render/skies.h>
#include <render/world/worldRendererQueries.h>
#include <render/world/cameraParams.h>
#include <render/rendererFeatures.h>
#include <render/daBfg/bfg.h>

#include <shaders/dag_shaderBlock.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_convar.h>

#include "frameGraphNodes.h"
#include <drv/3d/dag_renderTarget.h>


extern ConVarT<bool, false> sky_is_unreachable;


eastl::fixed_vector<dabfg::NodeHandle, 3> makeEnvironmentNodes()
{
  auto *skies = get_daskies();
  G_ASSERT(skies);
  const bool panorama = skies->panoramaEnabled();

  eastl::fixed_vector<dabfg::NodeHandle, 3> result;

  result.push_back(dabfg::register_node("prepare_skies", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    auto farDownsampledDepthHndl =
      registry.read("far_downsampled_depth").texture().atStage(dabfg::Stage::PS_OR_CS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();
    auto farDownsampledDepthHistoryHndl = registry.historyFor("far_downsampled_depth")
                                            .texture()
                                            .atStage(dabfg::Stage::PS_OR_CS)
                                            .useAs(dabfg::Usage::SHADER_RESOURCE)
                                            .handle();

    auto currentCameraHndl = registry.readBlob<CameraParams>("current_camera")
                               .bindAsView<&CameraParams::viewTm>()
                               .bindAsProj<&CameraParams::jitterProjTm>()
                               .handle();

    auto belowCloudsHndl = registry.readBlob<bool>("below_clouds").handle();

    auto mainPovSkiesDataHndl = registry.read("main_pov_skies_data").blob<SkiesData *>().handle();

    return [farDownsampledDepthHndl, farDownsampledDepthHistoryHndl, currentCameraHndl, belowCloudsHndl, mainPovSkiesDataHndl]() {
      G_ASSERTF(!renderer_has_feature(FeatureRenderFlags::FORWARD_RENDERING), "This is not supposed to be run on forward though?");

      if (DAGOR_UNLIKELY(has_custom_sky_render(CustomSkyRequest::CHECK_ONLY)))
        return;

      auto *skies = get_daskies();
      G_ASSERT_RETURN(skies, );

      const CameraParams &cam = currentCameraHndl.ref();
      const bool infinite = sky_is_unreachable && belowCloudsHndl.ref();
      const DPoint3 origin = skies->calcViewPos(cam.viewItm);
      const DPoint3 dir = dpoint3(cam.viewItm.getcol(2));

      skies->prepareSkyAndClouds(infinite, origin, dir, /*render_sun_moon*/ 3, farDownsampledDepthHndl.view(),
        farDownsampledDepthHistoryHndl.view(), mainPovSkiesDataHndl.ref(), skies->calcViewTm(cam.viewTm), cam.jitterProjTm,
        /*update_sky*/ true, /*fixed_offset*/ false);
    };
  }));

  result.push_back(dabfg::register_node("render_skies", DABFG_PP_NODE_SRC, [panorama](dabfg::Registry registry) {
    auto state = registry.requestState();

    // Optimization: keep the depth in RO state
    auto pass = registry.requestRenderPass()
                  .color({"opaque_with_envi"})
                  .depthRoAndBindToShaderVars({"opaque_depth_with_water_before_clouds"}, {});

    // Only enable VRS with panorama
    if (panorama)
      use_default_vrs(pass, state);

    auto currentCameraHndl = registry.readBlob<CameraParams>("current_camera")
                               .bindAsView<&CameraParams::viewTm>()
                               .bindAsProj<&CameraParams::jitterProjTm>()
                               .handle();

    // Little trick to get the correct ordering
    auto mainPovSkiesDataHndl =
      registry.rename("main_pov_skies_data", "main_pov_skies_data_after_skies", dabfg::History::No).blob<SkiesData *>().handle();

    return [currentCameraHndl, mainPovSkiesDataHndl]() {
      G_ASSERTF(!renderer_has_feature(FeatureRenderFlags::FORWARD_RENDERING), "This is not supposed to be run on forward though?");

      if (DAGOR_UNLIKELY(has_custom_sky_render(CustomSkyRequest::RENDER_IF_EXISTS)))
        return;

      auto *skies = get_daskies();
      G_ASSERT_RETURN(skies, );

      const CameraParams &cam = currentCameraHndl.ref();

      skies->renderSky(mainPovSkiesDataHndl.ref(), skies->calcViewTm(cam.viewTm), cam.jitterProjTm, cam.jitterPersp, true, nullptr);
    };
  }));


  result.push_back(dabfg::register_node("render_clouds", DABFG_PP_NODE_SRC, [panorama](dabfg::Registry registry) {
    registry.readTexture("checkerboard_depth")
      .atStage(dabfg::Stage::PS_OR_CS)
      .bindToShaderVar("downsampled_checkerboard_depth_tex")
      .optional();
    registry.read("checkerboard_depth_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("downsampled_checkerboard_depth_tex_samplerstate")
      .optional();

    auto farDownsampledDepthHndl =
      registry.readTexture("far_downsampled_depth").atStage(dabfg::Stage::PS_OR_CS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();

    auto finalTargetHndlReq = registry.modify("opaque_with_envi").texture();
    auto gbufDepthAfterResolveReq = registry.read("opaque_depth_with_water_before_clouds").texture();

    auto pass = registry.requestRenderPass().color({finalTargetHndlReq}).depthRoAndBindToShaderVars(gbufDepthAfterResolveReq, {});

    auto state = registry.requestState();

    // Only enable VRS with panorama
    if (panorama)
      use_default_vrs(pass, state);


    auto gbufDepthAfterResolveHndl = eastl::move(gbufDepthAfterResolveReq)
                                       .atStage(dabfg::Stage::POST_RASTER)
                                       .useAs(dabfg::Usage::DEPTH_ATTACHMENT_AND_SHADER_RESOURCE)
                                       .handle();

    auto currentCameraHndl = registry.readBlob<CameraParams>("current_camera")
                               .bindAsView<&CameraParams::viewTm>()
                               .bindAsProj<&CameraParams::jitterProjTm>()
                               .handle();

    auto belowCloudsHndl = registry.readBlob<bool>("below_clouds").handle();

    auto mainPovSkiesDataHndl = registry.read("main_pov_skies_data_after_skies").blob<SkiesData *>().handle();

    return [farDownsampledDepthHndl, gbufDepthAfterResolveHndl, currentCameraHndl, belowCloudsHndl, mainPovSkiesDataHndl]() {
      G_ASSERTF(!renderer_has_feature(FeatureRenderFlags::FORWARD_RENDERING), "This is not supposed to be run on forward though?");

      if (DAGOR_UNLIKELY(has_custom_sky_render(CustomSkyRequest::RENDER_IF_EXISTS)))
        return;

      auto *skies = get_daskies();
      G_ASSERT_RETURN(skies, );

      const CameraParams &cam = currentCameraHndl.ref();

      const bool infinite = sky_is_unreachable && belowCloudsHndl.ref();
      skies->renderClouds(infinite, farDownsampledDepthHndl.view(), gbufDepthAfterResolveHndl.view().getTexId(),
        mainPovSkiesDataHndl.ref(), skies->calcViewTm(cam.viewTm), cam.jitterProjTm);
    };
  }));

  return result;
}
