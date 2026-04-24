// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1 // fixme: move to jam

#include <render/scopeRenderTarget.h>

#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_render.h>

#include <shaders/dag_shaderBlock.h>
#include <perfMon/dag_statDrv.h>
#include <math/dag_math3d.h>
// #include <math/dag_TMatrix4more.h>
// #include <math/dag_viewMatrix.h>
#include <render/volumetricLights/volumetricLights.h>

#include "render/fx/fxRenderTags.h"
#include "global_vars.h"
#include "private_worldRenderer.h"

void WorldRenderer::performVolfogMediaInjection()
{
  TIME_D3D_PROFILE(volfog_ff_media_injection)
  if (!volumeLight)
    return;

  volumeLight->renderIntoVolfogMedia(STAGE_PS, [this](const IPoint3 &froxel_res) {
    SCOPE_RENDER_TARGET;
    d3d::set_render_target({}, DepthAccess::RW, {});
    d3d::setview(0, 0, froxel_res.x, froxel_res.y, 0., 1.);

    ShaderGlobal::setBlock(globalFrameBlockId, ShaderGlobal::LAYER_FRAME);

    shaders::overrides::set(depthClipState);
    acesfx::renderTransSpecial(ERT_TAG_VOLFOG_INJECTION);
    shaders::overrides::reset();

    ShaderGlobal::setBlock(globalFrameBlockId, ShaderGlobal::LAYER_FRAME); // this restore can be removed, if we will rely on "everyone
                                                                           // set for themselves"
  });
}
