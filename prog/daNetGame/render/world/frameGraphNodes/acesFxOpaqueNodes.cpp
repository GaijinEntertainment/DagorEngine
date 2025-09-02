// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "frameGraphNodes.h"

#include <render/daFrameGraph/daFG.h>
#include <render/fx/fx.h>
#include <render/fx/fxRenderTags.h>
#include <render/world/defaultVrsSettings.h>
#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/cameraViewVisibilityManager.h>
#include <shaders/dag_shaderBlock.h>
#include <math/dag_TMatrix4.h>


dafg::NodeHandle makeAcesFxUpdateNode()
{
  return dafg::register_node("acesfx_update_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto depthHndl = registry.readTextureHistory("far_downsampled_depth")
                       .atStage(dafg::Stage::COMPUTE)
                       .bindToShaderVar("effects_depth_tex")
                       .optional()
                       .handle();
    registry.read("far_downsampled_depth_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("effects_depth_tex_samplerstate")
      .optional();

    // TODO: When we stop using flowps2 we can remove downsampled_normals history
    auto normalsHndl = registry.readTextureHistory("downsampled_normals")
                         .atStage(dafg::Stage::COMPUTE)
                         .bindToShaderVar("downsampled_normals")
                         .optional()
                         .handle();

    registry.create("acesfx_update_token", dafg::History::No).blob<OrderingToken>();

    // Virtual's dep to move this node further in frame (since it waits for async jobs completion)
    (registry.root() / "opaque" / "dynamics").read("acesfx_start_update_token").blob<OrderingToken>();

    auto cameraParamsHndl = registry.readBlob<CameraParams>("current_camera").handle();
    return [depthHndl, normalsHndl, cameraParamsHndl](dafg::multiplexing::Index multiplexing_index) {
      bool isFirstIteration = multiplexing_index == dafg::multiplexing::Index{};
      if (!isFirstIteration)
        return;

      if (ManagedTexView downsampledDepth = depthHndl.view())
        acesfx::setDepthTex(downsampledDepth.getTex2D());

      if (const BaseTexture *downsampledNormals = normalsHndl.get())
        acesfx::setNormalsTex(downsampledNormals);

      const bool useOcclusion = !camera_in_camera::is_lens_render_active();

      // acesfx::finish_update dispatch compute shaders.
      Occlusion *occlusion = useOcclusion ? cameraParamsHndl.ref().jobsMgr->getOcclusion() : nullptr;
      acesfx::finish_update(cameraParamsHndl.ref().jitterGlobtm, occlusion);
    };
  });
}

dafg::NodeHandle makeAcesFxOpaqueNode()
{
  auto decoNs = dafg::root() / "opaque" / "decorations";

  return decoNs.registerNode("acesfx_opaque_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.read("acesfx_update_token").blob<OrderingToken>();

    registry.requestState().allowWireframe().setFrameBlock("global_frame");

    render_to_gbuffer(registry).vrsRate(VRS_RATE_TEXTURE_NAME);

    return [globalFrameBlockId = ShaderGlobal::getBlockId("global_frame")]() { acesfx::renderToGbuffer(globalFrameBlockId); };
  });
}
