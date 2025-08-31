// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/daFG.h>
#include <render/cinematicMode.h>
#include <render/renderEvent.h>
#include <render/world/frameGraphHelpers.h>
#include <3d/dag_stereoIndex.h>
#include <ecs/core/entityManager.h>

#define INSIDE_RENDERER 1
#include "../private_worldRenderer.h"
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>

dafg::NodeHandle makeAfterWorldRenderNode()
{
  return dafg::register_node("after_world_render_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.multiplex(dafg::multiplexing::Mode::Viewport);
    registry.executionHas(dafg::SideEffects::External);
    registry.readBlob<IPoint2>("super_sub_pixels");
    auto beforeUINs = registry.root() / "before_ui";
    auto frameToPresentHndl =
      beforeUINs.readTexture("frame_done").atStage(dafg::Stage::POST_RASTER).useAs(dafg::Usage::COLOR_ATTACHMENT).handle();
    return [frameToPresentHndl, world_view_posVarId = ::get_shader_variable_id("world_view_pos")] {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      process_video_encoder(frameToPresentHndl.view().getTex2D());
      d3d::set_render_target(frameToPresentHndl.view().getTex2D(), 0);
      d3d::settm(TM_VIEW, wr.currentFrameCamera.viewTm);
      d3d::settm(TM_PROJ, (mat44f_cref)wr.currentFrameCamera.jitterProjTm);

      ShaderGlobal::set_color4(world_view_posVarId, wr.currentFrameCamera.viewItm.col[3], 1);

      g_entity_mgr->broadcastEventImmediate(AfterRenderWorld(wr.currentFrameCamera.noJitterPersp));

      // Paranoid render target reset, what if an event changed this global state?
      d3d::set_render_target(frameToPresentHndl.view().getTex2D(), 0);
    };
  });
}