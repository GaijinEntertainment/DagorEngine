// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvTouchScreenHoverButton.h"
#include "bhvTouchScreenButton.h"

#include <daRg/dag_guiScene.h>
#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_scriptHandlers.h>

#include <drv/hid/dag_hiPointing.h>
#include <drv/hid/dag_hiGlobals.h>
#include <startup/dag_inpDevClsDrv.h>

#include "input/touchInput.h"

#include <cstring>

using namespace darg;
using namespace dainput;

SQ_PRECACHED_STRINGS_REGISTER_WITH_BHV(BhvTouchScreenHoverButton, bhv_touch_screen_hover_button, cstr);

BhvTouchScreenHoverButton::BhvTouchScreenHoverButton() : BhvTouchScreenButton() {}

int BhvTouchScreenHoverButton::touchEvent(ElementTree *,
  Element *elem,
  InputEvent event,
  HumanInput::IGenPointing * /*pnt*/,
  int touch_idx,
  const HumanInput::PointingRawState::Touch &touch,
  int accum_res)
{
  if (event != INP_EV_PRESS && event != INP_EV_RELEASE && event != INP_EV_POINTER_MOVE)
    return 0;

  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, 0);
  TouchButtonState *buttonState = elem->props.storage.RawGetSlotValue<TouchButtonState *>(strings->buttonState, nullptr);

  if (!buttonState || buttonState->ahs.size() == 0)
    return 0;

  Sqrat::Function onTouchEnd = elem->props.scriptDesc.GetFunction("onTouchEnd");
  bool useActionOnTouchEnd = elem->props.scriptDesc.RawGetSlotValue("useActionOnTouchEnd", false);

  if (event == INP_EV_PRESS || event == INP_EV_POINTER_MOVE)
  {
    if (buttonState->touchIdx < 0 && elem->hitTest(touch.x, touch.y) && !(accum_res & R_PROCESSED))
    {
      buttonState->touchIdx = touch_idx;
      if (!useActionOnTouchEnd)
        handlePress(buttonState);
      elem->setGroupStateFlags(Element::S_TOUCH_ACTIVE);
      return R_PROCESSED;
    }

    if (buttonState->touchIdx == touch_idx && !elem->hitTest(touch.x, touch.y))
    {
      buttonState->touchIdx = -1;
      if (!useActionOnTouchEnd)
        handleRelease(buttonState);
      elem->clearGroupStateFlags(Element::S_TOUCH_ACTIVE);
      return R_PROCESSED;
    }
  }
  else if (event == INP_EV_RELEASE) //-V547
  {
    if (buttonState->touchIdx == touch_idx && elem->hitTest(touch.x, touch.y))
    {
      buttonState->touchIdx = -1;
      if (useActionOnTouchEnd)
      {
        handlePress(buttonState);
        if (!onTouchEnd.IsNull())
          onTouchEnd();
      }

      elem->clearGroupStateFlags(Element::S_TOUCH_ACTIVE);
      return R_PROCESSED;
    }
  }
  return 0;
}
