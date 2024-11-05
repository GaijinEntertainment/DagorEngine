// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvBoundToArea.h"
#include "scriptUtil.h"

#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_guiTypes.h>
#include <daRg/dag_transform.h>

#include <gui/dag_stdGuiRender.h>


namespace darg
{


BhvBoundToArea bhv_bound_to_area;


BhvBoundToArea::BhvBoundToArea() : Behavior(darg::Behavior::STAGE_BEFORE_RENDER, 0) {}


int BhvBoundToArea::update(UpdateStage /*stage*/, darg::Element *elem, float /*dt*/)
{
  if (!elem->transform)
    return 0;

  elem->transform->translate.zero();

  Sqrat::Table &scriptDesc = elem->props.scriptDesc;
  Sqrat::Object marginsObj = scriptDesc.RawGetSlot(elem->csk->safeAreaMargin);
  float margins[4] = {0};
  if (!script_parse_offsets(elem, marginsObj, margins, nullptr))
    return 0;

  BBox2 bbox = elem->calcTransformedBbox();

  float boundL = ceilf(margins[3]);
  float boundT = ceilf(margins[0]);
  float boundR = floorf(StdGuiRender::screen_width() - margins[1]);
  float boundB = floorf(StdGuiRender::screen_height() - margins[2]);

  float l = bbox.lim[0].x, t = bbox.lim[0].y, r = bbox.lim[1].x, b = bbox.lim[1].y;

  if (bbox.width()[0] < boundR - boundL)
  {
    if (l < boundL)
      elem->transform->translate.x += boundL - l;
    if (r > boundR)
      elem->transform->translate.x += boundR - r;
  }

  if (bbox.width()[1] < boundB - boundT)
  {
    if (t < boundT)
      elem->transform->translate.y += boundT - t;
    if (b > boundB)
      elem->transform->translate.y += boundB - b;
  }

  return 0;
}


} // namespace darg
