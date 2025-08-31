// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvPannable.h"

#include <daRg/dag_element.h>
#include <daRg/dag_inputIds.h>
#include <daRg/dag_stringKeys.h>

#include "elementTree.h"
#include "behaviorHelpers.h"


namespace darg
{


BhvPannable bhv_pannable;

static const char *dataSlotName = "pannable:data";


struct BhvPannableData
{
  InputDevice activeDevice = DEVID_NONE;
  int activePointer = -1;
  int activeButton = -1;
  Point2 lastPointerPos = Point2(0, 0);

  KineticTouchTracker kineticTracker;

  bool isActive() { return activeDevice != DEVID_NONE; }

  bool isActivatedBy(InputDevice device, int pointer, int button)
  {
    return activeDevice == device && activePointer == pointer && activeButton == button;
  }

  bool isActivatedBy(InputDevice device, int pointer) { return activeDevice == device && activePointer == pointer; }

  void start(InputDevice device, int pointer, int button, const Point2 &pos)
  {
    G_ASSERT(!isActive());
    activeDevice = device;
    activePointer = pointer;
    activeButton = button;
    lastPointerPos = pos;
    kineticTracker.reset();
    kineticTracker.onTouch(pos);
  }

  void finish()
  {
    activeDevice = DEVID_NONE;
    kineticTracker.reset();
  }
};


BhvPannable::BhvPannable() : Behavior(0, F_HANDLE_MOUSE | F_HANDLE_TOUCH | F_ALLOW_AUTO_SCROLL) {}

void BhvPannable::onAttach(Element *elem)
{
  BhvPannableData *bhvData = new BhvPannableData();
  elem->props.storage.SetValue(dataSlotName, bhvData);
}


void BhvPannable::onDetach(Element *elem, DetachMode)
{
  elem->etree->releaseOverscroll(elem);
  BhvPannableData *bhvData = elem->props.storage.RawGetSlotValue<BhvPannableData *>(dataSlotName, nullptr);
  if (bhvData)
  {
    elem->props.storage.DeleteSlot(dataSlotName);
    delete bhvData;
  }
}

int BhvPannable::mouseEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int pointer_id, int data,
  short mx, short my, int /*buttons*/, int accum_res)
{
  return pointingEvent(etree, elem, device, event, pointer_id, data, Point2(mx, my), accum_res);
}


int BhvPannable::pointingEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int pointer_id, int button_id,
  const Point2 &pos, int accum_res)
{
  BhvPannableData *bhvData = elem->props.storage.RawGetSlotValue<BhvPannableData *>(dataSlotName, nullptr);
  if (!bhvData)
    return 0;

  if (device == DEVID_MOUSE)
  {
    int filter = elem->props.scriptDesc.RawGetSlotValue(elem->csk->panMouseButton, -1);
    if (filter >= 0 && button_id != filter)
      return 0;
  }

  int result = 0;

  int activeStateFlag = active_state_flags_for_device(device);

  if (event == INP_EV_PRESS)
  {
    if (!bhvData->isActive() && elem->hitTest(pos) && !(accum_res & R_STOPPED))
    {
      elem->setGroupStateFlags(activeStateFlag);
      Sqrat::Function onTouchBegin = elem->props.scriptDesc.GetFunction(elem->csk->onTouchBegin);
      if (!onTouchBegin.IsNull())
        onTouchBegin();

      bhvData->start(device, pointer_id, button_id, pos);
      etree->stopKineticScroll(elem);

      result = R_PROCESSED;
    }
  }
  else if (event == INP_EV_RELEASE)
  {
    if (bhvData->isActivatedBy(device, pointer_id, button_id))
    {
      elem->clearGroupStateFlags(activeStateFlag);

      Sqrat::Function kineticScrollOnTouchEnd = elem->props.scriptDesc.GetFunction(elem->csk->kineticScrollOnTouchEnd);
      Point2 scrollVel = bhvData->kineticTracker.curVelocity();
      if (lengthSq(scrollVel) > 0)
      {
        float kineticAxisLockAngle = elem->props.scriptDesc.RawGetSlotValue("kineticAxisLockAngle", 0.0f);
        if (kineticAxisLockAngle > 0.0f)
        {
          float angle = atan2f(scrollVel.y, scrollVel.x);
          float nearestAxisAngle = floorf(angle / HALFPI + 0.5f) * HALFPI;
          if (fabsf(angle - nearestAxisAngle) < kineticAxisLockAngle * DEG_TO_RAD)
            scrollVel = Point2(cosf(nearestAxisAngle), sinf(nearestAxisAngle)) * length(scrollVel);
        }

        if (kineticScrollOnTouchEnd.IsNull())
          etree->startKineticScroll(elem, scrollVel);
      }

      if (!kineticScrollOnTouchEnd.IsNull())
        kineticScrollOnTouchEnd(scrollVel);

      bhvData->finish();

      etree->releaseOverscroll(elem);

      result = R_PROCESSED;
    }
  }
  else if (event == INP_EV_POINTER_MOVE)
  {
    if (bhvData->isActivatedBy(device, pointer_id))
    {
      if (!(accum_res & R_PROCESSED))
      {
        elem->scrollVel.zero();

        Point2 overscroll;
        elem->scroll(bhvData->lastPointerPos - pos, &overscroll);
        etree->setOverscroll(elem, overscroll);
      }

      bhvData->kineticTracker.onTouch(pos);
      bhvData->lastPointerPos = pos;
      result = R_PROCESSED;
    }
  }

  return result;
}


int BhvPannable::touchEvent(ElementTree *etree, Element *elem, InputEvent event, HumanInput::IGenPointing *, int touch_idx,
  const HumanInput::PointingRawState::Touch &touch, int accum_res)
{
  return pointingEvent(etree, elem, DEVID_TOUCH, event, touch_idx, 0, Point2(touch.x, touch.y), accum_res);
}


int BhvPannable::onDeactivateInput(Element *elem, InputDevice device, int pointer_id)
{
  BhvPannableData *bhvData = elem->props.storage.RawGetSlotValue<BhvPannableData *>(dataSlotName, nullptr);
  G_ASSERT_RETURN(bhvData, 0);

  if (bhvData->isActivatedBy(device, pointer_id))
  {
    elem->clearGroupStateFlags(active_state_flags_for_device(bhvData->activeDevice));
    bhvData->finish();
  }
  return 0;
}

} // namespace darg
