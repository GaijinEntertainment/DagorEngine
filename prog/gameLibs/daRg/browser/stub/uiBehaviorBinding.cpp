// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bindQuirrelEx/autoBind.h>

#include <daRg/dag_renderObject.h>

namespace darg
{

class RenderObjectBrowserStub : public RenderObject
{
public:
  virtual void render(StdGuiRender::GuiContext & /*ctx*/, const Element * /*elem*/, const ElemRenderData * /*rdata*/,
    const RenderState & /*render_state*/) override
  {
    LOGERR_ONCE("Trying to render ROBJ_BROWSER stub");
  }
};

ROBJ_FACTORY_IMPL(RenderObjectBrowserStub, RendObjEmptyParams)

void bind_browser_behavior(SqModules *) {}

void register_browser_rendobj_factories() { add_rendobj_factory("ROBJ_BROWSER", ROBJ_FACTORY_PTR(RenderObjectBrowserStub)); }

} // namespace darg
