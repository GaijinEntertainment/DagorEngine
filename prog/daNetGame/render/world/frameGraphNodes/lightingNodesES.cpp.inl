// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <ecs/render/renderPasses.h>
#include <frustumCulling/frustumPlanes.h>
#include <rendInst/gpuObjects.h>
#include <rendInst/rendInstGenRender.h>
#include <rendInst/visibility.h>
#include <render/daFrameGraph/daFG.h>
#include <render/viewVecs.h>
#include <render/volumetricLights/volumetricLights.h>

#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaderBlock.h>
#include <util/dag_convar.h>

#include <render/renderEvent.h>
#include <render/world/cameraInCamera.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/overridden_params.h>

#define INSIDE_RENDERER 1
#include "../private_worldRenderer.h"
#include <render/world/wrDispatcher.h>
#include "../dynamicShadowRenderExtender.h"
#include "frameGraphNodes.h"
#include <render/tiled_light_consts.hlsli>


extern ConVarT<bool, false> dynamic_lights;
extern ConVarT<bool, false> volfog_enabled;

CONSOLE_INT_VAL("render", volfog_force_invalidate, 0, 0, 2); // 0 - off, 1 - invalidate once, 2 - force

// defines bbox around camera that limit dynamic objects passed into dynamic light shadow updates
static constexpr float DEFAULT_LIGHTS_SHADOW_DYN_OBJECTS_UPDATE_RANGE = 5;
CONSOLE_FLOAT_VAL("render", lights_shadow_dyn_objects_update_range, DEFAULT_LIGHTS_SHADOW_DYN_OBJECTS_UPDATE_RANGE);

dafg::NodeHandle makePrepareLightsNode()
{
  return dafg::register_node("prepare_lights_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("downsample_depth_node");
    int dynamic_lights_countVarId = get_shader_variable_id("dynamic_lights_count");
    auto hasAnyDynamicLightsHndl = registry.createBlob<bool>("has_any_dynamic_lights").handle();
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();

    return [hasAnyDynamicLightsHndl, dynamic_lights_countVarId, cameraHndl]() {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      cameraHndl.ref().jobsMgr->waitLights();
      if (dynamic_lights.get() && (wr.lights.hasClusteredLights() || wr.lights.hasDeferredLights()))
        wr.lights.fillAndSetInsideOfFrustumLightsBuffers();

      const int spotsCount = wr.lights.getVisibleClusteredSpotsCount();
      const int omniCount = wr.lights.getVisibleClusteredOmniCount();

      DynLightsOptimizationMode dynLightsMode = (dynamic_lights.get() && wr.lights.hasClusteredLights())
                                                  ? get_lights_count_interval(spotsCount, omniCount)
                                                  : DynLightsOptimizationMode::NO_LIGHTS;

      ShaderGlobal::set_int(dynamic_lights_countVarId, eastl::to_underlying(dynLightsMode));
      hasAnyDynamicLightsHndl.ref() = dynLightsMode != DynLightsOptimizationMode::NO_LIGHTS;
    };
  });
}

// TODO: separate this into more fine-grained nodes.
eastl::array<dafg::NodeHandle, 2> makeSceneShadowPassNodes(const DataBlock *level_blk)
{
  lights_shadow_dyn_objects_update_range =
    lvl_override::getReal(level_blk, "lightsShadowDynObjectsUpdateRange", DEFAULT_LIGHTS_SHADOW_DYN_OBJECTS_UPDATE_RANGE);
  auto prepareNode = dafg::register_node("scene_shadow_prepare_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("prepare_lights_node");
    registry.requestState().setFrameBlock("global_frame");

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto volumesHndl = registry.createBlob<dynamic_shadow_render::VolumesVector>("scene_shadow_volumes_to_render").handle();
    auto updatesHndl = registry.createBlob<dynamic_shadow_render::FrameUpdates>("scene_shadow_updates").handle();

    return [cameraHndl, volumesHndl, updatesHndl] {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      // May change every frame, therefore no node recreation
      if (wr.canChangeAltitudeUnexpectedly)
      {
        // No need to update shadows, but we still need to update const buffer with shadow matrices
        // because lights are culled every frame and corresponding matrices may need different offset
        // in this const buffer.
        OSSpinlockScopedLock scopedLock{wr.lights.lightLock};
        wr.lights.updateShadowBuffers();
        return;
      }

      const auto &camera = cameraHndl.ref();
      auto &volumesToRender = volumesHndl.ref();
      auto &updates = updatesHndl.ref();

      vec4f vpos = v_ldu_p3(camera.viewItm[3]);
      vec4f dynRange = v_splats(lights_shadow_dyn_objects_update_range);
      bbox3f dynBox = {v_sub(vpos, dynRange), v_add(vpos, dynRange)};

      mat44f globTm;
      v_mat44_make_from_44cu(globTm, &camera.jitterGlobtm._11);

      wr.lights.framePrepareShadows(volumesToRender, camera.viewItm.getcol(3), (mat44f_cref)camera.jitterGlobtm, camera.jitterPersp.hk,
        make_span_const(&dynBox, 1), &updates);
    };
  });

  auto renderNode = dafg::register_node("scene_shadow_render_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeBefore("combine_shadows_node");
    registry.requestState().setFrameBlock("global_frame");
    registry.executionHas(dafg::SideEffects::External);

    registry.createBlob<OrderingToken>("dynamic_lights_shadow_buffers_ready_token");

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto volumesHndl = registry.readBlob<dynamic_shadow_render::VolumesVector>("scene_shadow_volumes_to_render").handle();

    auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
    if (wr->shadowRenderExtender)
      wr->shadowRenderExtender->declareAll(registry);

    return [wr, cameraHndl, volumesHndl, rendinstDepthSceneBlockId = ShaderGlobal::getBlockId("rendinst_depth_scene")] {
      const auto &camera = cameraHndl.ref();
      const auto &volumesToRender = volumesHndl.ref();

      wr->lights.frameRenderShadows(
        volumesToRender,
        [wr, rendinstDepthSceneBlockId](mat44f_cref globTm, mat44f_cref /*projTm*/, const TMatrix &viewItm, int updateIndex,
          int viewIndex, DynamicShadowRenderGPUObjects render_gpu_objects) {
          Point3 cameraPos = viewItm.getcol(3);
          {
            // Added culling here because renderRendinst no longer uses the rendinst::renderRIGen
            // function with built in visibility.
            // Also forShadow is false, because it's rendered with depth flag, not shadow.
            SCENE_LAYER_GUARD(rendinstDepthSceneBlockId);
            rendinst::prepareRIGenExtraVisibility(globTm, cameraPos, *wr->rendinst_dynamic_shadow_visibility, false, nullptr);
            rendinst::prepareRIGenVisibility(globTm, cameraPos, wr->rendinst_dynamic_shadow_visibility, false, nullptr);
          }
          if (render_gpu_objects == DynamicShadowRenderGPUObjects::NO)
            rendinst::gpuobjects::clear_from_visibility(wr->rendinst_dynamic_shadow_visibility);
          else
            rendinst::render::before_draw(rendinst::RenderPass::Depth, wr->rendinst_dynamic_shadow_visibility, globTm, nullptr);

          wr->renderStaticSceneOpaque(RENDER_DYNAMIC_SHADOW, cameraPos, viewItm, globTm);

          if (wr->shadowRenderExtender && updateIndex != -1 && viewIndex != -1)
            wr->shadowRenderExtender->executeAll(updateIndex, viewIndex);
        },
        [wr, &camera](const TMatrix &view_itm, const mat44f &view_tm, const mat44f &proj_tm) {
          alignas(16) TMatrix viewTm;
          v_mat_43ca_from_mat44(viewTm.m[0], view_tm);

          alignas(16) TMatrix4 projTm;
          (mat44f &)projTm = proj_tm;

          ScopeFrustumPlanesShaderVars scopedFrustumPlaneVars;
          wr->renderDynamicOpaque(RENDER_DYNAMIC_SHADOW, view_itm, viewTm, projTm, camera.viewItm.getcol(3));
        });
    };
  });

  return {
    eastl::move(prepareNode),
    eastl::move(renderNode),
  };
}

dafg::NodeHandle makePrepareTiledLightsNode()
{
  return dafg::register_node("prepare_tiled_lights_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("prepare_lights_node");
    registry.orderMeBefore("combine_shadows_node");
    registry.createBlob<OrderingToken>("tiled_lights_ready_token");
    auto closeDepthHndl = registry.readTexture("close_depth")
                            .atStage(dafg::Stage::POST_RASTER)
                            .bindToShaderVar("downsampled_close_depth_tex")
                            .optional()
                            .handle();
    registry.read("close_depth_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("downsampled_close_depth_tex_samplerstate")
      .optional();
    auto farDepthHndl = registry.readTexture("far_downsampled_depth")
                          .atStage(dafg::Stage::POST_RASTER)
                          .bindToShaderVar("downsampled_far_depth_tex")
                          .handle();
    registry.read("far_downsampled_depth_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("downsampled_far_depth_tex_samplerstate");
    registry.requestState().setFrameBlock("global_frame");
    auto hasAnyDynamicLightsHndl = registry.readBlob<bool>("has_any_dynamic_lights").handle();

    auto camera = use_camera_in_camera(registry);
    auto cameraHndl = CameraViewShvars{camera}.bindViewVecs().toHandle();

    return [hasAnyDynamicLightsHndl, cameraHndl, closeDepthHndl, farDepthHndl,
             use_downsampled_depth_in_tiled_lightsVarId = get_shader_variable_id("use_downsampled_depth_in_tiled_lights")](
             const dafg::multiplexing::Index multiplex_index) {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      auto downsampleDepthUsable = [](const BaseTexture *dowsampled_depth) {
        if (!dowsampled_depth)
          return false;
        TextureInfo info;
        dowsampled_depth->getinfo(info);
        return info.mipLevels >= DIVIDE_RESOLUTION_BITS;
      };
      bool closeDepthUsable = downsampleDepthUsable(closeDepthHndl.get());
      bool farDepthUsable = downsampleDepthUsable(farDepthHndl.get());


      if (closeDepthUsable && farDepthUsable)
        ShaderGlobal::set_int(use_downsampled_depth_in_tiled_lightsVarId, 2);
      else if (farDepthUsable)
        ShaderGlobal::set_int(use_downsampled_depth_in_tiled_lightsVarId, 1);
      else
        ShaderGlobal::set_int(use_downsampled_depth_in_tiled_lightsVarId, 0);

      if (wr.lights.hasClusteredLights() && hasAnyDynamicLightsHndl.ref())
        wr.lights.setInsideOfFrustumLightsToShader();

      camera_in_camera::ApplyPostfxState camcam{multiplex_index, cameraHndl.ref()};
      const bool needToClearLights = multiplex_index.subCamera == 0;

      wr.lights.prepareTiledLights(needToClearLights);
    };
  });
}


eastl::array<dafg::NodeHandle, 9> makeVolumetricLightsNodes()
{
  auto bindShaderVar = [](dafg::Registry registry, const char *res_name, const char *shader_var_name) {
    return eastl::move(registry.read(res_name)).texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar(shader_var_name);
  };

  auto volfog_ff_occlusion_node =
    dafg::register_node("volfog_ff_occlusion_node", DAFG_PP_NODE_SRC, [bindShaderVar](dafg::Registry registry) {
      registry.createBlob<OrderingToken>("volfog_ff_occlusion_token");

      bindShaderVar(registry, "far_downsampled_depth", "downsampled_far_depth_tex");
      registry.read("far_downsampled_depth_sampler")
        .blob<d3d::SamplerHandle>()
        .bindToShaderVar("downsampled_far_depth_tex_samplerstate");

      auto camera = registry.readBlob<CameraParams>("current_camera");
      auto cameraHndl = CameraViewShvars{camera}.bindViewVecs().toHandle();
      registry.requestState().setFrameBlock("global_frame");
      return [cameraHndl] {
        camera_in_camera::ActivateOnly camcam{};

        auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

        float volFogRange =
          cvt(wr.cameraHeight, wr.volumeLightLowHeight, wr.volumeLightHighHeight, wr.volumeLightLowRange, wr.volumeLightHighRange);

        wr.volumeLight->setRange(volFogRange);

        if (volfog_force_invalidate)
        {
          wr.invalidateVolumeLight();
          if (volfog_force_invalidate == 1)
            volfog_force_invalidate = 0;
        }

        const auto &camera = cameraHndl.ref();
        wr.volumeLight->performStartFrame(camera.viewTm, camera.jitterProjTm, camera.jitterGlobtm, camera.viewItm.getcol(3));
        wr.volumeLight->performFroxelFogOcclusion();
      };
    });

  auto volfog_ff_fill_media_node = dafg::register_node("volfog_ff_fill_media_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.readBlob<OrderingToken>("acesfx_update_token");
    registry.createBlob<OrderingToken>("volfog_ff_media_token");

    registry.readBlob<OrderingToken>("volfog_ff_occlusion_token");
    registry.readTexture("clouds_rain_map_tex").optional().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("clouds_rain_map_tex");

    /* TODO: split GE_defaultExternalsAdditional into several parts.
        downsampled depth are not used at this stage, but nbs manager binds
        them anyway, because they included into GE_defaultExternalsAdditional
    */
    registry.readTexture("checkerboard_depth")
      .atStage(dafg::Stage::PS_OR_CS)
      .bindToShaderVar("downsampled_checkerboard_depth_tex")
      .optional();
    registry.readTexture("far_downsampled_depth").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("downsampled_far_depth_tex");
    registry.historyFor("far_downsampled_depth")
      .texture()
      .atStage(dafg::Stage::PS_OR_CS)
      .bindToShaderVar("prev_downsampled_far_depth_tex")
      .optional();
    registry.readTexture("close_depth").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("downsampled_close_depth_tex").optional();

    registry.requestState().setFrameBlock("global_frame");

    auto camera = registry.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewTm>();
    CameraViewShvars{camera}.bindViewVecs();
    return [] {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      wr.volumeLight->performFroxelFogFillMedia();
      wr.performVolfogMediaInjection();
    };
  });

  auto volfog_shadow_node = dafg::register_node("volfog_shadow_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeBefore("combine_shadows_node");

    registry.createBlob<OrderingToken>("volfog_shadow_token");

    registry.readBlob<OrderingToken>("volfog_ff_occlusion_token");

    registry.readTexture("clouds_rain_map_tex").optional().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("clouds_rain_map_tex");

    /* TODO: split GE_defaultExternalsAdditional into several parts.
        downsampled depth are not used at this stage, but nbs manager binds
        them anyway, because they included into GE_defaultExternalsAdditional
    */
    registry.readTexture("checkerboard_depth")
      .atStage(dafg::Stage::PS_OR_CS)
      .bindToShaderVar("downsampled_checkerboard_depth_tex")
      .optional();
    registry.readTexture("far_downsampled_depth").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("downsampled_far_depth_tex");
    registry.historyFor("far_downsampled_depth")
      .texture()
      .atStage(dafg::Stage::PS_OR_CS)
      .bindToShaderVar("prev_downsampled_far_depth_tex")
      .optional();
    registry.readTexture("close_depth").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("downsampled_close_depth_tex").optional();

    registry.requestState().setFrameBlock("global_frame");

    auto camera = registry.readBlob<CameraParams>("current_camera");
    CameraViewShvars{camera}.bindViewVecs();

    return [] {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      wr.volumeLight->performVolfogShadow();
    };
  });

  auto volfog_df_per_camera_resources_node =
    dafg::register_node("volfog_df_per_camera_resources_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      // todo: import DF textures to FG here and do resource deps
      registry.readBlob<OrderingToken>("volfog_ff_occlusion_token");
      registry.createBlob<OrderingToken>("volfog_df_raymarch_token");
      registry.createBlob<OrderingToken>("volfog_df_mipgen_token");
      registry.createBlob<OrderingToken>("volfog_df_result_token");
      registry.createBlob<OrderingToken>("volfog_df_fx_mip_token");

      {
        d3d::SamplerInfo smpInfo;
        smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
        smpInfo.filter_mode = d3d::FilterMode::Point;
        registry.create("volfog_hist_far_downsampled_depth_sampler_1").blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));
      }
      {
        d3d::SamplerInfo smpInfo;
        smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
        smpInfo.filter_mode = d3d::FilterMode::Point;
        registry.create("volfog_hist_far_downsampled_depth_sampler_2").blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));
      }

      return []() {};
    });

  auto volfog_df_raymarch_node =
    dafg::register_node("volfog_df_raymarch_node", DAFG_PP_NODE_SRC, [bindShaderVar](dafg::Registry registry) {
      registry.modifyBlob<OrderingToken>("volfog_df_raymarch_token");
      registry.modifyBlob<OrderingToken>("volfog_df_mipgen_token");

      registry.readBlob<OrderingToken>("volfog_ff_occlusion_token"); // for performStartFrame only, but it's better not to create a
                                                                     // separate node for that

      registry.readTexture("checkerboard_depth")
        .atStage(dafg::Stage::PS_OR_CS)
        .bindToShaderVar("downsampled_checkerboard_depth_tex")
        .optional();
      registry.read("checkerboard_depth_sampler")
        .blob<d3d::SamplerHandle>()
        .bindToShaderVar("downsampled_checkerboard_depth_tex_samplerstate")
        .optional();

      registry.readTexture("close_depth").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("downsampled_close_depth_tex").optional();
      registry.read("close_depth_sampler")
        .blob<d3d::SamplerHandle>()
        .bindToShaderVar("downsampled_close_depth_tex_samplerstate")
        .optional();

      registry.historyFor("far_downsampled_depth")
        .texture()
        .atStage(dafg::Stage::PS_OR_CS)
        .bindToShaderVar("prev_downsampled_far_depth_tex")
        .optional();

      registry.read("volfog_hist_far_downsampled_depth_sampler_1")
        .blob<d3d::SamplerHandle>()
        .bindToShaderVar("prev_downsampled_far_depth_tex_samplerstate")
        .bindToShaderVar("effects_depth_tex_samplerstate");

      bindShaderVar(registry, "far_downsampled_depth", "downsampled_far_depth_tex");
      registry.read("far_downsampled_depth_sampler")
        .blob<d3d::SamplerHandle>()
        .bindToShaderVar("downsampled_far_depth_tex_samplerstate");

      registry.readTexture("clouds_rain_map_tex").optional().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("clouds_rain_map_tex");
      registry.requestState().setFrameBlock("global_frame");

      registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
      auto camera = read_camera_in_camera(registry);
      auto cameraHndl = CameraViewShvars{camera}.bindViewVecs().toHandle();
      auto prevCameraHndl = read_history_camera_in_camera(registry).handle();

      return [cameraHndl, prevCameraHndl](dafg::multiplexing::Index multiplexing_index) {
        camera_in_camera::ApplyPostfxState camcam{multiplexing_index, cameraHndl.ref(), prevCameraHndl.ref()};

        const bool isMainView = camera_in_camera::is_main_view(multiplexing_index);

        auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
        wr.volumeLight->performDistantFogRaymarch(isMainView);
      };
    });

  auto volfog_df_occlusion_weights_mip_gen_node =
    dafg::register_node("volfog_df_occlusion_weights_mip_gen_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      registry.modifyBlob<OrderingToken>("volfog_df_raymarch_token");
      registry.readBlob<OrderingToken>("volfog_df_mipgen_token");

      return [] {
        auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
        wr.volumeLight->generateDistantFogOcclusionWeightsMips();
      };
    });

  auto volfog_ff_result_node =
    dafg::register_node("volfog_ff_result_node", DAFG_PP_NODE_SRC, [bindShaderVar](dafg::Registry registry) {
      registry.orderMeAfter("prepare_lights_node");

      registry.createBlob<OrderingToken>("volfog_ff_result_token");

      registry.readBlob<OrderingToken>("volfog_ff_occlusion_token");
      registry.readBlob<OrderingToken>("volfog_ff_media_token");
      registry.readBlob<OrderingToken>("volfog_shadow_token");

      bindShaderVar(registry, "far_downsampled_depth", "downsampled_far_depth_tex");
      registry.read("far_downsampled_depth_sampler")
        .blob<d3d::SamplerHandle>()
        .bindToShaderVar("downsampled_far_depth_tex_samplerstate");

      bindShaderVar(registry, "downsampled_shadows", "downsampled_shadows").optional();
      registry.read("downsampled_shadows_sampler")
        .blob<d3d::SamplerHandle>()
        .bindToShaderVar("downsampled_shadows_samplerstate")
        .optional();

      bindShaderVar(registry, "fom_shadows_sin", "fom_shadows_sin").optional();
      bindShaderVar(registry, "fom_shadows_cos", "fom_shadows_cos").optional();
      registry.read("fom_shadows_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("fom_shadows_cos_samplerstate").optional();

      auto hasDynLightsHndl = registry.readBlob<bool>("has_any_dynamic_lights").handle();

      auto camera = registry.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewTm>();
      CameraViewShvars{camera}.bindViewVecs();

      registry.root().readTexture("csm_texture").atStage(dafg::Stage::PS_OR_CS);

      return [hasDynLightsHndl] {
        auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

        if (wr.lights.hasClusteredLights() && hasDynLightsHndl.ref())
          wr.lights.setInsideOfFrustumLightsToShader();

        CascadeShadows *csm = wr.shadowsManager.getCascadeShadows();
        csm->setCascadesToShader();

        wr.volumeLight->performFroxelFogPropagate();
      };
    });

  auto volfog_df_result_node =
    dafg::register_node("volfog_df_result_node", DAFG_PP_NODE_SRC, [bindShaderVar](dafg::Registry registry) {
      registry.modifyBlob<OrderingToken>("volfog_df_result_token");
      registry.modifyBlob<OrderingToken>("volfog_df_fx_mip_token");

      registry.readBlob<OrderingToken>("volfog_df_raymarch_token");

      registry.readTexture("checkerboard_depth")
        .atStage(dafg::Stage::PS_OR_CS)
        .bindToShaderVar("downsampled_checkerboard_depth_tex")
        .optional();
      registry.read("checkerboard_depth_sampler")
        .blob<d3d::SamplerHandle>()
        .bindToShaderVar("downsampled_checkerboard_depth_tex_samplerstate")
        .optional();

      registry.readTexture("close_depth").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("downsampled_close_depth_tex").optional();
      registry.read("close_depth_sampler")
        .blob<d3d::SamplerHandle>()
        .bindToShaderVar("downsampled_close_depth_tex_samplerstate")
        .optional();

      registry.historyFor("far_downsampled_depth")
        .texture()
        .atStage(dafg::Stage::PS_OR_CS)
        .bindToShaderVar("prev_downsampled_far_depth_tex")
        .optional();

      registry.read("volfog_hist_far_downsampled_depth_sampler_2")
        .blob<d3d::SamplerHandle>()
        .bindToShaderVar("prev_downsampled_far_depth_tex_samplerstate")
        .bindToShaderVar("effects_depth_tex_samplerstate");

      bindShaderVar(registry, "far_downsampled_depth", "downsampled_far_depth_tex");
      registry.read("far_downsampled_depth_sampler")
        .blob<d3d::SamplerHandle>()
        .bindToShaderVar("downsampled_far_depth_tex_samplerstate");

      registry.requestState().setFrameBlock("global_frame");
      registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
      auto camera = read_camera_in_camera(registry);
      auto cameraHndl = CameraViewShvars{camera}.bindViewVecs().toHandle();
      auto prevCameraHndl = read_history_camera_in_camera(registry).handle();

      return [prevCameraHndl, cameraHndl](const dafg::multiplexing::Index multiplex_index) {
        camera_in_camera::ApplyPostfxState camcam{multiplex_index, cameraHndl.ref(), prevCameraHndl.ref()};

        auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

        wr.volumeLight->performDistantFogReconstruct();
      };
    });

  auto volfog_df_fx_mipgen_node = dafg::register_node("volfog_df_fx_mipgen_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.modifyBlob<OrderingToken>("volfog_df_result_token");
    registry.readBlob<OrderingToken>("volfog_df_fx_mip_token");

    return []() {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      wr.volumeLight->performDistantFogReconstructFxMipGen();
    };
  });

  return {
    eastl::move(volfog_ff_occlusion_node),
    eastl::move(volfog_ff_fill_media_node),
    eastl::move(volfog_shadow_node),
    eastl::move(volfog_df_per_camera_resources_node),
    eastl::move(volfog_df_raymarch_node),
    eastl::move(volfog_df_occlusion_weights_mip_gen_node),
    eastl::move(volfog_ff_result_node),
    eastl::move(volfog_df_result_node),
    eastl::move(volfog_df_fx_mipgen_node),
  };
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraNodeConstruction)
static void create_lighting_nodes_es(const OnCameraNodeConstruction &evt)
{
  evt.nodes->push_back(makePrepareLightsNode());

  if (WRDispatcher::getVolumeLight() && volfog_enabled)
    for (auto &&n : makeVolumetricLightsNodes())
      evt.nodes->push_back(eastl::move(n));

  if (renderer_has_feature(FeatureRenderFlags::TILED_LIGHTS))
    evt.nodes->push_back(makePrepareTiledLightsNode());

  if (dynamic_lights.get())
    for (auto &&n : makeSceneShadowPassNodes(WRDispatcher::getLevelSettings()))
      evt.nodes->push_back(eastl::move(n));
}
