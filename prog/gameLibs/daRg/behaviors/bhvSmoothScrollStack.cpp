// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvSmoothScrollStack.h"

#include <daRg/dag_element.h>
#include <daRg/dag_stringKeys.h>

#include <perfMon/dag_cpuFreq.h>


namespace darg
{


BhvSmoothScrollStack bhv_smooth_scroll_stack;


BhvSmoothScrollStack::BhvSmoothScrollStack() : Behavior(STAGE_BEFORE_RENDER, 0) {}


int BhvSmoothScrollStack::update(UpdateStage /*stage*/, darg::Element *elem, float dt)
{
  int nChildren = elem->children.size();
  if (!nChildren)
    return 0;

  float scrollSpeed = elem->props.getFloat(elem->csk->speed, 10.0f);
  float viscosity = elem->props.getFloat(elem->csk->viscosity, 0.025f);
  Orientation orient = elem->props.getInt<Orientation>(elem->csk->orientation, O_VERTICAL);

  int axis = (orient == O_VERTICAL) ? 1 : 0;
  bool direct = true;

  if (axis == 0 && elem->layout.hAlign == ALIGN_RIGHT)
    direct = false;
  else if (axis == 1 && elem->layout.vAlign == ALIGN_BOTTOM)
    direct = false;

  float startPos = direct ? 0 : elem->screenCoord.size[axis];
  int startIdx = direct ? 0 : (nChildren - 1);
  int endIdx = direct ? (nChildren - 1) : 0;
  int idxDelta = direct ? 1 : -1;

  for (int iChild = startIdx; direct ? (iChild <= endIdx) : (iChild >= endIdx); iChild += idxDelta)
  {
    Element *child = elem->children[iChild];
    ScreenCoord &sc = child->screenCoord;

    if (!direct)
      startPos -= sc.size[axis];

    Sqrat::Object lastPos = child->props.storage.RawGetSlot(child->csk->last);
    if (lastPos.IsNull())
      sc.relPos[axis] = startPos;
    else
      sc.relPos[axis] = lastPos.Cast<float>();

    if (direct)
    {
      if (sc.relPos[axis] > startPos)
      {
        float delta = scrollSpeed * dt;
        sc.relPos[axis] -= delta;

        // avoid big gaps
        if (viscosity >= 0 && sc.relPos[axis] > startPos + sc.size[axis] + elem->layout.gap)
          sc.relPos[axis] = approach(sc.relPos[axis], startPos, dt, viscosity);
      }
      if (sc.relPos[axis] < startPos)
        sc.relPos[axis] = startPos;
      startPos = sc.relPos[axis] + sc.size[axis] + elem->layout.gap;
    }
    else
    {
      if (sc.relPos[axis] < startPos)
      {
        float delta = scrollSpeed * dt;
        sc.relPos[axis] += delta;

        // avoid big gaps
        if (viscosity >= 0 && sc.relPos[axis] < startPos - sc.size[axis] - elem->layout.gap)
          sc.relPos[axis] = approach(sc.relPos[axis], startPos, dt, viscosity);
      }
      if (sc.relPos[axis] > startPos)
        sc.relPos[axis] = startPos;
      startPos = sc.relPos[axis] - elem->layout.gap;
    }

    child->props.storage.SetValue(child->csk->last, sc.relPos[axis]);
  }

  elem->recalcScreenPositions();

  return 0;
}


} // namespace darg
