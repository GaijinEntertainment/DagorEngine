// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entitySystem.h>
#include <shaders/dag_postFxRenderer.h>
#include <render/viewVecs.h>
#include <render/debugMesh.h>
#include <render/deferredRenderer.h>
#include <render/renderEvent.h>
#include <render/world/worldRendererQueries.h>
#include <render/world/gbufferConsts.h>
#include <render/world/defaultVrsSettings.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_convar.h>
#include <EASTL/fixed_string.h>
#include <math/dag_Point3.h>
#include <render/daFrameGraph/singleShaders.h>

#include <daECS/core/entityManager.h>

#define INSIDE_RENDERER 1
#include "../private_worldRenderer.h"
#include "frameGraphNodes.h"
#include <render/world/frameGraphHelpers.h>
#include <drv/3d/dag_renderTarget.h>
#include <render/antiAliasing_legacy.h>
#include <render/world/wrDispatcher.h>
#include <render/world/depthBounds.h>
#include <render/world/bvh.h>
#include <landMesh/virtualtexture.h>

CONSOLE_FLOAT_VAL_MINMAX("render", thin_scattering_start, 32.0f, 0.f, 1024);
extern ConVarT<bool, false> volfog_enabled;
extern ConVarT<bool, false> dynamic_lights;
extern ConVarT<bool, false> process_reactive_gbuffer_data;
extern ConVarT<int, true> gi_quality;


dafg::NodeHandle makePrepareGbufferDepthNode(uint32_t global_flags, int depth_format)
{
  auto ns = dafg::root() / "init";
  return ns.registerNode("prepare_gbuffer_depth_node", DAFG_PP_NODE_SRC, [global_flags, depth_format](dafg::Registry registry) {
    const auto gbufResolution = registry.getResolution<2>("main_view");

    // Weird names are used here for simpler integration with "opaque/closeups"
    registry.create("gbuf_depth_done")
      .texture({depth_format | global_flags | TEXCF_RTARGET, gbufResolution})
      .atStage(dafg::Stage::PS)
      .useAs(dafg::Usage::DEPTH_ATTACHMENT)
      .clear(make_clear_value(0.f, 0));
  });
}

dafg::NodeHandle makePrepareGbufferNode(
  uint32_t global_flags, uint32_t gbuf_cnt, eastl::span<uint32_t> main_gbuf_fmts, bool has_motion_vectors, bool is_rr_enabled)
{
  auto ns = dafg::root() / "init";
  return ns.registerNode("prepare_gbuffer_node", DAFG_PP_NODE_SRC,
    [global_flags, gbuf_cnt, has_motion_vectors, is_rr_enabled,
      gbufFmts = dag::RelocatableFixedVector<uint32_t, FULL_GBUFFER_RT_COUNT, false>(main_gbuf_fmts)](dafg::Registry registry) {
      const auto gbufResolution = registry.getResolution<2>("main_view");

      static const Point3 MOTION_MASK{65504, 65504, 0};
      // Weird names are used here for simpler integration with "opaque/closeups"
      for (uint32_t i = 0; i < gbuf_cnt; ++i)
      {
        eastl::fixed_string<char, 16> name(eastl::fixed_string<char, 16>::CtorSprintf{}, "gbuf_%d_done", i);
        auto gbufRtHndl = registry.create(name.c_str())
                            .texture({gbufFmts[i] | global_flags | TEXCF_RTARGET, gbufResolution})
                            .useAs(dafg::Usage::COLOR_ATTACHMENT)
                            .atStage(dafg::Stage::PS);
        if (i == 0 && is_rr_enabled)
          eastl::move(gbufRtHndl).clear(make_clear_value(0.0f, 0.0f, 0.0f, 0.0f));
        if (i == 1 && (!renderer_has_feature(FeatureRenderFlags::PREV_OPAQUE_TEX) || is_rr_enabled))
          eastl::move(gbufRtHndl).clear(make_clear_value(0, 0, 0, 0));
        if (i == 2 && (has_motion_vectors))
          eastl::move(gbufRtHndl).clear(make_clear_value(0, 0, 0, 0));
        if (i == 3)
          eastl::move(gbufRtHndl).clear(make_clear_value(MOTION_MASK.x, MOTION_MASK.y, MOTION_MASK.z, 0.0f));
      }

      registry.requestState().setFrameBlock("global_frame");

      registry.requestRenderPass()
        .depthRw("gbuf_depth_done")
        .color({"gbuf_0_done", "gbuf_1_done", registry.modify("gbuf_2_done").texture().optional(),
          registry.modify("gbuf_3_done").texture().optional()});

      const auto camera = registry.readBlob<CameraParams>("current_camera");
      CameraViewShvars{camera}.bindViewVecs().toHandle();
      use_jitter_frustum_plane_shader_vars(registry);
      return [gbufResolution, screenPosToTexcoordVard = get_shader_variable_id("screen_pos_to_texcoord"),
               gbufferViewSizeVarId = get_shader_variable_id("gbuffer_view_size")]() {
        auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
        if (wr.clipmap)
          wr.clipmap->resetUAVAtomicPrefix(); // Reset is required before each multiplex pass.

        debug_mesh::activate_mesh_coloring_master_override();

        const auto resolution = gbufResolution.get();
        ShaderGlobal::set_float4(screenPosToTexcoordVard, 1.f / resolution.x, 1.f / resolution.y, 0, 0);
        ShaderGlobal::set_int4(gbufferViewSizeVarId, resolution.x, resolution.y, 0, 0);
      };
    });
}

dafg::NodeHandle makeReactiveMaskFillNode(bool process_gbuffer_data)
{
  if (process_gbuffer_data)
    return dafg::register_node("reactive_mask_fill", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      registry.createTexture2d("reactive_mask", {TEXFMT_R8 | TEXCF_RTARGET, registry.getResolution<2>("main_view")});

      auto ns = registry.root() / "opaque" / "decorations";
      registry.requestRenderPass().color({"reactive_mask"});
      ns.read("gbuf_3_done").texture().atStage(dafg::Stage::PS).bindToShaderVar("reactive_gbuf");

      dafg::postFx("fill_reactive_mask_from_gbuffer", registry);
    });
  else
    return dafg::register_node("reactive_mask_clear", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      registry.createTexture2d("reactive_mask", {TEXFMT_R8 | TEXCF_RTARGET, registry.getResolution<2>("main_view")})
        .clear(make_clear_value(0.f, 0));
    });
}

static inline float w_to_depth(float w, const Point2 &zNearFar, float def)
{
  return w > zNearFar.x ? (zNearFar.y - w) * (zNearFar.x / (zNearFar.y - zNearFar.x)) / w : def; // reverse
  // return w > zNearFar.x ? (w-zNearFar.x)* zNearFar.y/(zNearFar.y-zNearFar.x)/w : 0.0;//fwd
}

#define RESOLVE_GBUFFER_SHADERVARS \
  VAR(reproject_screen_gi, true)   \
  VAR(thin_gbuf_resolve, true)     \
  VAR(frame_tex, false)            \
  VAR(dynamic_lights_count, true)  \
  VAR(gi_use_r9g9b9e5_tex_fmt, true)

#define VAR(a, o) static ShaderVariableInfo a##VarId(#a, o);
RESOLVE_GBUFFER_SHADERVARS
#undef VAR

// TODO Restore compute shader version, or delete the shader as a whole
// CONSOLE_BOOL_VAL("render", deferred_light_on_compute, false);
// deferredLightCompute = Ptr(new_compute_shader("deferred_light_compute"))
// And instead of render target just set current_specular and current_ambient as @uav shadervars
eastl::array<dafg::NodeHandle, 2> makeDeferredLightNode(bool reprojectGI)
{
  const bool giUseR9G9B9E5TexFmt = reprojectGI && d3d::check_texformat(TEXFMT_R9G9B9E5 | TEXCF_RTARGET);
  ShaderGlobal::set_int(gi_use_r9g9b9e5_tex_fmtVarId, giUseR9G9B9E5TexFmt ? 1 : 0);

  const uint32_t gi_fmt = giUseR9G9B9E5TexFmt ? TEXFMT_R9G9B9E5 : TEXFMT_R11G11B10F;
  const auto giHistory = reprojectGI ? dafg::History::ClearZeroOnFirstFrame : dafg::History::No;

  auto perCameraResNode = dafg::register_node("deferred_light_per_cam_res_node", DAFG_PP_NODE_SRC,
    [reprojectGI, gi_fmt, giHistory](dafg::Registry registry) {
      registry.create("current_specular")
        .texture({gi_fmt | TEXCF_RTARGET, registry.getResolution<2>("main_view"), 1})
        .withHistory(giHistory)
        .useAs(dafg::Usage::COLOR_ATTACHMENT)
        .atStage(dafg::Stage::PS)
        .clear(make_clear_value(0.f, 0.f, 0.f, 0.f));

      registry.create("current_ambient")
        .texture({gi_fmt | TEXCF_RTARGET, registry.getResolution<2>("main_view"), 1})
        .withHistory(giHistory)
        .useAs(dafg::Usage::COLOR_ATTACHMENT)
        .atStage(dafg::Stage::PS)
        .clear(make_clear_value(0.f, 0.f, 0.f, 0.f));

      if (reprojectGI)
      {
        registry.create("current_gi_pixel_age")
          .texture({TEXFMT_R8 | TEXCF_RTARGET, registry.getResolution<2>("main_view"), 1})
          .withHistory(dafg::History::ClearZeroOnFirstFrame)
          .useAs(dafg::Usage::COLOR_ATTACHMENT)
          .atStage(dafg::Stage::PS);
      }
    });

  auto renderNode = dafg::register_node("deferred_light_node", DAFG_PP_NODE_SRC, [reprojectGI](dafg::Registry registry) {
    auto gbufDepthHndl = registry.read("gbuf_depth").texture();
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("depth_gbuf_samplerstate");
    registry.read("gbuf_1").texture().atStage(dafg::Stage::PS).bindToShaderVar("normal_gbuf");
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("normal_gbuf_samplerstate");
    registry.read("gbuf_2").texture().atStage(dafg::Stage::PS).bindToShaderVar("material_gbuf");
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("material_gbuf_samplerstate");
    registry.read("motion_vecs").texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("motion_gbuf").optional();
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("motion_gbuf_samplerstate").optional();

    auto stateRequest = registry.requestState().setFrameBlock("global_frame");

    auto depthBoundsSet = [&](dafg::VirtualPassRequest &renderPass) {
      if (depth_bounds_enabled())
      {
        shaders::OverrideState state;
        state.set(shaders::OverrideState::Z_BOUNDS_ENABLED);
        eastl::move(stateRequest).enableOverride(state);
        eastl::move(renderPass).depthRoAndBindToShaderVars(gbufDepthHndl, {"depth_gbuf"});
      }
      else if (renderer_has_feature(CAMERA_IN_CAMERA))
        eastl::move(renderPass).depthRoAndBindToShaderVars(gbufDepthHndl, {"depth_gbuf"});
      else
        eastl::move(gbufDepthHndl).atStage(dafg::Stage::PS).bindToShaderVar("depth_gbuf");
    };

    if (reprojectGI)
    {
      registry.historyFor("depth_for_postfx").texture().atStage(dafg::Stage::PS).bindToShaderVar("prev_gbuf_depth");
      registry.historyFor("current_gi_pixel_age").texture().atStage(dafg::Stage::PS).bindToShaderVar("prev_gi_pixel_age");
      registry.historyFor("current_specular").texture().atStage(dafg::Stage::PS).bindToShaderVar("prev_specular");
      registry.historyFor("current_ambient").texture().atStage(dafg::Stage::PS).bindToShaderVar("prev_ambient");
      auto renderPass = registry.requestRenderPass()
                          .color({"current_specular", "current_ambient", "current_gi_pixel_age"})
                          .vrsRate(SHADING_VRS_RATE_TEXTURE_NAME);
      depthBoundsSet(renderPass);
    }
    else
    {
      auto renderPass =
        registry.requestRenderPass().color({"current_specular", "current_ambient"}).vrsRate(SHADING_VRS_RATE_TEXTURE_NAME);
      depthBoundsSet(renderPass);
    }
    const auto currentAmbientResolution = registry.getResolution<2>("main_view", 1);


    registry.read("gi_before_frame_lit_token").blob<OrderingToken>().optional();

    //  We apply indoor probes when resolving the gbuffer. We also use
    // them in most transparent stuff, but obviously transparent stuff
    // goes after the resolve, so no need for orderings there.
    (registry.root() / "indoor_probes").read("probes_ready_token").blob<OrderingToken>().optional();

    registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    auto camera = use_camera_in_camera(registry);
    auto cameraHndl = CameraViewShvars{camera}.bindViewVecs().toHandle();
    auto prevCameraHndl = read_history_camera_in_camera(registry).handle();

    return [reproject = reprojectGI, deferredLight = PostFxRenderer("deferredLight"), cameraHndl, prevCameraHndl,
             currentAmbientResolution](const dafg::multiplexing::Index &multiplexing_index) {
      camera_in_camera::ApplyPostfxState camcam{
        multiplexing_index, cameraHndl.ref(), prevCameraHndl.ref(), camera_in_camera::USE_STENCIL};

      float farPlaneDepth = far_plane_depth(get_gbuffer_depth_format());
      api_set_depth_bounds(farPlaneDepth, 1);
      const uint32_t histFrames = reproject ? static_cast<WorldRenderer *>(get_world_renderer())->getGIHistoryFrames() : 0;
      ShaderGlobal::set_int(reproject_screen_giVarId, histFrames ? 1 : 0);
      if (is_rtgi_enabled())
        bvh_bind_resources(currentAmbientResolution.get().x);
      deferredLight.render();
    };
  });

  return {eastl::move(perCameraResNode), eastl::move(renderNode)};
}
static void bindResolvePassResources(dafg::Registry registry)
{
  registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("depth_gbuf_samplerstate");
  registry.read("gbuf_0").texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("albedo_gbuf");
  registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("albedo_gbuf_samplerstate");
  registry.read("gbuf_1").texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("normal_gbuf");
  registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("normal_gbuf_samplerstate");
  registry.read("gbuf_2").texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("material_gbuf").optional();
  registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("material_gbuf_samplerstate").optional();
  registry.read("motion_vecs").texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("motion_gbuf").optional();
  registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("motion_gbuf_samplerstate").optional();
  registry.readTexture("close_depth").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("downsampled_close_depth_tex");
  registry.read("close_depth_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("downsampled_close_depth_tex_samplerstate");
  registry.readTexture("far_downsampled_depth").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("downsampled_far_depth_tex");
  registry.read("far_downsampled_depth_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("downsampled_far_depth_tex_samplerstate");

  if (renderer_has_feature(FeatureRenderFlags::COMBINED_SHADOWS))
  {
    registry.readTexture("combined_shadows").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("combined_shadows");
    registry.read("combined_shadows_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("combined_shadows_samplerstate");
  }

  if (renderer_has_feature(FeatureRenderFlags::DEFERRED_LIGHT))
  {
    registry.readTexture("current_specular").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("specular_tex");
    registry.readTexture("current_ambient").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("current_ambient");
  }
  registry.readTexture("ssr_target").atStage(dafg::Stage::PS).bindToShaderVar("ssr_target").optional();
  registry.read("ssr_target_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("ssr_target_samplerstate").optional();

  use_volfog(registry, dafg::Stage::PS_OR_CS);
  registry.readTexture("ssao_tex").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("ssao_tex").optional();
  registry.read("ssao_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("ssao_tex_samplerstate").optional();
  registry.readBlob<OrderingToken>("rtr_token").optional();
  registry.readBlob<OrderingToken>("rtao_token").optional();
  registry.readBlob<OrderingToken>("ptgi_token").optional();
  // for SSAO&SSR
  registry.read("upscale_sampling_tex").texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("upscale_sampling_tex").optional();
}

static dafg::NodeHandle makeFullResolveGbufferNode(const char *resolve_pshader_name, const char *resolve_cshader_name)
{
  return dafg::register_node("resolve_gbuffer_node", DAFG_PP_NODE_SRC,
    [resolve_pshader_name, resolve_cshader_name](dafg::Registry registry) {
      registry.read("gi_before_frame_lit_token").blob<OrderingToken>().optional();

      registry.bindBlob<bool>("has_any_dynamic_lights", "has_any_dynamic_lights");
      registry.readTexture("dynamic_lighting_texture").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("dynamic_lighting_texture");

      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      auto gbufDepthHndl = registry.read("gbuf_depth").texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("depth_gbuf").handle();
      bindResolvePassResources(registry);

      auto finalTargetHndl = registry.modifyTexture("opaque_resolved")
                               .atStage(wr.getBareMinimumPreset() ? dafg::Stage::POST_RASTER : dafg::Stage::PS_OR_CS)
                               .useAs(wr.getBareMinimumPreset() ? dafg::Usage::COLOR_ATTACHMENT : dafg::Usage::SHADER_RESOURCE)
                               .handle();
      registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");

      registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
      auto camera = use_camera_in_camera(registry);
      auto cameraHndl = CameraViewShvars{camera}.bindViewVecs().toHandle();

      registry.requestState().setFrameBlock("global_frame");
      auto waterModeHndl = registry.readBlob<WaterRenderMode>("water_render_mode").handle();
      registry.read(SHADING_VRS_RATE_TEXTURE_NAME).texture().atStage(dafg::Stage::CS).bindToShaderVar("vrs_rate_tex").optional();

      shaders::OverrideState state{};
      state.set(shaders::OverrideState::Z_BOUNDS_ENABLED);

      return [enabledDepthBoundsId = shaders::overrides::create(state),
               shadingResolver = eastl::make_unique<ShadingResolver>(resolve_pshader_name, resolve_cshader_name, nullptr,
                 ShadingResolver::PermutationsDesc{}),
               gbufDepthHndl, finalTargetHndl, waterModeHndl, cameraHndl](const dafg::multiplexing::Index &multiplexing_index) {
        camera_in_camera::ApplyPostfxState camcam{multiplexing_index, cameraHndl.ref()};

        ShadowsManager &shadowsManager = WRDispatcher::getShadowsManager();
        const CameraParams &camera = cameraHndl.ref();

        shadowsManager.setShadowFrameIndex(camera);

        bool useDepthBounds = ::depth_bounds_enabled();
        if (useDepthBounds)
        {
          float farPlaneDepth = far_plane_depth(get_gbuffer_depth_format());
          shaders::overrides::set(enabledDepthBoundsId);
          api_set_depth_bounds(farPlaneDepth, 1);
        }

        TIME_D3D_PROFILE(resolve);
        const ShadingResolver::ClearTarget needTargetClear =
          camera_in_camera::is_main_view(multiplexing_index) &&
              (has_custom_sky() || waterModeHndl.ref() == WaterRenderMode::EARLY_BEFORE_ENVI)
            ? ShadingResolver::ClearTarget::Yes
            : ShadingResolver::ClearTarget::No;
        shadingResolver->resolve(finalTargetHndl.get(), camera.viewTm, camera.jitterProjTm, gbufDepthHndl.get(), needTargetClear);

        if (useDepthBounds)
          shaders::overrides::reset();
      };
    });
}

static dafg::NodeHandle makeThinResolveGbufferNodeWithDepthBounds(const char *resolve_pshader_name, const char *resolve_cshader_name)
{
  return dafg::register_node("resolve_gbuffer_node", DAFG_PP_NODE_SRC,
    [resolve_pshader_name, resolve_cshader_name](dafg::Registry registry) {
      registry.read("gi_before_frame_lit_token").blob<OrderingToken>().optional();

      auto gbufDepthHndl = registry.read("gbuf_depth").texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("depth_gbuf").handle();
      bindResolvePassResources(registry);

      auto finalTargetHndl =
        registry.modifyTexture("opaque_resolved").atStage(dafg::Stage::POST_RASTER).useAs(dafg::Usage::COLOR_ATTACHMENT).handle();
      registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
      auto cameraHndl = use_current_camera(registry).handle();
      registry.requestState().setFrameBlock("global_frame");
      auto waterModeHndl = registry.readBlob<WaterRenderMode>("water_render_mode").handle();

      shaders::OverrideState state{};
      state.set(shaders::OverrideState::Z_BOUNDS_ENABLED);

      return [enabledDepthBoundsId = shaders::overrides::create(state),
               shadingResolver = eastl::make_unique<ShadingResolver>(resolve_pshader_name, resolve_cshader_name, nullptr,
                 ShadingResolver::PermutationsDesc{}),
               gbufDepthHndl, finalTargetHndl, waterModeHndl, cameraHndl]() {
        ShadowsManager &shadowsManager = WRDispatcher::getShadowsManager();
        const CameraParams &camera = cameraHndl.ref();

        shadowsManager.setShadowFrameIndex(camera);

        float farPlaneDepth = far_plane_depth(get_gbuffer_depth_format());
        shaders::overrides::set(enabledDepthBoundsId);
        api_set_depth_bounds(farPlaneDepth, 1);

        TIME_D3D_PROFILE(resolve);
        ShadingResolver::ClearTarget needTargetClear = has_custom_sky() || waterModeHndl.ref() == WaterRenderMode::EARLY_BEFORE_ENVI
                                                         ? ShadingResolver::ClearTarget::Yes
                                                         : ShadingResolver::ClearTarget::No;

        enum ThinGbufResolve
        {
          NO_DEPTH_BOUNDS = 0,
          SHADOWS_NO_SCATTERING = 1,
          ALL_SHADOWS_SCATTERING = 2,
          STATIC_SHADOWS_NO_SCATTERING = 3,
          STATIC_SHADOWS_SCATTERING = 4
        };

        CascadeShadows *csm = WRDispatcher::getShadowsManager().getCascadeShadows();
        const bool hasVolumeLight = WRDispatcher::getVolumeLight() && volfog_enabled.get();
        const float scatteringStart = hasVolumeLight ? 0 : thin_scattering_start.get(),
                    csmEnd = csm ? csm->getMaxShadowDistance() : scatteringStart;
        const float scatteringStartRawD = w_to_depth(scatteringStart, Point2(camera.jitterPersp.zn, camera.jitterPersp.zf), 1.f);
        const float csm_distancRawD = w_to_depth(csmEnd, Point2(camera.jitterPersp.zn, camera.jitterPersp.zf), 1.f);

        if (scatteringStart < csmEnd)
        {
          if (scatteringStartRawD < 1.0f)
          {
            api_set_depth_bounds(scatteringStartRawD, 1);
            ShaderGlobal::set_int(thin_gbuf_resolveVarId, SHADOWS_NO_SCATTERING);
            shadingResolver->resolve(finalTargetHndl.get(), camera.viewTm, camera.jitterProjTm, gbufDepthHndl.get(), needTargetClear);
            needTargetClear = ShadingResolver::ClearTarget::No;
          }
          api_set_depth_bounds(csm_distancRawD, scatteringStartRawD);
          ShaderGlobal::set_int(thin_gbuf_resolveVarId, ALL_SHADOWS_SCATTERING);
          shadingResolver->resolve(finalTargetHndl.get(), camera.viewTm, camera.jitterProjTm, gbufDepthHndl.get(), needTargetClear);
          needTargetClear = ShadingResolver::ClearTarget::No;
        }
        else
        {
          if (csm_distancRawD < 1.0f)
          {
            api_set_depth_bounds(csm_distancRawD, 1);
            ShaderGlobal::set_int(thin_gbuf_resolveVarId, SHADOWS_NO_SCATTERING);
            shadingResolver->resolve(finalTargetHndl.get(), camera.viewTm, camera.jitterProjTm, gbufDepthHndl.get(), needTargetClear);
            needTargetClear = ShadingResolver::ClearTarget::No;
          }
          api_set_depth_bounds(scatteringStartRawD, csm_distancRawD);
          ShaderGlobal::set_int(thin_gbuf_resolveVarId, STATIC_SHADOWS_NO_SCATTERING);
          shadingResolver->resolve(finalTargetHndl.get(), camera.viewTm, camera.jitterProjTm, gbufDepthHndl.get(), needTargetClear);
          needTargetClear = ShadingResolver::ClearTarget::No;
        }

        api_set_depth_bounds(farPlaneDepth, scatteringStart < csmEnd ? csm_distancRawD : scatteringStartRawD);
        ShaderGlobal::set_int(thin_gbuf_resolveVarId, STATIC_SHADOWS_SCATTERING);
        shadingResolver->resolve(finalTargetHndl.get(), camera.viewTm, camera.jitterProjTm, gbufDepthHndl.get(), needTargetClear);
        api_set_depth_bounds(farPlaneDepth, 1); // restore default
        ShaderGlobal::set_int(thin_gbuf_resolveVarId, NO_DEPTH_BOUNDS);

        shaders::overrides::reset();
      };
    });
}

static dafg::NodeHandle makeThinResolveGbufferNode(const char *resolve_pshader_name, const char *resolve_cshader_name)
{
  return dafg::register_node("resolve_gbuffer_node", DAFG_PP_NODE_SRC,
    [resolve_pshader_name, resolve_cshader_name](dafg::Registry registry) {
      registry.read("gi_before_frame_lit_token").blob<OrderingToken>().optional();

      auto gbufDepthHndl = registry.read("gbuf_depth").texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("depth_gbuf").handle();
      bindResolvePassResources(registry);

      auto finalTargetHndl =
        registry.modifyTexture("opaque_resolved").atStage(dafg::Stage::POST_RASTER).useAs(dafg::Usage::COLOR_ATTACHMENT).handle();
      registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");

      auto cameraHndl = use_current_camera(registry).handle();

      registry.requestState().setFrameBlock("global_frame");
      auto waterModeHndl = registry.readBlob<WaterRenderMode>("water_render_mode").handle();

      return [shadingResolver = eastl::make_unique<ShadingResolver>(resolve_pshader_name, resolve_cshader_name, nullptr,
                ShadingResolver::PermutationsDesc{}),
               gbufDepthHndl, finalTargetHndl, waterModeHndl, cameraHndl]() {
        const CameraParams &camera = cameraHndl.ref();
        WRDispatcher::getShadowsManager().setShadowFrameIndex(camera);

        TIME_D3D_PROFILE(resolve);
        const ShadingResolver::ClearTarget needTargetClear =
          has_custom_sky() || waterModeHndl.ref() == WaterRenderMode::EARLY_BEFORE_ENVI ? ShadingResolver::ClearTarget::Yes
                                                                                        : ShadingResolver::ClearTarget::No;
        shadingResolver->resolve(finalTargetHndl.get(), camera.viewTm, camera.jitterProjTm, gbufDepthHndl.get(), needTargetClear);
      };
    });
}

static dafg::NodeHandle makeRenderOtherLightsNode()
{
  return dafg::register_node("render_other_lights", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto wr = static_cast<WorldRenderer *>(get_world_renderer());
    auto finalTargetHndl = registry.modifyTexture("opaque_resolved")
                             .atStage(wr->getBareMinimumPreset() ? dafg::Stage::POST_RASTER : dafg::Stage::PS_OR_CS)
                             .useAs(wr->getBareMinimumPreset() ? dafg::Usage::COLOR_ATTACHMENT : dafg::Usage::SHADER_RESOURCE)
                             .handle();
    auto gbufDepthHndl = registry.read("gbuf_depth").texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("depth_gbuf").handle();
    registry.orderMeAfter("resolve_gbuffer_node");
    registry.requestState().setFrameBlock("global_frame");
    bindResolvePassResources(registry);

    shaders::OverrideState state{};
    state.set(shaders::OverrideState::Z_BOUNDS_ENABLED);

    return [enabledDepthBoundsId = shaders::overrides::create(state), finalTargetHndl, gbufDepthHndl]() {
      ClusteredLights &lights = WRDispatcher::getClusteredLights();
      if (!dynamic_lights.get() || !lights.hasDeferredLights())
        return;

      const bool useDepthBounds = ::depth_bounds_enabled();
      if (useDepthBounds)
      {
        float farPlaneDepth = far_plane_depth(get_gbuffer_depth_format());
        shaders::overrides::set(enabledDepthBoundsId);
        api_set_depth_bounds(farPlaneDepth, 1);
      }
      {
        SCOPE_RENDER_TARGET;
        d3d::set_render_target({gbufDepthHndl.get(), 0, 0}, DepthAccess::SampledRO, {{finalTargetHndl.get(), 0, 0}});
        lights.renderOtherLights();
      }
      if (useDepthBounds)
        shaders::overrides::reset();
    };
  });
}

eastl::fixed_vector<dafg::NodeHandle, 2, false> makeResolveGbufferNodes(const char *resolve_pshader_name,
  const char *resolve_cshader_name)
{
  const bool isFullDeferred = renderer_has_feature(FULL_DEFERRED);
  const bool useDepthBounds = ::depth_bounds_enabled();

  eastl::fixed_vector<dafg::NodeHandle, 2, false> result;
  if (isFullDeferred)
  {
    result.push_back(makeFullResolveGbufferNode(resolve_pshader_name, resolve_cshader_name));
    result.push_back(makeRenderOtherLightsNode());
  }
  else if (useDepthBounds)
    result.push_back(makeThinResolveGbufferNodeWithDepthBounds(resolve_pshader_name, resolve_cshader_name));
  else
    result.push_back(makeThinResolveGbufferNode(resolve_pshader_name, resolve_cshader_name));
  return result;
}

dafg::NodeHandle makeRenameDepthNode()
{
  return dafg::register_node("rename_depth", DAFG_PP_NODE_SRC,
    [](dafg::Registry registry) { registry.renameTexture("gbuf_depth", "gbuf_depth_after_resolve"); });
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraNodeConstruction)
static void create_gbuffer_nodes_es(const OnCameraNodeConstruction &evt)
{
  evt.nodes->push_back(makeRenameDepthNode());

  auto *antiAliasing = WRDispatcher::getAntialiasing();
  if (antiAliasing && antiAliasing->supportsReactiveMask())
    evt.nodes->push_back(makeReactiveMaskFillNode(process_reactive_gbuffer_data));

  {
    const bool hasStencilTest = renderer_has_feature(FeatureRenderFlags::CAMERA_IN_CAMERA);
    const int gbufDepthFormat = get_gbuffer_depth_format(hasStencilTest);
    evt.nodes->push_back(makePrepareGbufferDepthNode(WRDispatcher::getGbufferTargetGlobalFlags(), gbufDepthFormat));
  }

  if (renderer_has_feature(FeatureRenderFlags::DEFERRED_LIGHT))
  {
    auto nodes = makeDeferredLightNode(evt.giNeedsReprojection);
    for (auto &n : nodes)
      evt.nodes->push_back(eastl::move(n));
  }

  {
    const char *resolve_shader = nullptr;
    const char *resolve_cshader = nullptr;

    const bool thinGBuffer = !renderer_has_feature(FeatureRenderFlags::FULL_DEFERRED);


    if (thinGBuffer)
      resolve_shader = THIN_GBUFFER_RESOLVE_SHADER;
    else
    {
      resolve_shader = evt.isBareMinimum ? "deferred_shadow_bare_minimum" : "deferred_shadow_to_buffer";
      resolve_cshader = evt.isBareMinimum ? "" : "deferred_shadow_compute";
    }

    auto nodes = makeResolveGbufferNodes(resolve_shader, resolve_cshader);
    for (auto &n : nodes)
      evt.nodes->push_back(eastl::move(n));

    // We expose the gbuffer to the outside world by renaming it to
    // "/gbuf_X", i.e. moving it to the global namespace
    const bool isNormalsPacked = renderer_has_feature(GBUFFER_PACKED_NORMALS);
    evt.nodes->push_back(dafg::register_node("gbuffer_publish_node", DAFG_PP_NODE_SRC, [isNormalsPacked](dafg::Registry registry) {
      auto ns = isNormalsPacked ? registry.root() / "opaque" / "mixing" : registry.root() / "opaque" / "decorations";
      ns.rename("gbuf_0_done", "gbuf_0").texture();
      if (!isNormalsPacked)
      {
        ns.rename("gbuf_1_done", "gbuf_1").texture().optional();
      }
      ns.rename("gbuf_2_done", "gbuf_2").texture().optional();
      ns.rename("gbuf_3_done", "gbuf_3").texture().optional();
      ns.rename("gbuf_depth_done", "gbuf_depth").texture();

      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      smpInfo.filter_mode = d3d::FilterMode::Point;
      registry.create("gbuf_sampler").blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));
    }));
  }
}
