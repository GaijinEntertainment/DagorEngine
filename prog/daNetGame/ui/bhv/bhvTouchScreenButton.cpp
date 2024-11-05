// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvTouchScreenButton.h"

#include <daRg/dag_guiScene.h>
#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_uiSound.h>
#include <daRg/dag_scriptHandlers.h>

#include <drv/hid/dag_hiPointing.h>
#include <drv/hid/dag_hiGlobals.h>
#include <startup/dag_inpDevClsDrv.h>
#include <daECS/net/time.h>

#include "input/touchInput.h"

#include <cstring>

using namespace darg;
using namespace dainput;


SQ_PRECACHED_STRINGS_REGISTER_WITH_BHV(BhvTouchScreenButton, bhv_touch_screen_button, cstr);


BhvTouchScreenButton::BhvTouchScreenButton() : Behavior(darg::Behavior::STAGE_ACT, F_HANDLE_TOUCH | F_USES_TOUCH_MARGIN) {}


void BhvTouchScreenButton::onAttach(Element *elem)
{
  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, );

  TouchButtonState *buttonState = new TouchButtonState();
  elem->props.storage.SetValue(strings->buttonState, buttonState);
  buttonState->forceResendAh = elem->props.scriptDesc.GetFunction("forceResendAh");
  buttonState->onTouchBegin = elem->props.scriptDesc.GetFunction("onTouchBegin");
  buttonState->onTouchEnd = elem->props.scriptDesc.GetFunction("onTouchEnd");

  updateAction(buttonState, elem, strings);
  uint16_t acts_type = TYPEGRP_DIGITAL;
  if (!buttonState->ahs.empty())
  {
    for (auto act : buttonState->ahs)
    {
      if (get_action_type(act) != get_action_type(buttonState->ahs[0]))
        LOGERR_CTX("Actions must be of the same type.(\"%d\" != \"%d\"", get_action_type(act), get_action_type(buttonState->ahs[0]));
    }
    acts_type = get_action_type(buttonState->ahs[0]); // multiple actions must be of the same type
  }

  switch (acts_type & TYPEGRP__MASK)
  {
    case TYPEGRP_DIGITAL:
    {
      const char *toggleBtn = "toggleBtn";
      const char *holdBtn = "holdBtn";
      Sqrat::Object objAction = elem->props.scriptDesc.RawGetSlot("mod");
      if (objAction.GetType() == OT_STRING)
      {
        Sqrat::Var<const char *> modName = objAction.GetVar<const char *>();
        if (stricmp(toggleBtn, modName.value) == 0)
          buttonState->type = TouchButtonState::TB_TOGGLE;
        else if (stricmp(holdBtn, modName.value) == 0)
          buttonState->type = TouchButtonState::TB_HOLD;
      }

      initClickThrough(buttonState, elem);

      break;
    }
    case TYPEGRP_AXIS:
    {
      const char *minBtn = "minBtn";
      const char *maxBtn = "maxBtn";
      Sqrat::Object objAction = elem->props.scriptDesc.RawGetSlot("mod");
      if (objAction.GetType() == OT_STRING)
      {
        Sqrat::Var<const char *> modName = objAction.GetVar<const char *>();
        if (stricmp(minBtn, modName.value) == 0)
          buttonState->type = TouchButtonState::TB_MIN;
        else if (stricmp(maxBtn, modName.value) == 0)
          buttonState->type = TouchButtonState::TB_MAX;
        else
          LOGERR_CTX("Bad input mod for axis button expect mod = \"%s\" or mod = \"%s\" getted: %s", minBtn, maxBtn, modName.value);
      }
      else
        LOGERR_CTX("Bad input mod for axis button expect mod = \"%s\" or mod = \"%s\" getted: not string or null", minBtn, maxBtn);

      initClickThrough(buttonState, elem);

      break;
    }
    case TYPEGRP_STICK:
    {
      const char *minXBtn = "minXBtn";
      const char *maxXBtn = "maxXBtn";
      const char *minYBtn = "minYBtn";
      const char *maxYBtn = "maxYBtn";
      Sqrat::Object objAction = elem->props.scriptDesc.RawGetSlot("mod");
      if (objAction.GetType() == OT_STRING)
      {
        Sqrat::Var<const char *> modName = objAction.GetVar<const char *>();
        if (stricmp(minXBtn, modName.value) == 0)
          buttonState->type = TouchButtonState::TB_MINX;
        else if (stricmp(maxXBtn, modName.value) == 0)
          buttonState->type = TouchButtonState::TB_MAXX;
        else if (stricmp(minYBtn, modName.value) == 0)
          buttonState->type = TouchButtonState::TB_MINY;
        else if (stricmp(maxYBtn, modName.value) == 0)
          buttonState->type = TouchButtonState::TB_MAXY;
        else
          LOGERR_CTX("Bad input mod for stick button expect mod = \"%s\" or \"%s\", \"%s\", \"%s\" getted: %s", minXBtn, maxXBtn,
            minYBtn, maxYBtn, modName.value);
      }
      else
        LOGERR_CTX("Bad input mod for stick button expect mod = \"%s\" or \"%s\", \"%s\", \"%s\" getted: not string or null", minXBtn,
          maxXBtn, minYBtn, maxYBtn);
      break;
    }
  }
}

void BhvTouchScreenButton::onDetach(Element *elem, DetachMode)
{
  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, );

  TouchButtonState *buttonState = elem->props.storage.RawGetSlotValue<TouchButtonState *>(strings->buttonState, nullptr);
  if (buttonState)
  {
    elem->props.storage.DeleteSlot(strings->buttonState);
    for (auto ah : buttonState->ahs)
    {
      switch (get_action_type(ah) & TYPEGRP__MASK)
      {
        case TYPEGRP_DIGITAL: touch_input.releaseButton(ah); break;
        case TYPEGRP_AXIS: touch_input.setAxisValue(ah, 0); break;
        case TYPEGRP_STICK: touch_input.setStickValue(ah, Point2(0, 0)); break;
      }
    }
    delete buttonState;
  }
}

void BhvTouchScreenButton::pressAh(TouchButtonState *buttonState, const eastl::vector<dainput::action_handle_t> &actions)
{
  for (auto ah : actions)
    switch (get_action_type(ah) & TYPEGRP__MASK)
    {
      case TYPEGRP_DIGITAL:
      {
        touch_input.pressButton(ah);
        break;
      }
      case TYPEGRP_AXIS:
      {
        G_ASSERT(buttonState->type == TouchButtonState::TB_MIN || buttonState->type == TouchButtonState::TB_MAX);
        const float value = buttonState->type == TouchButtonState::TB_MIN ? -1.0 : +1.0;
        touch_input.setAxisValue(ah, value);
        break;
      }
      break;
      case TYPEGRP_STICK:
      {
        G_ASSERT(buttonState->type == TouchButtonState::TB_MINX || buttonState->type == TouchButtonState::TB_MAXX ||
                 buttonState->type == TouchButtonState::TB_MINY || buttonState->type == TouchButtonState::TB_MAXY);
        const float dx = buttonState->type == TouchButtonState::TB_MINX ? -1.0 : +1.0;
        const float dy = buttonState->type == TouchButtonState::TB_MINY ? -1.0 : +1.0;
        touch_input.setStickValue(ah, Point2(dx, dy));
        break;
      }
    }
}

void BhvTouchScreenButton::releaseAh(const eastl::vector<dainput::action_handle_t> &actions)
{
  for (auto ah : actions)
    switch (get_action_type(ah) & TYPEGRP__MASK)
    {
      case TYPEGRP_DIGITAL:
      {
        touch_input.releaseButton(ah);
        break;
      }
      case TYPEGRP_AXIS: touch_input.setAxisValue(ah, 0); break;
      case TYPEGRP_STICK: touch_input.setStickValue(ah, Point2(0, 0)); break;
    }
}

bool BhvTouchScreenButton::isAnyAhPressed(const eastl::vector<dainput::action_handle_t> &ahs)
{
  for (auto ah : ahs)
  {
    if (touch_input.isButtonPressed(ah))
      return true;
  }
  return false;
}

void BhvTouchScreenButton::handlePress(TouchButtonState *buttonState)
{
  if (!isAnyAhPressed(buttonState->blockingAction))
  {
    releaseAh(buttonState->releaseAhsOnPress);
    pressAh(buttonState, buttonState->ahs);
  }
  if (!buttonState->onTouchBegin.IsNull())
  {
    Sqrat::Object retValue;
    buttonState->onTouchBegin.Evaluate(retValue);
  }
}

void BhvTouchScreenButton::handleRelease(TouchButtonState *buttonState)
{
  releaseAh(buttonState->ahs);
  if (!buttonState->onTouchEnd.IsNull())
  {
    Sqrat::Object retValue;
    buttonState->onTouchEnd.Evaluate(retValue);
  }
}

int BhvTouchScreenButton::touchEvent(ElementTree *,
  Element *elem,
  InputEvent event,
  HumanInput::IGenPointing * /*pnt*/,
  int touch_idx,
  const HumanInput::PointingRawState::Touch &touch,
  int accum_res)
{
  if (event != INP_EV_PRESS && event != INP_EV_RELEASE)
    return 0;

  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, 0);
  TouchButtonState *buttonState = elem->props.storage.RawGetSlotValue<TouchButtonState *>(strings->buttonState, nullptr);

  if (event == INP_EV_PRESS)
    if (!updateAction(buttonState, elem, strings))
      return 0;

  if (!buttonState)
    return 0;

  if (event == INP_EV_PRESS)
  {
    if (buttonState->touchIdx < 0 && elem->hitTestWithTouchMargin(Point2(touch.x, touch.y), DEVID_TOUCH) && !(accum_res & R_PROCESSED))
    {
      buttonState->touchIdx = touch_idx;
      if (!buttonState->delayedAhs.empty())
      {
        float delay = elem->props.scriptDesc.RawGetSlotValue<float>(strings->actionDelay, 0.f);
        buttonState->delayedActivationTime = get_sync_time() + delay;
      }
      if (buttonState->type == TouchButtonState::TB_TOGGLE)
      {
        if (isAnyAhPressed(buttonState->ahs))
          handleRelease(buttonState);
        else
          handlePress(buttonState);
      }
      else
        handlePress(buttonState);

      elem->setGroupStateFlags(Element::S_TOUCH_ACTIVE);

      return !buttonState->clickThrough ? R_PROCESSED : 0;
    }
  }
  else if (event == INP_EV_RELEASE) //-V547
  {
    if (buttonState->touchIdx == touch_idx)
    {
      bool delayedActAfterRelease = elem->props.scriptDesc.RawGetSlotValue<bool>(strings->delayedActionAfterRelease, false);
      if (!delayedActAfterRelease)
        buttonState->delayedActivationTime = FLT_MAX;
      releaseAh(buttonState->delayedAhs);
      buttonState->touchIdx = -1;
      if (buttonState->type != TouchButtonState::TB_TOGGLE)
        handleRelease(buttonState);

      elem->clearGroupStateFlags(Element::S_TOUCH_ACTIVE);

      return !buttonState->clickThrough ? R_PROCESSED : 0;
    }
  }
  return 0;
}

void BhvTouchScreenButton::initClickThrough(TouchButtonState *button_state, darg::Element *elem) const
{
  Sqrat::Object objAction = elem->props.scriptDesc.RawGetSlot("clickThrough");
  button_state->clickThrough = objAction.GetType() == OT_BOOL && objAction.GetVar<bool>().value;
}

bool BhvTouchScreenButton::addAction(const eastl::string &action, eastl::vector<dainput::action_handle_t> &vec)
{
  auto act = dainput::get_action_handle(action.c_str());
  if (act != dainput::BAD_ACTION_HANDLE)
    vec.push_back(act);
  else
  {
    LOGERR_CTX("Bad input action name %s", action.c_str());
    return false;
  }
  return true;
}

bool BhvTouchScreenButton::addMultipleActions(eastl::string objActions, eastl::vector<dainput::action_handle_t> &vec)
{
  if (objActions.size())
  {
    eastl::string actsString = objActions;
    int str_it = 0;
    while (str_it < actsString.size() - 1)
    {
      const char *dev_symb = ", ";
      str_it = actsString.find_first_of(dev_symb);
      if (str_it > 0)
      {
        eastl::string act = actsString.substr(0, str_it);
        actsString = actsString.substr(str_it + strlen(dev_symb), actsString.size() - 1);
        if (!addAction(act, vec))
          return false;
      }
    }
    if (!addAction(actsString, vec))
      return false;
  }
  return true;
}

bool BhvTouchScreenButton::updateAction(TouchButtonState *buttonState, darg::Element *elem, const CachedStrings *strings)
{
  Sqrat::Table &scriptDes = elem->props.scriptDesc;
  buttonState->ahs.clear();
  buttonState->releaseAhsOnPress.clear();
  buttonState->blockingAction.clear();
  buttonState->delayedAhs.clear();

  // TODO: make actions from SQArray or SQTable actions as well as string
  eastl::string objAction = scriptDes.RawGetSlotValue<eastl::string>(strings->action, "");
  eastl::string objReleaseOnPressAction = scriptDes.RawGetSlotValue<eastl::string>(strings->releaseOnPressAction, "");
  eastl::string objBlockingAction = scriptDes.RawGetSlotValue<eastl::string>(strings->blockingAction, "");
  eastl::string objDelayedAhs = scriptDes.RawGetSlotValue<eastl::string>(strings->delayedActions, "");

  bool ret = true;
  ret &= addMultipleActions(objAction, buttonState->ahs);
  ret &= addMultipleActions(objReleaseOnPressAction, buttonState->releaseAhsOnPress);
  ret &= addMultipleActions(objBlockingAction, buttonState->blockingAction);
  ret &= addMultipleActions(objDelayedAhs, buttonState->delayedAhs);

  return ret;
}

int BhvTouchScreenButton::update(UpdateStage /*stage*/, darg::Element *elem, float /*dt*/)
{
  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, 0);
  TouchButtonState *buttonState = elem->props.storage.RawGetSlotValue<TouchButtonState *>(strings->buttonState, nullptr);
  if (!buttonState->delayedAhs.empty() && get_sync_time() >= buttonState->delayedActivationTime)
  {
    pressAh(buttonState, buttonState->delayedAhs);
    buttonState->delayedActivationTime = FLT_MAX;
  }
  if (!buttonState->forceResendAh.IsNull())
  {
    Sqrat::Object retValue;
    if (buttonState->forceResendAh.Evaluate(retValue) && retValue.GetType() == OT_BOOL && retValue.GetVar<bool>().value)
    {
      handleRelease(buttonState);
      handlePress(buttonState);
    }
  }
  return 0;
}
