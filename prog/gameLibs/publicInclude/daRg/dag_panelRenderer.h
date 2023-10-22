//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>

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
void render_panels_in_world(const darg::IGuiScene &scene, const Point3 &view_point, RenderPass render_pass);

} // namespace darg_panel_renderer
