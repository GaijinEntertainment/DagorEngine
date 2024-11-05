// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvMoveResize.h"

#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>

#include "guiScene.h"
#include "dargDebugUtils.h"
#include "behaviorHelpers.h"


namespace darg
{


BhvMoveResize bhv_move_resize;

const float BhvMoveResize::handle_eps = 4;

static const char *dataSlotName = "moveresize:data";


struct BhvMoveResizeData
{
  InputDevice activeDevice = DEVID_NONE;
  int pointerId = -1;
  int buttonId = -1;

  BhvMoveResize::HandlePos handle = BhvMoveResize::MR_NONE;
  Point2 prevPos = Point2(-100, -100);

  void start(InputDevice device, int pointer, int button, const Point2 &pos, BhvMoveResize::HandlePos handle_)
  {
    G_ASSERT(!isStarted());
    G_ASSERT(handle_ != BhvMoveResize::MR_NONE);

    activeDevice = device;
    pointerId = pointer;
    buttonId = button;
    handle = handle_;
    prevPos = pos;
  }

  void finish()
  {
    activeDevice = DEVID_NONE;
    pointerId = -1;
    buttonId = -1;
    handle = BhvMoveResize::MR_NONE;
  }

  bool isStarted() { return activeDevice != DEVID_NONE; }

  bool isStartedBy(InputDevice device, int pointer) { return activeDevice == device && pointerId == pointer; }

  bool isStartedBy(InputDevice device, int pointer, int button)
  {
    return activeDevice == device && pointerId == pointer && buttonId == button;
  }
};


BhvMoveResize::BhvMoveResize() : Behavior(0, F_HANDLE_MOUSE | F_HANDLE_TOUCH) {}


void BhvMoveResize::onAttach(Element *elem)
{
  BhvMoveResizeData *bhvData = new BhvMoveResizeData();
  elem->props.storage.SetValue(dataSlotName, bhvData);
}


void BhvMoveResize::onDetach(Element *elem, DetachMode)
{
  BhvMoveResizeData *bhvData = elem->props.storage.RawGetSlotValue<BhvMoveResizeData *>(dataSlotName, nullptr);
  if (bhvData)
  {
    elem->props.storage.DeleteSlot(dataSlotName);
    delete bhvData;
  }
}


int BhvMoveResize::mouseEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int pointer_id, int data,
  short mx, short my, int /*buttons*/, int accum_res)
{
  return pointingEvent(etree, elem, device, event, pointer_id, data, Point2(mx, my), accum_res);
}

int BhvMoveResize::touchEvent(ElementTree *etree, Element *elem, InputEvent event, HumanInput::IGenPointing * /*pnt*/, int touch_idx,
  const HumanInput::PointingRawState::Touch &touch, int accum_res)
{
  return pointingEvent(etree, elem, DEVID_TOUCH, event, touch_idx, 0, Point2(touch.x, touch.y), accum_res);
}

int BhvMoveResize::pointingEvent(ElementTree *, Element *elem, InputDevice device, InputEvent event, int pointer_id, int button_id,
  const Point2 &pos, int accum_res)
{

  BhvMoveResizeData *bhvData = elem->props.storage.RawGetSlotValue<BhvMoveResizeData *>(dataSlotName, nullptr);
  G_ASSERT_RETURN(bhvData, 0);

  int result = 0;
  int activeStateFlags = active_state_flags_for_device(device);

  if (event == INP_EV_PRESS)
  {
    if (!bhvData->isStarted() && !(accum_res & R_PROCESSED))
    {
      HandlePos newHandle = findHandle(elem, pos);
      if (newHandle != MR_NONE)
      {
        elem->setGroupStateFlags(activeStateFlags);

        bhvData->start(device, pointer_id, button_id, pos, newHandle);
        bhvData->prevPos = pos;
        setHandleCursor(elem, newHandle);

        Sqrat::Table &scriptDesc = elem->props.scriptDesc;
        HSQUIRRELVM vm = scriptDesc.GetVM();
        Sqrat::Function onMoveResizeStarted(vm, scriptDesc, scriptDesc.RawGetSlot("onMoveResizeStarted"));
        if (!onMoveResizeStarted.IsNull())
          onMoveResizeStarted(pos.x, pos.y, elem->getElementBBox(vm));
        result |= R_PROCESSED;
      }
    }
  }
  else if (event == INP_EV_RELEASE)
  {
    if (bhvData->isStartedBy(device, pointer_id, button_id))
    {
      elem->clearGroupStateFlags(activeStateFlags);
      setHandleCursor(elem, MR_NONE);
      bhvData->finish();

      Sqrat::Table &scriptDesc = elem->props.scriptDesc;
      HSQUIRRELVM vm = scriptDesc.GetVM();
      Sqrat::Function onMoveResizeFinished(vm, scriptDesc, scriptDesc.RawGetSlot("onMoveResizeFinished"));
      if (!onMoveResizeFinished.IsNull())
        onMoveResizeFinished();

      result |= R_PROCESSED;
    }
  }
  else if (event == INP_EV_POINTER_MOVE)
  {
    if (bhvData->isStartedBy(device, pointer_id))
    {
      HandlePos curHandle = bhvData->handle;
      Point2 prevPointerPos = bhvData->prevPos;
      Point2 dMouse = pos - prevPointerPos;

      Point2 dPos(0, 0), dSize(0, 0);
      ElemAlign hPlace = (elem->layout.hPlacement != PLACE_DEFAULT) ? elem->layout.hPlacement : elem->parent->layout.hAlign;
      ElemAlign vPlace = (elem->layout.vPlacement != PLACE_DEFAULT) ? elem->layout.vPlacement : elem->parent->layout.vAlign;

      if (curHandle == MR_L || curHandle == MR_LT || curHandle == MR_LB)
      {
        if (hPlace == ALIGN_LEFT)
        {
          dPos.x += dMouse.x;
          dSize.x -= dMouse.x;
        }
        else if (hPlace == ALIGN_RIGHT)
          dSize.x -= dMouse.x;
        else if (hPlace == ALIGN_CENTER)
          dSize.x -= dMouse.x * 2;
      }

      if (curHandle == MR_T || curHandle == MR_LT || curHandle == MR_RT)
      {
        if (vPlace == ALIGN_TOP)
        {
          dPos.y += dMouse.y;
          dSize.y -= dMouse.y;
        }
        else if (vPlace == ALIGN_BOTTOM)
          dSize.y -= dMouse.y;
        else if (vPlace == ALIGN_CENTER)
          dSize.y -= dMouse.y * 2;
      }

      if (curHandle == MR_R || curHandle == MR_RT || curHandle == MR_RB)
      {
        if (hPlace == ALIGN_LEFT)
          dSize.x += dMouse.x;
        else if (hPlace == ALIGN_CENTER)
          dSize.x += dMouse.x * 2;
        else if (hPlace == ALIGN_RIGHT)
        {
          dSize.x += dMouse.x;
          dPos.x += dMouse.x;
        }
      }

      if (curHandle == MR_B || curHandle == MR_LB || curHandle == MR_RB)
      {
        if (vPlace == ALIGN_TOP)
          dSize.y += dMouse.y;
        else if (vPlace == ALIGN_CENTER)
          dSize.y += dMouse.y * 2;
        else if (vPlace == ALIGN_BOTTOM)
        {
          dSize.y += dMouse.y;
          dPos.y += dMouse.y;
        }
      }

      if (curHandle == MR_AREA)
      {
        dPos += dMouse;
      }

      bhvData->prevPos = pos;

      Sqrat::Table &scriptDesc = elem->props.scriptDesc;
      HSQUIRRELVM vm = scriptDesc.GetVM();
      Sqrat::Function onMoveResize(vm, scriptDesc, scriptDesc.RawGetSlot("onMoveResize"));
      if (!onMoveResize.IsNull())
      {
        Sqrat::Table rhRes;
        G_VERIFY(onMoveResize.Evaluate(dPos.x, dPos.y, dSize.x, dSize.y, rhRes));
        if (!rhRes.IsNull())
        {
          Sqrat::Object newPos = rhRes.RawGetSlot(elem->csk->pos);
          Sqrat::Object newSize = rhRes.RawGetSlot(elem->csk->size);
          if (!newPos.IsNull())
          {
            const char *errMsg = nullptr;
            if (!elem->layout.readPos(newPos, &errMsg))
              darg_assert_trace_var(errMsg, rhRes, elem->csk->pos);
          }
          if (!newSize.IsNull())
          {
            const char *errMsg = nullptr;
            if (!elem->layout.readSize(newSize, &errMsg))
              darg_assert_trace_var(errMsg, rhRes, elem->csk->size);
          }
          if (!newPos.IsNull() || !newSize.IsNull())
            elem->recalcLayout();
        }
      }
      result = R_PROCESSED;
    }
    else if (elem->getStateFlags() & Element::S_TOP_HOVER)
    {
      HandlePos hoverHandle = findHandle(elem, pos);
      setHandleCursor(elem, hoverHandle);
    }
    else
      setHandleCursor(elem, MR_NONE);
  }

  return result;
}


BhvMoveResize::HandlePos BhvMoveResize::findHandle(Element *elem, const Point2 &p)
{
  ScreenCoord &sc = elem->screenCoord;
  Point2 lt = sc.screenPos;
  Point2 rb = sc.screenPos + sc.size;

  IPoint2 screenRes = elem->etree->guiScene->getDeviceScreenSize();
  const float eps = handle_eps * ::min(screenRes.x, screenRes.y) / 1080.0f;

  bool top = (p.y >= lt.y) && (p.y <= lt.y + eps) && (p.x >= lt.x) && (p.x <= rb.x);
  bool bottom = (p.y <= rb.y) && (p.y >= rb.y - eps) && (p.x >= lt.x) && (p.x <= rb.x);
  bool left = (p.x >= lt.x) && (p.x <= lt.x + eps) && (p.y >= lt.y) && (p.y <= rb.y);
  bool right = (p.x <= rb.x) && (p.x >= rb.x - eps) && (p.y >= lt.y) && (p.y <= rb.y);

  int modes = elem->props.getInt(elem->csk->moveResizeModes, 0xFFFFFFFF);

  if (top)
  {
    if (left && (modes & MR_LT))
      return MR_LT;
    if (right && (modes & MR_RT))
      return MR_RT;
    if (modes & MR_T)
      return MR_T;
  }

  if (bottom)
  {
    if (left && (modes & MR_LB))
      return MR_LB;
    if (right && (modes & MR_RB))
      return MR_RB;
    if (modes & MR_B)
      return MR_B;
  }

  if (left && (modes & MR_L))
    return MR_L;
  if (right && (modes & MR_L))
    return MR_R;

  if (p.x >= lt.x && p.y >= lt.y && p.x <= rb.x && p.y <= rb.y && (modes & MR_AREA))
    return MR_AREA;
  return MR_NONE;
}


void BhvMoveResize::setHandleCursor(Element *elem, HandlePos handle)
{
  Sqrat::Object cursor;

  if (handle != MR_NONE)
  {
    Sqrat::Table cursorsTbl = elem->props.getObject(elem->csk->moveResizeCursors);
    cursor = cursorsTbl.RawGetSlot(SQInteger(handle));
  }

  if (!cursor.IsNull())
    elem->props.currentOverrides().SetValue(elem->csk->cursor, cursor);
  else
    elem->props.currentOverrides().RawDeleteSlot(elem->csk->cursor);
}


int BhvMoveResize::onDeactivateInput(Element *elem, InputDevice device, int pointer_id)
{
  BhvMoveResizeData *bhvData = elem->props.storage.RawGetSlotValue<BhvMoveResizeData *>(dataSlotName, nullptr);
  G_ASSERT_RETURN(bhvData, 0);

  if (bhvData->isStartedBy(device, pointer_id))
  {
    elem->clearGroupStateFlags(active_state_flags_for_device(device));
    setHandleCursor(elem, MR_NONE);
    bhvData->finish();
  }
  return 0;
}

} // namespace darg
