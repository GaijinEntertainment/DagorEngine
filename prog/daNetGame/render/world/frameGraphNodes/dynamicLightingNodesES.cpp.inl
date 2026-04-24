// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/optional.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <drv/3d/dag_rwResource.h>
#include <render/clusteredLights.h>
#include <render/daFrameGraph/ecs/frameGraphNode.h>
#include <render/renderEvent.h>
#include <render/renderSettings.h>
#include <render/resolution.h>
#include <render/world/bvh.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/shadowsManager.h>
#include <render/world/wrDispatcher.h>
#include <shaders/dag_computeShaders.h>
#include <util/dag_console.h>

static dafg::AutoResolutionRequest<2> get_dynamic_lighting_texture_resolution(dafg::Registry registry)
{
  return registry.getResolution<2>("main_view");
}

static dafg::Texture2dCreateInfo get_dynamic_lighting_tex_create_info(dafg::Registry registry)
{
  return dafg::Texture2dCreateInfo{TEXFMT_R11G11B10F | TEXCF_UNORDERED, get_dynamic_lighting_texture_resolution(registry), 1};
}

static int divide_up(int x, int y) { return (x + y - 1) / y; }

static void recreate_dynamic_lighting_nodes(bool is_precomputed,
  bool is_rtsm,
  dafg::NodeHandle &prepare_resources_node,
  dafg::NodeHandle &generate_tiles_node,
  dafg::NodeHandle &lighting_node)
{
  if (!WRDispatcher::isReadyToUse())
    return;

  if ((!is_precomputed && !is_rtsm) || WRDispatcher::isBareMinimum())
  {
    prepare_resources_node = dafg::NodeHandle();
    generate_tiles_node = dafg::NodeHandle();
    lighting_node = dafg::NodeHandle();
    return;
  }

  if (is_rtsm)
  {
    prepare_resources_node =
      dafg::register_node("prepare_dynamic_lighting_texture_per_camera", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
        registry.createTexture2d("dynamic_lighting_texture", get_dynamic_lighting_tex_create_info(registry));
      });
    generate_tiles_node = dafg::NodeHandle();
    lighting_node = make_rtsm_dynamic_node();
  }
  else
  {
    const IPoint2 maxResolution = get_max_possible_rendering_resolution();
    static constexpr int TILE_SIZE = 8;
    int maxTileCount = divide_up(maxResolution.x, TILE_SIZE) * divide_up(maxResolution.y, TILE_SIZE);

    prepare_resources_node =
      dafg::register_node("prepare_dynamic_lighting_texture_per_camera", DAFG_PP_NODE_SRC, [maxTileCount](dafg::Registry registry) {
        registry.createTexture2d("dynamic_lighting_texture", get_dynamic_lighting_tex_create_info(registry))
          .clear(make_clear_value(0.f, 0.f, 0.f, 0.f));

        registry.create("render_direct_lights_tiles").byteAddressBuffer(maxTileCount);
      });

    generate_tiles_node = dafg::register_node("dynamic_lighting_generate_tiles", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      registry.readBlob<OrderingToken>("tiled_lights_ready_token").optional();
      registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);

      auto camera = read_camera_in_camera(registry);
      auto cameraHndl = CameraViewShvars{camera}.bindViewVecs().toHandle();

      auto argsHndl = registry.create("render_direct_lights_args")
                        .indirectBuffer(d3d::buffers::Indirect::Dispatch, 1)
                        .atStage(dafg::Stage::COMPUTE)
                        .bindToShaderVar("render_direct_lights_args")
                        .handle();

      registry.modify("render_direct_lights_tiles")
        .buffer()
        .atStage(dafg::Stage::COMPUTE)
        .bindToShaderVar("render_direct_lights_tiles");

      registry.bindTexCs("close_depth", "downsampled_close_depth_tex");
      registry.bindBlob<d3d::SamplerHandle>("close_depth_sampler", "downsampled_close_depth_tex_samplerstate");
      registry.bindTexCs("far_downsampled_depth", "downsampled_far_depth_tex");
      registry.bindBlob<d3d::SamplerHandle>("far_downsampled_depth_sampler", "downsampled_far_depth_tex_samplerstate");

      auto hasAnyDynamicLightsHndl = registry.readBlob<bool>("has_any_dynamic_lights").handle();
      auto dynamicLightingTexResolution = get_dynamic_lighting_texture_resolution(registry);

      return [argsHndl, cameraHndl, hasAnyDynamicLightsHndl, dynamicLightingTexResolution,
               generate_dynamic_lighting_tiles = Ptr(new_compute_shader("generate_dynamic_lighting_tiles"))](
               const dafg::multiplexing::Index &multiplexing_index) {
        if (!hasAnyDynamicLightsHndl.ref())
          return;

        d3d::zero_rwbufi(argsHndl.view().getBuf());

        camera_in_camera::ApplyPostfxState camcam{multiplexing_index, cameraHndl.ref()};

        auto &lights = WRDispatcher::getClusteredLights();
        lights.setInsideOfFrustumLightsToShader();

        const IPoint2 texResolution = dynamicLightingTexResolution.get();
        generate_dynamic_lighting_tiles->dispatchThreads(divide_up(texResolution.x, 2), divide_up(texResolution.y, 2), 1);
      };
    });

    lighting_node = dafg::register_node("dynamic_lighting", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      registry.readBlob<OrderingToken>("rtao_token").optional();
      registry.readBlob<OrderingToken>("dynamic_lights_shadow_buffers_ready_token").optional();

      registry.readTexture("ssao_tex").atStage(dafg::Stage::COMPUTE).bindToShaderVar("ssao_tex").optional();
      registry.bindBlob<d3d::SamplerHandle>("ssao_sampler", "ssao_tex_samplerstate").optional();
      registry.read("upscale_sampling_tex").texture().atStage(dafg::Stage::COMPUTE).bindToShaderVar("upscale_sampling_tex").optional();

      registry.bindTexCs("close_depth", "downsampled_close_depth_tex");
      registry.bindBlob<d3d::SamplerHandle>("close_depth_sampler", "downsampled_close_depth_tex_samplerstate");
      registry.bindTexCs("far_downsampled_depth", "downsampled_far_depth_tex");
      registry.bindBlob<d3d::SamplerHandle>("far_downsampled_depth_sampler", "downsampled_far_depth_tex_samplerstate");

      registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
      auto camera = read_camera_in_camera(registry);
      auto cameraHndl = CameraViewShvars{camera}.bindViewVecs().toHandle();

      read_gbuffer(registry, dafg::Stage::COMPUTE);
      read_gbuffer_depth(registry, dafg::Stage::COMPUTE);

      registry.modifyTexture("dynamic_lighting_texture").atStage(dafg::Stage::COMPUTE).bindToShaderVar("precomputed_dynamic_lights");
      registry.bindBlob<Point4>("world_view_pos", "world_view_pos");

      auto argsHndl = registry.read("render_direct_lights_args")
                        .buffer()
                        .atStage(dafg::Stage::COMPUTE)
                        .useAs(dafg::Usage::INDIRECTION_BUFFER)
                        .handle();

      registry.read("render_direct_lights_tiles").buffer().atStage(dafg::Stage::COMPUTE).bindToShaderVar("render_direct_lights_tiles");

      auto hasAnyDynamicLightsHndl = registry.readBlob<bool>("has_any_dynamic_lights").handle();

      return
        [cameraHndl, hasAnyDynamicLightsHndl, argsHndl, render_dynamic_lighting = Ptr(new_compute_shader("render_dynamic_lighting"))](
          const dafg::multiplexing::Index &multiplexing_index) {
          if (!hasAnyDynamicLightsHndl.ref())
            return;

          const CameraParams &camera = cameraHndl.ref();
          WRDispatcher::getShadowsManager().setShadowFrameIndex(camera);
          camera_in_camera::ApplyPostfxState camcam{multiplexing_index, camera};

          auto &lights = WRDispatcher::getClusteredLights();
          lights.setInsideOfFrustumLightsToShader();

          render_dynamic_lighting->dispatch_indirect(argsHndl.view().getBuf(), 0);
        };
    });
  }
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear, OnRenderSettingsReady)
static void dynamic_lighting_on_appear_es(const ecs::Event &,
  bool dynamic_lighting_is_precomputed,
  bool &dynamic_lighting_is_rtsm,
  dafg::NodeHandle &prepare_dynamic_lighting_texture_per_camera,
  dafg::NodeHandle &dynamic_lighting_generate_tiles_node,
  dafg::NodeHandle &dynamic_lighting_node)
{
  dynamic_lighting_is_rtsm = is_rtsm_dynamic_enabled();
  recreate_dynamic_lighting_nodes(dynamic_lighting_is_precomputed, dynamic_lighting_is_rtsm,
    prepare_dynamic_lighting_texture_per_camera, dynamic_lighting_generate_tiles_node, dynamic_lighting_node);
}

ECS_TAG(render)
static void dynamic_lighting_on_change_render_features_es(const ChangeRenderFeatures &evt,
  bool dynamic_lighting_is_precomputed,
  bool dynamic_lighting_is_rtsm,
  dafg::NodeHandle &prepare_dynamic_lighting_texture_per_camera,
  dafg::NodeHandle &dynamic_lighting_generate_tiles_node,
  dafg::NodeHandle &dynamic_lighting_node)
{
  if (!evt.isFeatureChanged(FeatureRenderFlags::CAMERA_IN_CAMERA) && !evt.isFeatureChanged(FeatureRenderFlags::CLUSTERED_LIGHTS) &&
      !evt.isFeatureChanged(FeatureRenderFlags::DYNAMIC_LIGHTS_SHADOWS))
    return;

  recreate_dynamic_lighting_nodes(dynamic_lighting_is_precomputed, dynamic_lighting_is_rtsm,
    prepare_dynamic_lighting_texture_per_camera, dynamic_lighting_generate_tiles_node, dynamic_lighting_node);
}

ECS_TAG(render)
static void dynamic_lighting_on_set_resolution_es(const SetResolutionEvent &evt,
  bool dynamic_lighting_is_precomputed,
  bool dynamic_lighting_is_rtsm,
  dafg::NodeHandle &prepare_dynamic_lighting_texture_per_camera,
  dafg::NodeHandle &dynamic_lighting_generate_tiles_node,
  dafg::NodeHandle &dynamic_lighting_node)
{
  if (evt.type == SetResolutionEvent::Type::DYNAMIC_RESOLUTION)
    return;

  recreate_dynamic_lighting_nodes(dynamic_lighting_is_precomputed, dynamic_lighting_is_rtsm,
    prepare_dynamic_lighting_texture_per_camera, dynamic_lighting_generate_tiles_node, dynamic_lighting_node);
}

template <typename Callable>
inline void set_dynamic_lighting_state_ecs_query(ecs::EntityId, Callable c);

static void set_dynamic_lighting_state(ecs::EntityId eid, eastl::optional<bool> set_precomputed, eastl::optional<bool> set_rtsm)
{
  set_dynamic_lighting_state_ecs_query(eid,
    [set_precomputed, set_rtsm](bool &dynamic_lighting_is_precomputed, bool &dynamic_lighting_is_rtsm,
      dafg::NodeHandle &prepare_dynamic_lighting_texture_per_camera, dafg::NodeHandle &dynamic_lighting_generate_tiles_node,
      dafg::NodeHandle &dynamic_lighting_node) {
      bool recreateNodes = false;
      if (set_precomputed.has_value() && dynamic_lighting_is_precomputed != set_precomputed.value())
      {
        dynamic_lighting_is_precomputed = set_precomputed.value();
        recreateNodes = true;
      }
      if (set_rtsm.has_value() && dynamic_lighting_is_rtsm != set_rtsm.value())
      {
        dynamic_lighting_is_rtsm = set_rtsm.value();
        recreateNodes = true;
      }
      if (recreateNodes)
        recreate_dynamic_lighting_nodes(dynamic_lighting_is_precomputed, dynamic_lighting_is_rtsm,
          prepare_dynamic_lighting_texture_per_camera, dynamic_lighting_generate_tiles_node, dynamic_lighting_node);
    });
}

void toggle_rtsm_dynamic(bool enable)
{
  if (ecs::EntityId eid = g_entity_mgr->getSingletonEntity(ECS_HASH("dynamic_lighting")))
    set_dynamic_lighting_state(eid, eastl::nullopt, enable);
}

using namespace console;
static bool dynamic_lighting_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  int found = 0;
  CONSOLE_CHECK_NAME("render", "use_precomputed_dynamic_lights", 2, 2)
  {
    if (ecs::EntityId eid = g_entity_mgr->getSingletonEntity(ECS_HASH("dynamic_lighting")))
    {
      bool usePrecomputedDynamicLights = to_bool(argv[1]);
      set_dynamic_lighting_state(eid, usePrecomputedDynamicLights, eastl::nullopt);
    }
  }
  return found;
}
REGISTER_CONSOLE_HANDLER(dynamic_lighting_console_handler);