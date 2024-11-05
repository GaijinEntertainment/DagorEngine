// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "frameGraphNodes.h"

#include <render/daBfg/bfg.h>
#include <render/fx/fx.h>
#include <render/fx/fxRenderTags.h>
#include <render/world/defaultVrsSettings.h>
#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include <shaders/dag_shaderBlock.h>
#include <math/dag_TMatrix4.h>


dabfg::NodeHandle makeAcesFxUpdateNode()
{
  return dabfg::register_node("acesfx_update_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    auto depthHndl = registry.readTextureHistory("far_downsampled_depth")
                       .atStage(dabfg::Stage::COMPUTE)
                       .bindToShaderVar("effects_depth_tex")
                       .optional()
                       .handle();
    registry.read("far_downsampled_depth_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("effects_depth_tex_samplerstate")
      .optional();

    // TODO: When we stop using flowps2 we can remove downsampled_normals history
    auto normalsHndl = registry.readTextureHistory("downsampled_normals")
                         .atStage(dabfg::Stage::COMPUTE)
                         .bindToShaderVar("downsampled_normals")
                         .optional()
                         .handle();

    registry.create("acesfx_update_token", dabfg::History::No).blob<OrderingToken>();

    // Virtual's dep to move this node further in frame (since it waits for async jobs completion)
    (registry.root() / "opaque" / "dynamics").read("main_dynamics_update_token").blob<OrderingToken>();

    auto cameraParamsHndl = registry.readBlob<CameraParams>("current_camera").handle();
    return [depthHndl, normalsHndl, cameraParamsHndl](dabfg::multiplexing::Index multiplexing_index) {
      bool isFirstIteration = multiplexing_index == dabfg::multiplexing::Index{};
      if (!isFirstIteration)
        return;

      if (ManagedTexView downsampledDepth = depthHndl.view())
        acesfx::setDepthTex(downsampledDepth.getTex2D());

      if (const BaseTexture *downsampledNormals = normalsHndl.get())
        acesfx::setNormalsTex(downsampledNormals);

      // acesfx::finish_update dispatch compute shaders.
      acesfx::finish_update(cameraParamsHndl.ref().jitterGlobtm);
    };
  });
}

dabfg::NodeHandle makeAcesFxOpaqueNode()
{
  auto decoNs = dabfg::root() / "opaque" / "decorations";

  return decoNs.registerNode("acesfx_opaque_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.read("acesfx_update_token").blob<OrderingToken>();

    auto state = registry.requestState().allowWireframe().setFrameBlock("global_frame");

    auto pass = render_to_gbuffer(registry);

    use_default_vrs(pass, state);

    return [globalFrameBlockId = ShaderGlobal::getBlockId("global_frame")]() { acesfx::renderToGbuffer(globalFrameBlockId); };
  });
}
