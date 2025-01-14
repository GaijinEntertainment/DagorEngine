// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_postFxRenderer.h>
#include <render/viewVecs.h>
#include <render/debugMesh.h>
#include <render/deferredRenderer.h>
#include <render/renderEvent.h>
#include <render/world/worldRendererQueries.h>
#include <render/world/gbufferConsts.h>
#include <render/set_reprojection.h>
#include <perfMon/dag_statDrv.h>
#include <frustumCulling/frustumPlanes.h>
#include <util/dag_convar.h>
#include <render/subFrameSample.h>
#include <EASTL/fixed_string.h>

#include <daECS/core/entityManager.h>

#define INSIDE_RENDERER 1
#include "../private_worldRenderer.h"
#include "frameGraphNodes.h"
#include <render/world/frameGraphHelpers.h>
#include <drv/3d/dag_renderTarget.h>
#include <render/antiAliasing.h>
#include <render/world/wrDispatcher.h>
#include <render/world/depthBounds.h>

CONSOLE_FLOAT_VAL_MINMAX("render", thin_scattering_start, 32.0f, 0.f, 1024);
extern ConVarT<bool, false> volfog_enabled;
extern ConVarT<bool, false> dynamic_lights;
extern ConVarT<int, true> gi_quality;

#ifdef __clang__
constexpr
#endif
  float INF = eastl::numeric_limits<float>::infinity();

dabfg::NodeHandle makePrepareGbufferNode(
  uint32_t global_flags, uint32_t gbuf_cnt, eastl::span<uint32_t> main_gbuf_fmts, int depth_format, bool has_motion_vectors)
{
  auto ns = dabfg::root() / "init";
  return ns.registerNode("prepare_gbuffer_node", DABFG_PP_NODE_SRC,
    [global_flags, gbuf_cnt, depth_format, has_motion_vectors,
      gbufFmts = dag::RelocatableFixedVector<uint32_t, FULL_GBUFFER_RT_COUNT, false>(main_gbuf_fmts)](dabfg::Registry registry) {
      const auto gbufResolution = registry.getResolution<2>("main_view");

      // Weird names are used here for simpler integration with "opaque/closeups"
      auto gbufHandles = dag::Vector<dabfg::VirtualResourceHandle<BaseTexture, true, false>>();
      auto gbufDepthHandle = registry.create("gbuf_depth_done", dabfg::History::No)
                               .texture({depth_format | global_flags | TEXCF_RTARGET, gbufResolution})
                               .atStage(dabfg::Stage::PS)
                               .useAs(dabfg::Usage::DEPTH_ATTACHMENT)
                               .handle();

      eastl::optional<dabfg::VirtualResourceHandle<BaseTexture, true, false>> materialHndl{};
      for (uint32_t i = 0; i < gbuf_cnt; ++i)
      {
        eastl::fixed_string<char, 16> name(eastl::fixed_string<char, 16>::CtorSprintf{}, "gbuf_%d_done", i);
        gbufHandles.push_back(registry.create(name.c_str(), dabfg::History::No)
                                .texture({gbufFmts[i] | global_flags | TEXCF_RTARGET, gbufResolution})
                                .useAs(dabfg::Usage::COLOR_ATTACHMENT)
                                .atStage(dabfg::Stage::PS)
                                .handle());
      }

      registry.requestState().setFrameBlock("global_frame");

      auto pass = registry.requestRenderPass()
                    .depthRw("gbuf_depth_done")
                    .color({"gbuf_0_done", "gbuf_1_done", registry.modify("gbuf_2_done").texture().optional(),
                      registry.modify("gbuf_3_done").texture().optional()});

      // PS4 uses uncompressed color targets so no clear is required (it takes 220us)
      // vulkan on immediate render GPUs will do clear write then rendering write,
      // instead of simply rendering write when clear is off, effectively wasting VRAM bandwith
      // any TBDR device always benefit from clear as there will be load from VRAM
      if (!d3d::get_driver_code().is(d3d::ps4 || d3d::vulkan) || d3d::get_driver_desc().caps.hasTileBasedArchitecture)
      {
        // TODO: clear motion vecs with INF here instead of through a shader.
        // This requires fixing generic RP implementation to support 32-bit clears.
        pass = eastl::move(pass)
                 .clear("gbuf_0_done", make_clear_value(0.f, 0.f, 0.f, 0.f))
                 .clear("gbuf_1_done", make_clear_value(0.f, 0.f, 0.f, 0.f))
                 .clear("gbuf_2_done", make_clear_value(0.f, 0.f, 0.f, 0.f))
                 .clear("gbuf_3_done", make_clear_value(INF, INF, INF, INF))
                 .clear("gbuf_depth_done", make_clear_value(0.f, 0));
      }
      else if (has_motion_vectors)
      {
        // we need to clear material_gbuf anyway because of dynamic bits
        pass = eastl::move(pass)
                 .clear("gbuf_2_done", make_clear_value(0.f, 0.f, 0.f, 0.f))
                 .clear("gbuf_3_done", make_clear_value(INF, INF, INF, INF))
                 .clear("gbuf_depth_done", make_clear_value(0.f, 0));
      }
      else
        pass = eastl::move(pass).clear("gbuf_depth_done", make_clear_value(0.f, 0));

      const auto cameraHandle = registry.readBlob<CameraParams>("current_camera").handle();
      use_jitter_frustum_plane_shader_vars(registry);
      return [gbufDepthHandle, cameraHandle, gbufResolution, gbufHandles = eastl::move(gbufHandles),
               screenPosToTexcoordVard = get_shader_variable_id("screen_pos_to_texcoord"),
               gbufferViewSizeVarId = get_shader_variable_id("gbuffer_view_size")]() {
        for (size_t i = 0; i < gbufHandles.size(); ++i)
        {
          auto gBufView = gbufHandles[i].view();
          gBufView->disableSampler();
        }
        auto depthView = gbufDepthHandle.view();
        depthView->disableSampler();
        const auto &camera = cameraHandle.ref();

        set_viewvecs_to_shader(camera.viewTm, camera.jitterProjTm);

#if DAGOR_DBGLEVEL > 0
        debug_mesh::activate_mesh_coloring_master_override();
#endif

        const auto resolution = gbufResolution.get();
        ShaderGlobal::set_color4(screenPosToTexcoordVard, 1.f / resolution.x, 1.f / resolution.y, 0, 0);
        ShaderGlobal::set_int4(gbufferViewSizeVarId, resolution.x, resolution.y, 0, 0);
      };
    });
}

dabfg::NodeHandle makeResolveMotionVectorsNode()
{
  return dabfg::register_node("resolve_motion_vectors_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.requestRenderPass().color({registry.renameTexture("gbuf_3", "motion_vecs", dabfg::History::No)});
    read_gbuffer(registry);
    read_gbuffer_depth(registry);
    auto cameraHndl = registry.read("current_camera").blob<CameraParams>().handle();
    auto prevCameraHndl = registry.historyFor("current_camera").blob<CameraParams>().handle();
    auto viewVecsHndl = registry.read("view_vectors").blob<ViewVecs>().handle();
    auto prevViewVecsHndl = registry.historyFor("view_vectors").blob<ViewVecs>().handle();
    auto rotProjTmHndl = registry.read("globtm_no_ofs").blob<TMatrix4>().handle();
    auto prevRotProjTmHndl = registry.historyFor("globtm_no_ofs").blob<TMatrix4>().handle();

    auto subFrameSampleHndl = registry.read("sub_frame_sample").blob<SubFrameSample>().handle();
    return [motionVectorsResolve = PostFxRenderer("motion_vectors_resolve"), subFrameSampleHndl, cameraHndl, prevCameraHndl,
             viewVecsHndl, prevViewVecsHndl, rotProjTmHndl, prevRotProjTmHndl]() {
      const auto &camera = cameraHndl.ref();
      const auto &prevCamera = prevCameraHndl.ref();
      const auto &viewVec = viewVecsHndl.ref();
      const auto &prevViewVec = prevViewVecsHndl.ref();
      const auto &rotProjTm = rotProjTmHndl.ref();
      const auto &prevRotProjTm = prevRotProjTmHndl.ref();
      SubFrameSample subFrameSample = subFrameSampleHndl.ref();
      if (subFrameSample == SubFrameSample::Single || subFrameSample == SubFrameSample::Last)
      {
        set_reprojection(camera.jitterProjTm, rotProjTm, Point2(prevCamera.znear, prevCamera.zfar), prevRotProjTm, viewVec.viewVecLT,
          viewVec.viewVecRT, viewVec.viewVecLB, viewVec.viewVecRB, prevViewVec.viewVecLT, prevViewVec.viewVecRT, prevViewVec.viewVecLB,
          prevViewVec.viewVecRB, camera.cameraWorldPos, prevCamera.cameraWorldPos);
      }
      motionVectorsResolve.render();
    };
  });
}

dabfg::NodeHandle makeReactiveMaskClearNode()
{
  return dabfg::register_node("reactive_mask_clear_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    auto reactiveMaskHndl =
      registry
        .createTexture2d("reactive_mask", dabfg::History::No, {TEXFMT_R8 | TEXCF_RTARGET, registry.getResolution<2>("main_view")})
        .atStage(dabfg::Stage::PS)
        .useAs(dabfg::Usage::COLOR_ATTACHMENT)
        .handle();
    return [reactiveMaskHndl]() { d3d::clear_rt({reactiveMaskHndl.view().getTex2D()}, make_clear_value(0.f, 0)); };
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
  VAR(use_rtr, true)

#define VAR(a, o) static ShaderVariableInfo a##VarId(#a, o);
RESOLVE_GBUFFER_SHADERVARS
#undef VAR

// TODO Restore compute shader version, or delete the shader as a whole
// CONSOLE_BOOL_VAL("render", deferred_light_on_compute, false);
// deferredLightCompute = Ptr(new_compute_shader("deferred_light_compute"))
// And instead of render target just set current_specular and current_ambient as @uav shadervars
dabfg::NodeHandle makeDeferredLightNode(bool reprojectGI)
{
  return dabfg::register_node("deferred_light_node", DABFG_PP_NODE_SRC, [reprojectGI](dabfg::Registry registry) {
    auto giHistory = reprojectGI ? dabfg::History::ClearZeroOnFirstFrame : dabfg::History::No;
    const uint32_t gi_fmt = reprojectGI && d3d::check_texformat(TEXFMT_R9G9B9E5 | TEXCF_RTARGET) ? TEXFMT_R9G9B9E5 : TEXFMT_R11G11B10F;

    auto currentSpecularHndl =
      registry.create("current_specular", giHistory).texture({gi_fmt | TEXCF_RTARGET, registry.getResolution<2>("main_view"), 1});
    auto currentAmbientHndl =
      registry.create("current_ambient", giHistory).texture({gi_fmt | TEXCF_RTARGET, registry.getResolution<2>("main_view"), 1});

    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    registry.create("current_ambient_sampler", dabfg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));

    auto gbufDepthHndl = registry.read("gbuf_depth").texture();
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("depth_gbuf_samplerstate");
    registry.read("gbuf_1").texture().atStage(dabfg::Stage::PS).bindToShaderVar("normal_gbuf");
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("normal_gbuf_samplerstate");
    registry.read("gbuf_2").texture().atStage(dabfg::Stage::PS).bindToShaderVar("material_gbuf");
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("material_gbuf_samplerstate");
    registry.read("motion_vecs").texture().atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar("motion_gbuf").optional();
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("motion_gbuf_samplerstate").optional();

    auto stateRequest = registry.requestState().setFrameBlock("global_frame");

    auto depthBoundsSet = [&](dabfg::VirtualPassRequest &renderPass) {
      if (depth_bounds_enabled())
      {
        shaders::OverrideState state;
        state.set(shaders::OverrideState::Z_BOUNDS_ENABLED);
        eastl::move(stateRequest).enableOverride(state);
        eastl::move(renderPass).depthRoAndBindToShaderVars(gbufDepthHndl, {"depth_gbuf"});
      }
      else
        eastl::move(gbufDepthHndl).atStage(dabfg::Stage::PS).bindToShaderVar("depth_gbuf");
    };

    if (reprojectGI)
    {
      registry.historyFor("depth_for_postfx").texture().atStage(dabfg::Stage::PS).bindToShaderVar("prev_gbuf_depth");
      auto currentAgeHndl = registry.create("current_gi_pixel_age", dabfg::History::ClearZeroOnFirstFrame)
                              .texture({TEXFMT_R8UI | TEXCF_RTARGET, registry.getResolution<2>("main_view"), 1});
      registry.historyFor("current_gi_pixel_age").texture().atStage(dabfg::Stage::PS).bindToShaderVar("prev_gi_pixel_age");
      registry.historyFor("current_specular").texture().atStage(dabfg::Stage::PS).bindToShaderVar("prev_specular");
      registry.historyFor("current_ambient").texture().atStage(dabfg::Stage::PS).bindToShaderVar("prev_ambient");
      auto renderPass = registry.requestRenderPass().color({currentSpecularHndl, currentAmbientHndl, currentAgeHndl});
      depthBoundsSet(renderPass);
    }
    else
    {
      auto renderPass = registry.requestRenderPass().color({currentSpecularHndl, currentAmbientHndl});
      depthBoundsSet(renderPass);
    }
    ShaderGlobal::set_int(reproject_screen_giVarId, reprojectGI ? 1 : 0);


    registry.read("gi_before_frame_lit_token").blob<OrderingToken>().optional();

    //  We apply indoor probes when resolving the gbuffer. We also use
    // them in most transparent stuff, but obviously transparent stuff
    // goes after the resolve, so no need for orderings there.
    (registry.root() / "indoor_probes").read("probes_ready_token").blob<OrderingToken>().optional();

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    use_current_camera(registry);

    return [deferredLight = PostFxRenderer("deferredLight"), cameraHndl]() {
      set_viewvecs_to_shader(cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm);
      float farPlaneDepth = far_plane_depth(get_gbuffer_depth_format());
      api_set_depth_bounds(farPlaneDepth, 1);
      deferredLight.render();
    };
  });
}

dabfg::NodeHandle makeResolveGbufferNode(const char *resolve_pshader_name,
  const char *resolve_cshader_name,
  const char *classify_cshader_name,
  const ShadingResolver::PermutationsDesc &permutations_desc)
{
  return dabfg::register_node("resolve_gbuffer_node", DABFG_PP_NODE_SRC,
    [resolve_pshader_name, resolve_cshader_name, classify_cshader_name, permutations_desc](dabfg::Registry registry) {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      registry.orderMeAfter("combine_shadows_node");
      auto rtrTokenHndl = registry.readBlob<OrderingToken>("rtr_token").optional().handle();
      registry.read("gi_before_frame_lit_token").blob<OrderingToken>().optional();

      auto gbufDepthHndl =
        registry.read("gbuf_depth").texture().atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar("depth_gbuf").handle();
      registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("depth_gbuf_samplerstate");
      registry.read("gbuf_0").texture().atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar("albedo_gbuf");
      registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("albedo_gbuf_samplerstate");
      registry.read("gbuf_1").texture().atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar("normal_gbuf");
      registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("normal_gbuf_samplerstate");
      registry.read("gbuf_2").texture().atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar("material_gbuf").optional();
      registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("material_gbuf_samplerstate").optional();
      registry.read("motion_vecs").texture().atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar("motion_gbuf").optional();
      registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("motion_gbuf_samplerstate").optional();
      registry.readTexture("close_depth").atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar("downsampled_close_depth_tex");
      registry.read("close_depth_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("downsampled_close_depth_tex_samplerstate");
      registry.readTexture("far_downsampled_depth").atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar("downsampled_far_depth_tex");
      registry.read("far_downsampled_depth_sampler")
        .blob<d3d::SamplerHandle>()
        .bindToShaderVar("downsampled_far_depth_tex_samplerstate");
      auto finalTargetHndl = registry.modifyTexture("opaque_resolved")
                               .atStage(wr.bareMinimumPreset ? dabfg::Stage::POST_RASTER : dabfg::Stage::PS_OR_CS)
                               .useAs(wr.bareMinimumPreset ? dabfg::Usage::COLOR_ATTACHMENT : dabfg::Usage::SHADER_RESOURCE)
                               .handle();
      if (wr.hasFeature(FeatureRenderFlags::COMBINED_SHADOWS))
      {
        registry.readTexture("combined_shadows").atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar("combined_shadows");
        registry.read("combined_shadows_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("combined_shadows_samplerstate");
      }

      if (wr.hasFeature(FeatureRenderFlags::DEFERRED_LIGHT))
      {
        registry.readTexture("current_specular").atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar("specular_tex");
        registry.readTexture("current_ambient").atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar("current_ambient");
        registry.readBlob<d3d::SamplerHandle>("current_ambient_sampler").bindToShaderVar("current_ambient_samplerstate");
      }
      registry.readTexture("ssr_target").atStage(dabfg::Stage::PS).bindToShaderVar("ssr_target").optional();
      registry.read("ssr_target_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("ssr_target_samplerstate").optional();

      use_volfog(registry, dabfg::Stage::PS_OR_CS);
      registry.readTexture("ssao_tex").atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar("ssao_tex").optional();
      registry.read("ssao_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("ssao_tex_samplerstate").optional();
      registry.readBlob<OrderingToken>("rtao_token").optional();
      // for SSAO&SSR
      registry.read("upscale_sampling_tex")
        .texture()
        .atStage(dabfg::Stage::PS_OR_CS)
        .bindToShaderVar("upscale_sampling_tex")
        .optional();

      auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
      use_current_camera(registry);
      registry.requestState().setFrameBlock("global_frame");
      auto hasAnyDynamicLightsHndl = registry.readBlob<bool>("has_any_dynamic_lights").handle();
      auto waterModeHndl = registry.readBlob<WaterRenderMode>("water_render_mode").handle();

      shaders::OverrideState state{};
      state.set(shaders::OverrideState::Z_BOUNDS_ENABLED);

      return [enabledDepthBoundsId = shaders::overrides::create(state),
               shadingResolver = eastl::make_unique<ShadingResolver>(resolve_pshader_name, resolve_cshader_name, classify_cshader_name,
                 permutations_desc),
               gbufDepthHndl, finalTargetHndl, cameraHndl, hasAnyDynamicLightsHndl, waterModeHndl, rtrTokenHndl]() {
        // Pointer is always non-zero because world renderer contains this node
        auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

        ShaderGlobal::set_int(use_rtrVarId, !!rtrTokenHndl.get());

        ManagedTexView finalTarget = finalTargetHndl.view();
        ManagedTexView gbufDepth = gbufDepthHndl.view();
        const auto &camera = cameraHndl.ref();

        WRDispatcher::getShadowsManager().setShadowFrameIndex();
        if (wr.lights.hasClusteredLights() && hasAnyDynamicLightsHndl.ref())
          wr.lights.setBuffersToShader();

        const bool useDepthBounds = ::depth_bounds_enabled();
        float farPlaneDepth = far_plane_depth(get_gbuffer_depth_format());
        if (useDepthBounds)
        {
          shaders::overrides::set(enabledDepthBoundsId);
          api_set_depth_bounds(farPlaneDepth, 1);
        }
        {
          TIME_D3D_PROFILE(resolve);

          ShadingResolver::ClearTarget needTargetClear =
            has_custom_sky_render(CustomSkyRequest::CHECK_ONLY) || waterModeHndl.ref() == WaterRenderMode::EARLY_BEFORE_ENVI
              ? ShadingResolver::ClearTarget::Yes
              : ShadingResolver::ClearTarget::No;

          if (wr.hasFeature(FULL_DEFERRED) || !useDepthBounds)
          {
            shadingResolver->resolve(finalTarget.getTex2D(), camera.viewTm, camera.jitterProjTm, gbufDepth.getTex2D(),
              needTargetClear);
          }
          else
          {
            enum ThinGbufResolve
            {
              NO_DEPTH_BOUNDS = 0,
              SHADOWS_NO_SCATTERING = 1,
              ALL_SHADOWS_SCATTERING = 2,
              STATIC_SHADOWS_NO_SCATTERING = 3,
              STATIC_SHADOWS_SCATTERING = 4
            };

            CascadeShadows *csm = wr.shadowsManager.getCascadeShadows();
            const bool hasVolumeLight = wr.volumeLight && volfog_enabled.get();
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
              shadingResolver->resolve(finalTarget.getTex2D(), camera.viewTm, camera.jitterProjTm, gbufDepth.getTex2D(),
                needTargetClear);
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
              shadingResolver->resolve(finalTarget.getTex2D(), camera.viewTm, camera.jitterProjTm, gbufDepth.getTex2D(),
                needTargetClear);
              needTargetClear = ShadingResolver::ClearTarget::No;
            }
            api_set_depth_bounds(farPlaneDepth, scatteringStart < csmEnd ? csm_distancRawD : scatteringStartRawD);
            ShaderGlobal::set_int(thin_gbuf_resolveVarId, STATIC_SHADOWS_SCATTERING);
            shadingResolver->resolve(finalTarget.getTex2D(), camera.viewTm, camera.jitterProjTm, gbufDepth.getTex2D(),
              needTargetClear);
            api_set_depth_bounds(farPlaneDepth, 1); // restore default
            ShaderGlobal::set_int(thin_gbuf_resolveVarId, NO_DEPTH_BOUNDS);
          }
        }

        {
          SCOPE_RENDER_TARGET;
          d3d::set_render_target(finalTarget.getTex2D(), 0);
          d3d::set_depth(gbufDepth.getTex2D(), DepthAccess::SampledRO);
          if (wr.hasFeature(FULL_DEFERRED) && dynamic_lights.get() && wr.lights.hasDeferredLights())
            wr.lights.renderOtherLights();
        }
        if (useDepthBounds)
          shaders::overrides::reset();

        ShaderGlobal::set_texture(frame_texVarId, finalTarget);
      };
    });
}

dabfg::NodeHandle makeRenameDepthNode()
{
  return dabfg::register_node("rename_depth", DABFG_PP_NODE_SRC,
    [](dabfg::Registry registry) { registry.renameTexture("gbuf_depth", "gbuf_depth_after_resolve", dabfg::History::No); });
}
