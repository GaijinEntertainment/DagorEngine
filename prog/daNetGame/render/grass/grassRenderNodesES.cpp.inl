// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/world/cameraInCamera.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <heightmap/heightmapHandler.h>
#include <heightmap/heightmapMetricsCalc.h>
#include <landMesh/lmeshManager.h>
#include <render/daFrameGraph/ecs/frameGraphNode.h>

#include <render/renderEvent.h>
#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/wrDispatcher.h>
#include <render/world/renderPrecise.h>

#include <render/world/frameGraphNodes/frameGraphNodes.h>

#include "grassRender.h"
#include "grassRenderer.h"


CONSOLE_BOOL_VAL("grass", use_gpu_occlusion, true);

extern ConVarB prepass;
extern ConVarB fast_grass_render;

template <typename Callable>
static void get_grass_render_ecs_query(ecs::EntityManager &manager, Callable c);

template <typename Callable>
static void init_grass_render_ecs_query(ecs::EntityManager &manager, Callable c);

namespace var
{
ShaderVariableInfo reprojected_hzb("reprojected_hzb");
ShaderVariableInfo reprojected_hzb_depth_mip_count("reprojected_hzb_depth_mip_count");
ShaderVariableInfo grass_use_hzb_occlusion("grass_use_hzb_occlusion");
} // namespace var

void GrassRenderer::makeFastGrassNodes()
{
  auto ns = dafg::root() / "opaque" / "statics";

  fastGrassPrecompNode = ns.registerNode("fast_grass_precomp_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto cameraHndl = use_camera_in_camera(registry).handle();
    registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    const auto grassPrepDataHndl = registry.createBlob<FastGrassRenderer::PreparedData>("gatheredInfo").handle();

    return [cameraHndl, grassPrepDataHndl](const dafg::multiplexing::Index &) {
      if (!fast_grass_render.get())
        return;
      get_grass_render_ecs_query(*g_entity_mgr, [&](GrassRenderer &grass_render) {
        if (auto *lmeshManager = WRDispatcher::getLandMeshManager())
        {
          if (auto hmapHandler = lmeshManager->getHmapHandler())
          {
            auto &cam = cameraHndl.ref();
            HeightmapFrustumCullingInfo fi{
              .world_pos = cam.cameraWorldPos,
              .water_level = hmapHandler->getPreparedWaterLevel(),
              .frustum = cam.jitterFrustum,
              .metrics =
                {
                  .distanceScale = proj_to_distance_scale(cam.noJitterProjTm),
                  .maxRelativeTexelTess = 0,
                  .heightRelativeErr = 0.01f,
                },
            };
            grass_render.fastGrass.prepare(grassPrepDataHndl.ref(), cam.jitterGlobtm, *hmapHandler, fi);
          }
        }
      });
    };
  });

  fastGrassRenderNode = ns.registerNode("fast_grass_render_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    // Grass is likely to be occluded by rendInsts and other statics
    // except for the ground, so we want it to run as late as possible.
    registry.setPriority(dafg::PRIO_AS_LATE_AS_POSSIBLE);

    render_to_gbuffer(registry).vrsRate(VRS_RATE_TEXTURE_NAME);
    request_common_opaque_state(registry);
    const auto grassPrepDataHndl = registry.readBlob<FastGrassRenderer::PreparedData>("gatheredInfo").handle();

    return [grassPrepDataHndl](const dafg::multiplexing::Index &multiplexing_index) {
      const camera_in_camera::ApplyMasterState camcam{multiplexing_index};
      fast_grass_baker::fast_grass_baker_on_render();
      if (!fast_grass_render.get())
        return;
      if (auto *lmeshManager = WRDispatcher::getLandMeshManager())
      {
        if (auto hmapHandler = lmeshManager->getHmapHandler())
        {
          get_grass_render_ecs_query(*g_entity_mgr,
            [&](GrassRenderer &grass_render) { grass_render.fastGrass.render(grassPrepDataHndl.ref(), *hmapHandler); });
        }
      }
    };
  });
}

static dafg::NodeHandle make_grass_per_camera_res_node()
{
  return dafg::register_node("grass_per_camera_res_node", DAFG_PP_NODE_SRC,
    [](dafg::Registry registry) { registry.createBlob<OrderingToken>("grass_generated_token"); });
}

static bool use_occlusion_culling()
{
  const bool hasGpuOcclusion = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("grassHasGpuOcclusion", false);
  const bool hasVisibilityPrepass = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("grassHasVisibilityPrepass", true);
  if (hasGpuOcclusion && hasVisibilityPrepass)
  {
    LOGERR_ONCE("Grass: GPU occlusion culling enabled. But it is incompatible with graphics/grassHasVisibilityPrepass:b=yes. Disable "
                "visibility prepass in the settings first and restart the game.");
    return false;
  }
  return use_gpu_occlusion && hasGpuOcclusion;
}

static dafg::NodeHandle make_grass_generation_node()
{
  return dafg::register_node("grass_generation_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
    auto camera = use_camera_in_camera(registry);
    auto cameraHndl = CameraViewShvars{camera}.bindViewVecs().toHandle();
    auto cameraHistory = read_history_camera_in_camera(registry).handle();

    auto hzbHndl = registry.readTexture("reprojected_hzb").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    registry.readBlob<int>("reprojected_hzb_mip_count").bindToShaderVar("reprojected_hzb_depth_mip_count");

    registry.modifyBlob<OrderingToken>("grass_generated_token");

    return [cameraHndl, cameraHistory, hzbHndl](const dafg::multiplexing::Index &multiplexing_index) {
      camera_in_camera::ApplyPostfxState camcam{multiplexing_index, cameraHndl.ref()};

      const bool useOcclusion = use_occlusion_culling();
      ShaderGlobal::set_int(var::grass_use_hzb_occlusion, useOcclusion ? 1 : 0);
      ShaderGlobal::set_texture(var::reprojected_hzb.get_var_id(), useOcclusion ? hzbHndl.get() : nullptr);

      const CameraParams &camera = cameraHndl.ref();
      const bool isMainView = multiplexing_index.subCamera == 0;

      if (isMainView)
        grass_prepare_per_camera(camera);

      grass_prepare_per_view(camera, cameraHistory.ref(), isMainView);

      ShaderGlobal::set_texture(var::reprojected_hzb.get_var_id(), nullptr);
    };
  });
}

ECS_ON_EVENT(on_appear, ChangeRenderFeatures)
ECS_REQUIRE(const GrassRenderer &grass_render)
void init_grass_render_es(const ecs::Event &evt,
  ecs::EntityManager &manager,
  dafg::NodeHandle &grass_per_camera_res_node,
  dafg::NodeHandle &grass_generation_node,
  dafg::NodeHandle &grass_node)
{
  if (auto *changedFeatures = evt.cast<ChangeRenderFeatures>())
  {
    if (!changedFeatures->isFeatureChanged(FeatureRenderFlags::CAMERA_IN_CAMERA))
      return;
    else
      init_grass_render_ecs_query(manager,
        [&](GrassRenderer &grass_render) { grass_render.toggleCameraInCamera(changedFeatures->hasFeature(CAMERA_IN_CAMERA)); });
  }

  grass_per_camera_res_node = make_grass_per_camera_res_node();
  grass_generation_node = make_grass_generation_node();

  auto decoNs = dafg::root() / "opaque" / "decorations";
  grass_node = decoNs.registerNode("fullres_grass_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.readBlob<OrderingToken>("grass_generated_token").optional();

    auto grassNs = registry.root() / "burnt_grass";
    grassNs.readBlob<OrderingToken>("burnt_grass_prepared_token").optional();

    shaders::OverrideState st;
    const uint32_t zWrite = prepass ? shaders::OverrideState::Z_WRITE_DISABLE : 0;
    st.set(shaders::OverrideState::Z_FUNC | zWrite);
    st.zFunc = CMPF_EQUAL;

    auto cameraHndl = use_camera_in_camera(registry).handle();
    registry.requestState().allowWireframe().setFrameBlock("global_frame").enableOverride(st);

    render_to_gbuffer(registry).vrsRate(VRS_RATE_TEXTURE_NAME);

    if (grass_supports_visibility_prepass())
      registry.read("grass_visibility_resolved").blob<OrderingToken>();

    // For grass billboards
    registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");

    return [cameraHndl](const dafg::multiplexing::Index &multiplexing_index) {
      const camera_in_camera::ApplyMasterState camcam{
        multiplexing_index, prepass ? camera_in_camera::OpaqueFlags::NoStencil : camera_in_camera::OpaqueFlags{}};

      const GrassView gv = multiplexing_index.subCamera == 0 ? GrassView::Main : GrassView::CameraInCamera;
      RenderPrecise renderPrecise(cameraHndl.ref().viewTm, cameraHndl.ref().cameraWorldPos);
      render_grass(gv);
    };
  });
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraNodeConstruction)
static void create_grass_prepass_nodes_es(const OnCameraNodeConstruction &evt)
{
  // Grass is not a static thing, but we want a grass prepass before we
  // start rendering the ground, which is static, because grass
  // occludes a lot of the ground.
  if (!evt.hasOpaquePrepass)
    return;

  auto ns = dafg::root() / "opaque" / "statics";

  evt.nodes->push_back(ns.registerNode("grass_prepass_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    // Grass is likely to be occluded by rendInsts and other statics
    // except for the ground, so we want it to run as late as possible.
    registry.setPriority(dafg::PRIO_AS_LATE_AS_POSSIBLE);

    render_to_gbuffer_prepass(registry);
    auto [cameraHndl, _] = request_common_opaque_state(registry);

    registry.readBlob<OrderingToken>("grass_generated_token").optional();
    registry.create("grass_prepass_done").blob<OrderingToken>();

    return [cameraHndl = cameraHndl](const dafg::multiplexing::Index &multiplexing_index) {
      const camera_in_camera::ApplyMasterState camcam{multiplexing_index};
      const GrassView gv = multiplexing_index.subCamera == 0 ? GrassView::Main : GrassView::CameraInCamera;
      RenderPrecise renderPrecise(cameraHndl.ref().viewTm, cameraHndl.ref().cameraWorldPos);
      render_grass_prepass(gv);
    };
  }));

  if (grass_supports_visibility_prepass())
  {
    // TODO: grass needs to be unified between WT&DNG, the nodes should move into
    // gamesLibs and we should replace the stupid tokens with proper buffers that
    // we are filling in & reading from.
    evt.nodes->push_back(ns.registerNode("grass_visibility_pass_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      registry.read("grass_prepass_done").blob<OrderingToken>();

      registry.allowAsyncPipelines().requestRenderPass().depthRo("gbuf_depth").vrsRate(VRS_RATE_TEXTURE_NAME);
      auto [cameraHndl, _] = request_common_opaque_state(registry);

      registry.create("grass_visibility_prepared").blob<OrderingToken>();

      return [cameraHndl = cameraHndl](const dafg::multiplexing::Index &multiplexing_index) {
        const camera_in_camera::ApplyMasterState camcam{multiplexing_index};
        const GrassView gv = multiplexing_index.subCamera == 0 ? GrassView::Main : GrassView::CameraInCamera;
        RenderPrecise renderPrecise(cameraHndl.ref().viewTm, cameraHndl.ref().cameraWorldPos);
        render_grass_visibility_pass(gv);
      };
    }));

    evt.nodes->push_back(dafg::root().registerNode("grass_visibility_resolve", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      (registry.root() / "opaque" / "statics").read("grass_visibility_prepared").blob<OrderingToken>();
      registry.create("grass_visibility_resolved").blob<OrderingToken>();

      return [](const dafg::multiplexing::Index &multiplexing_index) {
        const GrassView gv = multiplexing_index.subCamera == 0 ? GrassView::Main : GrassView::CameraInCamera;
        resolve_grass_visibility(gv);
      };
    }));
  }
}
