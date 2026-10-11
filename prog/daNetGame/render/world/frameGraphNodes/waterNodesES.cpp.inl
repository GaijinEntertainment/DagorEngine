// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <render/skies.h>
#include <render/waterRender.h>
#include <render/world/worldRendererQueries.h>
#include <render/deferredRenderer.h>
#include <render/downsampleDepth.h>
#include <render/world/frameGraphHelpers.h>
#include <util/dag_convar.h>
#include <fftWater/fftWater.h>
#include <main/water.h>

#define INSIDE_RENDERER 1
#include "../private_worldRenderer.h"
#include "frameGraphNodes.h"
#include <shaders/dag_shaderBlock.h>
#include <render/viewVecs.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <render/renderEvent.h>
#include <render/world/bvh.h>
#include <ecs/render/renderEvent.h>
#include <ecs/render/resPtr.h>
#include <image/dag_texPixel.h>
#include <EASTL/tuple.h>

CONSOLE_BOOL_VAL("water", distantWater, true);

namespace var
{
static ShaderVariableInfo water_rt_enabled("water_rt_enabled", true);
static ShaderVariableInfo source_depth_for_copy_const_no("source_depth_for_copy_const_no");
} // namespace var

// Only 1 water node exists, ordering is kept through depth renaming
const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT_WITH_RENAMES)> WATER_SSR_DEPTH_TEX = {"downsampled_depth",
  "downsampled_depth_with_early_before_envi_water", "downsampled_depth_with_early_after_envi_water",
  "downsampled_depth_with_late_water"};

static const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT)> WATER_DEPTH_RENAME_NODE_NAMES = {
  "water_depth_rename_early_before_envi_node", "water_depth_rename_early_after_envi_node", "water_depth_rename_late_node"};

static const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT)> WATER_DEPTH_TEX = {
  "opaque_depth_with_water_before_clouds", "opaque_depth_with_water", "depth_for_transparency"};

static const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT)> WATER_DEPTH_COPY_SOURCE_TEX = {
  "gbuf_depth_after_resolve", "opaque_depth_with_water_before_clouds", "depth_before_water_late"};

static bool is_water_reflection_full_res() { return is_rr_enabled() && is_rt_water_enabled(); }

// RR produces noisy image if water refraction is enabled, so we disable it for RR
static bool is_water_refraction_enabled() { return !is_rr_enabled(); }

dafg::NodeHandle makePrepareWaterNode()
{
  return dafg::register_node("prepare_water_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    int water_ssr_intensityVarId = get_shader_variable_id("water_ssr_intensity", true);
    int water_levelVarId = get_shader_variable_id("water_level");
    int underwater_renderVarId = get_shader_variable_id("underwater_render", true);
    int use_underwater_reflectionsVarId = get_shader_variable_id("use_underwater_reflections", true);

    auto currentCameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto waterLevelHndl = registry.readBlob<float>("water_level").handle();
    auto enableWaterSsrHndl = registry.create("enable_water_ssr").blob<bool>().withHistory().handle();

    const bool fullRes = is_water_reflection_full_res();
    const auto history = fullRes ? dafg::History::No : dafg::History::ClearZeroOnFirstFrame;

    registry.create("water_ssr_color")
      .texture({TEXFMT_R11G11B10F | TEXCF_RTARGET | TEXCF_UNORDERED, registry.getResolution<2>("main_view", fullRes ? 1.0f : 0.5f), 1})
      .withHistory(history)
      .atStage(dafg::Stage::PS)
      .useAs(dafg::Usage::COLOR_ATTACHMENT);
    uint32_t strengthFormat = is_rt_water_enabled() ? TEXFMT_A8R8G8B8 : TEXFMT_R8G8;
    registry.create("water_ssr_strength")
      .texture({strengthFormat | TEXCF_RTARGET | TEXCF_UNORDERED, registry.getResolution<2>("main_view", fullRes ? 1.0f : 0.5f), 1})
      .withHistory(history)
      .atStage(dafg::Stage::PS)
      .useAs(dafg::Usage::COLOR_ATTACHMENT);

    registry.createTexture2d("water_normal_dir",
      {TEXFMT_A2B10G10R10 | TEXCF_RTARGET | TEXCF_UNORDERED, registry.getResolution<2>("main_view", fullRes ? 1.0f : 0.5f), 1});

    {
      d3d::SamplerInfo smpInfo;
      smpInfo.filter_mode = d3d::FilterMode::Point;
      smpInfo.anisotropic_max = 1;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Border;
      smpInfo.border_color = d3d::BorderColor::Color::TransparentBlack;
      registry.create("water_ssr_point_sampler").blob(d3d::request_sampler(smpInfo));
    }

    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      smpInfo.filter_mode = d3d::FilterMode::Point;
      registry.create("water_ssr_point_clamp_sampler").blob(d3d::request_sampler(smpInfo));
    }

    return [currentCameraHndl, waterLevelHndl, enableWaterSsrHndl, water_ssr_intensityVarId, water_levelVarId, underwater_renderVarId,
             use_underwater_reflectionsVarId]() {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      ShaderGlobal::set_float(water_levelVarId, waterLevelHndl.ref());

      if (wr.water)
      {
        float height;
        fft_water::getHeightAboveWater(wr.water, currentCameraHndl.ref().viewItm.getcol(3), height, true);

        float waterSsrIntensity = 1.0;

        bool isUnderwater = height < 0;
        ShaderGlobal::set_int(underwater_renderVarId, isUnderwater);

#if _TARGET_C1 || _TARGET_XBOXONE
        {
          constexpr float waterSSRMaxHeight = 80.0f;
          constexpr float waterSSRFadeoutRange = 30.0f;
          waterSsrIntensity = 1.0 - eastl::max(0.0f, (height - waterSSRMaxHeight) / waterSSRFadeoutRange);
        }
#endif
        if (!wr.isWaterSSREnabled())
          waterSsrIntensity = 0;

        ShaderGlobal::set_int(use_underwater_reflectionsVarId, wr.isWaterSSREnabled());
        ShaderGlobal::set_float(water_ssr_intensityVarId, waterSsrIntensity);
        enableWaterSsrHndl.ref() = waterSsrIntensity > 0.0;
      }
    };
  });
}

const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT)> DOWNSAMPLED_FRAME_TEX_NAMES = {
  "prev_frame_tex_for_water_early", "prev_frame_tex", "prev_frame_tex"};

// The normal prepass and the SSR resolve must request identical multiplexing,
// camera and enable state: a mismatch skips one half of the pass per sub-view
// or reprojects history with the wrong view.
static auto request_water_ssr_camera_state(dafg::Registry registry)
{
  registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
  auto camera = use_camera_in_camera(registry);
  auto cameraHndl = CameraViewShvars{camera}.bindViewVecs().toHandle();
  auto prevCameraHndl = read_history_camera_in_camera(registry).handle();

  registry.readBlobHistory<bool>("enable_water_ssr").bindToShaderVar("water_ssr_enabled_prev_frame");
  auto enableWaterSsrHndl = registry.readBlob<bool>("enable_water_ssr").handle();

  return eastl::make_tuple(cameraHndl, prevCameraHndl, enableWaterSsrHndl);
}

eastl::fixed_vector<dafg::NodeHandle, 3, false> makeWaterDepthRenameNodes()
{
  eastl::fixed_vector<dafg::NodeHandle, 3, false> nodes;
  for (uint32_t modeIdx = 0; modeIdx < WATER_DEPTH_RENAME_NODE_NAMES.size(); ++modeIdx)
    nodes.push_back(dafg::register_node(WATER_DEPTH_RENAME_NODE_NAMES[modeIdx], DAFG_PP_NODE_SRC,
      [modeIdx](dafg::Registry registry) { registry.renameTexture(WATER_SSR_DEPTH_TEX[modeIdx], WATER_SSR_DEPTH_TEX[modeIdx + 1]); }));
  return nodes;
}

eastl::fixed_vector<dafg::NodeHandle, 3, false> makeWaterSSRNode(WaterRenderMode mode)
{
  const uint32_t modeIdx = eastl::to_underlying(mode);
  eastl::fixed_vector<dafg::NodeHandle, 3, false> nodes;

  if (is_water_reflection_full_res())
  {
    nodes.push_back(dafg::register_node("water_rt_depth_copy_node", DAFG_PP_NODE_SRC, [modeIdx](dafg::Registry registry) {
      const bool hasStencil = renderer_has_feature(CAMERA_IN_CAMERA);
      const uint32_t depthFormat = get_gbuffer_depth_format(hasStencil);

      registry.requestRenderPass().depth(
        registry.create("water_rt_depth").texture({depthFormat | TEXCF_RTARGET, registry.getResolution<2>("main_view", 1.0f)}));

      auto depthHndl = registry.read(WATER_DEPTH_COPY_SOURCE_TEX[modeIdx])
                         .texture()
                         .atStage(dafg::Stage::PS)
                         .useAs(dafg::Usage::SHADER_RESOURCE)
                         .handle();
      return [depthHndl, renderer = PostFxRenderer("copy_depth")] {
        d3d::settex(var::source_depth_for_copy_const_no.get_int(), depthHndl.get());
        d3d::set_sampler(STAGE_PS, var::source_depth_for_copy_const_no.get_int(), d3d::request_sampler({}));
        renderer.render();
        d3d::settex(var::source_depth_for_copy_const_no.get_int(), nullptr);
      };
    }));
  }

  nodes.push_back(dafg::register_node("water_normal_node", DAFG_PP_NODE_SRC, [modeIdx](dafg::Registry registry) {
    registry.allowAsyncPipelines();
    registry.requestState().setFrameBlock("global_frame");

    registry.read("wfx_hmap").texture().atStage(dafg::Stage::VS | dafg::Stage::PS).bindToShaderVar().optional();
    registry.read("wfx_normals").texture().atStage(dafg::Stage::PS).bindToShaderVar().optional();

    auto downsampledDepthHndl =
      registry.modifyTexture(is_water_reflection_full_res() ? "water_rt_depth" : WATER_SSR_DEPTH_TEX[modeIdx + 1])
        .atStage(dafg::Stage::PS)
        .useAs(dafg::Usage::DEPTH_ATTACHMENT)
        .handle();
    auto normalDirHndl =
      registry.modifyTexture("water_normal_dir").atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT).handle();

    auto [cameraHndl, prevCameraHndl, enableWaterSsrHndl] = request_water_ssr_camera_state(registry);
    G_UNUSED(prevCameraHndl); // the prepass has no temporal inputs, only the resolve does

    const int wfxEffectsTexEnabledVarId = get_shader_glob_var_id("wfx_effects_tex_enabled");

    return [cameraHndl = cameraHndl, enableWaterSsrHndl = enableWaterSsrHndl, downsampledDepthHndl, normalDirHndl,
             wfxEffectsTexEnabledVarId](const dafg::multiplexing::Index &multiplexing_index) {
      if (!enableWaterSsrHndl.ref())
        return;

      d3d::set_render_target({downsampledDepthHndl.get(), 0}, DepthAccess::RW, {{normalDirHndl.get(), 0}});

      // This is a mesh pass: it must WRITE the water surface depth the SSR
      // resolve reconstructs its ray origins from, so it takes the opaque-pass
      // camcam state. ApplyPostfxState's stencil override would disable z-write.
      const camera_in_camera::ApplyMasterState camcam{multiplexing_index};
      // water_normal_dir is shared by camcam sub-views and each sub-view's render is
      // stencil-masked to its own region, so clear once on the main view only.
      if (camera_in_camera::is_main_view(multiplexing_index))
        d3d::clear_rt({normalDirHndl.get(), 0}, make_clear_value(0.0f, 0.0f, 0.0f, 0.0f));

      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      const auto &camera = cameraHndl.ref();

      ShaderGlobal::set_int(wfxEffectsTexEnabledVarId, int(use_wfx_textures()));

      wr.renderWaterNormals(camera, multiplexing_index.subCamera);
    };
  }));

  if (!is_rt_water_enabled())
  {
    nodes.push_back(dafg::register_node("water_ssr_node", DAFG_PP_NODE_SRC, [modeIdx](dafg::Registry registry) {
      registry.allowAsyncPipelines();
      registry.requestState().setFrameBlock("water3d_block");

      auto colorHndl =
        registry.modifyTexture("water_ssr_color").atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT).handle();
      auto strengthHndl =
        registry.modifyTexture("water_ssr_strength").atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT).handle();

      registry.read("water_normal_dir").texture().atStage(dafg::Stage::PS).bindToShaderVar("water_normal_dir");
      registry.read(WATER_SSR_DEPTH_TEX[modeIdx + 1]).texture().atStage(dafg::Stage::PS).bindToShaderVar("downsampled_depth");

      registry.read("water_ssr_point_clamp_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("water_ssr_point_clamp_samplerstate");
      registry.read("close_depth").texture().atStage(dafg::Stage::PS).bindToShaderVar("downsampled_close_depth_tex");
      registry.read("far_downsampled_depth").texture().atStage(dafg::Stage::PS).bindToShaderVar("downsampled_far_depth_tex");
      registry.historyFor("water_ssr_color").texture().atStage(dafg::Stage::PS).bindToShaderVar("water_reflection_tex");
      registry.historyFor("water_ssr_strength").texture().atStage(dafg::Stage::PS).bindToShaderVar("water_reflection_strength_tex");
      registry.readTexture("water_planar_reflection_terrain").atStage(dafg::Stage::PS).bindToShaderVar().optional();
      registry.readTexture("water_planar_reflection_terrain_depth").atStage(dafg::Stage::PS).bindToShaderVar().optional();
      if (renderer_has_feature(FeatureRenderFlags::PREV_OPAQUE_TEX))
      {
        registry.read("prev_frame_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("prev_frame_tex_samplerstate");
        registry.read(DOWNSAMPLED_FRAME_TEX_NAMES[modeIdx]).texture().atStage(dafg::Stage::PS).bindToShaderVar("prev_frame_tex");
      }
      // SSR uses probes, if they are present
      (registry.root() / "indoor_probes").read("probes_ready_token").blob<OrderingToken>().optional();

      auto [cameraHndl, prevCameraHndl, enableWaterSsrHndl] = request_water_ssr_camera_state(registry);

      return [cameraHndl = cameraHndl, prevCameraHndl = prevCameraHndl, enableWaterSsrHndl = enableWaterSsrHndl, colorHndl,
               strengthHndl, renderer = PostFxRenderer("water_ssr")](const dafg::multiplexing::Index &multiplexing_index) {
        if (!enableWaterSsrHndl.ref())
          return;

        // No depth target is bound, so no USE_STENCIL here;
        // view separation is done by DISCARD_IF_INVALID_VIEW_AREA_PS in the shader.
        camera_in_camera::ApplyPostfxState camcam{multiplexing_index, cameraHndl.ref(), prevCameraHndl.ref()};

        d3d::set_render_target({nullptr, 0}, DepthAccess::RW, {{colorHndl.get(), 0}, {strengthHndl.get(), 0}});
        // the targets are shared by camcam sub-views, so clear once on the main view only
        if (camera_in_camera::is_main_view(multiplexing_index))
        {
          d3d::clear_rt({colorHndl.get(), 0}, make_clear_value(0.0f, 0.0f, 0.0f, 0.0f));
          d3d::clear_rt({strengthHndl.get(), 0}, make_clear_value(1.0f, 0.0f, 0.0f, 0.0f));
        }

        auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
        if (wr.hasWaterSSRAlternateReflections())
          wr.setGILightsToShader(false /*allow_frustum_lights*/);

        renderer.render();
      };
    }));
  }

  if (is_water_reflection_full_res())
  {
    nodes.push_back(dafg::register_node("water_rt_depth_downsample_node", DAFG_PP_NODE_SRC, [modeIdx](dafg::Registry registry) {
      auto rtDepthHndl =
        registry.read("water_rt_depth").texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      auto downsampledDepthHndl = registry.modifyTexture(WATER_SSR_DEPTH_TEX[modeIdx + 1])
                                    .atStage(dafg::Stage::PS)
                                    .useAs(dafg::Usage::DEPTH_ATTACHMENT)
                                    .handle();
      auto mainViewResolutionHndl = registry.getResolution<2>("main_view");

      return [rtDepthHndl, downsampledDepthHndl, mainViewResolutionHndl](const dafg::multiplexing::Index &) {
        auto [renderingWidth, renderingHeight] = mainViewResolutionHndl.get();
        downsample_depth::downsamplePS(rtDepthHndl.get(), renderingWidth, renderingHeight, downsampledDepthHndl.get(),
          nullptr /*close_depth*/, nullptr /*normals*/);
      };
    }));
  }

  return nodes;
}

static void create_water_refraction_stub(UniqueTexWithShaderVar &water_refraction_stub)
{
  static constexpr uint32_t WATER_REFRACTION_STUB_COLOR = 0xFF182618;
  TexImage32 image[2];
  image[0].w = image[0].h = 1;
  *reinterpret_cast<E3DCOLOR *>(image + 1) = WATER_REFRACTION_STUB_COLOR;
  water_refraction_stub.close();
  water_refraction_stub = UniqueTexWithShaderVar(
    dag::create_tex(image, 1, 1, TEXFMT_A8R8G8B8 | TEXCF_LOADONCE, 1, "water_refraction_stub"), "water_refraction_tex");
}

dafg::NodeHandle makeWaterNode(WaterRenderMode mode)
{
  return dafg::register_node("water_node", DAFG_PP_NODE_SRC, [mode](dafg::Registry registry) {
    const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT)> COLOR_NAMES = {
      "opaque_with_water_before_clouds", "opaque_with_envi_and_water", "target_for_transparency"};
    const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT)> FOLLOW_NODE_NAMES = {
      nullptr, nullptr, "transparent_scene_late_node"};

    const uint32_t modeIdx = eastl::to_underlying(mode);

    if (FOLLOW_NODE_NAMES[modeIdx])
      registry.orderMeBefore(FOLLOW_NODE_NAMES[modeIdx]);

    registry.requestState().allowWireframe().setFrameBlock("global_frame");
    registry.allowAsyncPipelines();

    auto finalTargetHndl =
      registry.modify(COLOR_NAMES[modeIdx]).texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT).handle();
    auto depthHndl =
      registry.modify(WATER_DEPTH_TEX[modeIdx]).texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::DEPTH_ATTACHMENT).handle();

    use_volfog(registry, dafg::Stage::PS);

    registry.read("far_downsampled_depth").texture().atStage(dafg::Stage::PS).bindToShaderVar("downsampled_far_depth_tex");
    registry.read("close_depth").texture().atStage(dafg::Stage::PS).bindToShaderVar("downsampled_close_depth_tex");

    if (renderer_has_feature(FeatureRenderFlags::PREV_OPAQUE_TEX) && is_water_refraction_enabled())
      registry.read(DOWNSAMPLED_FRAME_TEX_NAMES[modeIdx]).texture().atStage(dafg::Stage::PS).bindToShaderVar("water_refraction_tex");

    registry.read("wfx_hmap").texture().atStage(dafg::Stage::VS | dafg::Stage::PS).bindToShaderVar().optional();
    registry.read("wfx_normals").texture().atStage(dafg::Stage::PS).bindToShaderVar().optional();

    registry.read("water_ssr_color").texture().atStage(dafg::Stage::PS).bindToShaderVar("water_reflection_tex");
    registry.read("water_ssr_strength").texture().atStage(dafg::Stage::PS).bindToShaderVar("water_reflection_strength_tex");

    registry.readTexture("water_planar_reflection_clouds").atStage(dafg::Stage::PS).bindToShaderVar().optional();
    registry.read("water_planar_reflection_clouds_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("water_planar_reflection_clouds_samplerstate")
      .optional();

    auto camera = use_camera_in_camera(registry);
    auto cameraHndl = CameraViewShvars{camera}.bindViewVecs().toHandle();

    auto enableWaterSsrHndl = registry.readBlob<bool>("enable_water_ssr").handle();
    auto isWaterRtEnabledHndl = registry.readBlob<int>("water_rt_enabled").optional().handle();

    auto reactiveMaskHndl =
      registry.modifyTexture("reactive_mask").atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT).optional().handle();

    return [isWaterRtEnabledHndl, mode, cameraHndl, enableWaterSsrHndl, finalTargetHndl, depthHndl, reactiveMaskHndl,
             wfxEffectsTexEnabledVarId = get_shader_glob_var_id("wfx_effects_tex_enabled")](
             const dafg::multiplexing::Index &multiplexing_index) {
      const bool isWaterRtEnabled = isWaterRtEnabledHndl.get() ? *isWaterRtEnabledHndl.get() : 0;
      ShaderGlobal::set_int(var::water_rt_enabled, isWaterRtEnabled);

      d3d::set_render_target({depthHndl.get(), 0}, DepthAccess::RW, {{finalTargetHndl.get(), 0}, {reactiveMaskHndl.get(), 0}});
      const camera_in_camera::ApplyMasterState camcam{multiplexing_index};

      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      ShaderGlobal::set_int(wfxEffectsTexEnabledVarId, int(use_wfx_textures()));
      wr.renderWater(cameraHndl.ref(), WorldRenderer::DistantWater{mode != WaterRenderMode::LATE && distantWater},
        enableWaterSsrHndl.ref(), multiplexing_index.subCamera);
    };
  });
}

template <typename Callable>
static void water_refraction_stub_ecs_query(ecs::EntityManager &manager, Callable c);

void bind_water_refraction_stub_if_unset()
{
  static int water_refraction_texVarId = get_shader_variable_id("water_refraction_tex", true);
  // FG does not bind the refraction tex when it is disabled, so force the var NULL here: a stale
  // binding from any prior source (e.g. a portal that bound the stub) would otherwise leak in and
  // break the render.
  if (!is_water_refraction_enabled())
  {
    ShaderGlobal::set_texture(water_refraction_texVarId, BAD_TEXTUREID);
    return;
  }
  // check if it's already set by the caller: get_tex_ptr works for FG managed textures too
  if (ShaderGlobal::get_tex_ptr(water_refraction_texVarId) != nullptr)
    return;
  water_refraction_stub_ecs_query(*g_entity_mgr, [](UniqueTexWithShaderVar &water_refraction_stub) {
    if (!water_refraction_stub)
      create_water_refraction_stub(water_refraction_stub);
    water_refraction_stub.setVar();
  });
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraNodeConstruction)
static void create_water_nodes_es(const OnCameraNodeConstruction &)
{
  static_cast<WorldRenderer *>(get_world_renderer())->recreateWaterNodes();
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_REQUIRE(FFTWater water)
static void create_water_refraction_stub_es(const ecs::Event &, ecs::EntityManager &manager)
{
  manager.getOrCreateSingletonEntity(ECS_HASH("water_refraction_stub"));
}

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(FFTWater water)
static void destroy_water_refraction_stub_es(const ecs::Event &, ecs::EntityManager &manager)
{
  manager.destroyEntity(manager.getSingletonEntity(ECS_HASH("water_refraction_stub")));
}

ECS_TAG(render)
ECS_ON_EVENT(EventAfterDeviceReset)
static void recreate_water_refraction_stub_es(const ecs::Event &, UniqueTexWithShaderVar &water_refraction_stub)
{
  create_water_refraction_stub(water_refraction_stub);
}
