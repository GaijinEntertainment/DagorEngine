// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvElementEditor.h"

#include <daRg/dag_guiScene.h>
#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_scriptHandlers.h>

#include <drv/hid/dag_hiPointing.h>
#include <drv/hid/dag_hiGlobals.h>
#include <startup/dag_inpDevClsDrv.h>
#include <daRg/dag_transform.h>

#include "input/touchInput.h"

#include <cstring>

#define NO_TOUCH -1

using namespace darg;
using namespace dainput;


SQ_PRECACHED_STRINGS_REGISTER_WITH_BHV(BhvElementEditor, bhv_element_editor, cstr);

struct ElementEditorState
{
  Point2 startTranslate = ZERO<Point2>();
  Point2 startScale = ZERO<Point2>();

  Point2 startPointerPos = ZERO<Point2>();
  Point2 firstTouchPos = ZERO<Point2>();

  Point2 secondStartPointerPos = ZERO<Point2>();
  Point2 secondTouchPos = ZERO<Point2>();

  int touchIdx = -1;
  int secondTouchIdx = -1;
  float twoTouchStartDist = 0;
  bool blockMove = false; // block move after scaling
};

BhvElementEditor::BhvElementEditor() : Behavior(darg::Behavior::STAGE_ACT, F_HANDLE_TOUCH) {}

void BhvElementEditor::onAttach(Element *elem)
{
  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, );

  ElementEditorState *elementEditorState = new ElementEditorState();
  elem->props.storage.SetValue(strings->elementEditorState, elementEditorState);
}

void BhvElementEditor::onDetach(Element *elem, DetachMode)
{
  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, );

  ElementEditorState *elementEditorState =
    elem->props.storage.RawGetSlotValue<ElementEditorState *>(strings->elementEditorState, nullptr);
  if (elementEditorState)
  {
    elem->props.storage.DeleteSlot(strings->elementEditorState);
    delete elementEditorState;
  }
}

int BhvElementEditor::touchEvent(ElementTree *,
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
  ElementEditorState *elementEditorState =
    elem->props.storage.RawGetSlotValue<ElementEditorState *>(strings->elementEditorState, nullptr);

  if (!elementEditorState)
    return 0;

  if (event == INP_EV_PRESS && (elementEditorState->touchIdx == NO_TOUCH || elementEditorState->secondTouchIdx == NO_TOUCH))
  {
    touchBegin(elementEditorState, elem, touch_idx, accum_res, Point2(touch.x, touch.y));
    if (!(accum_res & R_PROCESSED) && elem->hitTest(touch.x, touch.y))
      return R_PROCESSED;
  }
  else if (event == INP_EV_POINTER_MOVE)
  {
    if (elementEditorState->touchIdx == touch_idx || elementEditorState->secondTouchIdx == touch_idx)
    {
      touchMove(elementEditorState, touch_idx, Point2(touch.x, touch.y));

      if (elem->transform)
      {
        if (elementEditorState->touchIdx != NO_TOUCH && elementEditorState->secondTouchIdx != NO_TOUCH)
        {
          Sqrat::Table &scriptDesc = elem->props.scriptDesc;
          float elementMaxScale = scriptDesc.RawGetSlotValue<float>(strings->elementMaxScale, 2.f);
          float elementMinScale = scriptDesc.RawGetSlotValue<float>(strings->elementMinScale, 0.5f);
          float elementScaleRatio = scriptDesc.RawGetSlotValue<float>(strings->elementScaleRatio, 0.2f);

          float curDist = (elementEditorState->firstTouchPos - elementEditorState->secondTouchPos).lengthF();
          float scale = (curDist / elementEditorState->twoTouchStartDist) - 1.f;
          scale = elementEditorState->startScale.x + (scale * elementScaleRatio);
          scale = ::clamp(scale, elementMinScale, elementMaxScale);

          elem->transform->scale = Point2(scale, scale);
        }
        else if (elementEditorState->touchIdx != NO_TOUCH && !elementEditorState->blockMove)
        {
          Point2 dPos = Point2(touch.x, touch.y) - elementEditorState->startPointerPos;
          elem->transform->translate = elementEditorState->startTranslate + dPos;
        }
        onElementEdited(elem, elem->transform->translate, elem->transform->scale);
      }
    }
    if (elem->hitTest(touch.x, touch.y))
      return R_PROCESSED;
  }
  else if (event == INP_EV_RELEASE)
  {
    touchEnd(elementEditorState, elem, touch_idx);
    if (elementEditorState->touchIdx == touch_idx)
      elem->clearGroupStateFlags(Element::S_TOUCH_ACTIVE);
    if (elem->hitTest(touch.x, touch.y))
      return R_PROCESSED;
  }
  return 0;
}

void BhvElementEditor::touchBegin(ElementEditorState *state, Element *elem, int idx, int elemFlags, Point2 pos)
{
  if (state->touchIdx == NO_TOUCH)
  {
    if (!(elemFlags & R_PROCESSED) && elem->hitTest(pos.x, pos.y))
    {
      state->startPointerPos = pos;
      state->startTranslate = elem->transform->translate;
      state->startScale = elem->transform->scale;
      state->touchIdx = idx;
      elem->setGroupStateFlags(Element::S_TOUCH_ACTIVE);

      Sqrat::Function onElementEdited = elem->props.scriptDesc.GetFunction("onTouch");
      if (!onElementEdited.IsNull())
        onElementEdited();
    }
  }
  else
  {
    state->secondTouchIdx = idx;
    state->secondStartPointerPos = pos;
  }

  if (state->touchIdx != NO_TOUCH && state->secondTouchIdx != NO_TOUCH)
  {
    state->blockMove = true;
    state->twoTouchStartDist = (state->startPointerPos - state->secondStartPointerPos).lengthF();
  }
  touchMove(state, idx, pos);
}

void BhvElementEditor::touchEnd(ElementEditorState *state, Element *elem, int idx)
{
  if (state->touchIdx == idx)
  {
    state->touchIdx = NO_TOUCH;
    state->startPointerPos = ZERO<Point2>();
    state->startTranslate = elem->transform->translate;
    state->startScale = elem->transform->scale;
    state->firstTouchPos = ZERO<Point2>();
  }
  else if (state->secondTouchIdx == idx)
  {
    state->secondTouchIdx = NO_TOUCH;
    state->secondStartPointerPos = ZERO<Point2>();
    state->secondTouchPos = ZERO<Point2>();
    state->startScale = elem->transform->scale;
  }

  if (state->touchIdx == NO_TOUCH || state->secondTouchIdx == NO_TOUCH)
  {
    state->twoTouchStartDist = 0;
  }
  if (state->touchIdx == NO_TOUCH && state->secondTouchIdx == NO_TOUCH)
    state->blockMove = false;
}

void BhvElementEditor::touchMove(ElementEditorState *state, int idx, Point2 pos)
{
  if (state->touchIdx == idx)
  {
    state->firstTouchPos = pos;
  }
  else if (state->secondTouchIdx == idx)
  {
    state->secondTouchPos = pos;
  }
}

void BhvElementEditor::onElementEdited(darg::Element *elem, Point2 translate, Point2 scale)
{
  Sqrat::Function onElementEdited = elem->props.scriptDesc.GetFunction("onElementEdited");
  if (onElementEdited.IsNull())
    return;
  HSQUIRRELVM vm = onElementEdited.GetVM();

  Sqrat::Table item(vm);
  item.SetValue("translate", translate);
  item.SetValue("scale", scale);

  onElementEdited(item);
}