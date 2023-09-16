#include "bhvPannable2Touch.h"

#include <daRg/dag_element.h>
#include <daRg/dag_inputIds.h>
#include <daRg/dag_stringKeys.h>


namespace darg
{


BhvPannable2touch bhv_pannable_2touch;

static const char *dataSlotName = "pannable2touch:data";


BhvPannable2touch::BhvPannable2touch() : Behavior(0, F_HANDLE_TOUCH | F_ALLOW_AUTO_SCROLL) {}


void BhvPannable2touch::onAttach(Element *elem)
{
  BhvPannable2touchData *panData = new BhvPannable2touchData();
  elem->props.storage.SetValue(dataSlotName, panData);
}


void BhvPannable2touch::onDetach(Element *elem, DetachMode)
{
  BhvPannable2touchData *panData = elem->props.storage.RawGetSlotValue<BhvPannable2touchData *>(dataSlotName, nullptr);
  if (panData)
  {
    elem->props.storage.DeleteSlot(dataSlotName);
    delete panData;
  }
}


int BhvPannable2touch::touchEvent(ElementTree *, Element *elem, InputEvent event, HumanInput::IGenPointing *, int touch_idx,
  const HumanInput::PointingRawState::Touch &touch, int accum_res)
{
  BhvPannable2touchData *panData = elem->props.storage.RawGetSlotValue<BhvPannable2touchData *>(dataSlotName, nullptr);
  G_ASSERT_RETURN(panData, 0);

  if (event == INP_EV_PRESS)
  {
    for (int slot = 0; slot < 2; ++slot)
    {
      if (panData->activeTouchIdx[slot] < 0 && elem->hitTest(touch.x, touch.y))
      {
        panData->activeTouchIdx[slot] = touch_idx;
        panData->lastTouchPos[slot].set(touch.x, touch.y);
        return R_PROCESSED;
      }
    }
  }
  else if (event == INP_EV_POINTER_MOVE)
  {
    if (panData->activeTouchIdx[0] >= 0 && panData->activeTouchIdx[1] >= 0)
    {
      Point2 curTouchPos(touch.x, touch.y);
      for (int slot = 0; slot < 2; ++slot)
      {
        if (touch_idx == panData->activeTouchIdx[slot])
        {
          if (!(accum_res & R_PROCESSED))
            elem->scroll((panData->lastTouchPos[slot] - curTouchPos) / 2);
          panData->lastTouchPos[slot] = curTouchPos;
          return R_PROCESSED;
        }
      }
    }
  }
  else if (event == INP_EV_RELEASE)
  {
    for (int slot = 0; slot < 2; ++slot)
    {
      if (touch_idx == panData->activeTouchIdx[slot])
      {
        panData->activeTouchIdx[slot] = -1;
        return R_PROCESSED;
      }
    }
  }

  return 0;
}


int BhvPannable2touch::onDeactivateInput(Element *elem, InputDevice device, int pointer_id)
{
  if (device != DEVID_TOUCH)
    return 0;

  BhvPannable2touchData *panData = elem->props.storage.RawGetSlotValue<BhvPannable2touchData *>(dataSlotName, nullptr);
  G_ASSERT_RETURN(panData, 0);

  for (int slot = 0; slot < 2; ++slot)
  {
    if (panData->activeTouchIdx[slot] == pointer_id)
      panData->activeTouchIdx[slot] = -1;
  }
  return 0;
}

} // namespace darg
