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


int BhvTrackMouse::pointingEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int /*pointer_id*/,
  int button_id, Point2 pos, int accum_res)
{
#if _TARGET_XBOX

  G_UNUSED(etree);
  G_UNUSED(elem);
  G_UNUSED(device);
  G_UNUSED(event);
  G_UNUSED(button_id);
  G_UNUSED(pos);
  G_UNUSED(accum_res);

  return 0;

#else

  if (device != DEVID_MOUSE)
    return 0;

  if (event == INP_EV_MOUSE_WHEEL)
  {
    if (elem->hitTest(pos) && !(accum_res & R_PROCESSED))
    {
      int scroll = button_id; // special case
      if (call_mouse_wheel_handler(etree->guiScene, elem, scroll, pos.x, pos.y))
      {
        if (!elem->props.getBool(elem->csk->eventPassThrough, false))
          return R_PROCESSED;
      }
    }
  }

  if (event == INP_EV_POINTER_MOVE && elem->hitTest(pos) && !(accum_res & R_PROCESSED))
  {
    if (call_mouse_move_handler(etree->guiScene, elem, pos.x, pos.y))
    {
      if (!elem->props.getBool(elem->csk->eventPassThrough, false))
        return R_PROCESSED;
    }
  }

  return 0;

#endif
}


int BhvTrackMouse::update(UpdateStage /*stage*/, Element *elem, float /*dt*/)
{
#if _TARGET_XBOX

  // This is a workaround against XBox mouse driver not sending MOUSEMOVE events

  GuiScene *guiScene = elem->etree->guiScene;
  Point2 mousePos = guiScene->activePointerPos();

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