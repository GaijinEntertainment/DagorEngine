// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/skies.h>
#include <render/waterRender.h>
#include <render/world/worldRendererQueries.h>
#include <render/deferredRenderer.h>
#include <render/world/frameGraphHelpers.h>
#include <util/dag_convar.h>
#include <fftWater/fftWater.h>

#define INSIDE_RENDERER 1
#include "../private_worldRenderer.h"
#include "frameGraphNodes.h"
#include <shaders/dag_shaderBlock.h>
#include <render/viewVecs.h>
#include <drv/3d/dag_matricesAndPerspective.h>

CONSOLE_BOOL_VAL("water", distantWater, true);

dabfg::NodeHandle makePrepareWaterNode()
{
  return dabfg::register_node("prepare_water_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    int water_depth_aboveVarId = get_shader_variable_id("water_depth_above");
    int water_ssr_intensityVarId = get_shader_variable_id("water_ssr_intensity");
    int water_levelVarId = get_shader_variable_id("water_level");

    registry.orderMeAfter("resolve_gbuffer_node");
    registry.readTexture("far_downsampled_depth").atStage(dabfg::Stage::POST_RASTER).bindToShaderVar("downsampled_far_depth_tex");
    registry.readTextureHistory("far_downsampled_depth").atStage(dabfg::Stage::POST_RASTER).useAs(dabfg::Usage::SHADER_RESOURCE);
    auto currentCameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto waterLevelHndl = registry.readBlob<float>("water_level").handle();
    registry.requestState().setFrameBlock("global_frame");

    auto enableWaterSsrHndl = registry.create("enable_water_ssr", dabfg::History::No).blob<bool>().handle();

    return
      [currentCameraHndl, waterLevelHndl, enableWaterSsrHndl, water_depth_aboveVarId, water_ssr_intensityVarId, water_levelVarId]() {
        auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

        ShaderGlobal::set_real(water_levelVarId, waterLevelHndl.ref());

        if (wr.water)
        {
          float height;
          fft_water::getHeightAboveWater(wr.water, currentCameraHndl.ref().viewItm.getcol(3), height, true);
          ShaderGlobal::set_real(water_depth_aboveVarId, -height);

          float waterSsrIntensity = 1.0;

#if _TARGET_C1 || _TARGET_XBOXONE
          {
            constexpr float waterSSRMaxHeight = 80.0f;
            constexpr float waterSSRFadeoutRange = 30.0f;
            waterSsrIntensity = 1.0 - eastl::max(0.0f, (height - waterSSRMaxHeight) / waterSSRFadeoutRange);
            ShaderGlobal::set_real(water_ssr_intensityVarId, waterSsrIntensity);
          }
#else
          G_UNUSED(water_ssr_intensityVarId);
#endif

          enableWaterSsrHndl.ref() = waterSsrIntensity > 0.0 && wr.isWaterSSREnabled();
        }
        else
        {
          ShaderGlobal::set_real(water_depth_aboveVarId, -10000);
        }
      };
  });
}

const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT)> WATER_SSR_NODE_NAMES = {
  "water_ssr_early_before_envi_node", "water_ssr_early_after_envi_node", "water_ssr_late_node"};
const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT)> WATER_NODE_NAMES = {
  "water_early_before_envi_node", "water_early_after_envi_node", "water_late_node"};

const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT)> DOWNSAMPLED_FRAME_TEX_NAMES = {
  "prev_frame_tex_for_water_early", "prev_frame_tex", "prev_frame_tex"};

dabfg::NodeHandle makeWaterSSRNode(WaterRenderMode mode)
{
  return dabfg::register_node(WATER_SSR_NODE_NAMES[eastl::to_underlying(mode)], DABFG_PP_NODE_SRC, [mode](dabfg::Registry registry) {
    const uint32_t modeIdx = eastl::to_underlying(mode);

    registry.orderMeBefore(WATER_NODE_NAMES[modeIdx]);

    // SSR uses probes, if they are present
    (registry.root() / "indoor_probes").read("probes_ready_token").blob<OrderingToken>().optional();

    registry.requestState().setFrameBlock("global_frame");

    registry.read("gbuf_2").texture().atStage(dabfg::Stage::PS).bindToShaderVar("material_gbuf").optional();
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("material_gbuf_samplerstate").optional();

    auto downsampledDepthHndl =
      (mode == WaterRenderMode::LATE ? registry.rename("downsampled_depth", "downsampled_depth_with_late_water", dabfg::History::No)
                                     : registry.modify("downsampled_depth"))
        .texture()
        .atStage(dabfg::Stage::PS)
        .useAs(dabfg::Usage::DEPTH_ATTACHMENT)
        .handle();

    registry.read("close_depth").texture().atStage(dabfg::Stage::PS).bindToShaderVar("downsampled_close_depth_tex");
    auto farDownsampledDepthHndl =
      registry.read("far_downsampled_depth").texture().atStage(dabfg::Stage::PS).bindToShaderVar("downsampled_far_depth_tex").handle();

    registry.read("wfx_hmap").texture().atStage(dabfg::Stage::VS | dabfg::Stage::PS).bindToShaderVar().optional();
    registry.read("wfx_hmap_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("wfx_hmap_samplerstate").optional();
    registry.read("wfx_normals").texture().atStage(dabfg::Stage::PS).bindToShaderVar().optional();
    registry.read("wfx_normals_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("wfx_normals_samplerstate").optional();

    registry.read("ssr_target").texture().atStage(dabfg::Stage::PS).useAs(dabfg::Usage::SHADER_RESOURCE).optional();

    auto waterSSRColorHndl =
      (mode == WaterRenderMode::EARLY_BEFORE_ENVI ? registry.create("water_ssr_color", dabfg::History::ClearZeroOnFirstFrame)
                                                      .texture({TEXFMT_R11G11B10F | TEXCF_RTARGET | TEXCF_GENERATEMIPS,
                                                        registry.getResolution<2>("main_view", 0.5f), dabfg::AUTO_MIP_COUNT})
                                                  : registry.modify("water_ssr_color").texture())
        .atStage(dabfg::Stage::PS)
        .useAs(dabfg::Usage::COLOR_ATTACHMENT)
        .handle();
    auto waterSSRStrenghtHndl =
      (mode == WaterRenderMode::EARLY_BEFORE_ENVI ? registry.create("water_ssr_strenght", dabfg::History::ClearZeroOnFirstFrame)
                                                      .texture({TEXFMT_R8G8 | TEXCF_RTARGET | TEXCF_GENERATEMIPS,
                                                        registry.getResolution<2>("main_view", 0.5f), dabfg::AUTO_MIP_COUNT})
                                                  : registry.modify("water_ssr_strenght").texture())
        .atStage(dabfg::Stage::PS)
        .useAs(dabfg::Usage::COLOR_ATTACHMENT)
        .handle();

    auto waterSSRColorHistHndl =
      registry.historyFor("water_ssr_color").texture().atStage(dabfg::Stage::PS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();

    auto waterSSRStrenghtHistHndl =
      registry.historyFor("water_ssr_strenght").texture().atStage(dabfg::Stage::PS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();

    auto waterModeHndl = registry.readBlob<WaterRenderMode>("water_render_mode").handle();
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();

    if (renderer_has_feature(FeatureRenderFlags::PREV_OPAQUE_TEX))
    {
      registry.read("prev_frame_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("prev_frame_tex_samplerstate");
      registry.read(DOWNSAMPLED_FRAME_TEX_NAMES[modeIdx]).texture().atStage(dabfg::Stage::PS).bindToShaderVar("prev_frame_tex");
    }

    registry.readTexture("water_planar_reflection_terrain").atStage(dabfg::Stage::PS).bindToShaderVar().optional();
    registry.read("water_planar_reflection_terrain_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("water_planar_reflection_terrain_samplerstate")
      .optional();
    registry.readTexture("water_planar_reflection_terrain_depth").atStage(dabfg::Stage::PS).bindToShaderVar().optional();

    auto enableWaterSsrHndl = registry.readBlob<bool>("enable_water_ssr").handle();

    struct WaterSSRTextures
    {
      dabfg::VirtualResourceHandle<BaseTexture, true, false> downsampledDepthHndl;
      dabfg::VirtualResourceHandle<BaseTexture, true, false> waterSSRColorHndl;
      dabfg::VirtualResourceHandle<BaseTexture, true, false> waterSSRStrenghtHndl;
      dabfg::VirtualResourceHandle<const BaseTexture, true, false> waterSSRColorHistHndl;
      dabfg::VirtualResourceHandle<const BaseTexture, true, false> waterSSRStrenghtHistHndl;
      dabfg::VirtualResourceHandle<const BaseTexture, true, false> farDownsampledDepthHndl;
    };

    return [enableWaterSsrHndl,
             textureHndls = eastl::unique_ptr<WaterSSRTextures>(new WaterSSRTextures{downsampledDepthHndl, waterSSRColorHndl,
               waterSSRStrenghtHndl, waterSSRColorHistHndl, waterSSRStrenghtHistHndl, farDownsampledDepthHndl}),
             waterModeHndl, cameraHndl, mode, wfxEffectsTexEnabledVarId = get_shader_glob_var_id("wfx_effects_tex_enabled")] {
      if (mode != waterModeHndl.ref())
        return;
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      const auto &camera = cameraHndl.ref();

      ShaderGlobal::set_int(wfxEffectsTexEnabledVarId, int(use_wfx_textures()));

      ManagedTexView waterSSRColor = textureHndls->waterSSRColorHndl.view();
      ManagedTexView waterSSRColorPrev = textureHndls->waterSSRColorHistHndl.view();
      waterSSRColor->disableSampler();

      ManagedTexView waterSSRStrenght = textureHndls->waterSSRStrenghtHndl.view();
      ManagedTexView waterSSRStrenghtPrev = textureHndls->waterSSRStrenghtHistHndl.view();

      d3d::settm(TM_VIEW, camera.viewTm);
      wr.renderWaterSSR(enableWaterSsrHndl.ref(), textureHndls->downsampledDepthHndl.get(), camera.viewItm, camera.jitterPersp,
        textureHndls->farDownsampledDepthHndl.view().getTex2D(), &waterSSRColor, &waterSSRColorPrev, &waterSSRStrenght,
        &waterSSRStrenghtPrev);
    };
  });
}

dabfg::NodeHandle makeWaterNode(WaterRenderMode mode)
{
  return dabfg::register_node(WATER_NODE_NAMES[eastl::to_underlying(mode)], DABFG_PP_NODE_SRC, [mode](dabfg::Registry registry) {
    const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT)> COLOR_NAMES = {
      "opaque_with_water_before_clouds", "opaque_with_envi_and_water", "target_for_transparency"};
    const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT)> DEPTH_NAMES = {
      "opaque_depth_with_water_before_clouds", "opaque_depth_with_water", "depth_for_transparency"};
    const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT)> FOLLOW_NODE_NAMES = {
      nullptr, nullptr, "transparent_scene_late_node"};

    const uint32_t modeIdx = eastl::to_underlying(mode);

    registry.orderMeAfter(WATER_SSR_NODE_NAMES[modeIdx]);
    if (FOLLOW_NODE_NAMES[modeIdx])
      registry.orderMeBefore(FOLLOW_NODE_NAMES[modeIdx]);

    registry.requestState().allowWireframe().setFrameBlock("global_frame");

    auto finalTargetHndl =
      registry.modify(COLOR_NAMES[modeIdx]).texture().atStage(dabfg::Stage::PS).useAs(dabfg::Usage::COLOR_ATTACHMENT).handle();
    auto depthHndl =
      registry.modify(DEPTH_NAMES[modeIdx]).texture().atStage(dabfg::Stage::PS).useAs(dabfg::Usage::DEPTH_ATTACHMENT).handle();

    use_volfog(registry, dabfg::Stage::PS);

    registry.read("far_downsampled_depth").texture().atStage(dabfg::Stage::PS).bindToShaderVar("downsampled_far_depth_tex");

    if (renderer_has_feature(FeatureRenderFlags::PREV_OPAQUE_TEX))
      registry.read(DOWNSAMPLED_FRAME_TEX_NAMES[modeIdx]).texture().atStage(dabfg::Stage::PS).bindToShaderVar("water_refraction_tex");

    registry.read("wfx_hmap").texture().atStage(dabfg::Stage::VS | dabfg::Stage::PS).bindToShaderVar().optional();
    registry.read("wfx_hmap_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("wfx_hmap_samplerstate").optional();
    registry.read("wfx_normals").texture().atStage(dabfg::Stage::PS).bindToShaderVar().optional();
    registry.read("wfx_normals_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("wfx_normals_samplerstate").optional();

    // TODO: make read when we have only one water node
    registry.modify("water_ssr_color").texture().atStage(dabfg::Stage::UNKNOWN).useAs(dabfg::Usage::UNKNOWN);
    registry.modify("water_ssr_strenght").texture().atStage(dabfg::Stage::UNKNOWN).useAs(dabfg::Usage::UNKNOWN);

    registry.readTexture("water_planar_reflection_clouds").atStage(dabfg::Stage::PS).bindToShaderVar().optional();
    registry.read("water_planar_reflection_clouds_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("water_planar_reflection_clouds_samplerstate")
      .optional();

    auto waterModeHndl = registry.readBlob<WaterRenderMode>("water_render_mode").handle();
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();

    auto enableWaterSsrHndl = registry.readBlob<bool>("enable_water_ssr").handle();

    return [finalTargetHndl, depthHndl, mode, waterModeHndl, cameraHndl, enableWaterSsrHndl,
             wfxEffectsTexEnabledVarId = get_shader_glob_var_id("wfx_effects_tex_enabled")] {
      if (mode != waterModeHndl.ref())
        return;

      d3d::settm(TM_VIEW, cameraHndl.ref().viewTm);
      set_viewvecs_to_shader(cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm);
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      ShaderGlobal::set_int(wfxEffectsTexEnabledVarId, int(use_wfx_textures()));
      wr.renderWater(finalTargetHndl.get(), cameraHndl.ref().viewItm, depthHndl.get(),
        WorldRenderer::DistantWater{mode != WaterRenderMode::LATE && distantWater}, enableWaterSsrHndl.ref());
    };
  });
}
