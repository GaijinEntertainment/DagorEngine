// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvSlider.h"

#include <daRg/dag_element.h>
#include <daRg/dag_inputIds.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_properties.h>

#include "elementTree.h"
#include "behaviorHelpers.h"
#include "dargDebugUtils.h"

#include <drv/hid/dag_hiKeybIds.h>

#define DEF_MIN       0
#define DEF_MAX       100
#define DEF_VAL       0
#define DEF_UNIT      1
#define DEF_PAGE_SIZE 10


namespace darg
{


BhvSlider bhv_slider;

static const char *dataSlotName = "slider:data";


struct BhvSliderData
{
  InputDevice clickedByDevice = DEVID_NONE;
  int pointerId = -1;

  bool isDraggingKnob = false;
  float inKnobPtrOffset = 0;
  float sliderHoverValue = -1;
  bool isSliderHover = false;

  void click(InputDevice device, int pointer_id)
  {
    G_ASSERT(!isClicked());
    clickedByDevice = device;
    pointerId = pointer_id;
  }

  void release()
  {
    clickedByDevice = DEVID_NONE;
    pointerId = -1;
    isDraggingKnob = false;
  }

  bool isClicked() { return clickedByDevice != DEVID_NONE; }

  bool isClickedBy(InputDevice device, int pointer_id) { return clickedByDevice == device && pointerId == pointer_id; }
};


BhvSlider::BhvSlider() : Behavior(0, F_HANDLE_KEYBOARD | F_HANDLE_MOUSE | F_HANDLE_TOUCH | F_FOCUS_ON_CLICK | F_CAN_HANDLE_CLICKS) {}


static Element *find_knob(Element *elem)
{
  Sqrat::Object knobCtor = elem->props.scriptDesc.RawGetSlot(elem->csk->knob);
  if (knobCtor.IsNull())
    return nullptr;
  return elem->findChildByScriptCtor(knobCtor, true);
}


void BhvSlider::onAttach(Element *elem)
{
  BhvSliderData *bhvData = new BhvSliderData();
  elem->props.storage.SetValue(dataSlotName, bhvData);
}


void BhvSlider::onDetach(Element *elem, DetachMode)
{
  BhvSliderData *bhvData = elem->props.storage.RawGetSlotValue<BhvSliderData *>(dataSlotName, nullptr);
  if (bhvData)
  {
    elem->props.storage.DeleteSlot(dataSlotName);
    delete bhvData;
  }
}


int BhvSlider::kbdEvent(ElementTree *, Element *elem, InputEvent event, int key_idx, bool /*repeat*/, wchar_t /*wc*/, int accum_res)
{
  using namespace HumanInput;

  G_ASSERT((flags & F_HANDLE_KEYBOARD_GLOBAL) == (flags & F_HANDLE_KEYBOARD)); // must be only focusable

  bool needAction = (event == INP_EV_PRESS) && !(accum_res & R_PROCESSED);

  if (key_idx == DKEY_HOME)
  {
    if (needAction)
    {
      float minVal = elem->props.getFloat(elem->csk->min, DEF_MIN);
      setVal(elem, minVal);
    }
  }
  else if (key_idx == DKEY_END)
  {
    if (needAction)
    {
      float maxVal = elem->props.getFloat(elem->csk->max, DEF_MAX);
      setVal(elem, maxVal);
    }
  }
  else if (key_idx == DKEY_LEFT || key_idx == DKEY_UP)
  {
    Orientation orient = elem->props.getInt<Orientation>(elem->csk->orientation, O_HORIZONTAL);
    if (needAction && ((key_idx == DKEY_LEFT && orient == O_HORIZONTAL) || (key_idx == DKEY_UP && orient == O_VERTICAL)))
    {
      float prevVal = elem->props.getFloat(elem->csk->fValue, DEF_VAL);
      float unit = elem->props.getFloat(elem->csk->unit, DEF_UNIT);
      setVal(elem, prevVal - unit);
    }
  }
  else if (key_idx == DKEY_RIGHT || key_idx == DKEY_DOWN)
  {
    Orientation orient = elem->props.getInt<Orientation>(elem->csk->orientation, O_HORIZONTAL);
    if (needAction && ((key_idx == DKEY_RIGHT && orient == O_HORIZONTAL) || (key_idx == DKEY_DOWN && orient == O_VERTICAL)))
    {
      float prevVal = elem->props.getFloat(elem->csk->fValue, DEF_VAL);
      float unit = elem->props.getFloat(elem->csk->unit, DEF_UNIT);
      setVal(elem, prevVal + unit);
    }
  }
  else if (key_idx == DKEY_PRIOR)
  {
    if (needAction)
    {
      float prevVal = elem->props.getFloat(elem->csk->fValue, DEF_VAL);
      float pageScroll = elem->props.getFloat(elem->csk->pageScroll, DEF_PAGE_SIZE);
      setVal(elem, prevVal - pageScroll);
    }
  }
  else if (key_idx == DKEY_NEXT)
  {
    if (needAction)
    {
      float prevVal = elem->props.getFloat(elem->csk->fValue, DEF_VAL);
      float pageScroll = elem->props.getFloat(elem->csk->pageScroll, DEF_PAGE_SIZE);
      setVal(elem, prevVal + pageScroll);
    }
  }
  else
    return 0;

  return R_PROCESSED;
}


int BhvSlider::pointingEvent(ElementTree *, Element *elem, InputDevice device, InputEvent event, int pointer_id, int button_id,
  Point2 pointer_pos, int accum_res)
{
  BhvSliderData *bhvData = elem->props.storage.RawGetSlotValue<BhvSliderData *>(dataSlotName, nullptr);
  G_ASSERT_RETURN(bhvData, 0);

  if (event == INP_EV_MOUSE_WHEEL) // special case
  {
    int scroll = button_id;
    if (elem->hitTest(pointer_pos) && onMouseWheel(elem, scroll))
      return R_PROCESSED;
    return 0;
  }


  int activeStateFlag = active_state_flags_for_device(device);

  int result = 0;
  if (event == INP_EV_PRESS)
  {
    if (!(accum_res & R_PROCESSED) && elem->hitTest(pointer_pos))
    {
      if (!bhvData->isClicked())
      {
        bhvData->click(device, pointer_id);
        bhvData->isDraggingKnob = false;

        elem->setGroupStateFlags(activeStateFlag);

        Element *knob = find_knob(elem);
        if (knob && (knob->clippedScreenRect & pointer_pos))
        {
          bhvData->isDraggingKnob = true;
          knob->setGroupStateFlags(activeStateFlag);

          Orientation orient = elem->props.getInt<Orientation>(elem->csk->orientation, O_HORIZONTAL);

          Point2 localPos = knob->screenPosToElemLocal(pointer_pos);
          if (orient == O_HORIZONTAL)
            bhvData->inKnobPtrOffset = localPos.x;
          else
            bhvData->inKnobPtrOffset = localPos.y;
        }
        else
        {
          setValueFromMousePos(bhvData, elem, pointer_pos.x, pointer_pos.y, false);
        }
      }
      result = R_PROCESSED;
    }
  }
  else if (event == INP_EV_RELEASE)
  {
    if (bhvData->isClickedBy(device, pointer_id))
    {
      elem->clearGroupStateFlags(activeStateFlag);

      if (!bhvData->isDraggingKnob)
        setValueFromMousePos(bhvData, elem, pointer_pos.x, pointer_pos.y, false);

      bhvData->release();

      Element *knob = find_knob(elem);
      if (knob)
        knob->clearGroupStateFlags(activeStateFlag);

      result = R_PROCESSED;
    }
  }
  else if (event == INP_EV_POINTER_MOVE)
  {
    updateSliderHover(elem, bhvData, pointer_pos);

    if (bhvData->isClickedBy(device, pointer_id))
    {
      setValueFromMousePos(bhvData, elem, pointer_pos.x, pointer_pos.y, bhvData->isDraggingKnob);
      result = R_PROCESSED;
    }
  }

  return result;
}


void BhvSlider::updateSliderHover(darg::Element *elem, darg::BhvSliderData *bhvData, const Point2 &pointer_pos)
{
  if (elem->hitTest(pointer_pos))
  {
    float value;
    Sqrat::Function onSliderMouseMove = elem->props.scriptDesc.GetFunction(elem->csk->onSliderMouseMove);
    if (!onSliderMouseMove.IsNull() && getValueFromMousePos(bhvData, elem, pointer_pos.x, pointer_pos.y, false /*knob drag*/, value))
    {
      value = alignValue(elem, value);
      if (bhvData->sliderHoverValue != value || !bhvData->isSliderHover)
      {
        bhvData->sliderHoverValue = value;
        onSliderMouseMove(value);
      }
      bhvData->isSliderHover = true;
    }
  }
  else
  {
    if (bhvData->isSliderHover)
    {
      Sqrat::Function onSliderMouseMove = elem->props.scriptDesc.GetFunction(elem->csk->onSliderMouseMove);
      if (!onSliderMouseMove.IsNull())
        onSliderMouseMove(Sqrat::Object(elem->getVM()));
      bhvData->isSliderHover = false;
    }
  }
}


bool BhvSlider::getValueFromMousePos(BhvSliderData *bhvData, Element *elem, int mx, int my, bool knob_drag, float &out) const
{
  const Sqrat::Table &scriptDesc = elem->props.scriptDesc;
  Orientation orient = elem->props.getInt<Orientation>(elem->csk->orientation, O_HORIZONTAL);
  Element *knob = find_knob(elem);

  float minVal = elem->props.getFloat(elem->csk->min, DEF_MIN);
  float maxVal = elem->props.getFloat(elem->csk->max, DEF_MAX);

  Point2 localPos = elem->screenPosToElemLocal(Point2(mx, my));

  if (orient == O_HORIZONTAL)
  {
    if (elem->screenCoord.size.x <= 1e-3f)
      return false;

    float knobW = knob ? knob->screenCoord.size.x : 0;
    float dragOffs = 0;
    if (knob_drag)
      dragOffs = bhvData->inKnobPtrOffset;
    else
      dragOffs = knobW * 0.5f;
    float ratio = (localPos.x - dragOffs) / (elem->screenCoord.size.x - knobW);
    out = minVal + ratio * (maxVal - minVal);
    return true;
  }
  else if (orient == O_VERTICAL)
  {
    if (elem->screenCoord.size.y <= 1e-3f)
      return false;

    float knobH = knob ? knob->screenCoord.size.y : 0;
    float dragOffs = 0;
    if (knob_drag)
      dragOffs = bhvData->inKnobPtrOffset;
    else
      dragOffs = knobH * 0.5f;
    float ratio = (localPos.y - dragOffs) / (elem->screenCoord.size.y - knobH);
    out = minVal + ratio * (maxVal - minVal);
    return true;
  }
  else
    darg_assert_trace_var(String(0, "Unexpected orientation value %d", orient), scriptDesc, elem->csk->orientation);
  return false;
}

float BhvSlider::alignValue(Element *elem, float progress)
{
  float minVal = elem->props.getFloat(elem->csk->min, DEF_MIN);
  float maxVal = elem->props.getFloat(elem->csk->max, DEF_MAX);
  float unit = elem->props.getFloat(elem->csk->unit, DEF_UNIT);

  if (unit > 0)
    progress = floorf(progress / unit + 0.5f) * unit;

  return clamp(progress, minVal, maxVal);
}

void BhvSlider::setValueFromMousePos(BhvSliderData *bhvData, Element *elem, int mx, int my, bool knob_drag)
{
  float value;
  if (getValueFromMousePos(bhvData, elem, mx, my, knob_drag, value))
    setVal(elem, value);
}


bool BhvSlider::onMouseWheel(Element *elem, int delta)
{
  const Sqrat::Table &scriptDesc = elem->props.scriptDesc;
  if (scriptDesc.RawGetSlotValue(elem->csk->ignoreWheel, false))
    return false;
  float prevVal = elem->props.getFloat(elem->csk->fValue, DEF_VAL);
  float pageScroll = elem->props.getFloat(elem->csk->pageScroll, DEF_PAGE_SIZE);
  Orientation orient = elem->props.getInt<Orientation>(elem->csk->orientation, O_HORIZONTAL);
  float dVal = (delta > 0) == (orient == O_HORIZONTAL) ? pageScroll : -pageScroll;
  setVal(elem, prevVal + dVal);
  return true;
}


void BhvSlider::setVal(Element *elem, float req_val)
{
  float prevVal = elem->props.getFloat(elem->csk->fValue, DEF_VAL);
  float fVal = alignValue(elem, req_val);
  if (fVal != prevVal)
  {
    Sqrat::Function onChange = elem->props.scriptDesc.GetFunction(elem->csk->onChange);
    if (!onChange.IsNull())
      onChange(fVal);
  }
}


static void deactivate_input_impl(Element *elem, BhvSliderData *bhvData)
{
  int activeStateFlag = active_state_flags_for_device(bhvData->clickedByDevice);
  elem->clearGroupStateFlags(activeStateFlag);

  Element *knob = find_knob(elem);
  if (knob)
    knob->clearGroupStateFlags(activeStateFlag);

  bhvData->release();
}


int BhvSlider::onDeactivateInput(Element *elem, InputDevice device, int pointer_id)
{
  BhvSliderData *bhvData = elem->props.storage.RawGetSlotValue<BhvSliderData *>(dataSlotName, nullptr);
  G_ASSERT_RETURN(bhvData, 0);

  if (bhvData->isClickedBy(device, pointer_id))
    deactivate_input_impl(elem, bhvData);

  return 0;
}


int BhvSlider::onDeactivateAllInput(Element *elem)
{
  BhvSliderData *bhvData = elem->props.storage.RawGetSlotValue<BhvSliderData *>(dataSlotName, nullptr);
  G_ASSERT_RETURN(bhvData, 0);

  if (bhvData->isClicked())
    deactivate_input_impl(elem, bhvData);

  return 0;
}


} // namespace darg
