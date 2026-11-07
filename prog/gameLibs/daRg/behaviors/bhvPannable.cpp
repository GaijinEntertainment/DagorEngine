// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvPannable.h"

#include <daRg/dag_element.h>
#include <daRg/dag_inputIds.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_scriptHandlers.h>

#include "elementTree.h"
#include "guiScene.h"
#include "behaviorHelpers.h"


namespace darg
{


BhvPannable bhv_pannable;


struct BhvPannableData
{
  static constexpr int PAIR_POINTER = -2; // panTouches = 2: the center of the two touches pans

  InputDevice activeDevice = DEVID_NONE;
  int activePointer = -1;
  int activeButton = -1;
  Point2 lastPointerPos = Point2(0, 0);

  // panTouches = 2: the touches that are down on the element
  int touchIdx[2] = {-1, -1};
  Point2 touchPos[2] = {Point2(0, 0), Point2(0, 0)};

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

  int touchSlot(int touch_idx) const { return touchIdx[0] == touch_idx ? 0 : touchIdx[1] == touch_idx ? 1 : -1; }
  Point2 touchCenter() const { return (touchPos[0] + touchPos[1]) * 0.5f; }
};


BhvPannable::BhvPannable() : Behavior(0, F_HANDLE_MOUSE | F_HANDLE_TOUCH | F_HANDLE_TOUCHPAD_PAIR | F_ALLOW_AUTO_SCROLL) {}

void BhvPannable::onAttach(Element *elem)
{
  BhvPannableData *bhvData = new BhvPannableData();
  elem->props.storage.SetValue(elem->csk->pannableData, bhvData);
}


void BhvPannable::onDetach(Element *elem, DetachMode)
{
  elem->etree->releaseOverscroll(elem);
  BhvPannableData *bhvData = elem->props.storage.RawGetSlotValue<BhvPannableData *>(elem->csk->pannableData, nullptr);
  if (bhvData)
  {
    elem->props.storage.DeleteSlot(elem->csk->pannableData);
    delete bhvData;
  }
}


static void begin_pan(ElementTree *etree, Element *elem, BhvPannableData *bhv_data, InputDevice device, int pointer, int button,
  const Point2 &pos)
{
  elem->setGroupStateFlags(active_state_flags_for_device(device));
  Sqrat::Function onTouchBegin = elem->props.scriptDesc.GetFunction(elem->csk->onTouchBegin);
  if (!onTouchBegin.IsNull())
    etree->guiScene->queueScriptHandler(new ScriptHandlerSqFunc<>(onTouchBegin));

  bhv_data->start(device, pointer, button, pos);
  etree->stopKineticScroll(elem);
}


static void move_pan(ElementTree *etree, Element *elem, BhvPannableData *bhv_data, const Point2 &pos, int accum_res)
{
  if (accum_res & R_PROCESSED) // another element took the move, so the release must not fling this one by it
    bhv_data->kineticTracker.reset();
  else
  {
    elem->scrollVel.zero();

    Point2 overscroll;
    elem->scroll(bhv_data->lastPointerPos - pos, &overscroll);
    etree->setOverscroll(elem, overscroll);
  }

  bhv_data->kineticTracker.onTouch(pos);
  bhv_data->lastPointerPos = pos;
}


static void end_pan(ElementTree *etree, Element *elem, BhvPannableData *bhv_data)
{
  elem->clearGroupStateFlags(active_state_flags_for_device(bhv_data->activeDevice));

  Sqrat::Function kineticScrollOnTouchEnd = elem->props.scriptDesc.GetFunction(elem->csk->kineticScrollOnTouchEnd);
  Point2 scrollVel = bhv_data->kineticTracker.curVelocity();
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
    etree->guiScene->queueScriptHandler(new ScriptHandlerSqFunc<Point2>(kineticScrollOnTouchEnd, scrollVel));

  bhv_data->finish();

  etree->releaseOverscroll(elem);
}


// panTouches = 2: two touches pan by the move of their center, one touch is left to the content
static int two_touches_event(ElementTree *etree, Element *elem, BhvPannableData *bhv_data, InputEvent event, int touch_idx,
  const Point2 &pos, int accum_res)
{
  int slot = bhv_data->touchSlot(touch_idx);
  if (event == INP_EV_PRESS)
  {
    if (slot >= 0 || (accum_res & R_STOPPED) || !elem->hitTest(pos))
      return 0;
    slot = bhv_data->touchSlot(-1);
    if (slot < 0)
      return 0;
    bhv_data->touchIdx[slot] = touch_idx;
    bhv_data->touchPos[slot] = pos;
    if (bhv_data->touchIdx[1 - slot] >= 0 && !bhv_data->isActive())
      begin_pan(etree, elem, bhv_data, DEVID_TOUCH, BhvPannableData::PAIR_POINTER, 0, bhv_data->touchCenter());
    return R_PROCESSED;
  }

  if (slot < 0)
    return 0;

  const bool panning = bhv_data->isActivatedBy(DEVID_TOUCH, BhvPannableData::PAIR_POINTER);
  if (event == INP_EV_POINTER_MOVE)
  {
    bhv_data->touchPos[slot] = pos;
    if (!panning)
      return 0;
    move_pan(etree, elem, bhv_data, bhv_data->touchCenter(), accum_res);
    return R_PROCESSED;
  }

  if (event == INP_EV_RELEASE)
  {
    if (panning)
      end_pan(etree, elem, bhv_data);
    bhv_data->touchIdx[slot] = -1;
    return R_PROCESSED;
  }

  return 0;
}


int BhvPannable::pointingEvent(ElementTree *etree, Element *elem, InputDevice device, TouchSource touch_src, InputEvent event,
  int pointer_id, int button_id, Point2 pos, int accum_res)
{
  BhvPannableData *bhvData = elem->props.storage.RawGetSlotValue<BhvPannableData *>(elem->csk->pannableData, nullptr);
  if (!bhvData)
    return 0;

  if (device != DEVID_TOUCH && elem->props.scriptDesc.RawGetSlotValue(elem->csk->panTouchOnly, false))
    return 0;

  if (device == DEVID_MOUSE)
  {
    int filter = elem->props.scriptDesc.RawGetSlotValue(elem->csk->panMouseButton, -1);
    if (filter >= 0 && button_id != filter)
      return 0;
  }

  if (device == DEVID_TOUCH && elem->props.scriptDesc.RawGetSlotValue(elem->csk->panTouches, 1) == 2)
    return two_touches_event(etree, elem, bhvData, event, pointer_id, pos, accum_res);

  int result = 0;

  if (event == INP_EV_PRESS)
  {
    if (!bhvData->isActive() && elem->hitTest(pos) && !(accum_res & R_STOPPED))
    {
      begin_pan(etree, elem, bhvData, device, pointer_id, button_id, pos);
      result = R_PROCESSED;
    }
    else if (touch_src == TouchSource::TS_touchPad && bhvData->activeDevice == DEVID_TOUCH && elem->hitTest(pos))
      result = R_PROCESSED; // the other finger of a pad pair is a part of the pan
  }
  else if (event == INP_EV_RELEASE)
  {
    if (bhvData->isActivatedBy(device, pointer_id, button_id))
    {
      end_pan(etree, elem, bhvData);
      result = R_PROCESSED;
    }
  }
  else if (event == INP_EV_POINTER_MOVE)
  {
    if (bhvData->isActivatedBy(device, pointer_id))
    {
      move_pan(etree, elem, bhvData, pos, accum_res);
      result = R_PROCESSED;
    }
  }

  return result;
}


int BhvPannable::onDeactivateInput(Element *elem, InputDevice device, int pointer_id)
{
  BhvPannableData *bhvData = elem->props.storage.RawGetSlotValue<BhvPannableData *>(elem->csk->pannableData, nullptr);
  G_ASSERT_RETURN(bhvData, 0);

  const int slot = device == DEVID_TOUCH ? bhvData->touchSlot(pointer_id) : -1;
  if (slot >= 0)
  {
    bhvData->touchIdx[slot] = -1;
    pointer_id = BhvPannableData::PAIR_POINTER;
  }

  if (bhvData->isActivatedBy(device, pointer_id))
  {
    elem->clearGroupStateFlags(active_state_flags_for_device(bhvData->activeDevice));
    bhvData->finish();
  }
  return 0;
}

int BhvPannable::onDeactivateAllInput(Element *elem)
{
  BhvPannableData *bhvData = elem->props.storage.RawGetSlotValue<BhvPannableData *>(elem->csk->pannableData, nullptr);
  G_ASSERT_RETURN(bhvData, 0);

  bhvData->touchIdx[0] = bhvData->touchIdx[1] = -1;
  if (bhvData->isActive())
  {
    elem->clearGroupStateFlags(active_state_flags_for_device(bhvData->activeDevice));
    bhvData->finish();
  }
  return 0;
}

} // namespace darg
