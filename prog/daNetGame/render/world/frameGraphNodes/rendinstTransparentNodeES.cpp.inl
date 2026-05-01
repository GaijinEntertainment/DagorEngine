// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <render/daFrameGraph/daFG.h>
#include <rendInst/rendInstGenRender.h>
#include <rendInst/rendInstExtraRender.h>

#include <render/renderEvent.h>
#include <render/rendererFeatures.h>
#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/frameGraphNodes/prevFrameTexRequests.h>
#include <render/world/renderPrecise.h>
#include <util/dag_convar.h>
#include "frameGraphNodes.h"
#include <shaders/dag_shaderBlock.h>
#include <triangleSizeDebug/triangleSizeDebug.h>

#define INSIDE_RENDERER 1
#include "render/world/private_worldRenderer.h"


namespace var
{
static ShaderVariableInfo globtm_no_ofs_psf_0("globtm_no_ofs_psf_0", true);
static ShaderVariableInfo globtm_no_ofs_psf_1("globtm_no_ofs_psf_1", true);
static ShaderVariableInfo globtm_no_ofs_psf_2("globtm_no_ofs_psf_2", true);
static ShaderVariableInfo globtm_no_ofs_psf_3("globtm_no_ofs_psf_3", true);
static ShaderVariableInfo rendinst_transparent_triangle_size_debug("rendinst_transparent_triangle_size_debug", true);
} // namespace var

extern ConVarT<bool, false> sort_transparent_riex_instances;

// TODO: this should be moved to the rendinst gamelib later down the line
dafg::NodeHandle makeRendinstTransparentNode(bool is_early, bool is_triangle_debug)
{
  auto nodeNs = is_triangle_debug ? dafg::root() / "tringle_size_debug" / "transparent"
                                  : dafg::root() / "transparent" / (is_early ? "far" : "close");
  return nodeNs.registerNode("rendinst_node", DAFG_PP_NODE_SRC, [is_early, is_triangle_debug](dafg::Registry registry) {
    // We use depthRw to allow z-writes for:
    // 1. Transparent rendinsts that mimic refraction (they draw distorted read_prev_frame_tex).
    // 2. Opaque glasses on thermal vision.

    if (is_triangle_debug)
    {
      auto ns = registry.root() / "transparent" / (is_early ? "far" : "close");
      registry.requestRenderPass().depthRo(ns.readTexture("depth")).color({"triangle_size_tex"});
    }
    else
      registry.allowAsyncPipelines().requestRenderPass().color({"color_target"}).depthRw("depth");
    registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    auto cameraHndl = use_camera_in_camera(registry).handle();

    registry.setPriority(TRANSPARENCY_NODE_PRIORITY_RENDINST);

    read_prev_frame_tex(registry);
    registry.read("glass_depth_for_merge").texture().atStage(dafg::Stage::POST_RASTER).bindToShaderVar("glass_hole_mask").optional();
    registry.read("glass_target").texture().atStage(dafg::Stage::POST_RASTER).bindToShaderVar("glass_rt_reflection").optional();
    read_gbuffer_material_only(registry);

    registry.requestState().allowWireframe().setFrameBlock("global_frame");
    int rendinstTransSceneBlockid = ShaderGlobal::getBlockId("rendinst_trans_scene");

    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();
    return [cameraHndl, strmCtxHndl, rendinstTransSceneBlockid, is_early, is_triangle_debug](
             const dafg::multiplexing::Index &multiplexing_index) {
      RenderPrecise renderPrecise(cameraHndl.ref().viewTm, cameraHndl.ref().cameraWorldPos);

      TMatrix4 globTm = cameraHndl.ref().viewRotJitterProjTm.transpose();
      ShaderGlobal::set_float4(var::globtm_no_ofs_psf_0, Color4(globTm[0]));
      ShaderGlobal::set_float4(var::globtm_no_ofs_psf_1, Color4(globTm[1]));
      ShaderGlobal::set_float4(var::globtm_no_ofs_psf_2, Color4(globTm[2]));
      ShaderGlobal::set_float4(var::globtm_no_ofs_psf_3, Color4(globTm[3]));

      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      PartitionSphere partitionSphere = wr.getTransparentPartitionSphere();

      if (is_early && partitionSphere.status == PartitionSphere::Status::NO_SPHERE)
        return;

      const camera_in_camera::ApplyMasterState camcam{multiplexing_index};
      RiGenVisibility *riMainVisibility = cameraHndl.ref().jobsMgr->getRiMainVisibility();

      SCENE_LAYER_GUARD(rendinstTransSceneBlockid);
      STATE_GUARD_0(ShaderGlobal::set_int(var::rendinst_transparent_triangle_size_debug, VALUE), is_triangle_debug);

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

dafg::NodeHandle makeRendinstTransparentNode(MainNodeRenderPass mode)
{
  return makeRendinstTransparentNode(false, mode == MainNodeRenderPass::TriangleSizeDebugPass);
}

dafg::NodeHandle makeRendinstTransparentEarlyNode() { return makeRendinstTransparentNode(true, false); }


ECS_TAG(render)
ECS_ON_EVENT(OnCameraNodeConstruction)
static void create_transparent_nodes_es(const OnCameraNodeConstruction &evt)
{
  evt.nodes->push_back(makeRendinstTransparentNode(MainNodeRenderPass::MainColorPass));
}


ECS_TAG(render, dev)
static void create_transparent_triangle_debug_es(const CreateTriangleDebugNodes &evt)
{
  if (!evt.systems.isTransparent)
    return;
  evt.nodes->push_back(makeRendinstTransparentNode(MainNodeRenderPass::TriangleSizeDebugPass));
}