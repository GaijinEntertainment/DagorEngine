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

namespace var
{
static ShaderVariableInfo water_rt_enabled("water_rt_enabled", true);
}

const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT_WITH_RENAMES)> WATER_SSR_DEPTH_TEX = {"downsampled_depth",
  "downsampled_depth_with_early_before_envi_water", "downsampled_depth_with_early_after_envi_water",
  "downsampled_depth_with_late_water"};

const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT_WITH_RENAMES)> WATER_SSR_COLOR_TEX = {"water_ssr_color",
  "water_ssr_color_early_before_envi_water", "water_ssr_color_early_after_envi_water", "water_ssr_color_late_water"};

const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT)> WATER_SSR_COLOR_TOKEN = {
  "water_ssr_color_token_early_before_envi_water", "water_ssr_color_token_early_after_envi_water", "water_ssr_color_token_late_water"};

const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT_WITH_RENAMES)> WATER_SSR_STRENGTH_TEX = {
  "water_ssr_strength", "water_ssr_strength_early_before_envi_water", "water_ssr_strength_early_after_envi_water",
  "water_ssr_strength_with_late_water"};

// TODO Maybe use history for color history rejection purposes?
const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT)> WATER_NORMAL_DIR_TEX = {
  "water_normal_dir_early_before_envi_water", "water_normal_dir_early_after_envi_water", "water_normal_dir_late_water"};

dafg::NodeHandle makePrepareWaterNode()
{
  return dafg::register_node("prepare_water_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    int water_depth_aboveVarId = get_shader_variable_id("water_depth_above");
    int water_ssr_intensityVarId = get_shader_variable_id("water_ssr_intensity", true);
    int water_levelVarId = get_shader_variable_id("water_level");

    auto currentCameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto waterLevelHndl = registry.readBlob<float>("water_level").handle();
    auto enableWaterSsrHndl = registry.create("enable_water_ssr", dafg::History::ClearZeroOnFirstFrame).blob<bool>().handle();

    registry.create(WATER_SSR_COLOR_TEX[0], dafg::History::No)
      .texture({TEXFMT_R11G11B10F | TEXCF_RTARGET | TEXCF_UNORDERED, registry.getResolution<2>("main_view", 0.5f), 1})
      .atStage(dafg::Stage::PS)
      .useAs(dafg::Usage::COLOR_ATTACHMENT);
    uint32_t strengthFormat = is_rt_water_enabled() ? TEXFMT_A8R8G8B8 : TEXFMT_R8G8;
    registry.create(WATER_SSR_STRENGTH_TEX[0], dafg::History::No)
      .texture({strengthFormat | TEXCF_RTARGET | TEXCF_UNORDERED, registry.getResolution<2>("main_view", 0.5f), 1})
      .atStage(dafg::Stage::PS)
      .useAs(dafg::Usage::COLOR_ATTACHMENT);

    {
      d3d::SamplerInfo smpInfo;
      smpInfo.filter_mode = d3d::FilterMode::Point;
      smpInfo.anisotropic_max = 1;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Border;
      smpInfo.border_color = d3d::BorderColor::Color::TransparentBlack;
      registry.create("water_ssr_point_sampler", dafg::History::No).blob(d3d::request_sampler(smpInfo));
    }

    {
      d3d::SamplerInfo smpInfo;
      smpInfo.filter_mode = d3d::FilterMode::Linear;
      smpInfo.anisotropic_max = 1;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Border;
      smpInfo.border_color = d3d::BorderColor::Color::TransparentBlack;
      registry.create("water_ssr_linear_sampler", dafg::History::No).blob(d3d::request_sampler(smpInfo));
    }

    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      smpInfo.filter_mode = d3d::FilterMode::Point;
      registry.create("water_ssr_point_clamp_sampler", dafg::History::No).blob(d3d::request_sampler(smpInfo));
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
          }
#endif
          if (!wr.isWaterSSREnabled())
            waterSsrIntensity = 0;
          ShaderGlobal::set_real(water_ssr_intensityVarId, waterSsrIntensity);
          enableWaterSsrHndl.ref() = waterSsrIntensity > 0.0;
        }
        else
        {
          ShaderGlobal::set_real(water_depth_aboveVarId, -10000);
        }
      };
  });
}

const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT)> WATER_SSR_CAM_RES_PROVIDER_NODE_NAMES = {
  "water_ssr_cam_res_provider_early_before_envi_node", "water_ssr_cam_res_provider_early_after_envi_node",
  "water_ssr_cam_res_provider_late_node"};
const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT)> WATER_SSR_NODE_NAMES = {
  "water_ssr_early_before_envi_node", "water_ssr_early_after_envi_node", "water_ssr_late_node"};
const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT)> WATER_NODE_NAMES = {
  "water_early_before_envi_node", "water_early_after_envi_node", "water_late_node"};

const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT)> DOWNSAMPLED_FRAME_TEX_NAMES = {
  "prev_frame_tex_for_water_early", "prev_frame_tex", "prev_frame_tex"};

eastl::fixed_vector<dafg::NodeHandle, 2, false> makeWaterSSRNode(WaterRenderMode mode)
{
  const char *camResNodeName = WATER_SSR_CAM_RES_PROVIDER_NODE_NAMES[eastl::to_underlying(mode)];
  eastl::fixed_vector<dafg::NodeHandle, 2, false> nodes;
  nodes.push_back(dafg::register_node(camResNodeName, DAFG_PP_NODE_SRC, [mode](dafg::Registry registry) {
    const auto history = mode == WaterRenderMode::LATE ? dafg::History::ClearZeroOnFirstFrame : dafg::History::No;
    const uint32_t modeIdx = eastl::to_underlying(mode);

    registry.renameTexture(WATER_SSR_DEPTH_TEX[modeIdx], WATER_SSR_DEPTH_TEX[modeIdx + 1], dafg::History::No);
    registry.renameTexture(WATER_SSR_COLOR_TEX[modeIdx], WATER_SSR_COLOR_TEX[modeIdx + 1], history);
    registry.renameTexture(WATER_SSR_STRENGTH_TEX[modeIdx], WATER_SSR_STRENGTH_TEX[modeIdx + 1], history);

    registry.createBlob<OrderingToken>(WATER_SSR_COLOR_TOKEN[modeIdx], dafg::History::No);

    if (is_rt_water_enabled())
    {
      registry.createTexture2d(WATER_NORMAL_DIR_TEX[modeIdx], dafg::History::No,
        {TEXFMT_A2B10G10R10 | TEXCF_RTARGET | TEXCF_GENERATEMIPS | TEXCF_UNORDERED, registry.getResolution<2>("main_view", 0.5f), 1});
    }
  }));

  const char *ssrNodeName = WATER_SSR_NODE_NAMES[eastl::to_underlying(mode)];
  nodes.push_back(dafg::register_node(ssrNodeName, DAFG_PP_NODE_SRC, [mode](dafg::Registry registry) {
    const uint32_t modeIdx = eastl::to_underlying(mode);

    registry.requestState().setFrameBlock("global_frame");

    registry.read("wfx_hmap").texture().atStage(dafg::Stage::VS | dafg::Stage::PS).bindToShaderVar().optional();
    registry.read("wfx_hmap_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("wfx_hmap_samplerstate").optional();
    registry.read("wfx_normals").texture().atStage(dafg::Stage::PS).bindToShaderVar().optional();
    registry.read("wfx_normals_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("wfx_normals_samplerstate").optional();

    registry.modifyBlob<OrderingToken>(WATER_SSR_COLOR_TOKEN[modeIdx]);

    auto downsampledDepthHndl =
      registry.modifyTexture(WATER_SSR_DEPTH_TEX[modeIdx + 1]).atStage(dafg::Stage::PS).useAs(dafg::Usage::DEPTH_ATTACHMENT).handle();
    auto firstRtHndl =
      registry.modifyTexture(WATER_SSR_COLOR_TEX[modeIdx + 1]).atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT).handle();
    auto waterSSRStrengthHndl = registry.modifyTexture(WATER_SSR_STRENGTH_TEX[modeIdx + 1])
                                  .atStage(dafg::Stage::PS)
                                  .useAs(dafg::Usage::COLOR_ATTACHMENT)
                                  .handle();

    if (is_rt_water_enabled())
    {
      firstRtHndl =
        registry.modifyTexture(WATER_NORMAL_DIR_TEX[modeIdx]).atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT).handle();
    }
    else
    {
      registry.read("water_ssr_point_clamp_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("water_ssr_point_clamp_samplerstate");
      registry.read("close_depth").texture().atStage(dafg::Stage::PS).bindToShaderVar("downsampled_close_depth_tex");
      registry.read("far_downsampled_depth").texture().atStage(dafg::Stage::PS).bindToShaderVar("downsampled_far_depth_tex");
      registry.historyFor(WATER_SSR_COLOR_TEX[eastl::to_underlying(WaterRenderMode::COUNT)])
        .texture()
        .atStage(dafg::Stage::PS)
        .bindToShaderVar("water_reflection_tex");
      registry.historyFor(WATER_SSR_STRENGTH_TEX[eastl::to_underlying(WaterRenderMode::COUNT)])
        .texture()
        .atStage(dafg::Stage::PS)
        .bindToShaderVar("water_reflection_strength_tex");
      registry.read("water_ssr_linear_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("water_reflection_tex_samplerstate");
      registry.readTexture("water_planar_reflection_terrain").atStage(dafg::Stage::PS).bindToShaderVar().optional();
      registry.readTexture("water_planar_reflection_terrain_depth").atStage(dafg::Stage::PS).bindToShaderVar().optional();
      if (renderer_has_feature(FeatureRenderFlags::PREV_OPAQUE_TEX))
      {
        registry.read("prev_frame_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("prev_frame_tex_samplerstate");
        registry.read(DOWNSAMPLED_FRAME_TEX_NAMES[modeIdx]).texture().atStage(dafg::Stage::PS).bindToShaderVar("prev_frame_tex");
      }
      // SSR uses probes, if they are present
      (registry.root() / "indoor_probes").read("probes_ready_token").blob<OrderingToken>().optional();
    }

    auto waterModeHndl = registry.readBlob<WaterRenderMode>("water_render_mode").handle();

    registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
    auto cameraHndl = use_camera_in_camera(registry);
    auto prevCameraHndl = read_history_camera_in_camera(registry).handle();

    registry.readBlobHistory<bool>("enable_water_ssr").bindToShaderVar("water_ssr_enabled_prev_frame");
    auto enableWaterSsrHndl = registry.readBlob<bool>("enable_water_ssr").handle();
    auto isWaterRtEnabledHndl = registry.readBlob<int>("water_rt_enabled").optional().handle();

    const int wfxEffectsTexEnabledVarId = get_shader_glob_var_id("wfx_effects_tex_enabled");
    auto resources = eastl::make_tuple(enableWaterSsrHndl, isWaterRtEnabledHndl, waterModeHndl, cameraHndl, mode, downsampledDepthHndl,
      firstRtHndl, waterSSRStrengthHndl, wfxEffectsTexEnabledVarId, prevCameraHndl);

    return [resources = eastl::make_unique<decltype(resources)>(resources)](const dafg::multiplexing::Index &multiplexing_index) {
      auto [enableWaterSsrHndl, isWaterRtEnabledHndl, waterModeHndl, cameraHndl, mode, downsampledDepthHndl, firstRtHndl,
        waterSSRStrengthHndl, wfxEffectsTexEnabledVarId, prevCameraHndl] = *resources;

      if (mode != waterModeHndl.ref())
        return;
      if (!enableWaterSsrHndl.ref())
        return;

      camera_in_camera::ApplyPostfxState camcam{
        multiplexing_index, cameraHndl.ref(), prevCameraHndl.ref(), camera_in_camera::USE_STENCIL};

      const bool isMainView = multiplexing_index.subCamera == 0;
      if (isMainView)
        set_viewvecs_to_shader(cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm);

      const bool isWaterRtEnabled = isWaterRtEnabledHndl.get() ? *isWaterRtEnabledHndl.get() : 0;
      ShaderGlobal::set_int(var::water_rt_enabled, isWaterRtEnabled);

      d3d::set_render_target({downsampledDepthHndl.get(), 0}, DepthAccess::RW,
        {{firstRtHndl.get(), 0}, {waterSSRStrengthHndl.get(), 0}});

      d3d::clear_rt({firstRtHndl.get(), 0}, make_clear_value(0.0f, 0.0f, 0.0f, 0.0f));
      d3d::clear_rt({waterSSRStrengthHndl.get(), 0}, make_clear_value(1.0f, 0.0f, 0.0f, 0.0f));

      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      const auto &camera = cameraHndl.ref();

      ShaderGlobal::set_int(wfxEffectsTexEnabledVarId, int(use_wfx_textures()));

      wr.renderWaterSSR(camera.viewItm, camera.jitterPersp);
    };
  }));

  return nodes;
}

dafg::NodeHandle makeWaterNode(WaterRenderMode mode)
{
  return dafg::register_node(WATER_NODE_NAMES[eastl::to_underlying(mode)], DAFG_PP_NODE_SRC, [mode](dafg::Registry registry) {
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
      registry.modify(COLOR_NAMES[modeIdx]).texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT).handle();
    auto depthHndl =
      registry.modify(DEPTH_NAMES[modeIdx]).texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::DEPTH_ATTACHMENT).handle();

    use_volfog(registry, dafg::Stage::PS);

    registry.read("far_downsampled_depth").texture().atStage(dafg::Stage::PS).bindToShaderVar("downsampled_far_depth_tex");

    if (renderer_has_feature(FeatureRenderFlags::PREV_OPAQUE_TEX))
      registry.read(DOWNSAMPLED_FRAME_TEX_NAMES[modeIdx]).texture().atStage(dafg::Stage::PS).bindToShaderVar("water_refraction_tex");

    registry.read("wfx_hmap").texture().atStage(dafg::Stage::VS | dafg::Stage::PS).bindToShaderVar().optional();
    registry.read("wfx_hmap_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("wfx_hmap_samplerstate").optional();
    registry.read("wfx_normals").texture().atStage(dafg::Stage::PS).bindToShaderVar().optional();
    registry.read("wfx_normals_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("wfx_normals_samplerstate").optional();

    registry.read(WATER_SSR_COLOR_TEX[modeIdx + 1]).texture().atStage(dafg::Stage::PS).bindToShaderVar("water_reflection_tex");
    registry.read(WATER_SSR_STRENGTH_TEX[modeIdx + 1])
      .texture()
      .atStage(dafg::Stage::PS)
      .bindToShaderVar("water_reflection_strength_tex");
    registry.read("water_ssr_linear_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("water_reflection_tex_samplerstate");


    registry.readTexture("water_planar_reflection_clouds").atStage(dafg::Stage::PS).bindToShaderVar().optional();
    registry.read("water_planar_reflection_clouds_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("water_planar_reflection_clouds_samplerstate")
      .optional();

    auto waterModeHndl = registry.readBlob<WaterRenderMode>("water_render_mode").handle();

    auto cameraHndl = use_camera_in_camera(registry);

    auto enableWaterSsrHndl = registry.readBlob<bool>("enable_water_ssr").handle();
    auto isWaterRtEnabledHndl = registry.readBlob<int>("water_rt_enabled").optional().handle();

    auto reactiveMaskHndl =
      registry.modifyTexture("reactive_mask").atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT).optional().handle();

    return [isWaterRtEnabledHndl, mode, waterModeHndl, cameraHndl, enableWaterSsrHndl, finalTargetHndl, depthHndl, reactiveMaskHndl,
             wfxEffectsTexEnabledVarId = get_shader_glob_var_id("wfx_effects_tex_enabled")](
             const dafg::multiplexing::Index &multiplexing_index) {
      if (mode != waterModeHndl.ref())
        return;

      const bool isWaterRtEnabled = isWaterRtEnabledHndl.get() ? *isWaterRtEnabledHndl.get() : 0;
      ShaderGlobal::set_int(var::water_rt_enabled, isWaterRtEnabled);

      const camera_in_camera::ApplyMasterState camcam{multiplexing_index};

      d3d::set_render_target({depthHndl.get(), 0}, DepthAccess::RW, {{finalTargetHndl.get(), 0}, {reactiveMaskHndl.get(), 0}});

      set_viewvecs_to_shader(cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm);
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      ShaderGlobal::set_int(wfxEffectsTexEnabledVarId, int(use_wfx_textures()));
      wr.renderWater(cameraHndl.ref().viewItm, WorldRenderer::DistantWater{mode != WaterRenderMode::LATE && distantWater},
        enableWaterSsrHndl.ref());
    };
  });
}
