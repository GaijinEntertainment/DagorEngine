// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_renderObject.h>
#include <daRg/dag_renderState.h>
#include <daRg/dag_element.h>

#include <gui/dag_stdGuiRender.h>

namespace darg
{

class RobjParamsBrowser : public RendObjParams
{
public:
  E3DCOLOR color = E3DCOLOR(255, 255, 255);

  virtual bool load(const Element *elem) override;
};

class RenderObjectBrowser : public RenderObject
{
public:
  virtual void render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *rdata,
    const RenderState &render_state) override;
};

ROBJ_FACTORY_IMPL(RenderObjectBrowser, RobjParamsBrowser)
void register_browser_rendobj_factories() { add_rendobj_factory("ROBJ_BROWSER", ROBJ_FACTORY_PTR(RenderObjectBrowser)); }

} // namespace darg
