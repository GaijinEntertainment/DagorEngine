// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/componentTypes.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <perfMon/dag_statDrv.h>
#include <render/daBfg/bfg.h>
#include <render/daBfg/ecs/frameGraphNode.h>
#include <render/heatHazeRenderer.h>
#include <render/deferredRenderer.h>
#include <render/renderEvent.h>
#include <render/renderSettings.h>
#include <render/world/cameraParams.h>
#include <rendInst/rendInstGenRender.h>
#include <rendInst/rendInstExtra.h>
#include <drv/3d/dag_texture.h>

struct HeatHazeManager
{
  eastl::unique_ptr<HeatHazeRenderer> renderer;
};

ECS_DECLARE_RELOCATABLE_TYPE(HeatHazeManager)
ECS_REGISTER_RELOCATABLE_TYPE(HeatHazeManager, nullptr)

extern bool animchar_has_any_visible_distortion();

static bool will_render_haze(const RiGenVisibility *rendinstMainVisibility)
{
  if (!acesfx::hasVisibleHaze() && !rendinst::hasRIExtraOnLayers(rendinstMainVisibility, rendinst::LayerFlag::Distortion) &&
      !animchar_has_any_visible_distortion())
    return false;
  return true;
}

static dabfg::NodeHandle makeHeatHazeRenderClearNode(HeatHazeRenderer *heatHazeRenderer)
{
  return dabfg::register_node("heat_haze_render_clear_node", DABFG_PP_NODE_SRC, [heatHazeRenderer](dabfg::Registry registry) {
    auto const hazeResolution = registry.getResolution<2>("main_view", 1.0f / heatHazeRenderer->getHazeResolutionDivisor());

    // Only supported in WT
    G_ASSERT(!heatHazeRenderer->isHazeAppliedManual());

    auto hazeRenderedHndl = registry.createBlob<bool>("haze_rendered", dabfg::History::No).handle();
    auto hazeDepthHndl =
      registry.createTexture2d("haze_depth", dabfg::History::No, {TEXFMT_R16F | TEXCF_RTARGET | TEXCF_ESRAM_ONLY, hazeResolution})
        .atStage(dabfg::Stage::PS)
        .useAs(dabfg::Usage::COLOR_ATTACHMENT)
        .handle();

    auto hazeOffsetHndl =
      registry
        .createTexture2d("haze_offset", dabfg::History::No, {TEXFMT_A16B16G16R16F | TEXCF_RTARGET | TEXCF_ESRAM_ONLY, hazeResolution})
        .atStage(dabfg::Stage::PS)
        .useAs(dabfg::Usage::COLOR_ATTACHMENT)
        .handle();

    eastl::optional<dabfg::VirtualResourceHandle<BaseTexture, true, false>> hazeColorHndl;
    bool isColoredHazeSuppored = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("coloredHaze", false);
    if (isColoredHazeSuppored)
    {
      hazeColorHndl =
        registry.createTexture2d("haze_color", dabfg::History::No, {TEXFMT_DEFAULT | TEXCF_RTARGET | TEXCF_ESRAM_ONLY, hazeResolution})
          .atStage(dabfg::Stage::PS)
          .useAs(dabfg::Usage::COLOR_ATTACHMENT)
          .handle();
    }

    return [hazeRenderedHndl, hazeOffsetHndl, hazeDepthHndl, hazeColorHndl, heatHazeRenderer]() {
      hazeRenderedHndl.ref() = false;
      heatHazeRenderer->clearTargets(hazeColorHndl ? hazeColorHndl->get() : nullptr, hazeOffsetHndl.get(), hazeDepthHndl.get());
    };
  });
}

static dabfg::NodeHandle makeHeatHazeRenderParticlesNode(HeatHazeRenderer *heatHazeRenderer, int heatHazeLod)
{
  return dabfg::register_node("heat_haze_render_particles_node", DABFG_PP_NODE_SRC,
    [heatHazeRenderer, heatHazeLod](dabfg::Registry registry) {
      registry.orderMeAfter("under_water_fog_node");
      registry.orderMeBefore("transparent_scene_late_node");
      registry.allowAsyncPipelines();

      auto hazeRenderedHndl = registry.modifyBlob<bool>("haze_rendered").handle();
      auto farDownsampledDepth = registry.readTexture("far_downsampled_depth")
                                   .atStage(dabfg::Stage::POST_RASTER)
                                   .bindToShaderVar("downsampled_far_depth_tex")
                                   .handle();
      auto depthForTransparencyHndl =
        registry.readTexture("depth_for_transparency").atStage(dabfg::Stage::PS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();

      auto texCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();
      auto visibilityHndl = registry.readBlob<RiGenVisibility *>("rendinst_main_visibility").handle();
      auto cameraHndl = registry.readBlob<CameraParams>("current_camera")
                          .bindAsView<&CameraParams::viewTm>()
                          .bindAsProj<&CameraParams::noJitterProjTm>()
                          .handle();

      registry.requestState().setFrameBlock("global_frame");

      auto hazeDepthHndl =
        registry.modifyTexture("haze_depth").atStage(dabfg::Stage::POST_RASTER).useAs(dabfg::Usage::COLOR_ATTACHMENT).handle();
      auto hazeOffsetHndl =
        registry.modifyTexture("haze_offset").atStage(dabfg::Stage::POST_RASTER).useAs(dabfg::Usage::COLOR_ATTACHMENT).handle();
      auto hazeColorHndl = registry.modifyTexture("haze_color")
                             .atStage(dabfg::Stage::POST_RASTER)
                             .useAs(dabfg::Usage::COLOR_ATTACHMENT)
                             .optional()
                             .handle();

      struct HeatHazeNodeHandles
      {
        dabfg::VirtualResourceHandle<const BaseTexture, true, false> farDownsampledDepth;
        dabfg::VirtualResourceHandle<const BaseTexture, true, false> depthForTransparencyHndl;
        dabfg::VirtualResourceHandle<BaseTexture, true, false> hazeDepthHndl;
        dabfg::VirtualResourceHandle<BaseTexture, true, false> hazeOffsetHndl;
        dabfg::VirtualResourceHandle<BaseTexture, true, true> hazeColorHndl;

        dabfg::VirtualResourceHandle<const TexStreamingContext, false, false> texCtxHndl;
        dabfg::VirtualResourceHandle<RiGenVisibility *const, false, false> visibilityHndl;
        dabfg::VirtualResourceHandle<const CameraParams, false, false> cameraHndl;
        dabfg::VirtualResourceHandle<bool, false, false> hazeRenderedHndl;
      };

      return [handles = eastl::make_unique<HeatHazeNodeHandles>(HeatHazeNodeHandles{farDownsampledDepth, depthForTransparencyHndl,
                hazeDepthHndl, hazeOffsetHndl, hazeColorHndl, texCtxHndl, visibilityHndl, cameraHndl, hazeRenderedHndl}),
               heatHazeRenderer, heatHazeLod]() {
        auto [farDownsampledDepth, depthForTransparencyHndl, hazeDepthHndl, hazeOffsetHndl, hazeColorHndl, texCtxHndl, visibilityHndl,
          cameraHndl, hazeRenderedHndl] = *handles;
        const CameraParams &camera = cameraHndl.ref();
        auto riHazeRender = [&]() {
          TIME_D3D_PROFILE(rendinst_distortion);
          rendinst::render::renderRIGen(rendinst::RenderPass::Normal, handles->visibilityHndl.ref(), camera.viewItm,
            rendinst::LayerFlag::Distortion, rendinst::OptimizeDepthPass::No, 1U, rendinst::AtestStage::All, nullptr,
            handles->texCtxHndl.ref());

          g_entity_mgr->broadcastEventImmediate(
            UpdateStageInfoRenderDistortion(camera.viewTm, camera.noJitterProjTm, camera.viewItm, handles->texCtxHndl.ref()));
        };

        if (!will_render_haze(visibilityHndl.ref()))
          return;

        hazeRenderedHndl.ref() = true;

        G_ASSERT(heatHazeLod >= 0);
        const auto lodDepth = heatHazeLod == 0 ? depthForTransparencyHndl.view() : farDownsampledDepth.view();

        heatHazeRenderer->renderHazeParticles(hazeDepthHndl.get(), hazeOffsetHndl.get(), lodDepth.getTexId(),
          eastl::max(heatHazeLod - 1, 0), acesfx::renderTransHaze, riHazeRender);

        if (hazeColorHndl.get())
          heatHazeRenderer->renderColorHaze(hazeColorHndl.get(), acesfx::renderTransHaze, riHazeRender);
      };
    });
}

static dabfg::NodeHandle makeHeatHazeApplyNode(HeatHazeRenderer *heatHazeRenderer)
{
  return dabfg::register_node("heat_haze_apply_node", DABFG_PP_NODE_SRC, [heatHazeRenderer](dabfg::Registry registry) {
    registry.orderMeAfter("heat_haze_render_particles_node");
    // Heat haze is actually applied inside post fx node. We need to just set shader variables here before post_fx
    registry.orderMeBefore("post_fx_node");

    auto depthForTransparencyHndl = registry.readTexture("depth_for_transparency")
                                      .atStage(dabfg::Stage::ALL_GRAPHICS)
                                      .bindToShaderVar("haze_scene_depth_tex")
                                      .handle();
    auto hazeRenderedHndl = registry.readBlob<bool>("haze_rendered").handle();
    auto gameTimeHndl = registry.readBlob<float>("game_time").handle();
    registry.requestState().setFrameBlock("global_frame");

    return [hazeRenderedHndl, depthForTransparencyHndl, gameTimeHndl, heatHazeRenderer]() {
      if (!hazeRenderedHndl.ref())
        return;
      TextureInfo backBufferInfo;
      depthForTransparencyHndl.view()->getinfo(backBufferInfo);

      heatHazeRenderer->applyHaze(gameTimeHndl.ref(), nullptr, nullptr, BAD_TEXTUREID, depthForTransparencyHndl.view().getTexId(),
        nullptr, BAD_TEXTUREID, {backBufferInfo.w, backBufferInfo.h});
    };
  });
}

static void setup_heat_haze(HeatHazeManager &heatHazeManager,
  dabfg::NodeHandle &heatHazeRenderClearNode,
  dabfg::NodeHandle &heatHazeRenderParticlesNode,
  dabfg::NodeHandle &heatHazeApplyNode,
  int &heatHazeLod)
{
  heatHazeManager.renderer.reset();
  heatHazeLod = -1;

  heatHazeRenderClearNode = {};
  heatHazeRenderParticlesNode = {};
  heatHazeApplyNode = {};

  if (renderer_has_feature(FeatureRenderFlags::HAZE))
  {
    float hazeDivisor = float(max(::dgs_get_game_params()->getInt("hazeDivisor", 4), 1));
    heatHazeManager.renderer = eastl::make_unique<HeatHazeRenderer>(hazeDivisor);
    heatHazeLod = get_log2w(hazeDivisor);

    heatHazeRenderClearNode = makeHeatHazeRenderClearNode(heatHazeManager.renderer.get());
    heatHazeRenderParticlesNode = makeHeatHazeRenderParticlesNode(heatHazeManager.renderer.get(), heatHazeLod);
    heatHazeApplyNode = makeHeatHazeApplyNode(heatHazeManager.renderer.get());
  }
}

ECS_TAG(render)
ECS_ON_EVENT(OnRenderSettingsReady)
static void init_heat_haze_es_event_handler(const ecs::Event &,
  HeatHazeManager &heat_haze__manager,
  dabfg::NodeHandle &heat_haze__render_clear_node,
  dabfg::NodeHandle &heat_haze__render_particles_node,
  dabfg::NodeHandle &heat_haze__apply_node,
  int &heat_haze__lod)
{
  setup_heat_haze(heat_haze__manager, heat_haze__render_clear_node, heat_haze__render_particles_node, heat_haze__apply_node,
    heat_haze__lod);
}

ECS_TAG(render)
ECS_ON_EVENT(ChangeRenderFeatures)
static void heat_haze_render_features_changed_es_event_handler(const ecs::Event &evt,
  HeatHazeManager &heat_haze__manager,
  dabfg::NodeHandle &heat_haze__render_clear_node,
  dabfg::NodeHandle &heat_haze__render_particles_node,
  dabfg::NodeHandle &heat_haze__apply_node,
  int &heat_haze__lod)
{
  if (auto *changedFeatures = evt.cast<ChangeRenderFeatures>())
    if (!changedFeatures->isFeatureChanged(FeatureRenderFlags::HAZE))
      return;
  setup_heat_haze(heat_haze__manager, heat_haze__render_clear_node, heat_haze__render_particles_node, heat_haze__apply_node,
    heat_haze__lod);
}