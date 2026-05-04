// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/entitySystem.h>
#include <render/collimatorMoaCore/render.h>

#include <render/daFrameGraph/daFG.h>
#include <render/daFrameGraph/ecs/frameGraphNode.h>

#include <render/world/cameraParams.h>


ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void create_collimator_moa_lens_render_fg_node_es_event_handler(const ecs::Event &,
  dafg::NodeHandle &collimator_moa_render__lens_render_node)
{
  auto ns = dafg::root();
  collimator_moa_render__lens_render_node = ns.registerNode("collimator_moa_lens", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.requestRenderPass().color({"final_target_with_motion_blur"});

    registry.readTexture("depth_after_transparency").atStage(dafg::Stage::PS).bindToShaderVar("depth_gbuf");

    registry.readBlob<CameraParams>("current_camera")
      .bindAsView<&CameraParams::viewRotTm>()
      .bindAsProj<&CameraParams::noJitterProjTm>();

    auto displayResolution = registry.getResolution<2>("displayResolution");

    return [displayResolution]() { collimator_moa::render(displayResolution.get()); };
  });
}