// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "frameGraphNodes.h"

#include <render/daFrameGraph/daFG.h>
#include <render/denoiser/denoiser_api.h>
#include <3d/dag_texStreamingContext.h>

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>

#include <render/renderEvent.h>
#include <render/world/wrDispatcher.h>
#include <render/cameraParams.h>
#include <render/world/frameGraphNodes/prevFrameTexRequests.h>
#include <render/world/frameGraphHelpers.h>
#include <triangleSizeDebug/triangleSizeDebug.h>

namespace var
{
static ShaderVariableInfo rendinst_transparent_triangle_size_debug("rendinst_transparent_triangle_size_debug", true);
}

dafg::NodeHandle makeTransparentSceneLateNode(MainNodeRenderPass mode)
{
  bool debugTriangle = mode == MainNodeRenderPass::TriangleSizeDebugPass;
  auto ns = debugTriangle ? dafg::root() / "tringle_size_debug" / "transparent" : dafg::root();
  return ns.registerNode("transparent_scene_late_node", DAFG_PP_NODE_SRC, [debugTriangle](dafg::Registry registry) {
    read_all_camera_view_tokens(registry.currNameSpace(), "after_heat_haze_render_particles_nodes");

    if (debugTriangle)
      registry.requestRenderPass().color({"triangle_size_tex"}).depthReadTestOnly("depth_for_transparency");
    else
      registry.requestRenderPass()
        .color({"target_for_transparency"})
        .depthReadTestAndSample("depth_for_transparency", {"effects_depth_tex"});
    registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    use_current_camera(registry);

    registry.requestState().allowWireframe().setFrameBlock("global_frame");
    denoiser::read_shadow_output(registry);

    auto prevFrameTexHndl = read_prev_frame_tex(registry);
    auto currentTexCtxHndl = registry.read("tex_ctx").blob<TexStreamingContext>().handle();
    auto cameraHndl = registry.read("current_camera").blob<CameraParams>().handle();

    return [prevFrameTexHndl, currentTexCtxHndl, cameraHndl, debugTriangle] {
      if (!g_entity_mgr)
        return;

      // Note: we use CSM inside this node and used to explicitly call
      // CascadeShadows::setCascadesToShader, but this node is always
      // run after some other type of transparent stuff, which already
      // calls that function, so it's probably ok to omit calling it here.

      auto prevFrameTex = prevFrameTexHndl.has_value() ? prevFrameTexHndl.value().get() : nullptr;
      STATE_GUARD_0(ShaderGlobal::set_int(var::rendinst_transparent_triangle_size_debug, VALUE), debugTriangle);
      g_entity_mgr->broadcastEventImmediate(
        RenderLateTransEvent(cameraHndl.ref().viewTm, cameraHndl.ref().cameraWorldPos, currentTexCtxHndl.ref(), prevFrameTex));
    };
  });
}

extern void render_late_trans(const TMatrix &view_tm);
extern void render_late_trans_cockpit(const TMatrix &view_tm);

static void request_common_late_trans_state(dafg::Registry &registry, const bool debug_triangle)
{
  registry.orderMeAfter("transparent_scene_late_node");
  (registry.root() / "transparent" / "close")
    .read("glass_depth_for_merge")
    .texture()
    .atStage(dafg::Stage::POST_RASTER)
    .bindToShaderVar("glass_hole_mask")
    .optional();
  (registry.root() / "transparent" / "close")
    .read("glass_target")
    .texture()
    .atStage(dafg::Stage::POST_RASTER)
    .bindToShaderVar("glass_rt_reflection")
    .optional();
  denoiser::read_shadow_output(registry);

  if (debug_triangle)
    registry.requestRenderPass().color({"triangle_size_tex"}).depthReadTestOnly("depth_for_transparency");
  else
    registry.requestRenderPass().color({"target_for_transparency"}).depthReadTestOnly("depth_for_transparency");

  // implicitly relies on multiplexed glass resources
  registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);

  registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
  read_prev_frame_tex(registry);

  if (!debug_triangle)
    request_and_bind_scope_lens_reflections(registry);

  registry.allowAsyncPipelines();

  registry.requestState().allowWireframe().setFrameBlock("global_frame");
}

dafg::NodeHandle makeLateTransOrderingNode()
{
  return dafg::root().registerNode("late_trans_ordering_node", DAFG_PP_NODE_SRC,
    [](dafg::Registry registry) { registry.createBlob<OrderingToken>("after_late_trans_token"); });
}

dafg::NodeHandle makeLateTrans(MainNodeRenderPass mode)
{
  bool debugTriangle = mode == MainNodeRenderPass::TriangleSizeDebugPass;
  auto ns = debugTriangle ? dafg::root() / "tringle_size_debug" / "transparent" : dafg::root();

  return ns.registerNode("late_trans_node", DAFG_PP_NODE_SRC, [debugTriangle](dafg::Registry registry) {
    request_common_late_trans_state(registry, debugTriangle);
    auto cameraHndl =
      read_camera_in_camera(registry).bindAsView<&CameraParams::viewTm>().bindAsProj<&CameraParams::jitterProjTm>().handle();

    registry.modifyBlob<OrderingToken>("after_late_trans_token");

    return [cameraHndl, debugTriangle](const dafg::multiplexing::Index multiplex_index) {
      camera_in_camera::ApplyMasterState camcam(multiplex_index);

      STATE_GUARD_0(ShaderGlobal::set_int(var::rendinst_transparent_triangle_size_debug, VALUE), debugTriangle);
      render_late_trans(cameraHndl.ref().viewTm);
    };
  });
}

dafg::NodeHandle makeLateTransCockpit(MainNodeRenderPass mode)
{
  bool debugTriangle = mode == MainNodeRenderPass::TriangleSizeDebugPass;
  auto ns = debugTriangle ? dafg::root() / "tringle_size_debug" / "transparent" : dafg::root();

  return ns.registerNode("late_trans_cockpit_node", DAFG_PP_NODE_SRC, [debugTriangle](dafg::Registry registry) {
    request_common_late_trans_state(registry, debugTriangle);
    auto cameraHndl = registry.readBlob<CameraParams>("current_cockpit_camera")
                        .bindAsView<&CameraParams::viewTm>()
                        .bindAsProj<&CameraParams::jitterProjTm>()
                        .handle();

    registry.readBlob<OrderingToken>("after_late_trans_token");

    return [cameraHndl, debugTriangle](const dafg::multiplexing::Index multiplex_index) {
      if (multiplex_index.subCamera != 0)
        return;

      STATE_GUARD_0(ShaderGlobal::set_int(var::rendinst_transparent_triangle_size_debug, VALUE), debugTriangle);
      render_late_trans_cockpit(cameraHndl.ref().viewTm);
    };
  });
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraNodeConstruction)
static void create_transparent_scene_late_nodes_es(const OnCameraNodeConstruction &evt)
{
  evt.nodes->push_back(makeLateTransOrderingNode());
  evt.nodes->push_back(makeTransparentSceneLateNode(MainNodeRenderPass::MainColorPass));
  evt.nodes->push_back(makeLateTrans(MainNodeRenderPass::MainColorPass));
  evt.nodes->push_back(makeLateTransCockpit(MainNodeRenderPass::MainColorPass));
}

ECS_TAG(render, dev)
static void create_transparent_scene_triangle_debug_es(const CreateTriangleDebugNodes &evt)
{
  if (!evt.systems.isTransparent)
    return;

  evt.nodes->push_back(makeTransparentSceneLateNode(MainNodeRenderPass::TriangleSizeDebugPass));
  evt.nodes->push_back(makeLateTrans(MainNodeRenderPass::TriangleSizeDebugPass));
  evt.nodes->push_back(makeLateTransCockpit(MainNodeRenderPass::TriangleSizeDebugPass));
}