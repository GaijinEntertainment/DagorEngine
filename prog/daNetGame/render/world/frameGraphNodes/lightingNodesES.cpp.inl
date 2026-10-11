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
#include <util/dag_threadPool.h>
#include <memory/dag_mem.h>

#include <render/renderEvent.h>
#include <render/cameraInCamera/cameraInCamera.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/overridden_params.h>

#define INSIDE_RENDERER 1
#include "../private_worldRenderer.h"
#include <render/world/wrDispatcher.h>
#include "../dynamicShadowRenderExtender.h"
#include "frameGraphNodes.h"
#include <render/lights/tiled_light_consts.hlsli>


extern ConVarT<bool, false> dynamic_lights;
extern ConVarT<bool, false> volfog_enabled;

CONSOLE_INT_VAL("render", volfog_force_invalidate, 0, 0, 2); // 0 - off, 1 - invalidate once, 2 - force

// defines bbox around camera that limit dynamic objects passed into dynamic light shadow updates
static constexpr float DEFAULT_LIGHTS_SHADOW_DYN_OBJECTS_UPDATE_RANGE = 5;
CONSOLE_FLOAT_VAL("render", lights_shadow_dyn_objects_update_range, DEFAULT_LIGHTS_SHADOW_DYN_OBJECTS_UPDATE_RANGE);
CONSOLE_BOOL_VAL("render", async_scene_shadow_ri, true);

static void cull_scene_shadow_ri_view(RiGenVisibility *vis, mat44f_cref glob_tm, const Point3 &light_pos, bool dynamic_casters)
{
  rendinst::setRIGenVisibilityRendering(vis,
    dynamic_casters ? rendinst::VisibilityRenderingFlag::Dynamic : rendinst::VisibilityRenderingFlag::Static);
  rendinst::prepareRIGenExtraVisibility(glob_tm, light_pos, *vis, false, nullptr);
  if (!dynamic_casters) // the dynamic casters pass draws riex only
    rendinst::prepareRIGenVisibility(Frustum(glob_tm), light_pos, vis, false, nullptr);
}

struct SceneShadowRiCullJob final : public cpujobs::IJob
{
  mat44f globTm;
  Point3 lightPos;
  RiGenVisibility *visibility = nullptr;
  bool dynamicCasters = false;

  const char *getJobName(bool &) const override { return DAPROFILER_STRING("sceneShadowRiVisibility"); }
  void doJob() override { cull_scene_shadow_ri_view(visibility, globTm, lightPos, dynamicCasters); }
};
static constexpr int SCENE_SHADOW_RI_CULL_JOBS = 8;
static carray<SceneShadowRiCullJob, SCENE_SHADOW_RI_CULL_JOBS> scene_shadow_ri_cull_jobs;

static RiGenVisibility *take_scene_shadow_ri_visibility(int update_index,
  int view_index,
  const dynamic_shadow_render::FrameUpdates &updates,
  const dynamic_shadow_render::FrameVector<int> &slot_map)
{
  if (update_index < 0 || update_index >= (int)slot_map.size() || update_index >= (int)updates.size() || slot_map[update_index] < 0 ||
      view_index < 0 || view_index >= updates[update_index].numViews)
    return nullptr;
  TIME_PROFILE(wait_scene_shadow_ri_visibility);
  auto &job = scene_shadow_ri_cull_jobs[slot_map[update_index] + view_index];
  threadpool::wait(&job, 0, threadpool::PRIO_NORMAL);
  return job.visibility;
}

void wait_scene_shadow_ri_cull_jobs()
{
  TIME_PROFILE(wait_scene_shadow_ri_cull_jobs);
  for (auto &job : scene_shadow_ri_cull_jobs)
    threadpool::wait(&job);
}

void close_scene_shadow_ri_visibility()
{
  wait_scene_shadow_ri_cull_jobs();
  for (auto &job : scene_shadow_ri_cull_jobs)
    if (job.visibility)
      rendinst::destroyRIGenVisibility(eastl::exchange(job.visibility, nullptr));
}

void shrink_scene_shadow_ri_visibility()
{
  wait_scene_shadow_ri_cull_jobs();
  for (auto &job : scene_shadow_ri_cull_jobs)
    if (job.visibility)
      rendinst::shrinkRIGenVisibility(job.visibility);
}

static int start_scene_shadow_ri_cull_jobs(const dynamic_shadow_render::FrameUpdates &updates,
  dynamic_shadow_render::FrameVector<int> &slot_map,
  bool dynamic_casters,
  int first_slot,
  bool enabled)
{
  slot_map.assign(updates.size(), -1);
  if (!enabled)
    return first_slot;

  int nextSlot = first_slot;
  for (int i = 0; i < updates.size(); ++i)
  {
    const auto &upd = updates[i];
    // GPU objects need a gpu objects cascade on the visibility, only the shared one has it
    const bool needsGpuObjects = !dynamic_casters && upd.renderGPUObjects != DynamicShadowRenderGPUObjects::NO;
    if (needsGpuObjects || nextSlot + upd.numViews > SCENE_SHADOW_RI_CULL_JOBS)
      continue;
    slot_map[i] = nextSlot;
    for (int v = 0; v < upd.numViews; ++v, ++nextSlot) // nextSlot increase [we start jobs for views]
    {
      auto &job = scene_shadow_ri_cull_jobs[nextSlot];
      threadpool::wait(&job); // a job from a frame whose render node did not run may still be pending
      if (!job.visibility)
        job.visibility = rendinst::createRIGenVisibility(midmem);
      job.dynamicCasters = dynamic_casters;
      v_mat44_mul(job.globTm, upd.proj, upd.views[v].view);
      v_stu_p3(&job.lightPos.x, upd.views[v].invView.col3);
      threadpool::add(&job, threadpool::PRIO_NORMAL, false);
    }
  }
  return nextSlot;
}

void prepare_scene_shadows_in_lights_job(WorldRenderer &wr, vec3f view_pos, mat44f_cref globtm, float hk)
{
  if (!dynamic_lights.get() || wr.canChangeAltitudeUnexpectedly)
    return;

  vec4f dynRange = v_splats(lights_shadow_dyn_objects_update_range);
  bbox3f dynBox = {v_sub(view_pos, dynRange), v_add(view_pos, dynRange)};
  Point3 viewPos;
  v_stu_p3(&viewPos.x, view_pos);

  SceneShadowRenderData &renderData = wr.sceneShadowRenderData;
  wr.lights.framePrepareShadows(renderData.volumeData, viewPos, globtm, hk, make_span_const(&dynBox, 1), true);

  TIME_PROFILE(start_scene_shadow_ri_cull_jobs);
  const bool async = async_scene_shadow_ri.get() && !wr.sceneShadowRiGpuObjectsPending;
  int nextSlot = start_scene_shadow_ri_cull_jobs(renderData.volumeData.staticUpdates, renderData.staticRiSlots, false, 0, async);
  nextSlot = start_scene_shadow_ri_cull_jobs(renderData.volumeData.dynamicUpdates, renderData.dynamicRiSlots, true, nextSlot, async);
  if (nextSlot > 0)
    threadpool::wake_up_all();
}

dafg::NodeHandle makePrepareLightsNode()
{
  return dafg::register_node("prepare_lights_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("downsample_depth_node");

    registry.createBlob<OrderingToken>("after_prepare_lights_node_token");

    int dynamic_lights_countVarId = get_shader_variable_id("dynamic_lights_count");
    auto hasAnyDynamicLightsHndl = registry.createBlob<bool>("has_any_dynamic_lights").handle();
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();

    return [hasAnyDynamicLightsHndl, dynamic_lights_countVarId, cameraHndl]() {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      cameraHndl.ref().jobsMgr->waitLights();
      if (dynamic_lights.get() && (wr.lights.hasClusteredLights() || wr.lights.hasDeferredLights()))
        wr.lights.fillAndSetInsideOfFrustumLightsBuffers();

      DynLightsOptimizationMode dynLightsMode =
        dynamic_lights.get() ? wr.lights.getLightsCountInterval() : DynLightsOptimizationMode::NO_LIGHTS;

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

    auto renderDataHndl = registry.createBlob<SceneShadowRenderData>("scene_shadow_render_data").handle();

    return [renderDataHndl](const dafg::multiplexing::Index multiplex_index) {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      SceneShadowRenderData &renderData = renderDataHndl.ref();
      renderData.clear();

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

      if (multiplex_index != dafg::multiplexing::Index{})
        return;

      renderData = eastl::move(wr.sceneShadowRenderData);
    };
  });

  auto renderNode = dafg::register_node("scene_shadow_render_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeBefore("combine_shadows_node");
    registry.requestState().setFrameBlock("global_frame");
    registry.executionHas(dafg::SideEffects::External);

    registry.createBlob<OrderingToken>("dynamic_lights_shadow_buffers_ready_token");

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto renderDataHndl = registry.readBlob<SceneShadowRenderData>("scene_shadow_render_data").handle();

    auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
    if (wr->shadowRenderExtender)
      wr->shadowRenderExtender->declareAll(registry);

    return [wr, cameraHndl, renderDataHndl, rendinstDepthSceneBlockId = ShaderGlobal::getBlockId("rendinst_depth_scene")] {
      const auto &camera = cameraHndl.ref();
      const SceneShadowRenderData &renderData = renderDataHndl.ref();
      const Point3 dynCameraPos = camera.viewItm.getcol(3);

      wr->lights.frameRenderShadows(
        renderData.volumeData,
        [&](mat44f_cref globTm, mat44f_cref /*projTm*/, const TMatrix &viewItm, int updateIndex, int viewIndex,
          DynamicShadowRenderGPUObjects render_gpu_objects) {
          Point3 cameraPos = viewItm.getcol(3);
          RiGenVisibility *vis =
            take_scene_shadow_ri_visibility(updateIndex, viewIndex, renderData.volumeData.staticUpdates, renderData.staticRiSlots);
          if (!vis) // synchronous path
          {
            vis = wr->rendinst_dynamic_shadow_visibility;
            SCENE_LAYER_GUARD(rendinstDepthSceneBlockId);
            cull_scene_shadow_ri_view(vis, globTm, cameraPos, false);
          }
          if (render_gpu_objects == DynamicShadowRenderGPUObjects::NO)
            rendinst::gpuobjects::clear_from_visibility(vis);
          else
            rendinst::render::before_draw(rendinst::RenderPass::Depth, vis, globTm, nullptr);

          wr->renderStaticSceneOpaque(RENDER_DYNAMIC_SHADOW, cameraPos, viewItm, globTm, vis);

          if (wr->shadowRenderExtender && updateIndex != -1 && viewIndex != -1)
            wr->shadowRenderExtender->executeAll(updateIndex, viewIndex);
        },
        [&](const TMatrix &view_itm, const mat44f &view_tm, const mat44f &proj_tm, int updateIndex, int viewIndex) {
          alignas(16) TMatrix viewTm;
          v_mat_43ca_from_mat44(viewTm.m[0], view_tm);

          alignas(16) TMatrix4 projTm;
          (mat44f &)projTm = proj_tm;

          {
            RiGenVisibility *vis =
              take_scene_shadow_ri_visibility(updateIndex, viewIndex, renderData.volumeData.dynamicUpdates, renderData.dynamicRiSlots);
            SCENE_LAYER_GUARD(rendinstDepthSceneBlockId);
            if (!vis) // synchronous path
            {
              vis = wr->rendinst_dynamic_shadow_visibility;
              mat44f globTm;
              v_mat44_mul(globTm, proj_tm, view_tm);
              cull_scene_shadow_ri_view(vis, globTm, view_itm.getcol(3), true);
            }
            rendinst::gpuobjects::clear_from_visibility(vis);
            rendinst::render::renderRIGen(rendinst::RenderPass::Depth, vis, view_itm, rendinst::LayerFlag::Opaque,
              rendinst::OptimizeDepthPass::No);
          }

          ScopeFrustumPlanesShaderVars scopedFrustumPlaneVars;
          wr->renderDynamicOpaque(RENDER_DYNAMIC_SHADOW, view_itm, viewTm, projTm, dynCameraPos);
        });

      rendinst::setRIGenVisibilityRendering(wr->rendinst_dynamic_shadow_visibility, rendinst::VisibilityRenderingFlag::All);
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


eastl::array<dafg::NodeHandle, 10> makeVolumetricLightsNodes()
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
    });

  auto volfog_df_raymarch_node =
    dafg::register_node("volfog_df_raymarch_node", DAFG_PP_NODE_SRC, [bindShaderVar](dafg::Registry registry) {
      registry.modifyBlob<OrderingToken>("volfog_df_raymarch_token");
      registry.modifyBlob<OrderingToken>("volfog_df_mipgen_token");
      registry.readBlob<OrderingToken>("volfog_df_prepared");

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
      registry.readBlob<OrderingToken>("volfog_df_prepared");
      registry.readBlob<OrderingToken>("volfog_df_mipgen_token");

      return [] {
        auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
        wr.volumeLight->generateDistantFogOcclusionWeightsMips();
      };
    });

  auto volfog_ff_result_node =
    dafg::register_node("volfog_ff_result_node", DAFG_PP_NODE_SRC, [bindShaderVar](dafg::Registry registry) {
      registry.orderMeAfter("prepare_lights_node");
      registry.readBlob<OrderingToken>("dynamic_lights_shadow_buffers_ready_token").optional();

      registry.createBlob<OrderingToken>("volfog_ff_result_token");

      registry.readBlob<OrderingToken>("volfog_ff_occlusion_token");
      registry.readBlob<OrderingToken>("volfog_ff_media_token");
      registry.readBlob<OrderingToken>("volfog_shadow_token");

      bindShaderVar(registry, "far_downsampled_depth", "downsampled_far_depth_tex");
      registry.read("far_downsampled_depth_sampler")
        .blob<d3d::SamplerHandle>()
        .bindToShaderVar("downsampled_far_depth_tex_samplerstate");

      bindShaderVar(registry, "downsampled_shadows", "downsampled_shadows").optional();

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

  auto volfog_df_prepare_node = dafg::register_node("volfog_df_prepare", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto resHndl = registry.getResolution<2>("main_view");
    registry.readBlob<OrderingToken>("volfog_ff_occlusion_token");
    registry.createBlob<OrderingToken>("volfog_df_prepared");

    return [resHndl]() {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      wr.volumeLight->prepareDistantFog(resHndl.get());
    };
  });

  return {eastl::move(volfog_ff_occlusion_node), eastl::move(volfog_ff_fill_media_node), eastl::move(volfog_shadow_node),
    eastl::move(volfog_df_per_camera_resources_node), eastl::move(volfog_df_raymarch_node),
    eastl::move(volfog_df_occlusion_weights_mip_gen_node), eastl::move(volfog_ff_result_node), eastl::move(volfog_df_result_node),
    eastl::move(volfog_df_fx_mipgen_node), eastl::move(volfog_df_prepare_node)};
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
