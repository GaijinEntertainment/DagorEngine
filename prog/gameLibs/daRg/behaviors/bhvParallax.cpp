// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvParallax.h"

#include <daRg/dag_element.h>
#include <daRg/dag_transform.h>
#include <daRg/dag_stringKeys.h>

#include "guiScene.h"


namespace darg
{


BhvParallax bhv_parallax;


BhvParallax::BhvParallax() : Behavior(STAGE_BEFORE_RENDER, 0) {}


int BhvParallax::update(UpdateStage /*stage*/, darg::Element *elem, float /*dt*/)
{
  if (!elem->transform)
    return 0;

  GuiScene *scene = GuiScene::get_from_elem(elem);

  Point2 mousePos = scene->getMousePos();
  float halfW = StdGuiRender::screen_width() / 2;
  float halfH = StdGuiRender::screen_height() / 2;

  Point2 mouseRel(::clamp(mousePos.x - halfW, -halfW, halfW), ::clamp(mousePos.y - halfH, -halfH, halfH));

  float k = elem->props.getFloat(elem->csk->parallaxK, 0.0f);
  elem->transform->translate = mouseRel * k;

  return 0;
}


} // namespace darg
