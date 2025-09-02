//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_TMatrix.h>

namespace darg
{
struct IGuiScene;
}

namespace darg_panel_renderer
{

enum class RenderPass : int
{
  Translucent,
  TranslucentWithoutDepth,
  Shadow,
  GBuffer,
};

enum RenderFeatures
{
  CastShadow = 1 << 0,
  Opaque = 1 << 1,
  AlwaysOnTop = 1 << 2,
};

// TODO: move this into GuiScene
void render_panels_in_world(const darg::IGuiScene &scene, RenderPass render_pass, const Point3 &view_point, const TMatrix &view_tm,
  const TMatrix *prev_view_tm = nullptr, const TMatrix4 *prev_proj_current_jitter = nullptr, int view_index = 0);

} // namespace darg_panel_renderer
