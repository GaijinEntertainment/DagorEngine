//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "unitedGeomBuffers.h"
#include <3d/dag_multidrawContext.h>


namespace frx
{

using RiMultidrawPerInstData = uint32_t;
struct GlobalRenderContext
{
  UnifiedGeomBuffers unifiedBufs;
  MultidrawContext<RiMultidrawPerInstData> riMultidrawContext;

  GlobalRenderContext();
  ~GlobalRenderContext();
};
GlobalRenderContext &get_render_ctx();

void init_render();
void shutdown_render();

} // namespace frx
