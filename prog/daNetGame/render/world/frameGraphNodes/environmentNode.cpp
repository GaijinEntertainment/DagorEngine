// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/world/defaultVrsSettings.h>
#include <render/deferredRenderer.h>
#include <render/skies.h>
#include <render/world/worldRendererQueries.h>
#include <render/world/cameraParams.h>
#include <render/rendererFeatures.h>
#include <render/viewVecs.h>
#include <render/daFrameGraph/daFG.h>
#include <render/set_reprojection.h>

#include <shaders/dag_shaderBlock.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_convar.h>

#include "frameGraphNodes.h"
#include <render/world/frameGraphHelpers.h>
#include <drv/3d/dag_renderTarget.h>


extern ConVarT<bool, false> sky_is_unreachable;


eastl::fixed_vector<dafg::NodeHandle, 4> makeEnvironmentNodes()
{
  auto *skies = get_daskies();
  G_ASSERT(skies);
  const bool panorama = skies->panoramaEnabled();

  eastl::fixed_vector<dafg::NodeHandle, 4> result;

  auto requestCommonPrepareSkiesState = [](dafg::Registry registry) {
    auto farDownsampledDepthHndl =
      registry.read("far_downsampled_depth").texture().atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    auto farDownsampledDepthHistoryHndl = registry.historyFor("far_downsampled_depth")
                                            .texture()
                                            .atStage(dafg::Stage::PS_OR_CS)
                                            .useAs(dafg::Usage::SHADER_RESOURCE)
                                            .handle();
    auto belowCloudsHndl = registry.readBlob<bool>("below_clouds").handle();
    auto mainPovSkiesDataHndl = registry.read("main_pov_skies_data").blob<SkiesData *>().handle();

    return eastl::make_tuple(farDownsampledDepthHndl, farDownsampledDepthHistoryHndl, belowCloudsHndl, mainPovSkiesDataHndl);
  };

  auto doPrepareSkies = [](const auto &currentCameraHndl, auto &resPack, const bool acquare_new_resource = true) {
    auto [farDownsampledDepthHndl, farDownsampledDepthHistoryHndl, belowCloudsHndl, mainPovSkiesDataHndl] = resPack;

    G_ASSERTF(!renderer_has_feature(FeatureRenderFlags::FORWARD_RENDERING), "This is not supposed to be run on forward though?");

    if (DAGOR_UNLIKELY(has_custom_sky()))
      return;

    auto *skies = get_daskies();
    G_ASSERT_RETURN(skies, );

    const CameraParams &cam = currentCameraHndl.ref();
    const bool infinite = sky_is_unreachable && belowCloudsHndl.ref();
    const DPoint3 origin = skies->calcViewPos(cam.viewItm);
    const DPoint3 dir = dpoint3(cam.viewItm.getcol(2));

    const bool setCameraVars = false;
    skies->prepareSkyAndClouds(infinite, origin, dir, /*render_sun_moon*/ 3, farDownsampledDepthHndl.view(),
      farDownsampledDepthHistoryHndl.view(), mainPovSkiesDataHndl.ref(), skies->calcViewTm(cam.viewTm), cam.jitterProjTm,
      UpdateSky::OnWithoutCloudsVisibilityCheck, /*fixed_offset*/ false, SKY_PREPARE_THRESHOLD, nullptr, acquare_new_resource,
      setCameraVars);
  };

  if (panorama)
  {
    result.push_back(dafg::register_node("prepare_panorama_skies", DAFG_PP_NODE_SRC,
      [requestCommonPrepareSkiesState, doPrepareSkies](dafg::Registry registry) {
        auto currentCameraHndl = use_current_camera(registry);
        auto resPack = requestCommonPrepareSkiesState(registry);

        registry.orderMeBefore("prepare_skies_per_camera_token_provider");

        return [currentCameraHndl, resPack, doPrepareSkies]() {
          const CameraParams &camera = currentCameraHndl.ref();
          set_viewvecs_to_shader(camera.viewTm, camera.jitterProjTm);
          doPrepareSkies(currentCameraHndl, resPack);
        };
      }));
  }
  else
  {
    result.push_back(dafg::register_node("prepare_volume_skies", DAFG_PP_NODE_SRC,
      [requestCommonPrepareSkiesState, doPrepareSkies](dafg::Registry registry) {
        auto currentCameraHndl = use_camera_in_camera(registry);
        auto prevCameraHndl = read_history_camera_in_camera(registry).handle();
        auto resPack = requestCommonPrepareSkiesState(registry);

        auto viewVecsTuple = eastl::make_tuple(registry.readBlob<ViewVecs>("view_vectors").handle(),
          registry.readBlobHistory<ViewVecs>("view_vectors").handle());

        registry.orderMeBefore("prepare_skies_per_camera_token_provider");

        return [viewvecs = eastl::make_unique<decltype(viewVecsTuple)>(viewVecsTuple), currentCameraHndl, prevCameraHndl, resPack,
                 doPrepareSkies](const dafg::multiplexing::Index &multiplexing_index) {
          const auto &[mainViewVecs, prevMainViewVecs] = *viewvecs;

          camera_in_camera::ApplyPostfxState camcam{multiplexing_index, currentCameraHndl.ref(), prevCameraHndl.ref()};
          const CameraParams &camera = currentCameraHndl.ref();
          const CameraParams &prevCamera = prevCameraHndl.ref();
          const ViewVecs &vv = mainViewVecs.ref();
          const ViewVecs &prevVV = prevMainViewVecs.ref();

          const bool acquireNewResources = camera_in_camera::is_main_view(multiplexing_index);
          if (camera_in_camera::is_main_view(multiplexing_index))
          {
            set_reprojection(camera.jitterProjTm, camera.viewRotJitterProjTm, Point2(prevCamera.znear, prevCamera.zfar),
              prevCamera.viewRotJitterProjTm, vv.viewVecLT, vv.viewVecRT, vv.viewVecLB, vv.viewVecRB, prevVV.viewVecLT,
              prevVV.viewVecRT, prevVV.viewVecLB, prevVV.viewVecRB, camera.cameraWorldPos, prevCamera.cameraWorldPos);
          }

          doPrepareSkies(currentCameraHndl, resPack, acquireNewResources);
        };
      }));
  }

  result.push_back(dafg::register_node("prepare_skies_per_camera_token_provider", DAFG_PP_NODE_SRC,
    [](dafg::Registry registry) { registry.createBlob<OrderingToken>("prepare_skies_token", dafg::History::No); }));

  auto requestCommonSkiesState = [](dafg::Registry registry) {
    use_volfog(registry, dafg::Stage::PS_OR_CS);

    // Optimization: keep the depth in RO state
    auto pass = registry.requestRenderPass()
                  .color({"opaque_with_envi"})
                  .depthRoAndBindToShaderVars({"opaque_depth_with_water_before_clouds"}, {});

    // Little trick to get the correct ordering
    auto mainPovSkiesDataHndl =
      registry.rename("main_pov_skies_data", "main_pov_skies_data_after_skies", dafg::History::No).blob<SkiesData *>().handle();

    return eastl::make_tuple(eastl::move(pass), eastl::move(mainPovSkiesDataHndl));
  };

  auto execSkies = [](auto currentCameraHndl, auto mainPovSkiesDataHndl) {
    G_ASSERTF(!renderer_has_feature(FeatureRenderFlags::FORWARD_RENDERING), "This is not supposed to be run on forward though?");

    const CameraParams &cam = currentCameraHndl.ref();
    if (DAGOR_UNLIKELY(try_render_custom_sky(cam.viewTm, cam.jitterProjTm, cam.jitterPersp)))
      return;

    auto *skies = get_daskies();
    G_ASSERT_RETURN(skies, );

    skies->renderSky(mainPovSkiesDataHndl.ref(), skies->calcViewTm(cam.viewTm), cam.jitterProjTm, cam.jitterPersp,
      RenderPrepared::LowresOnFarPlane, nullptr, nullptr);
  };

  if (panorama)
  {
    result.push_back(dafg::register_node("render_skies_with_panorama", DAFG_PP_NODE_SRC,
      [execSkies, requestCommonSkiesState](dafg::Registry registry) {
        auto [pass, mainPovSkiesDataHndl] = requestCommonSkiesState(registry);

        // Only enable VRS with panorama
        eastl::move(pass).vrsRate(VRS_RATE_TEXTURE_NAME);
        auto currentCameraHndl = use_camera_in_camera(registry);

        return [execSkies, currentCameraHndl = currentCameraHndl, mainPovSkiesDataHndl = mainPovSkiesDataHndl](
                 const dafg::multiplexing::Index &multiplexing_index) {
          camera_in_camera::ApplyPostfxState camcam{multiplexing_index, currentCameraHndl.ref(), camera_in_camera::USE_STENCIL};

          const CameraParams &camera = currentCameraHndl.ref();
          if (camera_in_camera::is_main_view(multiplexing_index))
            set_viewvecs_to_shader(camera.viewTm, camera.jitterProjTm);

          execSkies(currentCameraHndl, mainPovSkiesDataHndl);
        };
      }));
  }
  else
  {
    result.push_back(
      dafg::register_node("render_skies_no_panorama", DAFG_PP_NODE_SRC, [execSkies, requestCommonSkiesState](dafg::Registry registry) {
        auto [_, mainPovSkiesDataHndl] = requestCommonSkiesState(registry);
        auto currentCameraHndl = use_current_camera(registry);

        return [execSkies, currentCameraHndl = currentCameraHndl, mainPovSkiesDataHndl = mainPovSkiesDataHndl]() {
          execSkies(currentCameraHndl, mainPovSkiesDataHndl);
        };
      }));
  }

  result.push_back(dafg::register_node("render_clouds", DAFG_PP_NODE_SRC, [panorama](dafg::Registry registry) {
    registry.readTexture("checkerboard_depth")
      .atStage(dafg::Stage::PS_OR_CS)
      .bindToShaderVar("downsampled_checkerboard_depth_tex")
      .optional();
    registry.read("checkerboard_depth_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("downsampled_checkerboard_depth_tex_samplerstate")
      .optional();

    auto farDownsampledDepthHndl =
      registry.readTexture("far_downsampled_depth").atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();

    registry.readTexture("close_depth").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("clouds_close_depth_tex").optional();

    auto finalTargetHndlReq = registry.modify("opaque_with_envi").texture();
    auto gbufDepthAfterResolveReq = registry.read("opaque_depth_with_water_before_clouds").texture();

    auto pass = registry.requestRenderPass().color({finalTargetHndlReq}).depthRoAndBindToShaderVars(gbufDepthAfterResolveReq, {});

    // Only enable VRS with panorama
    if (panorama)
      eastl::move(pass).vrsRate(VRS_RATE_TEXTURE_NAME);


    auto gbufDepthAfterResolveHndl = eastl::move(gbufDepthAfterResolveReq)
                                       .atStage(dafg::Stage::POST_RASTER)
                                       .useAs(dafg::Usage::DEPTH_ATTACHMENT_AND_SHADER_RESOURCE)
                                       .handle();

    auto currentCameraHndl = use_camera_in_camera(registry);

    auto belowCloudsHndl = registry.readBlob<bool>("below_clouds").handle();

    auto mainPovSkiesDataHndl = registry.read("main_pov_skies_data_after_skies").blob<SkiesData *>().handle();

    return [farDownsampledDepthHndl, gbufDepthAfterResolveHndl, currentCameraHndl, belowCloudsHndl, mainPovSkiesDataHndl](
             const dafg::multiplexing::Index &multiplexing_index) {
      G_ASSERTF(!renderer_has_feature(FeatureRenderFlags::FORWARD_RENDERING), "This is not supposed to be run on forward though?");

      const CameraParams &cam = currentCameraHndl.ref();
      camera_in_camera::ApplyPostfxState camcam{multiplexing_index, cam, camera_in_camera::USE_STENCIL};

      if (DAGOR_UNLIKELY(try_render_custom_sky(cam.viewTm, cam.jitterProjTm, cam.jitterPersp)))
        return;

      auto *skies = get_daskies();
      G_ASSERT_RETURN(skies, );

      const bool infinite = sky_is_unreachable && belowCloudsHndl.ref();
      skies->renderClouds(infinite, farDownsampledDepthHndl.view(), gbufDepthAfterResolveHndl.view().getTexId(),
        mainPovSkiesDataHndl.ref(), skies->calcViewTm(cam.viewTm), cam.jitterProjTm);
    };
  }));

  return result;
}
