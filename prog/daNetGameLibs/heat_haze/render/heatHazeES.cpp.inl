// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/componentTypes.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <perfMon/dag_statDrv.h>
#include <render/daFrameGraph/daFG.h>
#include <render/daFrameGraph/ecs/frameGraphNode.h>
#include <render/heatHazeRenderer.h>
#include <render/deferredRenderer.h>
#include <render/renderEvent.h>
#include <render/renderSettings.h>
#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/cameraViewVisibilityManager.h>
#include <rendInst/rendInstGenRender.h>
#include <rendInst/rendInstExtra.h>
#include <drv/3d/dag_texture.h>

struct HeatHazeManager
{
  eastl::unique_ptr<HeatHazeRenderer> renderer;
};

ECS_DECLARE_RELOCATABLE_TYPE(HeatHazeManager)
ECS_REGISTER_RELOCATABLE_TYPE(HeatHazeManager, nullptr)

static bool will_render_haze(const RiGenVisibility *rendinstMainVisibility)
{
  if (!acesfx::hasVisibleHaze() && !rendinst::hasRIExtraOnLayers(rendinstMainVisibility, rendinst::LayerFlag::Distortion))
    return false;
  return true;
}

static dafg::NodeHandle makeHeatHazeRenderClearNode(HeatHazeRenderer *heatHazeRenderer)
{
  return dafg::register_node("heat_haze_render_clear_node", DAFG_PP_NODE_SRC, [heatHazeRenderer](dafg::Registry registry) {
    auto const hazeResolution = registry.getResolution<2>("main_view", 1.0f / heatHazeRenderer->getHazeResolutionDivisor());

    // Only supported in WT
    G_ASSERT(!heatHazeRenderer->isHazeAppliedManual());

    registry.create("haze_rendered", dafg::History::No).blob<bool>(false);
    registry.createTexture2d("haze_depth", dafg::History::No, {TEXFMT_R16F | TEXCF_RTARGET | TEXCF_ESRAM_ONLY, hazeResolution})
      .clear(ResourceClearValue{});

    registry
      .createTexture2d("haze_offset", dafg::History::No, {TEXFMT_A16B16G16R16F | TEXCF_RTARGET | TEXCF_ESRAM_ONLY, hazeResolution})
      .clear(ResourceClearValue{});

    bool isColoredHazeSuppored = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("coloredHaze", false);
    if (isColoredHazeSuppored)
    {
      registry.createTexture2d("haze_color", dafg::History::No, {TEXFMT_DEFAULT | TEXCF_RTARGET | TEXCF_ESRAM_ONLY, hazeResolution})
        .clear(make_clear_value(1.0f, 1.0f, 1.0f, 1.0f));
    }
  });
}

static dafg::NodeHandle makeHeatHazeRenderParticlesNode(HeatHazeRenderer *heatHazeRenderer, int heatHazeLod)
{
  return dafg::register_node("heat_haze_render_particles_node", DAFG_PP_NODE_SRC,
    [heatHazeRenderer, heatHazeLod](dafg::Registry registry) {
      registry.orderMeAfter("under_water_fog_node");
      registry.orderMeBefore("transparent_scene_late_node");
      registry.allowAsyncPipelines();

      auto hazeRenderedHndl = registry.modifyBlob<bool>("haze_rendered").handle();
      auto farDownsampledDepth = registry.readTexture("far_downsampled_depth")
                                   .atStage(dafg::Stage::POST_RASTER)
                                   .bindToShaderVar("downsampled_far_depth_tex")
                                   .handle();

      const bool hasCamCamFeature = renderer_has_feature(FeatureRenderFlags::CAMERA_IN_CAMERA);
      const bool hasStencil = hasCamCamFeature;
      const dafg::Usage depthForTransparencyUsage =
        hasCamCamFeature ? dafg::Usage::DEPTH_ATTACHMENT_AND_SHADER_RESOURCE : dafg::Usage::SHADER_RESOURCE;
      auto depthForTransparencyHndl =
        registry.readTexture("depth_for_transparency").atStage(dafg::Stage::PS).useAs(depthForTransparencyUsage).handle();

      auto texCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();
      auto cameraHndl = use_camera_in_camera(registry, false);

      registry.requestState().setFrameBlock("global_frame");

      auto hazeDepthHndl =
        registry.modifyTexture("haze_depth").atStage(dafg::Stage::POST_RASTER).useAs(dafg::Usage::COLOR_ATTACHMENT).handle();
      auto hazeOffsetHndl =
        registry.modifyTexture("haze_offset").atStage(dafg::Stage::POST_RASTER).useAs(dafg::Usage::COLOR_ATTACHMENT).handle();
      auto hazeColorHndl = registry.modifyTexture("haze_color")
                             .atStage(dafg::Stage::POST_RASTER)
                             .useAs(dafg::Usage::COLOR_ATTACHMENT)
                             .optional()
                             .handle();

      struct HeatHazeNodeHandles
      {
        dafg::VirtualResourceHandle<const BaseTexture, true, false> farDownsampledDepth;
        dafg::VirtualResourceHandle<const BaseTexture, true, false> depthForTransparencyHndl;
        dafg::VirtualResourceHandle<BaseTexture, true, false> hazeDepthHndl;
        dafg::VirtualResourceHandle<BaseTexture, true, false> hazeOffsetHndl;
        dafg::VirtualResourceHandle<BaseTexture, true, true> hazeColorHndl;

        dafg::VirtualResourceHandle<const TexStreamingContext, false, false> texCtxHndl;
        dafg::VirtualResourceHandle<const CameraParams, false, false> cameraHndl;
        dafg::VirtualResourceHandle<bool, false, false> hazeRenderedHndl;

        bool hasStencil;
      };

      return [handles = eastl::make_unique<HeatHazeNodeHandles>(HeatHazeNodeHandles{farDownsampledDepth, depthForTransparencyHndl,
                hazeDepthHndl, hazeOffsetHndl, hazeColorHndl, texCtxHndl, cameraHndl, hazeRenderedHndl, hasStencil}),
               heatHazeRenderer, heatHazeLod](const dafg::multiplexing::Index &multiplexing_index) {
        auto [farDownsampledDepth, depthForTransparencyHndl, hazeDepthHndl, hazeOffsetHndl, hazeColorHndl, texCtxHndl, cameraHndl,
          hazeRenderedHndl, hasStencil] = *handles;

        const CameraParams &camera = cameraHndl.ref();
        RiGenVisibility *riMainVisibility = camera.jobsMgr->getRiMainVisibility();

        auto riHazeRender = [&]() {
          TIME_D3D_PROFILE(rendinst_distortion);
          rendinst::render::renderRIGen(rendinst::RenderPass::Normal, riMainVisibility, camera.viewItm,
            rendinst::LayerFlag::Distortion, rendinst::OptimizeDepthPass::No, 1U, rendinst::AtestStage::All, nullptr,
            handles->texCtxHndl.ref());

          g_entity_mgr->broadcastEventImmediate(
            UpdateStageInfoRenderDistortion(camera.viewTm, camera.noJitterProjTm, camera.viewItm, handles->texCtxHndl.ref()));
        };

        if (!will_render_haze(riMainVisibility))
        {
          UpdateStageInfoNeedDistortion needDistortion(camera.viewTm, camera.noJitterProjTm, camera.viewItm);
          g_entity_mgr->broadcastEventImmediate(needDistortion);
          if (!needDistortion.needed)
            return;
        }

        hazeRenderedHndl.ref() = true;

        G_ASSERT(heatHazeLod >= 0);
        const auto lodDepth = heatHazeLod == 0 ? depthForTransparencyHndl.view() : farDownsampledDepth.view();

        const camera_in_camera::ApplyMasterState camcam{multiplexing_index};
        Texture *stencil = hasStencil ? depthForTransparencyHndl.view().getTex2D() : nullptr;
        acesfx::set_dafx_globaldata(camera.jitterGlobtm, camera.viewItm, camera.viewItm.getcol(3));

        heatHazeRenderer->renderHazeParticles(hazeDepthHndl.get(), hazeOffsetHndl.get(), lodDepth.getTexId(),
          eastl::max(heatHazeLod - 1, 0), acesfx::renderTransHaze, riHazeRender, stencil);

        if (hazeColorHndl.get())
          heatHazeRenderer->renderColorHaze(hazeColorHndl.get(), acesfx::renderTransHaze, riHazeRender, stencil);
      };
    });
}

static dafg::NodeHandle makeHeatHazeApplyNode(HeatHazeRenderer *heatHazeRenderer)
{
  return dafg::register_node("heat_haze_apply_node", DAFG_PP_NODE_SRC, [heatHazeRenderer](dafg::Registry registry) {
    registry.orderMeAfter("heat_haze_render_particles_node");
    // Heat haze is actually applied inside post fx node. We need to just set shader variables here before post_fx
    registry.orderMeBefore("post_fx_node");

    auto depthForTransparencyHndl = registry.readTexture("depth_for_transparency")
                                      .atStage(dafg::Stage::ALL_GRAPHICS)
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
  dafg::NodeHandle &heatHazeRenderClearNode,
  dafg::NodeHandle &heatHazeRenderParticlesNode,
  dafg::NodeHandle &heatHazeApplyNode,
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
  dafg::NodeHandle &heat_haze__render_clear_node,
  dafg::NodeHandle &heat_haze__render_particles_node,
  dafg::NodeHandle &heat_haze__apply_node,
  int &heat_haze__lod)
{
  setup_heat_haze(heat_haze__manager, heat_haze__render_clear_node, heat_haze__render_particles_node, heat_haze__apply_node,
    heat_haze__lod);
}

ECS_TAG(render)
ECS_ON_EVENT(ChangeRenderFeatures)
static void heat_haze_render_features_changed_es_event_handler(const ecs::Event &evt,
  HeatHazeManager &heat_haze__manager,
  dafg::NodeHandle &heat_haze__render_clear_node,
  dafg::NodeHandle &heat_haze__render_particles_node,
  dafg::NodeHandle &heat_haze__apply_node,
  int &heat_haze__lod)
{
  if (auto *changedFeatures = evt.cast<ChangeRenderFeatures>())
    if (!(changedFeatures->isFeatureChanged(FeatureRenderFlags::HAZE) ||
          changedFeatures->isFeatureChanged(FeatureRenderFlags::CAMERA_IN_CAMERA)))
      return;
  setup_heat_haze(heat_haze__manager, heat_haze__render_clear_node, heat_haze__render_particles_node, heat_haze__apply_node,
    heat_haze__lod);
}