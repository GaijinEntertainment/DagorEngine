// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entitySystem.h>
#include <rendInst/rendInstGenRender.h>
#include <util/dag_convar.h>

static CONSOLE_BOOL_VAL("rendinst", show_rigen_cells, false);

ECS_NO_ORDER
ECS_TAG(render, dev)
static void show_rigen_cells_debug_es(const ecs::UpdateStageInfoRenderDebug &)
{
  if (show_rigen_cells)
    rendinst::render::renderDebug();
}
