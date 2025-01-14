// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvDragAndDrop.h"

#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_transform.h>
#include <daRg/dag_scriptHandlers.h>

#include "inputStack.h"
#include "guiScene.h"
#include "scriptUtil.h"
#include "behaviorHelpers.h"
#include "dargDebugUtils.h"

#include <perfMon/dag_cpuFreq.h>

namespace darg
{


struct DragAndDropState
{
  InputDevice activeDeviceId = DEVID_NONE;
  int activePointerId = -1;
  int activeButtonId = -1;
  Point2 startPointerPos = ZERO<Point2>();
  Point2 startPointerOffs = ZERO<Point2>();
  bool isDragMode = false;
  bool waitForDragEnable = false;
  float maxMoveDistance = 0;
  Sqrat::Object targetHandler;
  int lastClickTime = 0;
  bool needDelayedRecalcAfterWheel = false;

  bool isClicked() const { return activeDeviceId != DEVID_NONE; }
  bool isClicked(InputDevice device, int pointer_id) const { return activeDeviceId == device && activePointerId == pointer_id; }
  bool isClicked(InputDevice device, int pointer_id, int button_id) const
  {
    return activeButtonId == button_id && isClicked(device, pointer_id);
  }

  bool isWaitingForDragEnable() const { return waitForDragEnable && isClicked(); }
  bool isWaitingForDragEnable(InputDevice device, int pointer_id) const { return waitForDragEnable && isClicked(device, pointer_id); }
  void startWaitForDragEnable(InputDevice device, int pointer_id, int button_id, const Point2 &pos, const Point2 &offs)
  {
    startClick(device, pointer_id, button_id, pos, offs);
    waitForDragEnable = true;
  }

  bool isDragging() const { return isDragMode && isClicked(); }
  bool isDragging(InputDevice device, int pointer_id) const { return isDragMode && isClicked(device, pointer_id); }
  void startDrag(InputDevice device, int pointer_id, int button_id, const Point2 &pos, const Point2 &offs)
  {
    startClick(device, pointer_id, button_id, pos, offs);
    startDrag();
  }
  void startDrag()
  {
    isDragMode = true;
    waitForDragEnable = false;
  }

  bool isDraggingOrWaitingForDragEnable(InputDevice device, int pointer_id) const
  {
    return (isDragMode || waitForDragEnable) && isClicked(device, pointer_id);
  }

  void startClick(InputDevice device, int pointer_id, int button_id, const Point2 &pos, const Point2 &offs)
  {
    activeDeviceId = device;
    activePointerId = pointer_id;
    activeButtonId = button_id;
    startPointerPos = pos;
    startPointerOffs = offs;
    isDragMode = false;
    waitForDragEnable = false;
  }

  void resetState()
  {
    activeDeviceId = DEVID_NONE;
    activePointerId = -1;
    activeButtonId = -1;
    startPointerPos = ZERO<Point2>();
    startPointerOffs = ZERO<Point2>();
    targetHandler.Release();
    maxMoveDistance = 0;
    isDragMode = false;
    waitForDragEnable = false;
    needDelayedRecalcAfterWheel = false;
  }
};


BhvDragAndDrop bhv_drag_and_drop;


BhvDragAndDrop::BhvDragAndDrop() :
  Behavior(STAGE_BEFORE_RENDER, F_HANDLE_MOUSE | F_HANDLE_TOUCH | F_CAN_HANDLE_CLICKS), activeDrag(nullptr)
{}


bool BhvDragAndDrop::willHandleClick(Element *elem)
{
  Sqrat::Object dropData = elem->props.getObject(elem->csk->dropData);
  if (!dropData.IsNull())
    return true;

  if (!elem->props.scriptDesc.RawGetSlot(elem->csk->onClick).IsNull())
    return true;
  if (!elem->props.scriptDesc.RawGetSlot(elem->csk->onDoubleClick).IsNull())
    return true;

  return false;
}


static void trace_hit(const Point2 &cursor_pos, const Element *own_elem, const InputStack &input, const Sqrat::Object &drop_data,
  InputStack *stack_point_hit, InputStack *stack_cover, InputStack *stack_overlap)
{
  for (const InputEntry &src : input.stack)
  {
    Element *target = src.elem;

    if (target->isHidden())
      continue;
    if (target == own_elem)
      continue;

    bool point_hit = target->hitTest(cursor_pos);

    if (eastl::find(target->behaviors.begin(), target->behaviors.end(), &bhv_drag_and_drop) != target->behaviors.end())
    {
      bool put_to_cover = false, put_to_overlap = false;

      if (!own_elem->transformedBbox.isempty() && !target->clippedScreenRect.isempty())
      {
        const BBox2 &b = own_elem->transformedBbox;
        BBox2 &dst = target->clippedScreenRect;
        put_to_cover = (b.left() <= dst.left() && b.top() <= dst.top() && b.right() >= dst.right() && b.bottom() >= dst.bottom());
        if (b & dst)
        {
          BBox2 intersection(::max(b.leftTop(), dst.leftTop()), ::min(b.rightBottom(), dst.rightBottom()));
          G_ASSERT(!intersection.isempty());
          float intArea = intersection.width().x * intersection.width().y;
          float eArea = b.width().x * b.width().y;
          float dstArea = dst.width().x * dst.width().y;
          const float areaThreshold = 0.6;
          if (intArea > eArea * areaThreshold || intArea >= dstArea * areaThreshold)
            put_to_overlap = true;
        }
      }

      if (point_hit || put_to_cover || put_to_overlap)
      {
        Sqrat::Object onDrop = target->props.scriptDesc.RawGetSlot(target->csk->onDrop);
        if (!onDrop.IsNull())
        {
          bool allowDrop = false;
          Sqrat::Object canDrop = target->props.scriptDesc.RawGetSlot(target->csk->canDrop);
          if (!canDrop.IsNull())
          {
            Sqrat::Function dropCheck(canDrop.GetVM(), Sqrat::Object(canDrop.GetVM()), canDrop);
            bool checkRes = false;
            if (!dropCheck.Evaluate(drop_data, checkRes))
              darg_assert_trace_var("Failed to call canDrop handler", target->props.scriptDesc, target->csk->canDrop);
            else
              allowDrop = checkRes;
          }
          else
            allowDrop = true;

          if (allowDrop) //-V547 False positive: Expression 'allowDrop' is always true
          {
            InputEntry ie;
            ie.elem = target;
            ie.hierOrder = src.hierOrder;
            ie.zOrder = src.zOrder;
            if (point_hit)
              stack_point_hit->push(ie);
            if (put_to_cover)
              stack_cover->push(ie);
            if (put_to_overlap)
              stack_overlap->push(ie);
          }
        }
      }
    }

    if (point_hit && target->hasFlags(Element::F_STOP_MOUSE))
      break;
  }
}


Element *BhvDragAndDrop::findDropTarget(ElementTree *etree, const Point2 &cursor_pos, const Element *own_elem, Sqrat::Object &handler)
{
  Sqrat::Object dropData = own_elem->props.getObject(own_elem->csk->dropData);

  InputStack stack_point_hit, stack_cover, stack_overlap;
  trace_hit(cursor_pos, own_elem, etree->guiScene->getInputStack(), dropData, &stack_point_hit, &stack_cover, &stack_overlap);

  Element *target = nullptr;

  if (stack_cover.stack.size() == 1)
    target = stack_cover.stack.begin()->elem;
  else if (stack_overlap.stack.size() == 1)
    target = stack_overlap.stack.begin()->elem;
  else if (stack_point_hit.stack.size())
    target = stack_point_hit.stack.begin()->elem;

  if (target)
  {
    handler = target->props.scriptDesc.RawGetSlot(target->csk->onDrop);
    G_ASSERT(!handler.IsNull());
  }

  return target;
}


Element *BhvDragAndDrop::findElemByHandler(ElementTree *etree, const Sqrat::Object &handler)
{
  InputStack &inputStack = etree->guiScene->getInputStack();
  for (InputStack::Stack::iterator it = inputStack.stack.begin(); it != inputStack.stack.end(); ++it)
  {
    Sqrat::Object onDrop = it->elem->props.scriptDesc.RawGetSlot(it->elem->csk->onDrop);
    if (!onDrop.IsNull() && onDrop.IsEqual(handler))
      return it->elem;
  }
  return nullptr;
}


void BhvDragAndDrop::activateTarget(ElementTree *etree, Element *elem, DragAndDropState *dd_state, Element *new_target_elem,
  const Sqrat::Object &new_target_handler, int state_flags)
{
  G_UNUSED(elem);
  G_ASSERT_RETURN(dd_state, );
  G_ASSERT(!new_target_elem == new_target_handler.IsNull());

  if (!are_sq_obj_equal(etree->guiScene->getScriptVM(), new_target_handler.GetObject(), dd_state->targetHandler.GetObject()))
  {
    if (!dd_state->targetHandler.IsNull())
    {
      Element *prevTargetElem = findElemByHandler(etree, dd_state->targetHandler);
      if (prevTargetElem)
        prevTargetElem->clearGroupStateFlags(state_flags);
    }
    if (new_target_elem)
      new_target_elem->setGroupStateFlags(state_flags);

    dd_state->targetHandler = new_target_handler;
  }
}


void BhvDragAndDrop::callDragModeHandler(IGuiScene *scene, Element *elem, bool mode_on)
{
  Sqrat::Object onDragMode = elem->props.scriptDesc.RawGetSlot(elem->csk->onDragMode);
  if (!onDragMode.IsNull())
  {
    HSQUIRRELVM vm = onDragMode.GetVM();
    Sqrat::Function f(vm, Sqrat::Object(vm), onDragMode);
    Sqrat::Object data = elem->props.getObject(elem->csk->dropData);
    scene->queueScriptHandler(new ScriptHandlerSqFunc<bool, Sqrat::Object>(f, mode_on, data));
  }
}


int BhvDragAndDrop::mouseEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int pointer_id, int data,
  short mx, short my, int /*buttons*/, int accum_res)
{
  return pointingEvent(etree, elem, device, event, pointer_id, data, Point2(mx, my), accum_res);
}


int BhvDragAndDrop::touchEvent(ElementTree *etree, Element *elem, InputEvent event, HumanInput::IGenPointing *pnt, int touch_idx,
  const HumanInput::PointingRawState::Touch &touch, int accum_res)
{
  G_UNUSED(pnt);
  return pointingEvent(etree, elem, DEVID_TOUCH, event, touch_idx, 0, Point2(touch.x, touch.y), accum_res);
}


int BhvDragAndDrop::pointingEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int pointer_id,
  int button_id, const Point2 &pointer_pos, int accum_res)
{
  int result = 0;

  DragAndDropState *ddState = elem->props.storage.RawGetSlotValue<DragAndDropState *>(elem->csk->dragAndDropState, nullptr);
  if (!ddState)
    return 0;

  Sqrat::Object dropData = elem->props.getObject(elem->csk->dropData);

  int activeStateFlag = active_state_flags_for_device(device);

  if (event == INP_EV_PRESS)
  {
    if (ddState->isClicked())
      return 0;
    if (!elem->hitTest(pointer_pos))
      return 0;
    if (accum_res & R_PROCESSED)
      return 0;
    if (activeDrag != nullptr)
      return 0;

    bool canDrag = !dropData.IsNull();

    if (canDrag)
    {
      ddState->startWaitForDragEnable(device, pointer_id, button_id, pointer_pos, pointer_pos - elem->calcTransformedBbox().leftTop());
      activeDrag = ddState;
      // etree->guiScene->updateHover(pointer_pos);
      result |= R_PROCESSED | R_REBUILD_RENDER_AND_INPUT_LISTS;
    }
    else
    {
      bool canClick = !elem->props.scriptDesc.RawGetSlot(elem->csk->onClick).IsNull() ||
                      !elem->props.scriptDesc.RawGetSlot(elem->csk->onDoubleClick).IsNull();

      if (canClick)
      {
        ddState->startClick(device, pointer_id, button_id, pointer_pos, pointer_pos - elem->calcTransformedBbox().leftTop());
        elem->setGroupStateFlags(activeStateFlag);
        result |= R_PROCESSED;
      }
    }
  }
  else if (event == INP_EV_RELEASE)
  {
    if (ddState->isClicked(device, pointer_id, button_id))
    {
      Element *target = nullptr;
      if (ddState->isDragMode)
      {
        Sqrat::Object handler;
        target = findDropTarget(etree, pointer_pos, elem, handler);

        if (target)
        {
          target->clearGroupStateFlags(activeStateFlag);

          HSQUIRRELVM vm = handler.GetVM();
          Sqrat::Function f(vm, Sqrat::Object(vm), handler);
          f(elem->props.getObject(elem->csk->dropData));
        }

        callDragModeHandler(etree->guiScene, elem, false);

        activateTarget(etree, elem, ddState, nullptr, Sqrat::Object(), activeStateFlag);

        etree->updateSceneStateFlags(ElementTree::F_DRAG_ACTIVE, false);
      }

      if (!target && elem->hitTest(pointer_pos))
      {
        if (ddState->maxMoveDistance < move_click_threshold(etree->guiScene))
        {
          bool isDouble = false;

          int curTime = ::get_time_msec();
          if (ddState->lastClickTime != 0 && (curTime - ddState->lastClickTime <= double_click_interval_ms))
            isDouble = true;
          ddState->lastClickTime = curTime;

          call_click_handler(etree->guiScene, elem, device, isDouble, button_id, pointer_pos.x, pointer_pos.y);
        }
      }

      elem->clearGroupStateFlags(Element::S_DRAG | activeStateFlag);

      if (ddState == activeDrag)
        activeDrag = nullptr;
      ddState->resetState();
      if (elem->transform)
        elem->transform->translate.zero();
      elem->updFlags(Element::F_ZORDER_ON_TOP, false);
      result |= R_PROCESSED | R_REBUILD_RENDER_AND_INPUT_LISTS;
    }
  }
  else if (event == INP_EV_POINTER_MOVE)
  {
    if (ddState->isDraggingOrWaitingForDragEnable(device, pointer_id))
    {
      applyPointerMove(pointer_pos, ddState, elem, etree, activeStateFlag);

      result |= R_PROCESSED;
    }
  }
  else if (event == INP_EV_MOUSE_WHEEL)
  {
    if (ddState->isDraggingOrWaitingForDragEnable(device, pointer_id))
    {
      // handle hierarchic scroll
      ddState->needDelayedRecalcAfterWheel = true;
    }
  }

  return result;
}

void BhvDragAndDrop::startVisualDrag(ElementTree *etree, darg::Element *elem)
{
  elem->setGroupStateFlags(Element::S_DRAG);
  elem->updFlags(Element::F_ZORDER_ON_TOP, true);
  callDragModeHandler(etree->guiScene, elem, true);
  etree->updateSceneStateFlags(ElementTree::F_DRAG_ACTIVE, true);
}

void BhvDragAndDrop::applyPointerMove(const Point2 &pointer_pos, darg::DragAndDropState *ddState, darg::Element *elem,
  darg::ElementTree *etree, int activeStateFlag)
{
  Point2 dPos = pointer_pos - ddState->startPointerPos;

  ddState->maxMoveDistance = ::max(ddState->maxMoveDistance, ::max(fabsf(dPos.x), fabsf(dPos.y)));

  if (ddState->isWaitingForDragEnable())
  {
    if (ddState->maxMoveDistance < move_click_threshold(etree->guiScene))
      return;

    ddState->startDrag();
    startVisualDrag(etree, elem);
  }

  if (elem->transform)
  {
    // works only with transforms consisting of translations
    elem->transform->translate += pointer_pos - (elem->calcTransformedBbox().leftTop() + ddState->startPointerOffs);
  }

  Sqrat::Object curTargetHandler;
  Element *curTarget = findDropTarget(etree, pointer_pos, elem, curTargetHandler);
  activateTarget(etree, elem, ddState, curTarget, curTargetHandler, activeStateFlag);
}


void BhvDragAndDrop::onAttach(Element *elem)
{
  DragAndDropState *ddState = new DragAndDropState();
  elem->props.storage.SetInstance(elem->csk->dragAndDropState, ddState);
}


void BhvDragAndDrop::onDetach(Element *elem, DetachMode)
{
  DragAndDropState *ddState = elem->props.storage.RawGetSlotValue<DragAndDropState *>(elem->csk->dragAndDropState, nullptr);
  if (ddState)
  {
    if (ddState->isDragging())
    {
      GuiScene *guiScene = GuiScene::get_from_elem(elem);
      callDragModeHandler(guiScene, elem, false);
    }

    if (ddState == activeDrag)
      activeDrag = nullptr;
    delete ddState;
    elem->props.storage.DeleteSlot(elem->csk->dragAndDropState);
  }
}


int BhvDragAndDrop::onDeactivateInput(Element *elem, InputDevice device, int pointer_id)
{
  DragAndDropState *ddState = elem->props.storage.RawGetSlotValue<DragAndDropState *>(elem->csk->dragAndDropState, nullptr);

  G_ASSERT_RETURN(ddState, 0);
  if (!ddState->isClicked(device, pointer_id))
    return 0;

  if (ddState->isDragMode)
    elem->etree->updateSceneStateFlags(ElementTree::F_DRAG_ACTIVE, false);

  if (elem->transform)
    elem->transform->translate.zero();
  elem->updFlags(Element::F_ZORDER_ON_TOP, false);
  elem->clearGroupStateFlags(Element::S_DRAG | active_state_flags_for_device(ddState->activeDeviceId));

  if (ddState == activeDrag)
    activeDrag = nullptr;
  ddState->resetState();

  return R_REBUILD_RENDER_AND_INPUT_LISTS;
}


int BhvDragAndDrop::update(UpdateStage /*stage*/, Element *elem, float /*dt*/)
{
  DragAndDropState *ddState = elem->props.storage.RawGetSlotValue<DragAndDropState *>(elem->csk->dragAndDropState, nullptr);
  if (!ddState)
    return 0;

  if (ddState->needDelayedRecalcAfterWheel)
  {
    G_ASSERT(ddState->activeDeviceId != DEVID_NONE);
    // Assume that there is only one mouse maximum
    // If parent was scrolled, refresh relative positions
    Point2 mousePos = GuiScene::get_from_elem(elem)->getMousePos();
    ElementTree *etree = elem->etree;
    int activeStateFlag = active_state_flags_for_device(ddState->activeDeviceId);

    applyPointerMove(mousePos, ddState, elem, etree, activeStateFlag);

    ddState->needDelayedRecalcAfterWheel = false;
  }

  return 0;
}

} // namespace darg
