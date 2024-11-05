// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daRg/dag_renderObject.h>
#include <daRg/dag_renderState.h>
#include <daRg/dag_element.h>
#include <daRg/dag_stringKeys.h>

#include <gui/dag_stdGuiRender.h>

#include "robjBrowser.h"

namespace darg
{

bool RobjParamsBrowser::load(const Element *elem)
{
  color = elem->props.getColor(elem->csk->color);
  return true;
}

void RenderObjectBrowser::render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *rdata,
  const RenderState &render_state)
{
  G_UNUSED(render_state);
  G_UNUSED(elem);
  RobjParamsBrowser *params = static_cast<RobjParamsBrowser *>(rdata->params);
  G_ASSERT(params);
  ctx.set_color(params->color);

  ctx.render_rect_t(rdata->pos, rdata->pos + rdata->size);
}

} // namespace darg
