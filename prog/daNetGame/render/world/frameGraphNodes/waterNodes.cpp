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
#include <render/world/bvh.h>

CONSOLE_BOOL_VAL("water", distantWater, true);

const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT_WITH_RENAMES)> WATER_SSR_DEPTH_TEX = {"downsampled_depth",
  "downsampled_depth_with_early_before_envi_water", "downsampled_depth_with_early_after_envi_water",
  "downsampled_depth_with_late_water"};

const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT_WITH_RENAMES)> WATER_SSR_COLOR_TEX = {"water_ssr_color",
  "water_ssr_color_early_before_envi_water", "water_ssr_color_early_after_envi_water", "water_ssr_color_late_water"};

const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT_WITH_RENAMES)> WATER_SSR_STRENGTH_TEX = {
  "water_ssr_strength", "water_ssr_strength_early_before_envi_water", "water_ssr_strength_early_after_envi_water",
  "water_ssr_strength_with_late_water"};

// TODO Maybe use history for color history rejection purposes?
const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT)> WATER_REFLECT_DIR_TEX = {
  "water_reflect_dir_early_before_envi_water", "water_reflect_dir_early_after_envi_water", "water_reflect_dir_late_water"};

dabfg::NodeHandle makePrepareWaterNode()
{
  return dabfg::register_node("prepare_water_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    int water_depth_aboveVarId = get_shader_variable_id("water_depth_above");
    int water_ssr_intensityVarId = get_shader_variable_id("water_ssr_intensity");
    int water_levelVarId = get_shader_variable_id("water_level");

    auto currentCameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto waterLevelHndl = registry.readBlob<float>("water_level").handle();
    auto enableWaterSsrHndl = registry.create("enable_water_ssr", dabfg::History::ClearZeroOnFirstFrame).blob<bool>().handle();

    registry.create(WATER_SSR_COLOR_TEX[0], dabfg::History::No)
      .texture({TEXFMT_R11G11B10F | TEXCF_RTARGET | TEXCF_UNORDERED, registry.getResolution<2>("main_view", 0.5f), 1})
      .atStage(dabfg::Stage::PS)
      .useAs(dabfg::Usage::COLOR_ATTACHMENT);
    uint32_t strengthFormat = is_rt_water_enabled() ? TEXFMT_A8R8G8B8 : TEXFMT_R8G8;
    registry.create(WATER_SSR_STRENGTH_TEX[0], dabfg::History::No)
      .texture({strengthFormat | TEXCF_RTARGET | TEXCF_UNORDERED, registry.getResolution<2>("main_view", 0.5f), 1})
      .atStage(dabfg::Stage::PS)
      .useAs(dabfg::Usage::COLOR_ATTACHMENT);

    {
      d3d::SamplerInfo smpInfo;
      smpInfo.filter_mode = d3d::FilterMode::Point;
      smpInfo.anisotropic_max = 1;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Border;
      smpInfo.border_color = d3d::BorderColor::Color::TransparentBlack;
      registry.create("water_ssr_point_sampler", dabfg::History::No).blob(d3d::request_sampler(smpInfo));
    }

    {
      d3d::SamplerInfo smpInfo;
      smpInfo.filter_mode = d3d::FilterMode::Linear;
      smpInfo.anisotropic_max = 1;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Border;
      smpInfo.border_color = d3d::BorderColor::Color::TransparentBlack;
      registry.create("water_ssr_linear_sampler", dabfg::History::No).blob(d3d::request_sampler(smpInfo));
    }

    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      smpInfo.filter_mode = d3d::FilterMode::Point;
      registry.create("water_ssr_point_clamp_sampler", dabfg::History::No).blob(d3d::request_sampler(smpInfo));
    }

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
    auto history = mode == WaterRenderMode::LATE ? dabfg::History::ClearZeroOnFirstFrame : dabfg::History::No;
    const uint32_t modeIdx = eastl::to_underlying(mode);

    registry.requestState().setFrameBlock("global_frame");

    registry.read("wfx_hmap").texture().atStage(dabfg::Stage::VS | dabfg::Stage::PS).bindToShaderVar().optional();
    registry.read("wfx_hmap_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("wfx_hmap_samplerstate").optional();
    registry.read("wfx_normals").texture().atStage(dabfg::Stage::PS).bindToShaderVar().optional();
    registry.read("wfx_normals_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("wfx_normals_samplerstate").optional();

    auto downsampledDepthHndl = registry.rename(WATER_SSR_DEPTH_TEX[modeIdx], WATER_SSR_DEPTH_TEX[modeIdx + 1], dabfg::History::No)
                                  .texture()
                                  .atStage(dabfg::Stage::PS)
                                  .useAs(dabfg::Usage::DEPTH_ATTACHMENT)
                                  .handle();
    auto firstRtHndl = registry.renameTexture(WATER_SSR_COLOR_TEX[modeIdx], WATER_SSR_COLOR_TEX[modeIdx + 1], history)
                         .atStage(dabfg::Stage::PS)
                         .useAs(dabfg::Usage::COLOR_ATTACHMENT)
                         .handle();
    auto waterSSRStrengthHndl = registry.renameTexture(WATER_SSR_STRENGTH_TEX[modeIdx], WATER_SSR_STRENGTH_TEX[modeIdx + 1], history)
                                  .atStage(dabfg::Stage::PS)
                                  .useAs(dabfg::Usage::COLOR_ATTACHMENT)
                                  .handle();

    if (is_rt_water_enabled())
    {
      firstRtHndl = registry.create(WATER_REFLECT_DIR_TEX[modeIdx], dabfg::History::No)
                      .texture({TEXFMT_A2B10G10R10 | TEXCF_RTARGET | TEXCF_GENERATEMIPS | TEXCF_UNORDERED,
                        registry.getResolution<2>("main_view", 0.5f), 1})
                      .atStage(dabfg::Stage::PS)
                      .useAs(dabfg::Usage::COLOR_ATTACHMENT)
                      .handle();
    }
    else
    {
      registry.read("water_ssr_point_clamp_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("water_ssr_point_clamp_samplerstate");
      registry.read("close_depth").texture().atStage(dabfg::Stage::PS).bindToShaderVar("downsampled_close_depth_tex");
      registry.read("far_downsampled_depth").texture().atStage(dabfg::Stage::PS).bindToShaderVar("downsampled_far_depth_tex");
      registry.historyFor(WATER_SSR_COLOR_TEX[eastl::to_underlying(WaterRenderMode::COUNT)])
        .texture()
        .atStage(dabfg::Stage::PS)
        .bindToShaderVar("water_reflection_tex");
      registry.historyFor(WATER_SSR_STRENGTH_TEX[eastl::to_underlying(WaterRenderMode::COUNT)])
        .texture()
        .atStage(dabfg::Stage::PS)
        .bindToShaderVar("water_reflection_strength_tex");
      registry.read("water_ssr_point_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("water_reflection_tex_samplerstate");
      registry.readTexture("water_planar_reflection_terrain").atStage(dabfg::Stage::PS).bindToShaderVar().optional();
      registry.readTexture("water_planar_reflection_terrain_depth").atStage(dabfg::Stage::PS).bindToShaderVar().optional();
      if (renderer_has_feature(FeatureRenderFlags::PREV_OPAQUE_TEX))
      {
        registry.read("prev_frame_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("prev_frame_tex_samplerstate");
        registry.read(DOWNSAMPLED_FRAME_TEX_NAMES[modeIdx]).texture().atStage(dabfg::Stage::PS).bindToShaderVar("prev_frame_tex");
      }
      // SSR uses probes, if they are present
      (registry.root() / "indoor_probes").read("probes_ready_token").blob<OrderingToken>().optional();
    }

    auto waterModeHndl = registry.readBlob<WaterRenderMode>("water_render_mode").handle();
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera")
                        .bindAsView<&CameraParams::viewTm>()
                        .bindAsProj<&CameraParams::jitterProjTm>()
                        .handle();

    registry.readBlobHistory<bool>("enable_water_ssr").bindToShaderVar("water_ssr_enabled_prev_frame");
    auto enableWaterSsrHndl = registry.readBlob<bool>("enable_water_ssr").handle();
    registry.readBlob<int>("water_rt_enabled").bindToShaderVar("water_rt_enabled").optional();

    return [enableWaterSsrHndl, waterModeHndl, cameraHndl, mode, downsampledDepthHndl, firstRtHndl, waterSSRStrengthHndl,
             wfxEffectsTexEnabledVarId = get_shader_glob_var_id("wfx_effects_tex_enabled")] {
      if (mode != waterModeHndl.ref())
        return;
      if (!enableWaterSsrHndl.ref())
        return;

      d3d::set_render_target({downsampledDepthHndl.get(), 0}, DepthAccess::RW,
        {{firstRtHndl.get(), 0}, {waterSSRStrengthHndl.get(), 0}});

      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      const auto &camera = cameraHndl.ref();

      ShaderGlobal::set_int(wfxEffectsTexEnabledVarId, int(use_wfx_textures()));

      wr.renderWaterSSR(camera.viewItm, camera.jitterPersp);
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

    registry.read(WATER_SSR_COLOR_TEX[modeIdx + 1]).texture().atStage(dabfg::Stage::PS).bindToShaderVar("water_reflection_tex");
    registry.read(WATER_SSR_STRENGTH_TEX[modeIdx + 1])
      .texture()
      .atStage(dabfg::Stage::PS)
      .bindToShaderVar("water_reflection_strength_tex");
    registry.read("water_ssr_linear_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("water_reflection_tex_samplerstate");


    registry.readTexture("water_planar_reflection_clouds").atStage(dabfg::Stage::PS).bindToShaderVar().optional();
    registry.read("water_planar_reflection_clouds_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("water_planar_reflection_clouds_samplerstate")
      .optional();

    auto waterModeHndl = registry.readBlob<WaterRenderMode>("water_render_mode").handle();
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera")
                        .bindAsView<&CameraParams::viewTm>()
                        .bindAsProj<&CameraParams::jitterProjTm>()
                        .handle();

    auto enableWaterSsrHndl = registry.readBlob<bool>("enable_water_ssr").handle();
    registry.readBlob<int>("water_rt_enabled").bindToShaderVar("water_rt_enabled").optional();

    return [mode, waterModeHndl, cameraHndl, enableWaterSsrHndl, finalTargetHndl, depthHndl,
             wfxEffectsTexEnabledVarId = get_shader_glob_var_id("wfx_effects_tex_enabled")] {
      if (mode != waterModeHndl.ref())
        return;

      d3d::set_render_target({depthHndl.get(), 0}, DepthAccess::RW, {{finalTargetHndl.get(), 0}});

      set_viewvecs_to_shader(cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm);
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      ShaderGlobal::set_int(wfxEffectsTexEnabledVarId, int(use_wfx_textures()));
      wr.renderWater(cameraHndl.ref().viewItm, WorldRenderer::DistantWater{mode != WaterRenderMode::LATE && distantWater},
        enableWaterSsrHndl.ref());
    };
  });
}
