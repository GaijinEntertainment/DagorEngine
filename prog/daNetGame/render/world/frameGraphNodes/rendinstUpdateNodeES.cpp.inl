// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <rendInst/rendInstGen.h>
#include <render/daFrameGraph/daFG.h>

#include <render/cameraParams.h>
#include <render/renderEvent.h>
#include <render/world/cameraViewVisibilityManager.h>
#include <render/world/frameGraphHelpers.h>
#include "frameGraphNodes.h"


dafg::NodeHandle makeRendinstUpdateNode()
{
  auto rootNs = dafg::root();
  return rootNs.registerNode("ri_update_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    // RI cell buffer may be outdated and need to update it before ri rendering to avoid splitting render pass
    registry.createBlob<OrderingToken>("ri_update_token");
    return [] { rendinst::updateHeapVb(); };
  });
}

static dafg::NodeHandle makeRiExtraLodsByDistanceNode()
{
  return dafg::root().registerNode("ri_extra_lods_by_distance_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    // constraint: ri_opaque_rendered_token comes from a camera-in-camera multiplexed node, daFG rejects less multiplexed readers
    registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
    // Dependency is purely for optimization, the job can run at any time
    registry.readBlob<OrderingToken>("bvh_ready_token").optional();
    (registry.root() / "opaque" / "statics").readBlob<OrderingToken>("ri_opaque_rendered_token");
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    return [cameraHndl](const dafg::multiplexing::Index &multiplexing_index) {
      if (multiplexing_index != dafg::multiplexing::Index{})
        return;
      const CameraParams &camera = cameraHndl.ref();
      camera.jobsMgr->startLodsByDistance(camera.viewItm.getcol(3));
    };
  });
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraNodeConstruction)
static void create_rendinst_update_node_es(const OnCameraNodeConstruction &evt)
{
  evt.nodes->push_back(makeRendinstUpdateNode());
  evt.nodes->push_back(makeRiExtraLodsByDistanceNode());
}
