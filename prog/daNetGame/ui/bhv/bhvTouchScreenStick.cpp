// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvTouchScreenStick.h"

#include <daRg/dag_guiScene.h>
#include <daRg/dag_element.h>
#include <daRg/dag_transform.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_scriptHandlers.h>

#include <drv/hid/dag_hiPointing.h>
#include <drv/hid/dag_hiGlobals.h>
#include <startup/dag_inpDevClsDrv.h>

#include <gui/dag_stdGuiRender.h>

#include <perfMon/dag_cpuFreq.h>

#include "input/touchInput.h"

using namespace darg;


SQ_PRECACHED_STRINGS_REGISTER_WITH_BHV(BhvTouchScreenStick, bhv_touch_screen_stick, cstr);

static darg::Element *get_stick_image(darg::Element *elem)
{
  return (elem->children.size() && elem->children[0]->transform) ? elem->children[0] : nullptr;
}

BhvTouchScreenStick::BhvTouchScreenStick() : Behavior(darg::Behavior::STAGE_BEFORE_RENDER, F_HANDLE_TOUCH) {}

void BhvTouchScreenStick::updateStickValues(TouchStickState *stickState, darg::Element *elem, const CachedStrings *strings)
{
  Sqrat::Table &scriptDesc = elem->props.scriptDesc;
  Sqrat::Object objAction = scriptDesc.RawGetSlot(strings->action);
  stickState->range = scriptDesc.RawGetSlotValue<float>(strings->range, StdGuiRender::screen_height() * 0.3f);
  stickState->valueScale = scriptDesc.RawGetSlotValue<float>(strings->valueScale, 1.0f);
  stickState->useDeltaMove = scriptDesc.RawGetSlotValue<bool>(strings->useDeltaMove, false);
  stickState->deadZone = scriptDesc.RawGetSlotValue<float>(strings->deadZone, 0.0001f);
  stickState->accelerationMod = scriptDesc.RawGetSlotValue<float>(strings->accelerationMod, 1.f);
  stickState->acceleration = scriptDesc.RawGetSlotValue<bool>(strings->acceleration, false);
  stickState->stickFreezeMinVal = scriptDesc.RawGetSlotValue<float>(strings->stickFreezeMinVal, 0.0f);
}

void BhvTouchScreenStick::onAttach(Element *elem)
{
  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, );

  TouchStickState *stickState = new TouchStickState();
  elem->props.storage.SetValue(strings->stickState, stickState);

  updateStickValues(stickState, elem, strings);
  updateAction(stickState, elem, strings);
}


void BhvTouchScreenStick::onDetach(Element *elem, DetachMode)
{
  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, );

  TouchStickState *stickState = elem->props.storage.RawGetSlotValue<TouchStickState *>(strings->stickState, nullptr);
  if (stickState)
  {
    elem->props.storage.DeleteSlot(strings->stickState);
    setActionValue(stickState, Point2(0, 0));
    delete stickState;
  }
}

void BhvTouchScreenStick::setActionValue(TouchStickState *stick_state, Point2 value)
{
  if (stick_state->isStick)
    touch_input.setStickValue(stick_state->ah, value);
  else
  {
    touch_input.setAxisValue(stick_state->hh, value.x);
    touch_input.setAxisValue(stick_state->vh, value.y);
  }
}

int BhvTouchScreenStick::touchEvent(ElementTree *,
  Element *elem,
  InputEvent event,
  HumanInput::IGenPointing * /*pnt*/,
  int touch_idx,
  const HumanInput::PointingRawState::Touch &touch,
  int accum_res)
{
  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, 0);

  TouchStickState *stickState = elem->props.storage.RawGetSlotValue<TouchStickState *>(strings->stickState, nullptr);

  if (!stickState || stickState->ah == dainput::BAD_ACTION_HANDLE)
    return 0;

  Sqrat::Function onMove = elem->props.scriptDesc.GetFunction("onMove");
  Sqrat::Function onDeltaMove = elem->props.scriptDesc.GetFunction("onDeltaMove");
  Sqrat::Function onShow = elem->props.scriptDesc.GetFunction("onShow");
  Sqrat::Function onHide = elem->props.scriptDesc.GetFunction("onHide");
  Sqrat::Function freezeOnTouchEndMode = elem->props.scriptDesc.GetFunction("freezeOnTouchEndMode");

  darg::Element *stickImage = get_stick_image(elem);
  bool wasHidden = stickImage && stickImage->isHidden();

  int result = 0;

  const float fullDx = touch.x - touch.x0;
  const float fullDy = touch.y - touch.y0;
  const int emptyTouchIndex = -1;

  if (
    event == INP_EV_PRESS && elem->hitTest(touch.x, touch.y) && !(accum_res & R_PROCESSED) && stickState->touchIdx == emptyTouchIndex)
  {
    updateStickValues(stickState, elem, strings);
    if (!updateAction(stickState, elem, strings))
      return 0;

    if (stickState->useDeltaMove)
      stickState->prevPos = Point2(touch.x, touch.y);
    if (stickState->touchIdx < 0)
      stickState->touchIdx = touch_idx;

    elem->setGroupStateFlags(Element::S_TOUCH_ACTIVE);
    if (stickImage)
    {
      stickImage->setHidden(false);
      stickImage->props.setCurrentOpacity(1.0f);
      stickImage->transform->translate = Point2(touch.x0, touch.y0) - elem->screenCoord.screenPos;
    }

    if (!onMove.IsNull())
      onMove(Point2(touch.x, touch.y), Point2(0, 0), Point2(0, 0));
    if (!onShow.IsNull())
      onShow(Point2(touch.x0, touch.y0));

    result = R_PROCESSED;
  }

  if (stickState->touchIdx == touch_idx)
  {
    if (event == INP_EV_POINTER_MOVE)
    {
      Point2 dlt;
      if (stickState->useDeltaMove)
      {
        const float dx = touch.x - stickState->prevPos.x;
        const float dy = touch.y - stickState->prevPos.y;
        const float sy = StdGuiRender::screen_height();
        stickState->prevPos = Point2(touch.x, touch.y);

        dlt.x = ::clamp(dx / sy, -1.0f, 1.0f);
        dlt.y = -::clamp(dy / sy, -1.0f, 1.0f);

        if (!onDeltaMove.IsNull())
          onDeltaMove(dlt);

        stickState->accumulation += dlt;
      }
      else
      {
        dlt.x = ::clamp(fullDx / stickState->range, -1.0f, 1.0f);
        dlt.y = -::clamp(fullDy / stickState->range, -1.0f, 1.0f);
        dlt *= stickState->valueScale;
      }


      if (!onMove.IsNull())
        onMove(Point2(touch.x, touch.y), Point2(fullDx, fullDy), dlt);

      if (!stickState->useDeltaMove)
        setActionValue(stickState, dlt);

      result = R_PROCESSED;
    }
    else if (event == INP_EV_RELEASE)
    {
      if (!freezeOnTouchEndMode.IsNull())
      {
        Sqrat::Object retValue;
        if (freezeOnTouchEndMode.Evaluate(retValue) && retValue.GetType() == OT_BOOL && retValue.GetVar<bool>().value)
        {
          float stickDif = fabs(fullDx * fullDx) + fabs(fullDy * fullDy);
          float stickFreezeDif = stickState->stickFreezeMinVal * stickState->stickFreezeMinVal;
          float stickMaxDif = stickState->range * stickState->range;
          if (stickFreezeDif < stickDif / stickMaxDif)
          {
            stickState->touchIdx = emptyTouchIndex;
            return R_PROCESSED;
          }
        }
      }
      if (!onHide.IsNull())
      {
        Sqrat::Object retValue;
        if (onHide.Evaluate(retValue) && retValue.GetType() != OT_NULL)
        {
          G_ASSERT_RETURN(retValue.GetType() == OT_TABLE, R_PROCESSED);
          const Sqrat::Table params(retValue);
          const bool skipAction = params.GetSlotValue<bool>("skipAction", false);

          if (skipAction)
          {
            stickState->touchIdx = emptyTouchIndex;

            const bool resetX = params.GetSlotValue("resetX", false);
            if (resetX)
              setActionValue(stickState, Point2(0.0f, touch.y));

            return R_PROCESSED;
          }
        }
      }

      if (!onMove.IsNull())
        onMove(Point2(touch.x, touch.y), Point2(fullDx, fullDy), Point2(0, 0));

      stickState->touchIdx = emptyTouchIndex;
      setActionValue(stickState, Point2(0, 0));
      elem->clearGroupStateFlags(Element::S_TOUCH_ACTIVE);
      if (stickImage)
      {
        stickImage->props.setCurrentOpacity(0.0f);
        stickImage->setHidden(true);
      }

      result = R_PROCESSED;
    }
  }

  if (stickImage && (stickImage->isHidden() != wasHidden))
    result |= R_REBUILD_RENDER_AND_INPUT_LISTS;

  return result;
}

int BhvTouchScreenStick::update(UpdateStage /*stage*/, darg::Element *elem, float /*dt*/)
{
  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, 0);
  TouchStickState *stickState = elem->props.storage.RawGetSlotValue<TouchStickState *>(strings->stickState, nullptr);
  if (stickState->useDeltaMove)
  {
    int curTime = get_time_msec();
    int prevTime = stickState->prevTime;
    float dTime = (curTime - prevTime) / 1000.f;
    dTime = clamp(dTime, 0.001f, 0.05f); // clamp dt in range 20..1000 FPS to prevent too fast acceleration on FPS drops

    Point2 dlt = stickState->accumulation;

    dlt.x = fabs(dlt.x) <= stickState->deadZone ? 0.0f : dlt.x;
    dlt.y = fabs(dlt.y) <= stickState->deadZone ? 0.0f : dlt.y;

    if (stickState->acceleration)
    {
      const float rate = dlt.length() / (dTime * 1000.f);
      const float swipeAcceleration = (1.0f + rate * (stickState->accelerationMod));
      dlt *= swipeAcceleration;
    }

    setActionValue(stickState, dlt);

    stickState->accumulation = Point2(0.0f, 0.0f);
    stickState->prevTime = curTime;
  }

  return 0;
}

bool BhvTouchScreenStick::updateAction(TouchStickState *stickState, darg::Element *elem, const CachedStrings *strings)
{
  Sqrat::Table &scriptDesc = elem->props.scriptDesc;
  Sqrat::Object objAction = scriptDesc.RawGetSlot(strings->action);
  if (objAction.GetType() == OT_STRING)
  {
    Sqrat::Var<const char *> actionName = objAction.GetVar<const char *>();
    dainput::action_handle_t ah = dainput::get_action_handle(actionName.value);
    if (ah != dainput::BAD_ACTION_HANDLE)
      stickState->ah = ah;
    else
    {
      LOGERR_CTX("Bad input action name %s", actionName.value);
      return false;
    }
    stickState->isStick = true;
  }
  else if (objAction.GetType() == OT_TABLE)
  {
    auto applySlotToAction = [&objAction](const char *name, dainput::action_handle_t &action) {
      Sqrat::Object obj = objAction.RawGetSlot(name);
      if (obj.GetType() == OT_STRING)
      {
        Sqrat::Var<const char *> actionName = obj.GetVar<const char *>();
        action = dainput::get_action_handle(actionName.value);
        if (action == dainput::BAD_ACTION_HANDLE)
          LOGERR_CTX("Bad input action name %s for %s", actionName.value, name);
      }
      else
        LOGERR_CTX("Bad type expect string get %d for %s", name, obj.GetType(), name);
    };
    applySlotToAction("horizontal", stickState->hh);
    applySlotToAction("vertical", stickState->vh);
    G_ASSERT(stickState->hh != dainput::BAD_ACTION_HANDLE && stickState->vh != dainput::BAD_ACTION_HANDLE);
    stickState->isStick = false;
  }
  else
  {
    LOGERR_CTX("Bad action type expect string or array of string get %d", objAction.GetType());
    return false;
  }
  return true;
}