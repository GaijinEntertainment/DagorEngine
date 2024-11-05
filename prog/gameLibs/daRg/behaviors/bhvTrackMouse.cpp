// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvTrackMouse.h"

#include <daRg/dag_element.h>
#include <daRg/dag_stringKeys.h>

#include "guiScene.h"
#include "scriptUtil.h"
#include "eventData.h"

#include "behaviorHelpers.h"


namespace darg
{


BhvTrackMouse bhv_track_mouse;

#if _TARGET_XBOX
static const int update_stage = Behavior::STAGE_ACT;
#else
static const int update_stage = 0;
#endif


BhvTrackMouse::BhvTrackMouse() : Behavior(update_stage, F_HANDLE_MOUSE) {}


int BhvTrackMouse::mouseEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int /*pointer_id*/, int data,
  short mx, short my, int /*buttons*/, int accum_res)
{
#if _TARGET_XBOX

  G_UNUSED(etree);
  G_UNUSED(elem);
  G_UNUSED(device);
  G_UNUSED(event);
  G_UNUSED(data);
  G_UNUSED(mx);
  G_UNUSED(my);
  G_UNUSED(accum_res);

  return 0;

#else

  int result = 0;

  if (event == INP_EV_MOUSE_WHEEL)
  {
    if (elem->hitTest(mx, my) && !(accum_res & R_PROCESSED))
    {
      if (call_mouse_wheel_handler(etree->guiScene, elem, data, mx, my))
      {
        if (!elem->props.getBool(elem->csk->eventPassThrough, false))
          result = R_PROCESSED;
      }
    }

    return result;
  }

  if (event != INP_EV_POINTER_MOVE || device == DEVID_VR)
    return 0;

  if (elem->hitTest(mx, my) && !(accum_res & R_PROCESSED))
  {
    if (call_mouse_move_handler(etree->guiScene, elem, mx, my))
    {
      if (!elem->props.getBool(elem->csk->eventPassThrough, false))
        result = R_PROCESSED;
    }
  }

  return result;

#endif
}


int BhvTrackMouse::update(UpdateStage /*stage*/, Element *elem, float /*dt*/)
{
#if _TARGET_XBOX

  // This is a workaround against XBox mouse driver not sending MOUSEMOVE events

  GuiScene *guiScene = elem->etree->guiScene;
  Point2 mousePos = guiScene->getMousePos();

  Sqrat::Table &pprops = elem->props.storage;
  Sqrat::Object lastX = pprops.RawGetSlot("xbox-trackmouse-x");
  if (!lastX.IsNull())
  {
    Point2 prevPos(lastX.Cast<float>(), pprops.RawGetSlotValue("xbox-trackmouse-y", 0.0f));
    if (lengthSq(mousePos - prevPos) > 0.1f && elem->hitTest(mousePos))
    {
      call_mouse_move_handler(guiScene, elem, mousePos.x, mousePos.y);
    }
  }
  pprops.SetValue("xbox-trackmouse-x", mousePos.x);
  pprops.SetValue("xbox-trackmouse-y", mousePos.y);

  return 0;

#else

  G_UNUSED(elem);
  return 0;

#endif
}

} // namespace darg