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
  const RiGenVisibility *rendinstMainVisibility = evt.get<4>();
  if (rendinstMainVisibility)
  {
    TIME_D3D_PROFILE(rendinst_decals);
    const TMatrix &viewTm = evt.get<0>();
    const TMatrix &viewItm = evt.get<1>();
    const Point3 &cameraWorldPos = evt.get<2>();
    const TexStreamingContext texCtx = evt.get<3>();

    RenderPrecise renderPrecise(viewTm, cameraWorldPos);
    rendinst::render::renderRIGen(rendinst::RenderPass::Normal, rendinstMainVisibility, viewItm, rendinst::LayerFlag::Decals,
      rendinst::OptimizeDepthPass::No, 1, rendinst::AtestStage::All, nullptr, texCtx);
  }
}
