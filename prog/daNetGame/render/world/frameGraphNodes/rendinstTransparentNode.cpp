// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/daFG.h>
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
dafg::NodeHandle makeRendinstTransparentNode(bool is_early)
{
  auto nodeNs = dafg::root() / "transparent" / (is_early ? "far" : "close");
  return nodeNs.registerNode("rendinst_node", DAFG_PP_NODE_SRC, [is_early](dafg::Registry registry) {
    // We use depthRw to allow z-writes for:
    // 1. Transparent rendinsts that mimic refraction (they draw distorted read_prev_frame_tex).
    // 2. Opaque glasses on thermal vision.

    const bool isMobile = renderer_has_feature(FeatureRenderFlags::FORWARD_RENDERING);
    if (!isMobile)
    {
      registry.allowAsyncPipelines().requestRenderPass().color({"color_target"}).depthRw("depth");
      registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    }
    auto cameraHndl = isMobile ? request_common_transparent_state(registry) : use_camera_in_camera(registry);

    registry.setPriority(TRANSPARENCY_NODE_PRIORITY_RENDINST);

    read_prev_frame_tex(registry);
    registry.read("glass_depth_for_merge").texture().atStage(dafg::Stage::POST_RASTER).bindToShaderVar("glass_hole_mask").optional();
    registry.read("glass_target").texture().atStage(dafg::Stage::POST_RASTER).bindToShaderVar("glass_rt_reflection").optional();

    registry.requestState().allowWireframe().setFrameBlock("global_frame");
    int rendinstTransSceneBlockid = ShaderGlobal::getBlockId("rendinst_trans_scene");

    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();
    return [cameraHndl, strmCtxHndl, rendinstTransSceneBlockid, is_early](const dafg::multiplexing::Index &multiplexing_index) {
      RenderPrecise renderPrecise(cameraHndl.ref().viewTm, cameraHndl.ref().cameraWorldPos);
      SCENE_LAYER_GUARD(rendinstTransSceneBlockid);

      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      PartitionSphere partitionSphere = wr.getTransparentPartitionSphere();

      if (is_early && partitionSphere.status == PartitionSphere::Status::NO_SPHERE)
        return;

      const camera_in_camera::ApplyMasterState camcam{multiplexing_index};
      RiGenVisibility *riMainVisibility = cameraHndl.ref().jobsMgr->getRiMainVisibility();

      if (sort_transparent_riex_instances)
      {
        rendinst::render::renderSortedTransparentRiExtraInstances(*riMainVisibility, strmCtxHndl.ref(), is_early);
      }
      else
      {
        rendinst::render::renderRIGen(rendinst::RenderPass::Normal, riMainVisibility, cameraHndl.ref().viewItm,
          rendinst::LayerFlag::Transparent, rendinst::OptimizeDepthPass::No, 1, rendinst::AtestStage::All, nullptr, strmCtxHndl.ref());
      }
    };
  });
}

dafg::NodeHandle makeRendinstTransparentNode() { return makeRendinstTransparentNode(false); }

dafg::NodeHandle makeRendinstTransparentEarlyNode() { return makeRendinstTransparentNode(true); }