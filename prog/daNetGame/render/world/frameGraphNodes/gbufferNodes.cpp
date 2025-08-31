// Copyright (C) Gaijin Games KFT.  All rights reserved.

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

#include <daECS/core/entityManager.h>

#define INSIDE_RENDERER 1
#include "../private_worldRenderer.h"
#include "frameGraphNodes.h"
#include <render/world/frameGraphHelpers.h>
#include <drv/3d/dag_renderTarget.h>
#include <render/antiAliasing.h>
#include <render/world/wrDispatcher.h>
#include <render/world/depthBounds.h>
#include <landMesh/virtualtexture.h>

CONSOLE_FLOAT_VAL_MINMAX("render", thin_scattering_start, 32.0f, 0.f, 1024);
extern ConVarT<bool, false> volfog_enabled;
extern ConVarT<bool, false> dynamic_lights;
extern ConVarT<int, true> gi_quality;


dafg::NodeHandle makePrepareGbufferDepthNode(uint32_t global_flags, int depth_format)
{
  auto ns = dafg::root() / "init";
  return ns.registerNode("prepare_gbuffer_depth_node", DAFG_PP_NODE_SRC, [global_flags, depth_format](dafg::Registry registry) {
    const auto gbufResolution = registry.getResolution<2>("main_view");

    // Weird names are used here for simpler integration with "opaque/closeups"
    registry.create("gbuf_depth_done", dafg::History::No)
      .texture({depth_format | global_flags | TEXCF_RTARGET, gbufResolution})
      .atStage(dafg::Stage::PS)
      .useAs(dafg::Usage::DEPTH_ATTACHMENT)
      .clear(make_clear_value(0.f, 0));
  });
}

dafg::NodeHandle makePrepareGbufferNode(
  uint32_t global_flags, uint32_t gbuf_cnt, eastl::span<uint32_t> main_gbuf_fmts, bool has_motion_vectors)
{
  auto ns = dafg::root() / "init";
  return ns.registerNode("prepare_gbuffer_node", DAFG_PP_NODE_SRC,
    [global_flags, gbuf_cnt, has_motion_vectors,
      gbufFmts = dag::RelocatableFixedVector<uint32_t, FULL_GBUFFER_RT_COUNT, false>(main_gbuf_fmts)](dafg::Registry registry) {
      const auto gbufResolution = registry.getResolution<2>("main_view");

      static const Point3 MOTION_MASK{65504, 65504, 0};
      // Weird names are used here for simpler integration with "opaque/closeups"
      for (uint32_t i = 0; i < gbuf_cnt; ++i)
      {
        eastl::fixed_string<char, 16> name(eastl::fixed_string<char, 16>::CtorSprintf{}, "gbuf_%d_done", i);
        auto gbufRtHndl = registry.create(name.c_str(), dafg::History::No)
                            .texture({gbufFmts[i] | global_flags | TEXCF_RTARGET, gbufResolution})
                            .useAs(dafg::Usage::COLOR_ATTACHMENT)
                            .atStage(dafg::Stage::PS);
        if (i == 2 && has_motion_vectors)
          eastl::move(gbufRtHndl).clear(make_clear_value(0, 0, 0, 0));
        if (i == 3)
          eastl::move(gbufRtHndl).clear(make_clear_value(MOTION_MASK.x, MOTION_MASK.y, MOTION_MASK.z, 0.0f));
      }

      registry.requestState().setFrameBlock("global_frame");

      registry.requestRenderPass()
        .depthRw("gbuf_depth_done")
        .color({"gbuf_0_done", "gbuf_1_done", registry.modify("gbuf_2_done").texture().optional(),
          registry.modify("gbuf_3_done").texture().optional()});

      const auto cameraHandle = registry.readBlob<CameraParams>("current_camera").handle();
      use_jitter_frustum_plane_shader_vars(registry);
      return [cameraHandle, gbufResolution, screenPosToTexcoordVard = get_shader_variable_id("screen_pos_to_texcoord"),
               gbufferViewSizeVarId = get_shader_variable_id("gbuffer_view_size")]() {
        const auto &camera = cameraHandle.ref();

        set_viewvecs_to_shader(camera.viewTm, camera.jitterProjTm);

        auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
        if (wr.clipmap)
          wr.clipmap->resetUAVAtomicPrefix(); // Reset is required before each multiplex pass.

        debug_mesh::activate_mesh_coloring_master_override();

        const auto resolution = gbufResolution.get();
        ShaderGlobal::set_color4(screenPosToTexcoordVard, 1.f / resolution.x, 1.f / resolution.y, 0, 0);
        ShaderGlobal::set_int4(gbufferViewSizeVarId, resolution.x, resolution.y, 0, 0);
      };
    });
}

dafg::NodeHandle makeReactiveMaskClearNode()
{
  return dafg::register_node("reactive_mask_clear_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.createTexture2d("reactive_mask", dafg::History::No, {TEXFMT_R8 | TEXCF_RTARGET, registry.getResolution<2>("main_view")})
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
  VAR(has_any_dynamic_lights, true)

#define VAR(a, o) static ShaderVariableInfo a##VarId(#a, o);
RESOLVE_GBUFFER_SHADERVARS
#undef VAR

// TODO Restore compute shader version, or delete the shader as a whole
// CONSOLE_BOOL_VAL("render", deferred_light_on_compute, false);
// deferredLightCompute = Ptr(new_compute_shader("deferred_light_compute"))
// And instead of render target just set current_specular and current_ambient as @uav shadervars
eastl::array<dafg::NodeHandle, 2> makeDeferredLightNode(bool reprojectGI)
{
  const uint32_t gi_fmt = reprojectGI && d3d::check_texformat(TEXFMT_R9G9B9E5 | TEXCF_RTARGET) ? TEXFMT_R9G9B9E5 : TEXFMT_R11G11B10F;
  const auto giHistory = reprojectGI ? dafg::History::ClearZeroOnFirstFrame : dafg::History::No;

  auto perCameraResNode = dafg::register_node("deferred_light_per_cam_res_node", DAFG_PP_NODE_SRC,
    [reprojectGI, gi_fmt, giHistory](dafg::Registry registry) {
      registry.create("current_specular", giHistory)
        .texture({gi_fmt | TEXCF_RTARGET, registry.getResolution<2>("main_view"), 1})
        .useAs(dafg::Usage::COLOR_ATTACHMENT)
        .atStage(dafg::Stage::PS)
        .clear(make_clear_value(0.f, 0.f, 0.f, 0.f));

      registry.create("current_ambient", giHistory)
        .texture({gi_fmt | TEXCF_RTARGET, registry.getResolution<2>("main_view"), 1})
        .useAs(dafg::Usage::COLOR_ATTACHMENT)
        .atStage(dafg::Stage::PS)
        .clear(make_clear_value(0.f, 0.f, 0.f, 0.f));

      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      registry.create("current_ambient_sampler", dafg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));

      if (reprojectGI)
      {
        registry.create("current_gi_pixel_age", dafg::History::ClearZeroOnFirstFrame)
          .texture({TEXFMT_R8 | TEXCF_RTARGET, registry.getResolution<2>("main_view"), 1})
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
      auto renderPass = registry.requestRenderPass().color({"current_specular", "current_ambient", "current_gi_pixel_age"});
      depthBoundsSet(renderPass);
    }
    else
    {
      auto renderPass = registry.requestRenderPass().color({"current_specular", "current_ambient"});
      depthBoundsSet(renderPass);
    }


    registry.read("gi_before_frame_lit_token").blob<OrderingToken>().optional();

    //  We apply indoor probes when resolving the gbuffer. We also use
    // them in most transparent stuff, but obviously transparent stuff
    // goes after the resolve, so no need for orderings there.
    (registry.root() / "indoor_probes").read("probes_ready_token").blob<OrderingToken>().optional();

    registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    auto cameraHndl = use_camera_in_camera(registry);
    auto prevCameraHndl = read_history_camera_in_camera(registry).handle();

    return [reproject = reprojectGI, deferredLight = PostFxRenderer("deferredLight"), cameraHndl, prevCameraHndl](
             const dafg::multiplexing::Index &multiplexing_index) {
      camera_in_camera::ApplyPostfxState camcam{
        multiplexing_index, cameraHndl.ref(), prevCameraHndl.ref(), camera_in_camera::USE_STENCIL};

      if (camera_in_camera::is_main_view(multiplexing_index))
        set_viewvecs_to_shader(cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm);

      float farPlaneDepth = far_plane_depth(get_gbuffer_depth_format());
      api_set_depth_bounds(farPlaneDepth, 1);
      const uint32_t histFrames = reproject ? static_cast<WorldRenderer *>(get_world_renderer())->getGIHistoryFrames() : 0;
      ShaderGlobal::set_int(reproject_screen_giVarId, histFrames ? 1 : 0);
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
    registry.readBlob<d3d::SamplerHandle>("current_ambient_sampler").bindToShaderVar("current_ambient_samplerstate");
  }
  registry.readTexture("ssr_target").atStage(dafg::Stage::PS).bindToShaderVar("ssr_target").optional();
  registry.read("ssr_target_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("ssr_target_samplerstate").optional();

  use_volfog(registry, dafg::Stage::PS_OR_CS);
  registry.readTexture("ssao_tex").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("ssao_tex").optional();
  registry.read("ssao_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("ssao_tex_samplerstate").optional();
  registry.readBlob<OrderingToken>("rtao_token").optional();
  registry.readBlob<OrderingToken>("ptgi_token").optional();
  // for SSAO&SSR
  registry.read("upscale_sampling_tex").texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("upscale_sampling_tex").optional();
}

static dafg::NodeHandle makeFullResolveGbufferNode(const char *resolve_pshader_name,
  const char *resolve_cshader_name,
  const char *classify_cshader_name,
  const ShadingResolver::PermutationsDesc &permutations_desc)
{
  return dafg::register_node("resolve_gbuffer_node", DAFG_PP_NODE_SRC,
    [resolve_pshader_name, resolve_cshader_name, classify_cshader_name, permutations_desc](dafg::Registry registry) {
      registry.read("gi_before_frame_lit_token").blob<OrderingToken>().optional();

      auto hasAnyDynamicLightsHndl = registry.readBlob<bool>("has_any_dynamic_lights").handle();
      auto rtDynamicLightHndl = registry.readTexture("dynamic_lighting_texture")
                                  .atStage(dafg::Stage::PS_OR_CS)
                                  .bindToShaderVar("dynamic_lighting_texture")
                                  .optional()
                                  .handle();

      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      auto gbufDepthHndl = registry.read("gbuf_depth").texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("depth_gbuf").handle();
      bindResolvePassResources(registry);

      auto finalTargetHndl = registry.modifyTexture("opaque_resolved")
                               .atStage(wr.getBareMinimumPreset() ? dafg::Stage::POST_RASTER : dafg::Stage::PS_OR_CS)
                               .useAs(wr.getBareMinimumPreset() ? dafg::Usage::COLOR_ATTACHMENT : dafg::Usage::SHADER_RESOURCE)
                               .handle();
      registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");

      registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
      auto cameraHndl = use_camera_in_camera(registry);

      registry.requestState().setFrameBlock("global_frame");
      auto waterModeHndl = registry.readBlob<WaterRenderMode>("water_render_mode").handle();

      shaders::OverrideState state{};
      state.set(shaders::OverrideState::Z_BOUNDS_ENABLED);

      return [enabledDepthBoundsId = shaders::overrides::create(state),
               shadingResolver = eastl::make_unique<ShadingResolver>(resolve_pshader_name, resolve_cshader_name, classify_cshader_name,
                 permutations_desc),
               gbufDepthHndl, finalTargetHndl, hasAnyDynamicLightsHndl, waterModeHndl, rtDynamicLightHndl,
               cameraHndl](const dafg::multiplexing::Index &multiplexing_index) {
        camera_in_camera::ApplyPostfxState camcam{multiplexing_index, cameraHndl.ref()};

        ShadowsManager &shadowsManager = WRDispatcher::getShadowsManager();
        const CameraParams &camera = cameraHndl.ref();

        shadowsManager.setShadowFrameIndex(camera);

        int dynamicLightsCount = ShaderGlobal::get_int(dynamic_lights_countVarId);
        if (!rtDynamicLightHndl.get())
        {
          ClusteredLights &lights = WRDispatcher::getClusteredLights();
          if (lights.hasClusteredLights() && hasAnyDynamicLightsHndl.ref())
            lights.setBuffersToShader();
        }
        else
        {
          // Disable classification, and do a runtime check in shader if rtDynamicLightHndl contains valid data
          // We skip rendering of that texture, if there are no lights in frame
          ShaderGlobal::set_int(has_any_dynamic_lightsVarId, hasAnyDynamicLightsHndl.ref());
          ShaderGlobal::set_int(dynamic_lights_countVarId, eastl::to_underlying(DynLightsOptimizationMode::NO_LIGHTS));
        }

        bool useDepthBounds = ::depth_bounds_enabled();
        if (useDepthBounds)
        {
          float farPlaneDepth = far_plane_depth(get_gbuffer_depth_format());
          shaders::overrides::set(enabledDepthBoundsId);
          api_set_depth_bounds(farPlaneDepth, 1);
        }

        ManagedTexView finalTarget = finalTargetHndl.view();
        ManagedTexView gbufDepth = gbufDepthHndl.view();

        TIME_D3D_PROFILE(resolve);
        const ShadingResolver::ClearTarget needTargetClear =
          camera_in_camera::is_main_view(multiplexing_index) &&
              (has_custom_sky() || waterModeHndl.ref() == WaterRenderMode::EARLY_BEFORE_ENVI)
            ? ShadingResolver::ClearTarget::Yes
            : ShadingResolver::ClearTarget::No;
        shadingResolver->resolve(finalTarget.getTex2D(), camera.viewTm, camera.jitterProjTm, gbufDepth.getTex2D(), needTargetClear);

        if (useDepthBounds)
          shaders::overrides::reset();

        if (rtDynamicLightHndl.get())
          ShaderGlobal::set_int(dynamic_lights_countVarId, dynamicLightsCount);
      };
    });
}

static dafg::NodeHandle makeThinResolveGbufferNode(const char *resolve_pshader_name,
  const char *resolve_cshader_name,
  const char *classify_cshader_name,
  const ShadingResolver::PermutationsDesc &permutations_desc)
{
  return dafg::register_node("resolve_gbuffer_node", DAFG_PP_NODE_SRC,
    [resolve_pshader_name, resolve_cshader_name, classify_cshader_name, permutations_desc](dafg::Registry registry) {
      registry.read("gi_before_frame_lit_token").blob<OrderingToken>().optional();

      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      auto gbufDepthHndl = registry.read("gbuf_depth").texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("depth_gbuf").handle();
      bindResolvePassResources(registry);

      auto finalTargetHndl = registry.modifyTexture("opaque_resolved")
                               .atStage(wr.getBareMinimumPreset() ? dafg::Stage::POST_RASTER : dafg::Stage::PS_OR_CS)
                               .useAs(wr.getBareMinimumPreset() ? dafg::Usage::COLOR_ATTACHMENT : dafg::Usage::SHADER_RESOURCE)
                               .handle();
      registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
      auto cameraHndl = use_current_camera(registry);
      registry.requestState().setFrameBlock("global_frame");
      auto waterModeHndl = registry.readBlob<WaterRenderMode>("water_render_mode").handle();

      shaders::OverrideState state{};
      state.set(shaders::OverrideState::Z_BOUNDS_ENABLED);

      return [enabledDepthBoundsId = shaders::overrides::create(state),
               shadingResolver = eastl::make_unique<ShadingResolver>(resolve_pshader_name, resolve_cshader_name, classify_cshader_name,
                 permutations_desc),
               gbufDepthHndl, finalTargetHndl, waterModeHndl, cameraHndl]() {
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

        ManagedTexView finalTarget = finalTargetHndl.view();
        ManagedTexView gbufDepth = gbufDepthHndl.view();

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
            shadingResolver->resolve(finalTarget.getTex2D(), camera.viewTm, camera.jitterProjTm, gbufDepth.getTex2D(),
              needTargetClear);
            needTargetClear = ShadingResolver::ClearTarget::No;
          }
          api_set_depth_bounds(csm_distancRawD, scatteringStartRawD);
          ShaderGlobal::set_int(thin_gbuf_resolveVarId, ALL_SHADOWS_SCATTERING);
          shadingResolver->resolve(finalTarget.getTex2D(), camera.viewTm, camera.jitterProjTm, gbufDepth.getTex2D(), needTargetClear);
          needTargetClear = ShadingResolver::ClearTarget::No;
        }
        else
        {
          if (csm_distancRawD < 1.0f)
          {
            api_set_depth_bounds(csm_distancRawD, 1);
            ShaderGlobal::set_int(thin_gbuf_resolveVarId, SHADOWS_NO_SCATTERING);
            shadingResolver->resolve(finalTarget.getTex2D(), camera.viewTm, camera.jitterProjTm, gbufDepth.getTex2D(),
              needTargetClear);
            needTargetClear = ShadingResolver::ClearTarget::No;
          }
          api_set_depth_bounds(scatteringStartRawD, csm_distancRawD);
          ShaderGlobal::set_int(thin_gbuf_resolveVarId, STATIC_SHADOWS_NO_SCATTERING);
          shadingResolver->resolve(finalTarget.getTex2D(), camera.viewTm, camera.jitterProjTm, gbufDepth.getTex2D(), needTargetClear);
          needTargetClear = ShadingResolver::ClearTarget::No;
        }

        float farPlaneDepth = far_plane_depth(get_gbuffer_depth_format());
        api_set_depth_bounds(farPlaneDepth, scatteringStart < csmEnd ? csm_distancRawD : scatteringStartRawD);
        ShaderGlobal::set_int(thin_gbuf_resolveVarId, STATIC_SHADOWS_SCATTERING);
        shadingResolver->resolve(finalTarget.getTex2D(), camera.viewTm, camera.jitterProjTm, gbufDepth.getTex2D(), needTargetClear);
        api_set_depth_bounds(farPlaneDepth, 1); // restore default
        ShaderGlobal::set_int(thin_gbuf_resolveVarId, NO_DEPTH_BOUNDS);

        if (useDepthBounds)
          shaders::overrides::reset();
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

      ManagedTexView finalTarget = finalTargetHndl.view();
      ManagedTexView gbufDepth = gbufDepthHndl.view();

      const bool useDepthBounds = ::depth_bounds_enabled();
      if (useDepthBounds)
      {
        float farPlaneDepth = far_plane_depth(get_gbuffer_depth_format());
        shaders::overrides::set(enabledDepthBoundsId);
        api_set_depth_bounds(farPlaneDepth, 1);
      }
      {
        SCOPE_RENDER_TARGET;
        d3d::set_render_target(finalTarget.getTex2D(), 0);
        d3d::set_depth(gbufDepth.getTex2D(), DepthAccess::SampledRO);
        lights.renderOtherLights();
      }
      if (useDepthBounds)
        shaders::overrides::reset();
    };
  });
}

eastl::fixed_vector<dafg::NodeHandle, 2, false> makeResolveGbufferNodes(const char *resolve_pshader_name,
  const char *resolve_cshader_name,
  const char *classify_cshader_name,
  const ShadingResolver::PermutationsDesc &permutations_desc)
{
  const bool isFullDeferred = renderer_has_feature(FULL_DEFERRED);
  const bool useDepthBounds = ::depth_bounds_enabled();
  const bool useFullResolve = (isFullDeferred || !useDepthBounds);

  eastl::fixed_vector<dafg::NodeHandle, 2, false> result;
  result.push_back(
    useFullResolve ? makeFullResolveGbufferNode(resolve_pshader_name, resolve_cshader_name, classify_cshader_name, permutations_desc)
                   : makeThinResolveGbufferNode(resolve_pshader_name, resolve_cshader_name, classify_cshader_name, permutations_desc));
  if (isFullDeferred)
    result.push_back(makeRenderOtherLightsNode());
  return result;
}

dafg::NodeHandle makeRenameDepthNode()
{
  return dafg::register_node("rename_depth", DAFG_PP_NODE_SRC,
    [](dafg::Registry registry) { registry.renameTexture("gbuf_depth", "gbuf_depth_after_resolve", dafg::History::No); });
}
