// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <ecs/core/entitySystem.h>
#define INSIDE_RENDERER 1
#include <render/world/private_worldRenderer.h>
#include <render/renderEvent.h>
#include <rendInst/rendInstGenRender.h>
#include <render/world/renderPrecise.h>


static void rend_inst_decals_render_es(const OnRenderDecals &evt)
{
  if (auto wr = (const WorldRenderer *)get_world_renderer())
  {
    if (auto rendinst_main_visibility = wr->getRendInstMainVisibility())
    {
      TIME_D3D_PROFILE(rendinst_decals);
      RenderPrecise renderPrecise(evt.get<0>(), evt.get<2>());
      rendinst::render::renderRIGen(rendinst::RenderPass::Normal, rendinst_main_visibility, wr->getCameraViewItm(),
        rendinst::LayerFlag::Decals, rendinst::OptimizeDepthPass::No, 1, rendinst::AtestStage::All, nullptr, wr->getTexCtx());
    }
  }
}
