// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daBfg/bfg.h>
#include <ecs/render/renderPasses.h>
#include <render/volumetricLights/volumetricLights.h>
#include <shaders/dag_shaderBlock.h>
#include <util/dag_convar.h>
#include <perfMon/dag_statDrv.h>
#include <rendInst/gpuObjects.h>
#include <rendInst/visibility.h>
#include <rendInst/rendInstGenRender.h>
#include <frustumCulling/frustumPlanes.h>
#include <render/world/frameGraphHelpers.h>

#define INSIDE_RENDERER 1
#include "../private_worldRenderer.h"
#include "../dynamicShadowRenderExtender.h"
#include "frameGraphNodes.h"


extern ConVarT<bool, false> dynamic_lights;

CONSOLE_INT_VAL("render", volfog_force_invalidate, 0, 0, 2); // 0 - off, 1 - invalidate once, 2 - force

dabfg::NodeHandle makePrepareLightsNode()
{
  return dabfg::register_node("prepare_lights_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.orderMeAfter("downsample_depth_node");
    int dynamic_lights_countVarId = get_shader_variable_id("dynamic_lights_count");
    auto hasAnyDynamicLightsHndl = registry.createBlob<bool>("has_any_dynamic_lights", dabfg::History::No).handle();

    return [hasAnyDynamicLightsHndl, dynamic_lights_countVarId]() {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      wr.waitLights();

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
eastl::array<dabfg::NodeHandle, 2> makeSceneShadowPassNodes()
{
  auto prepareNode = dabfg::register_node("scene_shadow_prepare_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.orderMeAfter("prepare_lights_node");
    registry.requestState().setFrameBlock("global_frame");

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto volumesHndl =
      registry.createBlob<dynamic_shadow_render::VolumesVector>("scene_shadow_volumes_to_render", dabfg::History::No).handle();
    auto updatesHndl = registry.createBlob<dynamic_shadow_render::FrameUpdates>("scene_shadow_updates", dabfg::History::No).handle();

    return [cameraHndl, volumesHndl, updatesHndl] {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      // May change every frame, therefore no node recreation
      if (wr.canChangeAltitudeUnexpectedly)
      {
        // No need to update shadows, but we still need to update const buffer with shadow matrices
        // because lights are culled every frame and corresponding matrices may need different offset
        // in this const buffer.
        wr.lights.updateShadowBuffers();
        return;
      }

      const auto &camera = cameraHndl.ref();
      auto &volumesToRender = volumesHndl.ref();
      auto &updates = updatesHndl.ref();

      vec4f vpos = v_ldu_p3(camera.viewItm[3]);
      bbox3f dynBox = {v_sub(vpos, v_splats(5)), v_add(vpos, v_splats(5))};

      mat44f globTm;
      v_mat44_make_from_44cu(globTm, &camera.jitterGlobtm._11);

      wr.lights.framePrepareShadows(volumesToRender, camera.viewItm.getcol(3), (mat44f_cref)camera.jitterGlobtm, camera.jitterPersp.hk,
        make_span_const(&dynBox, 1), &updates);
    };
  });

  auto renderNode = dabfg::register_node("scene_shadow_render_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.orderMeBefore("combine_shadows_node");
    registry.requestState().setFrameBlock("global_frame");
    registry.executionHas(dabfg::SideEffects::External);

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
        [wr, rendinstDepthSceneBlockId](mat44f_cref globTm, const TMatrix &viewItm, int updateIndex, int viewIndex,
          DynamicShadowRenderGPUObjects render_gpu_objects) {
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

dabfg::NodeHandle makePrepareTiledLightsNode()
{
  return dabfg::register_node("prepare_tiled_lights_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.orderMeAfter("prepare_lights_node");
    registry.orderMeBefore("combine_shadows_node");
    registry.readTexture("close_depth").atStage(dabfg::Stage::POST_RASTER).bindToShaderVar("downsampled_close_depth_tex").optional();
    registry.read("close_depth_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("downsampled_close_depth_tex_samplerstate")
      .optional();
    registry.readTexture("far_downsampled_depth").atStage(dabfg::Stage::POST_RASTER).bindToShaderVar("downsampled_far_depth_tex");
    registry.read("far_downsampled_depth_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("downsampled_far_depth_tex_samplerstate");
    registry.requestState().setFrameBlock("global_frame");
    auto hasAnyDynamicLightsHndl = registry.readBlob<bool>("has_any_dynamic_lights").handle();
    registry.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewTm>().bindAsProj<&CameraParams::jitterProjTm>();

    return [hasAnyDynamicLightsHndl]() {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      if (wr.lights.hasClusteredLights() && hasAnyDynamicLightsHndl.ref())
        wr.lights.setBuffersToShader();
      wr.lights.prepareTiledLights();
    };
  });
}


eastl::array<dabfg::NodeHandle, 6> makeVolumetricLightsNodes()
{
  auto bindShaderVarBase = [](auto &&res_request, const char *shader_var_name) {
    return eastl::move(res_request).texture().atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar(shader_var_name);
  };
  auto bindShaderVar = [&bindShaderVarBase](dabfg::Registry registry, const char *res_name, const char *shader_var_name) {
    return bindShaderVarBase(registry.read(res_name), shader_var_name);
  };

  auto volfog_ff_occlusion_node =
    dabfg::register_node("volfog_ff_occlusion_node", DABFG_PP_NODE_SRC, [&bindShaderVar](dabfg::Registry registry) {
      registry.createBlob<OrderingToken>("volfog_ff_occlusion_token", dabfg::History::No);

      bindShaderVar(registry, "far_downsampled_depth", "downsampled_far_depth_tex");
      registry.read("far_downsampled_depth_sampler")
        .blob<d3d::SamplerHandle>()
        .bindToShaderVar("downsampled_far_depth_tex_samplerstate");

      auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
      registry.requestState().setFrameBlock("global_frame");
      return [cameraHndl] {
        auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
        const auto &camera = cameraHndl.ref();

        float volFogRange =
          cvt(wr.cameraHeight, wr.volumeLightLowHeight, wr.volumeLightHighHeight, wr.volumeLightLowRange, wr.volumeLightHighRange);

        wr.volumeLight->setRange(volFogRange);

        if (volfog_force_invalidate)
        {
          wr.invalidateVolumeLight();
          if (volfog_force_invalidate == 1)
            volfog_force_invalidate = 0;
        }

        wr.volumeLight->performStartFrame(camera.viewTm, camera.jitterProjTm, camera.jitterGlobtm, camera.viewItm.getcol(3));
        wr.volumeLight->performFroxelFogOcclusion();
      };
    });

  auto volfog_ff_fill_media_node = dabfg::register_node("volfog_ff_fill_media_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.createBlob<OrderingToken>("volfog_ff_media_token", dabfg::History::No);

    registry.readBlob<OrderingToken>("volfog_ff_occlusion_token");

    registry.requestState().setFrameBlock("global_frame");
    return [] {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      wr.volumeLight->performFroxelFogFillMedia();
      wr.performVolfogMediaInjection();
    };
  });

  auto volfog_shadow_node = dabfg::register_node("volfog_shadow_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.orderMeBefore("combine_shadows_node");

    registry.createBlob<OrderingToken>("volfog_shadow_token", dabfg::History::No);

    registry.readBlob<OrderingToken>("volfog_ff_occlusion_token");

    registry.requestState().setFrameBlock("global_frame");
    return [] {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      wr.volumeLight->performVolfogShadow();
    };
  });

  auto volfog_df_raymarch_node =
    dabfg::register_node("volfog_df_raymarch_node", DABFG_PP_NODE_SRC, [&bindShaderVar](dabfg::Registry registry) {
      registry.createBlob<OrderingToken>("volfog_df_raymarch_token", dabfg::History::No);

      registry.readBlob<OrderingToken>("volfog_ff_occlusion_token"); // for performStartFrame only, but it's better not to create a
                                                                     // separate node for that

      registry.readTexture("checkerboard_depth")
        .atStage(dabfg::Stage::PS_OR_CS)
        .bindToShaderVar("downsampled_checkerboard_depth_tex")
        .optional();
      registry.read("checkerboard_depth_sampler")
        .blob<d3d::SamplerHandle>()
        .bindToShaderVar("downsampled_checkerboard_depth_tex_samplerstate")
        .optional();

      registry.readTexture("close_depth").atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar("downsampled_close_depth_tex").optional();
      registry.read("close_depth_sampler")
        .blob<d3d::SamplerHandle>()
        .bindToShaderVar("downsampled_close_depth_tex_samplerstate")
        .optional();

      registry.historyFor("far_downsampled_depth")
        .texture()
        .atStage(dabfg::Stage::PS_OR_CS)
        .bindToShaderVar("prev_downsampled_far_depth_tex")
        .optional();
      {
        d3d::SamplerInfo smpInfo;
        smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
        smpInfo.filter_mode = d3d::FilterMode::Point;
        registry.create("volfog_hist_far_downsampled_depth_sampler_1", dabfg::History::No)
          .blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo))
          .bindToShaderVar("prev_downsampled_far_depth_tex_samplerstate")
          .bindToShaderVar("effects_depth_tex_samplerstate");
      }

      bindShaderVar(registry, "far_downsampled_depth", "downsampled_far_depth_tex");
      registry.read("far_downsampled_depth_sampler")
        .blob<d3d::SamplerHandle>()
        .bindToShaderVar("downsampled_far_depth_tex_samplerstate");

      registry.requestState().setFrameBlock("global_frame");
      return [] {
        auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

        wr.volumeLight->performDistantFogRaymarch();
      };
    });

  auto volfog_ff_result_node =
    dabfg::register_node("volfog_ff_result_node", DABFG_PP_NODE_SRC, [&bindShaderVar](dabfg::Registry registry) {
      registry.orderMeAfter("prepare_lights_node");

      registry.createBlob<OrderingToken>("volfog_ff_result_token", dabfg::History::No);

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
      registry.requestState().setFrameBlock("global_frame");
      return [hasDynLightsHndl] {
        auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

        if (wr.lights.hasClusteredLights() && hasDynLightsHndl.ref())
          wr.lights.setBuffersToShader();

        CascadeShadows *csm = wr.shadowsManager.getCascadeShadows();
        csm->setCascadesToShader();

        wr.volumeLight->performFroxelFogPropagate();
      };
    });

  auto volfog_df_result_node =
    dabfg::register_node("volfog_df_result_node", DABFG_PP_NODE_SRC, [&bindShaderVar](dabfg::Registry registry) {
      registry.createBlob<OrderingToken>("volfog_df_result_token", dabfg::History::No);

      registry.readBlob<OrderingToken>("volfog_df_raymarch_token");

      registry.readTexture("checkerboard_depth")
        .atStage(dabfg::Stage::PS_OR_CS)
        .bindToShaderVar("downsampled_checkerboard_depth_tex")
        .optional();
      registry.read("checkerboard_depth_sampler")
        .blob<d3d::SamplerHandle>()
        .bindToShaderVar("downsampled_checkerboard_depth_tex_samplerstate")
        .optional();

      registry.readTexture("close_depth").atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar("downsampled_close_depth_tex").optional();
      registry.read("close_depth_sampler")
        .blob<d3d::SamplerHandle>()
        .bindToShaderVar("downsampled_close_depth_tex_samplerstate")
        .optional();

      registry.historyFor("far_downsampled_depth")
        .texture()
        .atStage(dabfg::Stage::PS_OR_CS)
        .bindToShaderVar("prev_downsampled_far_depth_tex")
        .optional();
      {
        d3d::SamplerInfo smpInfo;
        smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
        smpInfo.filter_mode = d3d::FilterMode::Point;
        registry.create("volfog_hist_far_downsampled_depth_sampler_2", dabfg::History::No)
          .blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo))
          .bindToShaderVar("prev_downsampled_far_depth_tex_samplerstate")
          .bindToShaderVar("effects_depth_tex_samplerstate");
      }

      bindShaderVar(registry, "far_downsampled_depth", "downsampled_far_depth_tex");
      registry.read("far_downsampled_depth_sampler")
        .blob<d3d::SamplerHandle>()
        .bindToShaderVar("downsampled_far_depth_tex_samplerstate");

      registry.requestState().setFrameBlock("global_frame");
      return [] {
        auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

        wr.volumeLight->performDistantFogReconstruct();
      };
    });

  return {
    eastl::move(volfog_ff_occlusion_node),
    eastl::move(volfog_ff_fill_media_node),
    eastl::move(volfog_shadow_node),
    eastl::move(volfog_df_raymarch_node),
    eastl::move(volfog_ff_result_node),
    eastl::move(volfog_df_result_node),
  };
}
