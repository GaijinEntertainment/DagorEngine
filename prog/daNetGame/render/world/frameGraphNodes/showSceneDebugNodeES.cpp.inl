// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daEditorE/editorCommon/inGameEditor.h>

#include <render/daFrameGraph/daFG.h>
#include <render/renderEvent.h>
#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/wrDispatcher.h>

#include "frameGraphNodes.h"

dafg::NodeHandle makeUpsampleDepthForSceneDebugNode()
{
  return dafg::register_node("upsample_depth_for_scene_debug_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto downsampledDepthHndl =
      registry.readTexture("depth_for_postfx").atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    auto depthHndl = registry.create("depth_for_scene_debug")
                       .texture({get_gbuffer_depth_format() | TEXCF_RTARGET, registry.getResolution<2>("display")})
                       .atStage(dafg::Stage::PS)
                       .useAs(dafg::Usage::DEPTH_ATTACHMENT)
                       .handle();

    return [depthHndl, downsampledDepthHndl, renderer = PostFxRenderer("copy_depth")] {
      SCOPE_RENDER_TARGET;
      d3d::settex(15, downsampledDepthHndl.get());
      d3d::set_sampler(STAGE_PS, 15, d3d::request_sampler({}));
      d3d::set_render_target({depthHndl.get(), 0, 0}, DepthAccess::RW, {});
      renderer.render();
      d3d::settex(15, nullptr);
    };
  });
}

dafg::NodeHandle makeShowSceneDebugNode()
{
  return dafg::register_node("show_scene_debug_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto debugNs = registry.root() / "debug";
    auto cameraParamsHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto colorHndl = debugNs.modifyTexture("target_for_debug").atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT).handle();
    auto depthHndl = registry
                       .readTexture("depth_for_postfx") // FIXME: There should be modify for the depth, because it is used as RW depth
                       .atStage(dafg::Stage::PS)
                       .useAs(dafg::Usage::DEPTH_ATTACHMENT)
                       .handle();
    auto upsampledDepthHndl =
      registry.readTexture("depth_for_scene_debug").atStage(dafg::Stage::PS).useAs(dafg::Usage::DEPTH_ATTACHMENT).optional().handle();
    registry.readTexture("far_downsampled_depth").atStage(dafg::Stage::PS).bindToShaderVar("downsampled_far_depth_tex");
    return [cameraParamsHndl, colorHndl, depthHndl, upsampledDepthHndl] {
      extern void render_scene_debug(BaseTexture * target, BaseTexture * depth, const CameraParams &camera);

      BaseTexture *depth = upsampledDepthHndl.get() ? upsampledDepthHndl.get() : depthHndl.get();
      BaseTexture *color = colorHndl.get();

      TextureInfo depthInfo;
      depth->getinfo(depthInfo);
      TextureInfo colorInfo;
      color->getinfo(colorInfo);

      const bool canUseDepth = depthInfo.w == colorInfo.w && depthInfo.h == colorInfo.h;

      render_scene_debug(color, canUseDepth ? depth : nullptr, cameraParamsHndl.ref());
    };
  });
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraNodeConstruction)
static void create_scene_debug_nodes_es(const OnCameraNodeConstruction &evt)
{
#if DAGOR_DBGLEVEL == 0 && _TARGET_PC
  if (has_in_game_editor())
#elif DAGOR_DBGLEVEL == 0
  if constexpr (false)
#endif
  {
    if (WRDispatcher::isUpsampling())
      evt.nodes->push_back(makeUpsampleDepthForSceneDebugNode());
    evt.nodes->push_back(makeShowSceneDebugNode());
  }
}
