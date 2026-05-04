// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <render/viewVecs.h>
#include <render/daFrameGraph/daFG.h>

#include <render/renderEvent.h>
#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include "frameGraphNodes.h"

dafg::NodeHandle makeUnderWaterFogNode()
{
  return dafg::register_node("under_water_fog_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("darg_in_world_panels_trans_node");
    registry.requestRenderPass().color({"target_for_transparency"});
    registry.requestState().setFrameBlock("global_frame");
    auto underwaterHndl = registry.readBlob<bool>("is_underwater").handle();
    auto camera = registry.readBlob<CameraParams>("current_camera");
    CameraViewShvars{camera}.bindViewVecs();
    registry.readTexture("depth_for_transparency").atStage(dafg::Stage::PS).bindToShaderVar("depth_gbuf");
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("depth_gbuf_samplerstate");
    return [renderer = PostFxRenderer("underwater_fog"), underwaterHndl] {
      if (!underwaterHndl.ref())
        return;
      // todo: render underwater fog with volumeLoight if it is on
      renderer.render();
    };
  });
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraNodeConstruction)
static void create_under_water_fog_node_es(const OnCameraNodeConstruction &evt) { evt.nodes->push_back(makeUnderWaterFogNode()); }
