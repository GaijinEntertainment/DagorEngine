// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvSwipeScroll.h"

#include <daRg/dag_element.h>
#include <daRg/dag_inputIds.h>
#include <daRg/dag_stringKeys.h>
#include <math/dag_Point2.h>

#include "behaviorHelpers.h"
#include "dargDebugUtils.h"
#include "elementTree.h"
#include "scriptUtil.h"

namespace darg
{

BhvSwipeScroll bhv_swipe_scroll;

static const char *dataSlotName = "swipescroll:data";

struct BhvSwipeScrollData
{
  InputDevice activeDevice = DEVID_NONE;
  int activePointer = -1;
  int activeButton = -1;
  Point2 lastPointerPos = Point2(0, 0);
  Sqrat::Function onChange;

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
  }

  void finish() { activeDevice = DEVID_NONE; }
};

BhvSwipeScroll::BhvSwipeScroll() : Behavior(0, F_HANDLE_MOUSE | F_HANDLE_TOUCH) {}

void BhvSwipeScroll::onAttach(Element *elem)
{
  BhvSwipeScrollData *bhvData = new BhvSwipeScrollData();
  elem->props.storage.SetValue(dataSlotName, bhvData);

  Sqrat::Table &scriptDesc = elem->props.scriptDesc;
  HSQUIRRELVM vm = scriptDesc.GetVM();
  bhvData->onChange = Sqrat::Function(vm, scriptDesc, scriptDesc.RawGetSlot(elem->csk->onChange));

  if (!bhvData->onChange.IsNull())
  {
    SQInteger nparams = 0, nfreevars = 0;
    G_VERIFY(get_closure_info(bhvData->onChange, &nparams, &nfreevars));
    if (nparams != 3)
    {
      darg_assert_trace_var("onChange must have 2 args", scriptDesc, elem->csk->onChange);
    }
  }
}

void BhvSwipeScroll::onDetach(Element *elem, DetachMode)
{
  elem->etree->releaseOverscroll(elem);
  BhvSwipeScrollData *bhvData = elem->props.storage.RawGetSlotValue<BhvSwipeScrollData *>(dataSlotName, nullptr);
  if (bhvData)
  {
    elem->props.storage.DeleteSlot(dataSlotName);
    delete bhvData;
  }
}

int BhvSwipeScroll::onDeactivateInput(Element *elem, InputDevice device, int pointer_id)
{
  BhvSwipeScrollData *bhvData = elem->props.storage.RawGetSlotValue<BhvSwipeScrollData *>(dataSlotName, nullptr);
  G_ASSERT_RETURN(bhvData, 0);

  if (bhvData->isActivatedBy(device, pointer_id))
  {
    elem->clearGroupStateFlags(active_state_flags_for_device(bhvData->activeDevice));
    bhvData->finish();
  }
  return 0;
}

int BhvSwipeScroll::mouseEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int pointer_id, int data,
  short mx, short my, int /*buttons*/, int accum_res)
{
  return pointingEvent(etree, elem, device, event, pointer_id, data, Point2(mx, my), accum_res);
}

int BhvSwipeScroll::touchEvent(ElementTree *etree, Element *elem, InputEvent event, HumanInput::IGenPointing * /*pnt*/, int touch_idx,
  const HumanInput::PointingRawState::Touch &touch, int accum_res)
{
  return pointingEvent(etree, elem, DEVID_TOUCH, event, touch_idx, 0, Point2(touch.x, touch.y), accum_res);
}

static int getChildIdx(Element *elem, Element *scroll)
{
  int result = 0;
  float offset = 0;
  int dim = 0; // x or y

  if (scroll->getAvailableScrolls() & O_HORIZONTAL)
    offset = scroll->screenCoord.scrollOffs.x;
  else if (scroll->getAvailableScrolls() & O_VERTICAL)
  {
    offset = scroll->screenCoord.scrollOffs.y;
    dim = 1;
  }

  for (auto *c : elem->children)
  {
    auto size = c->calcTransformedBbox().size()[dim];
    if (offset - size < size)
    {
      if (2 * offset > size) // more than half of current child size
        result++;
      break;
    }
    offset -= size;
    result++;
  }

  return result;
}

int BhvSwipeScroll::pointingEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int pointer_id,
  int button_id, const Point2 &pointer_pos, int accum_res)
{
  BhvSwipeScrollData *bhvData = elem->props.storage.RawGetSlotValue<BhvSwipeScrollData *>(dataSlotName, nullptr);
  if (!bhvData)
    return 0;

  if (device == DEVID_MOUSE)
  {
    int filter = elem->props.scriptDesc.RawGetSlotValue(elem->csk->panMouseButton, -1);
    if (filter >= 0 && button_id != filter)
      return 0;
  }

  // note: it is not possible to detect scroll `onAttach` because the parents in the `onAttach`
  // event are not initialized fully
  Element *scroll = elem->getParent();
  if (!scroll)
    logerr("Element with Swipe behavior should not be scene root");

  while (scroll)
  {
    if ((scroll->getAvailableScrolls() & O_HORIZONTAL) || (scroll->getAvailableScrolls() & O_VERTICAL))
      break;
    scroll = scroll->getParent();
  }
  if (!scroll)
  {
    logerr("Element with Swipe behavior should have a parent with scroll behavior. No scroll "
           "found");
    return 0;
  }
  if ((scroll->getAvailableScrolls() & O_HORIZONTAL) && (scroll->getAvailableScrolls() & O_VERTICAL))
  {
    logerr("Swipe cannot handle scrolling in both directions");
    return 0;
  }

  int result = 0;
  int activeStateFlag = active_state_flags_for_device(device);

  if (event == INP_EV_PRESS)
  {
    if (!bhvData->isActive() && elem->hitTest(pointer_pos))
    {
      scroll->setGroupStateFlags(activeStateFlag);
      bhvData->start(device, pointer_id, button_id, pointer_pos);
      result = R_PROCESSED;
    }
  }
  else if (event == INP_EV_RELEASE)
  {
    if (bhvData->isActivatedBy(device, pointer_id, button_id))
    {
      // stick to child element on input end
      scroll->clearGroupStateFlags(activeStateFlag);
      int idx = getChildIdx(elem, scroll);

      Point2 acc = {0, 0};
      for (int i = 0; i < idx; ++i)
      {
        acc += elem->children[i]->calcTransformedBbox().size();
      }
      scroll->scrollTo(acc);
      bhvData->finish();
      bhvData->onChange(idx, /*swipeIsActive*/ false);
      etree->releaseOverscroll(scroll);

      result = R_PROCESSED;
    }
  }
  else if (event == INP_EV_POINTER_MOVE)
  {
    if (bhvData->isActivatedBy(device, pointer_id))
    {
      if (!(accum_res & R_PROCESSED))
      {
        Point2 overscroll;
        scroll->scroll(bhvData->lastPointerPos - pointer_pos, &overscroll);
        etree->setOverscroll(scroll, overscroll);

        int idx = getChildIdx(elem, scroll);
        bhvData->onChange(idx, /*swipeIsActive*/ true);
      }

      bhvData->lastPointerPos = pointer_pos;
      result = R_PROCESSED;
    }
  }

  return result;
}

} // namespace darg
