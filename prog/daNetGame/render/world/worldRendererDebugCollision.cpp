// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1 // fixme: move to jam

#include <memory/dag_framemem.h>
#include <gameRes/dag_collisionResource.h>
#include <rendInst/rendInstExtra.h>
#include <util/dag_convar.h>
#include "private_worldRenderer.h"
#include <render/debugCollisionDensityRenderer.h>
#include <render/deferredRenderer.h>


CONSOLE_BOOL_VAL("debug", collision_density, false);
CONSOLE_BOOL_VAL("debug", phys_density, false);
CONSOLE_INT_VAL("debug", collision_density_threshold, 1000, 10, 10000);

void WorldRenderer::renderDebugCollisionDensity()
{
  if (!collision_density.get() && !phys_density.get())
    return;
  if (!debugCollisionDensityRenderer)
    debugCollisionDensityRenderer.reset(new DebugCollisionDensityRenderer);
  debugCollisionDensityRenderer->prepare_render(getWorldBBox3(), collision_density_threshold.get(), collision_density.get(),
    phys_density.get());
  debugCollisionDensityRenderer->render();
}
