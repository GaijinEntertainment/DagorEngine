//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_span.h>

// these desc sub-blocks are read only when real model resources are loaded
// (RenderableInstanceLodsResource / DynamicRenderableSceneLodsResource); on builds where
// model factories are stubbed (e.g. dedicated server) pass them to
// gameres_final_optimize_desc to drop them
inline dag::ConstSpan<const char *> gameres_render_only_desc_block_names()
{
  static const char *render_block_names[] = {"mat", "tex", "texScale_data", "matR", "matS"};
  return make_span_const(render_block_names);
}
