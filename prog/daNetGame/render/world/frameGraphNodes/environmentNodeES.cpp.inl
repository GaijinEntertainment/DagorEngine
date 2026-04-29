// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
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
#include <render/renderEvent.h>
#include <render/world/frameGraphHelpers.h>
#include <drv/3d/dag_renderTarget.h>


extern ConVarT<bool, false> sky_is_unreachable;


namespace var
{
static ShaderVariableInfo clouds_use_smart_upscaling("clouds_use_smart_upscaling", true);
}

eastl::fixed_vector<dafg::NodeHandle, 5> makeEnvironmentNodes()
{
  auto *skies = get_daskies();
  G_ASSERT(skies);
  const bool panorama = skies->panoramaEnabled();

  decltype(makeEnvironmentNodes()) result;

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

  auto doPrepareSkies = [](const auto &currentCameraHndl, auto &resPack, CloudsRenderFlags flags = CloudsRenderFlags::None) {
    auto [farDownsampledDepthHndl, farDownsampledDepthHistoryHndl, belowCloudsHndl, mainPovSkiesDataHndl] = resPack;

    if (DAGOR_UNLIKELY(has_custom_sky()))
      return;

    auto *skies = get_daskies();
    G_ASSERT_RETURN(skies, );

    const CameraParams &cam = currentCameraHndl.ref();
    const bool canBeInsideClouds = sky_is_unreachable && belowCloudsHndl.ref();
    const DPoint3 origin = skies->calcViewPos(cam.viewItm);
    const DPoint3 dir = dpoint3(cam.viewItm.getcol(2));

    STATE_GUARD_0(ShaderGlobal::set_int(var::clouds_use_smart_upscaling, VALUE), 1);

    skies->prepareSkyAndClouds(canBeInsideClouds, origin, dir, /*render_sun_moon*/ 3, farDownsampledDepthHndl.get(),
      farDownsampledDepthHistoryHndl.get(), mainPovSkiesDataHndl.ref(), skies->calcViewTm(cam.viewTm), cam.jitterProjTm,
      UpdateSky::OnWithoutCloudsVisibilityCheck, /*fixed_offset*/ false, SKY_PREPARE_THRESHOLD, flags);
  };

  if (panorama)
  {
    result.push_back(dafg::register_node("prepare_panorama_skies", DAFG_PP_NODE_SRC,
      [requestCommonPrepareSkiesState, doPrepareSkies](dafg::Registry registry) {
        auto camera = use_current_camera(registry);
        auto currentCameraHndl = CameraViewShvars{camera}.bindViewVecs().toHandle();
        auto resPack = requestCommonPrepareSkiesState(registry);

        registry.modifyBlob<OrderingToken>("prepare_skies_token");

        return [currentCameraHndl, resPack, doPrepareSkies]() { doPrepareSkies(currentCameraHndl, resPack); };
      }));
  }
  else
  {
    result.push_back(dafg::register_node("prepare_volume_skies", DAFG_PP_NODE_SRC,
      [requestCommonPrepareSkiesState, doPrepareSkies](dafg::Registry registry) {
        auto currentCamera = use_camera_in_camera(registry);
        auto currentCameraHndl = CameraViewShvars{currentCamera}.bindViewVecs().toHandle();
        auto prevCameraHndl = read_history_camera_in_camera(registry).handle();
        auto resPack = requestCommonPrepareSkiesState(registry);

        auto viewVecsTuple = eastl::make_tuple(registry.readBlob<ViewVecs>("view_vectors").handle(),
          registry.readBlobHistory<ViewVecs>("view_vectors").handle());

        registry.modifyBlob<OrderingToken>("prepare_skies_token");

        return [viewvecs = eastl::make_unique<decltype(viewVecsTuple)>(viewVecsTuple), currentCameraHndl, prevCameraHndl, resPack,
                 doPrepareSkies](const dafg::multiplexing::Index &multiplexing_index) {
          const auto &[mainViewVecs, prevMainViewVecs] = *viewvecs;

          camera_in_camera::ApplyPostfxState camcam{multiplexing_index, currentCameraHndl.ref(), prevCameraHndl.ref()};
          const CameraParams &camera = currentCameraHndl.ref();
          const CameraParams &prevCamera = prevCameraHndl.ref();
          const ViewVecs &vv = mainViewVecs.ref();
          const ViewVecs &prevVV = prevMainViewVecs.ref();

          CloudsRenderFlags renderFlags = CloudsRenderFlags::None;
          if (camera_in_camera::is_main_view(multiplexing_index))
          {
            renderFlags = CloudsRenderFlags::MainView;
            set_reprojection(camera.jitterProjTm, camera.viewRotJitterProjTm, Point2(prevCamera.znear, prevCamera.zfar),
              prevCamera.viewRotJitterProjTm, vv.viewVecLT, vv.viewVecRT, vv.viewVecLB, vv.viewVecRB, prevVV.viewVecLT,
              prevVV.viewVecRT, prevVV.viewVecLB, prevVV.viewVecRB, camera.cameraWorldPos, prevCamera.cameraWorldPos);
          }

          if (camera_in_camera::is_frame_after_deactivation())
            renderFlags = renderFlags | CloudsRenderFlags::RestartTAA;

          doPrepareSkies(currentCameraHndl, resPack, renderFlags);
        };
      }));
  }

  result.push_back(dafg::register_node("prepare_skies_per_camera_token_provider", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.createBlob<OrderingToken>("prepare_skies_token");
    registry.createBlob<OrderingToken>("skies_rendered_token");
  }));

  auto requestCommonSkiesState = [](dafg::Registry registry, const char *from, const char *to) {
    use_volfog(registry, dafg::Stage::PS_OR_CS);

    // Optimization: keep the depth in RO state
    auto pass = registry.requestRenderPass()
                  .color({"opaque_with_envi"})
                  .depthRoAndBindToShaderVars({"opaque_depth_with_water_before_clouds"}, {});

    registry.rename(from, to).blob<OrderingToken>();
    auto mainPovSkiesDataHndl = registry.read("main_pov_skies_data").blob<SkiesData *>().handle();

    registry.modify("skies_rendered_token");

    return eastl::make_tuple(eastl::move(pass), eastl::move(mainPovSkiesDataHndl));
  };

  auto execSkies = [](auto currentCameraHndl, auto mainPovSkiesDataHndl, bool render_sky_overlay) {
    const CameraParams &cam = currentCameraHndl.ref();
    if (DAGOR_UNLIKELY(try_render_custom_sky(cam.viewTm, cam.jitterProjTm, cam.jitterPersp)))
      return;

    auto *skies = get_daskies();
    G_ASSERT_RETURN(skies, );

    if (render_sky_overlay)
      skies->renderSky(mainPovSkiesDataHndl.ref(), skies->calcViewTm(cam.viewTm), cam.jitterProjTm, cam.jitterPersp);
    else
      skies->renderSkyWithoutOverlay(mainPovSkiesDataHndl.ref(), skies->calcViewTm(cam.viewTm), cam.jitterProjTm, cam.jitterPersp);
  };

  if (panorama)
  {
    result.push_back(dafg::register_node("render_skies_with_panorama", DAFG_PP_NODE_SRC,
      [execSkies, requestCommonSkiesState](dafg::Registry registry) {
        auto [pass, mainPovSkiesDataHndl] =
          requestCommonSkiesState(registry, "prepare_skies_token", "prepare_skies_token_after_skies");

        // Only enable VRS with panorama
        eastl::move(pass).vrsRate(VRS_RATE_TEXTURE_NAME);
        auto camera = use_camera_in_camera(registry);
        auto currentCameraHndl = CameraViewShvars{camera}.bindViewVecs().toHandle();

        return [execSkies, currentCameraHndl = currentCameraHndl, mainPovSkiesDataHndl = mainPovSkiesDataHndl](
                 const dafg::multiplexing::Index &multiplexing_index) {
          camera_in_camera::ApplyPostfxState camcam{multiplexing_index, currentCameraHndl.ref(), camera_in_camera::USE_STENCIL};
          execSkies(currentCameraHndl, mainPovSkiesDataHndl, true);
        };
      }));
  }
  else
  {
    result.push_back(
      dafg::register_node("render_skies_no_panorama", DAFG_PP_NODE_SRC, [execSkies, requestCommonSkiesState](dafg::Registry registry) {
        auto [_, mainPovSkiesDataHndl] = requestCommonSkiesState(registry, "prepare_skies_token", "prepare_skies_token_before_env");
        auto currentCameraHndl = use_current_camera(registry).handle();

        return [execSkies, currentCameraHndl = currentCameraHndl, mainPovSkiesDataHndl = mainPovSkiesDataHndl]() {
          execSkies(currentCameraHndl, mainPovSkiesDataHndl, false);
        };
      }));

    result.push_back(
      dafg::register_node("render_skies_env", DAFG_PP_NODE_SRC, [execSkies, requestCommonSkiesState](dafg::Registry registry) {
        auto [_, mainPovSkiesDataHndl] =
          requestCommonSkiesState(registry, "prepare_skies_token_before_env", "prepare_skies_token_after_skies");
        auto camera = use_camera_in_camera(registry);
        auto currentCameraHndl = CameraViewShvars{camera}.bindViewVecs().toHandle();

        return [execSkies, currentCameraHndl = currentCameraHndl, mainPovSkiesDataHndl = mainPovSkiesDataHndl](
                 const dafg::multiplexing::Index &multiplexing_index) {
          if (DAGOR_UNLIKELY(has_custom_sky()))
            return;

          const CameraParams &cam = currentCameraHndl.ref();
          camera_in_camera::ApplyPostfxState camcam{multiplexing_index, cam, camera_in_camera::USE_STENCIL};
          auto *skies = get_daskies();
          G_ASSERT_RETURN(skies, );

          TMatrix viewTm = skies->calcViewTm(cam.viewTm);
          skies->renderSkyOverlay(mainPovSkiesDataHndl.ref(), viewTm, cam.jitterProjTm, cam.jitterPersp);
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

    auto currentCamera = use_camera_in_camera(registry);
    auto currentCameraHndl = CameraViewShvars{currentCamera}.bindViewVecs().toHandle();

    auto belowCloudsHndl = registry.readBlob<bool>("below_clouds").handle();

    registry.readBlob<OrderingToken>("prepare_skies_token_after_skies");
    auto mainPovSkiesDataHndl = registry.read("main_pov_skies_data").blob<SkiesData *>().handle();
    registry.readBlob<OrderingToken>("aurora_borealis").optional();

    return [farDownsampledDepthHndl, gbufDepthAfterResolveHndl, currentCameraHndl, belowCloudsHndl, mainPovSkiesDataHndl](
             const dafg::multiplexing::Index &multiplexing_index) {
      const CameraParams &cam = currentCameraHndl.ref();
      camera_in_camera::ApplyPostfxState camcam{multiplexing_index, cam, camera_in_camera::USE_STENCIL};

      if (DAGOR_UNLIKELY(try_render_custom_sky(cam.viewTm, cam.jitterProjTm, cam.jitterPersp)))
        return;

      auto *skies = get_daskies();
      G_ASSERT_RETURN(skies, );

      CloudsRenderFlags renderFlags = CloudsRenderFlags::SetCameraVars;
      if (camera_in_camera::is_main_view(multiplexing_index))
        renderFlags = renderFlags | CloudsRenderFlags::MainView;

      const bool canBeInsideClouds = sky_is_unreachable && belowCloudsHndl.ref();
      skies->renderCloudsToTarget(canBeInsideClouds, farDownsampledDepthHndl.get(), gbufDepthAfterResolveHndl.get(),
        mainPovSkiesDataHndl.ref(), skies->calcViewTm(cam.viewTm), cam.jitterProjTm, Point4(1, 1, 0, 0), renderFlags);
    };
  }));

  return result;
}


ECS_TAG(render)
ECS_ON_EVENT(OnCameraNodeConstruction)
static void create_envi_nodes_node_es(const OnCameraNodeConstruction &evt)
{
  if (auto *skies = get_daskies())
  {
    for (auto &&n : makeEnvironmentNodes())
      evt.nodes->push_back(eastl::move(n));
  }
}
