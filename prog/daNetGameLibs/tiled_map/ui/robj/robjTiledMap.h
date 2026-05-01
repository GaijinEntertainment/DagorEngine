// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_renderObject.h>
#include <math/dag_Point2.h>

class TiledMapContext;
class TMatrix;

class RobjTiledMap : public darg::RenderObject
{
  virtual void render(StdGuiRender::GuiContext &ctx,
    const darg::Element *elem,
    const darg::ElemRenderData * /*rdata*/,
    const darg::RenderState &render_state) override;
};

class RobjTiledMapVisCone : public darg::RenderObject
{
  virtual void render(StdGuiRender::GuiContext &ctx,
    const darg::Element *elem,
    const darg::ElemRenderData * /*rdata*/,
    const darg::RenderState &render_state) override;
};

class RobjTiledMapFogOfWar : public darg::RenderObject
{
  virtual void render(StdGuiRender::GuiContext &ctx,
    const darg::Element *elem,
    const darg::ElemRenderData * /*rdata*/,
    const darg::RenderState &render_state) override;
};
