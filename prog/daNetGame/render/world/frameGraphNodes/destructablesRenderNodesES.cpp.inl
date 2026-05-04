// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <render/daFrameGraph/daFG.h>
#include <render/renderEvent.h>
#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/defaultVrsSettings.h>
#include <render/world/dynModelRenderPass.h>
#include <render/world/dynModelRenderer.h>
#include <render/world/cameraViewVisibilityManager.h>
#include <gamePhys/phys/destructableObject.h>
#include <scene/dag_occlusion.h>
#include "frameGraphNodes.h"

dafg::NodeHandle makeDestructablesDepthPrepassNode()
{
  auto ns = dafg::root() / "opaque" / "dynamics";
  return ns.registerNode("destructables_depth_prepass", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    render_to_gbuffer_prepass(registry).vrsRate(VRS_RATE_TEXTURE_NAME);
    auto [cameraHndl, stateRequest] = request_common_opaque_state(registry);

    return [cameraHndl = cameraHndl, dyn_model_render_passVarId = ::get_shader_variable_id("dyn_model_render_pass")](
             const dafg::multiplexing::Index &multiplexing_index) {
      ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Depth));
      const auto &camera = cameraHndl.ref();
      const camera_in_camera::ApplyMasterState camcam{multiplexing_index};

      Occlusion *occlusion = camera.jobsMgr->getOcclusion();
      destructables::render_depth_prepass(camera.viewItm.getcol(3), camera.viewTm, camera.jitterFrustum, occlusion);
    };
  });
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraNodeConstruction)
static void create_destructables_depth_prepass_node_es(const OnCameraNodeConstruction &evt)
{
  if (evt.hasOpaquePrepass)
    evt.nodes->push_back(makeDestructablesDepthPrepassNode());
}
