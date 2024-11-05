// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daBfg/bfg.h>
#include <rendInst/rendInstGenRender.h>
#include <rendInst/rendInstExtraRender.h>
#include <render/rendererFeatures.h>
#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/frameGraphNodes/prevFrameTexRequests.h>
#include <render/world/renderPrecise.h>
#include <util/dag_convar.h>
#include "frameGraphNodes.h"
#include <shaders/dag_shaderBlock.h>

#define INSIDE_RENDERER 1
#include "render/world/private_worldRenderer.h"

extern ConVarT<bool, false> sort_transparent_riex_instances;

// TODO: this should be moved to the rendinst gamelib later down the line
dabfg::NodeHandle makeRendinstTransparentNode(bool is_early)
{
  auto nodeNs = dabfg::root() / "transparent" / (is_early ? "far" : "close");
  return nodeNs.registerNode("rendinst_node", DABFG_PP_NODE_SRC, [is_early](dabfg::Registry registry) {
    // We use depthRw to allow z-writes for:
    // 1. Transparent rendinsts that mimic refraction (they draw distorted read_prev_frame_tex).
    // 2. Opaque glasses on thermal vision.
    if (!renderer_has_feature(FeatureRenderFlags::FORWARD_RENDERING))
    {
      registry.allowAsyncPipelines().requestRenderPass().color({"color_target"}).depthRw("depth");
      use_current_camera(registry);
    }
    else
    {
      render_transparency(registry);
    }
    registry.setPriority(TRANSPARENCY_NODE_PRIORITY_RENDINST);

    read_prev_frame_tex(registry);

    registry.requestState().allowWireframe().setFrameBlock("global_frame");
    int rendinstTransSceneBlockid = ShaderGlobal::getBlockId("rendinst_trans_scene");

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();
    auto visibilityHndl = registry.readBlob<RiGenVisibility *>("rendinst_main_visibility").handle();
    return [cameraHndl, strmCtxHndl, visibilityHndl, rendinstTransSceneBlockid, is_early] {
      RenderPrecise renderPrecise(cameraHndl.ref().viewTm, cameraHndl.ref().cameraWorldPos);
      SCENE_LAYER_GUARD(rendinstTransSceneBlockid);

      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      PartitionSphere partitionSphere = wr.getTransparentPartitionSphere();

      if (is_early && partitionSphere.status == PartitionSphere::Status::NO_SPHERE)
        return;

      if (sort_transparent_riex_instances)
      {
        rendinst::render::renderSortedTransparentRiExtraInstances(*visibilityHndl.ref(), strmCtxHndl.ref(), is_early);
      }
      else
      {
        rendinst::render::renderRIGen(rendinst::RenderPass::Normal, visibilityHndl.ref(), cameraHndl.ref().viewItm,
          rendinst::LayerFlag::Transparent, rendinst::OptimizeDepthPass::No, 1, rendinst::AtestStage::All, nullptr, strmCtxHndl.ref());
      }
    };
  });
}

dabfg::NodeHandle makeRendinstTransparentNode() { return makeRendinstTransparentNode(false); }

dabfg::NodeHandle makeRendinstTransparentEarlyNode() { return makeRendinstTransparentNode(true); }