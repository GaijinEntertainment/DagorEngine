// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvMinimapCanvasPolygon.h"
#include "../minimapState.h"
#include <ecs/scripts/sqEntity.h>

#include <daRg/dag_element.h>
#include <daRg/dag_transform.h>

#include <gui/dag_stdGuiRender.h>
#include <3d/dag_render.h>
#include <math/dag_mathUtils.h>

#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>

#include <sqrat.h>

using namespace darg;


SQ_PRECACHED_STRINGS_REGISTER_WITH_BHV(BhvMinimapCanvasPolygon, bhv_minimap_canvas_polygon, cstr);


BhvMinimapCanvasPolygon::BhvMinimapCanvasPolygon() : Behavior(darg::Behavior::STAGE_BEFORE_RENDER, 0) {}


int BhvMinimapCanvasPolygon::update(UpdateStage /*stage*/, darg::Element *elem, float /*dt*/)
{
  TIME_PROFILE(bhv_minimap_canvas_polygon_update);

  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, 0);

  MinimapState *mmState = elem->props.scriptDesc.RawGetSlotValue<MinimapState *>(strings->minimapState, nullptr);
  if (!mmState)
    return 0;

  Sqrat::Array commands = elem->props.scriptDesc.GetSlot(strings->commands);
  Sqrat::Array canvasPolygon = commands.GetSlot(SQInteger(0));
  if (canvasPolygon.GetType() != OT_ARRAY || canvasPolygon.Length() < 1)
    return 0;

  Sqrat::Array points = elem->props.scriptDesc.GetSlot(strings->points);
  if (points.GetType() != OT_ARRAY)
    return 0;

  SQInteger n = points.Length();

  canvasPolygon.Resize(1 + n * 2);

  for (SQInteger i = 0; i < n; ++i)
  {
    const Point2 *pt = points.RawGetSlotValue<const Point2 *>(i, nullptr);
    if (!pt)
      continue;
    Point2 screenPt = mmState->worldToMap(Point3(pt->x, 0, pt->y));
    canvasPolygon.SetValue(1 + i * 2, screenPt.x * 50.0 + 50.0);
    canvasPolygon.SetValue(1 + i * 2 + 1, screenPt.y * -50.0 + 50.0);
  }

  return 0;
}
