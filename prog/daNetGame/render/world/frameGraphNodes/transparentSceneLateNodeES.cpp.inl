// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "frameGraphNodes.h"

#include <render/daFrameGraph/daFG.h>
#include <3d/dag_texStreamingContext.h>

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>

#include <render/renderEvent.h>
#include <render/world/wrDispatcher.h>
#include <render/world/cameraParams.h>
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
    if (debugTriangle)
      registry.requestRenderPass().color({"triangle_size_tex"}).depthRo("depth_for_transparency");
    else
      registry.requestRenderPass()
        .color({"target_for_transparency"})
        .depthRoAndBindToShaderVars("depth_for_transparency", {"effects_depth_tex"});
    registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    use_current_camera(registry);

    registry.requestState().allowWireframe().setFrameBlock("global_frame");

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

extern void render_mainhero_trans(const TMatrix &view_tm);

dafg::NodeHandle makeMainHeroTransNode(MainNodeRenderPass mode)
{
  bool debugTriangle = mode == MainNodeRenderPass::TriangleSizeDebugPass;
  auto ns = debugTriangle ? dafg::root() / "tringle_size_debug" / "transparent" : dafg::root();
  return ns.registerNode("main_hero_trans", DAFG_PP_NODE_SRC, [debugTriangle](dafg::Registry registry) {
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

    if (debugTriangle)
      registry.requestRenderPass().color({"triangle_size_tex"}).depthRo("depth_for_transparency");
    else
      registry.requestRenderPass().color({"target_for_transparency"}).depthRo("depth_for_transparency");

    registry.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewTm>().bindAsProj<&CameraParams::jitterProjTm>();
    registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");


    if (!debugTriangle)
      request_and_bind_scope_lens_reflections(registry);

    registry.allowAsyncPipelines();

    registry.requestState().allowWireframe().setFrameBlock("global_frame");

    registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
    auto cameraHndl = read_camera_in_camera(registry).handle();

    return [cameraHndl, debugTriangle](const dafg::multiplexing::Index multiplex_index) {
      const camera_in_camera::ApplyMasterState camcam{multiplex_index};
      STATE_GUARD_0(ShaderGlobal::set_int(var::rendinst_transparent_triangle_size_debug, VALUE), debugTriangle);
      render_mainhero_trans(cameraHndl.ref().viewTm);
    };
  });
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraNodeConstruction)
static void create_transparent_scene_late_nodes_es(const OnCameraNodeConstruction &evt)
{
  evt.nodes->push_back(makeTransparentSceneLateNode(MainNodeRenderPass::MainColorPass));
  evt.nodes->push_back(makeMainHeroTransNode(MainNodeRenderPass::MainColorPass));
}

ECS_TAG(render, dev)
static void create_transparent_scene_triangle_debug_es(const CreateTriangleDebugNodes &evt)
{
  if (!evt.systems.isTransparent)
    return;

  evt.nodes->push_back(makeTransparentSceneLateNode(MainNodeRenderPass::TriangleSizeDebugPass));
  evt.nodes->push_back(makeMainHeroTransNode(MainNodeRenderPass::TriangleSizeDebugPass));
}