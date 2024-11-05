//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <gui/dag_stdGuiRender.h>

namespace darg
{

struct RenderState;

namespace scenerender
{

BBox2 calcTransformedBbox(StdGuiRender::GuiContext &ctx, const Point2 lt, const Point2 rb, const RenderState &render_state);
BBox2 calcTransformedBbox(const Point2 lt, const Point2 rb, const GuiVertexTransform &gvtm);
void setTransformedViewPort(StdGuiRender::GuiContext &ctx, const Point2 lt, const Point2 rb, const RenderState &render_state);
void restoreTransformedViewPort(StdGuiRender::GuiContext &ctx);

} // namespace scenerender

} // namespace darg
