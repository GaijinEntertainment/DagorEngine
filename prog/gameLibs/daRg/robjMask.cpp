// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "robjMask.h"

#include <daRg/dag_stringKeys.h>
#include <daRg/dag_uiShader.h>
#include <daRg/dag_element.h>
#include <daRg/dag_helpers.h>
#include <daRg/dag_renderState.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_picture.h>

#include <math/dag_mathUtils.h>
#include <gui/dag_stdGuiRender.h>


namespace darg
{


bool RobjMaskParams::load(const Element *elem)
{
  G_UNUSED(elem);
  image = elem->props.getPicture(elem->csk->image);
  return true;
}


void RobjMask::render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *rdata, const RenderState &render_state)
{
  if (rdata->size.x < 1 || rdata->size.y < 1)
    return;

  G_UNUSED(elem);
  G_UNUSED(render_state);
  RobjMaskParams *params = static_cast<RobjMaskParams *>(rdata->params);
  G_ASSERT_RETURN(params, );

  if (!params->image)
    return;

  const PictureManager::PicDesc &pic = params->image->getPic();
  pic.updateTc();

  darg::uishader::set_mask(ctx, pic.tex, d3d::INVALID_SAMPLER_HANDLE, rdata->pos + rdata->size / 2, 0,
    Point2(2 * rdata->size.x / StdGuiRender::screen_width(), 2 * rdata->size.y / StdGuiRender::screen_height()), pic.tcLt,
    pic.tcRb); // TODO: Use actual sampler IDs
}

void RobjMask::postRender(StdGuiRender::GuiContext &ctx, const Element *elem)
{
  G_UNUSED(elem);
  uishader::set_mask(ctx, BAD_TEXTUREID, d3d::INVALID_SAMPLER_HANDLE, Point3(1, 0, 0), Point3(0, 1, 0));
}


} // namespace darg
