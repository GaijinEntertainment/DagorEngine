// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvDragAndDrop.h"

#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_transform.h>
#include <daRg/dag_scriptHandlers.h>
#include <drv/hid/dag_hiKeyboard.h>
#include <startup/dag_inpDevClsDrv.h>

#include "eventData.h"
#include "inputStack.h"
#include "guiScene.h"
#include "scriptUtil.h"
#include "behaviorHelpers.h"
#include "dargDebugUtils.h"

#include <perfMon/dag_cpuFreq.h>

// Attach element to cursor at the moment when drag starts.
// Otherwise drag it by the point where it was clicked initially.
// Pros: no jump over gap when drag starts.
// Cons: me be perceived as a lag.
// Turn off for now to keep the current behavior.
#define REANCHOR_ON_DRAG_START 0

namespace darg
{


static bool can_drag_elem(const Element *elem) { return !elem->props.getObject(elem->csk->dropData).IsNull(); }


struct DragAndDropState
{
  InputDevice activeDeviceId = DEVID_NONE;
  int activePointerId = -1;
  int activeButtonId = -1;
  Point2 startPointerPos = ZERO<Point2>();
  Point2 startPointerOffs = ZERO<Point2>();
  bool isDragMode = false;
  float maxMoveDistance = 0;
  Sqrat::Object targetHandler;
  int lastClickTime = 0;
  int pressStartTime = 0;
  int dragStartDelay = 0;
  bool needDelayedRecalcAfterWheel = false;

  bool isClicked() const { return activeDeviceId != DEVID_NONE; }
  bool isClicked(InputDevice device, int pointer_id) const { return activeDeviceId == device && activePointerId == pointer_id; }
  bool isClicked(InputDevice device, int pointer_id, int button_id) const
  {
    return activeButtonId == button_id && isClicked(device, pointer_id);
  }

  bool isWaitingToStartDrag(const Element *elem) const { return !isDragMode && isClicked() && can_drag_elem(elem); }

  bool isDragging() const { return isDragMode && isClicked(); }
  bool isDragging(InputDevice device, int pointer_id) const { return isDragMode && isClicked(device, pointer_id); }
  void startDrag(const Point2 &offs)
  {
    isDragMode = true;
    startPointerOffs = offs;
  }

  void startClick(InputDevice device, int pointer_id, int button_id, const Point2 &pos, const Point2 &offs)
  {
    activeDeviceId = device;
    activePointerId = pointer_id;
    activeButtonId = button_id;
    startPointerPos = pos;
    startPointerOffs = offs;
    isDragMode = false;
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
    needDelayedRecalcAfterWheel = false;
    pressStartTime = 0;
  }

  void readParams(Element *elem)
  {
    dragStartDelay = ::max<int>(0, int(elem->props.getFloat(elem->csk->dragStartDelay, 0, true) * 1000));
  }
};


BhvDragAndDrop bhv_drag_and_drop;


BhvDragAndDrop::BhvDragAndDrop() :
  Behavior(STAGE_BEFORE_RENDER, F_HANDLE_MOUSE | F_HANDLE_TOUCH | F_CAN_HANDLE_CLICKS), activeDrag(nullptr)
{}


bool BhvDragAndDrop::willHandleClick(Element *elem)
{
  if (can_drag_elem(elem))
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
            auto checkRes = dropCheck.Eval<bool>(drop_data);
            if (!checkRes)
              darg_assert_trace_var("Failed to call canDrop handler", target->props.scriptDesc, target->csk->canDrop);
            else
              allowDrop = checkRes.value();
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

    if (point_hit && target->hasFlags(Element::F_STOP_POINTING))
      break;
  }
}


Element *BhvDragAndDrop::findDropTarget(ElementTree *etree, const Point2 &cursor_pos, const Element *own_elem, Sqrat::Object &handler)
{
  Sqrat::Object dropData = own_elem->props.getObject(own_elem->csk->dropData);

  InputStack stack_point_hit, stack_cover, stack_overlap;
  trace_hit(cursor_pos, own_elem, etree->screen->getInputStack(), dropData, &stack_point_hit, &stack_cover, &stack_overlap);

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
  InputStack &inputStack = etree->screen->getInputStack();
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


int BhvDragAndDrop::pointingEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int pointer_id,
  int button_id, Point2 pointer_pos, int accum_res)
{
  int result = 0;

  DragAndDropState *ddState = elem->props.storage.RawGetSlotValue<DragAndDropState *>(elem->csk->dragAndDropState, nullptr);
  if (!ddState)
    return 0;

  int activeStateFlag = active_state_flags_for_device(device);
  const int isProcessed = !elem->props.getBool(elem->csk->eventPassThrough, false) ? R_PROCESSED : 0;

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

    if (ddState->dragStartDelay != 0)
      ddState->pressStartTime = ::get_time_msec();
    else
      ddState->pressStartTime = 0;

    if (can_drag_elem(elem))
    {
      ddState->startClick(device, pointer_id, button_id, pointer_pos, pointer_pos - elem->calcTransformedBbox().leftTop());
      elem->setGroupStateFlags(activeStateFlag);
      activeDrag = ddState;
      // etree->guiScene->updateMainScreenHover(pointer_pos);
      result |= isProcessed | R_REBUILD_RENDER_AND_INPUT_LISTS;
    }
    else
    {
      bool canClick = !elem->props.scriptDesc.RawGetSlot(elem->csk->onClick).IsNull() ||
                      !elem->props.scriptDesc.RawGetSlot(elem->csk->onDoubleClick).IsNull();

      if (canClick)
      {
        ddState->startClick(device, pointer_id, button_id, pointer_pos, pointer_pos - elem->calcTransformedBbox().leftTop());
        elem->setGroupStateFlags(activeStateFlag);
        result |= isProcessed;
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

          SQInteger nparams = 0, nfreevars = 0;
          G_VERIFY(get_closure_info(f, &nparams, &nfreevars));
          if (nparams == 2)
            f(elem->props.getObject(elem->csk->dropData));
          else if (nparams == 3)
          {
            MouseClickEventData evt;
            evt.button = button_id;
            evt.screenX = pointer_pos.x;
            evt.screenY = pointer_pos.y;
            evt.target = elem->getRef(f.GetVM());
            calc_elem_rect_for_event_handler(evt.targetRect, target);

            const HumanInput::IGenKeyboard *kbd = global_cls_drv_kbd ? global_cls_drv_kbd->getDevice(0) : NULL;
            if (kbd)
            {
              int modifiers = kbd->getRawState().shifts;
              evt.shiftKey = (modifiers & HumanInput::KeyboardRawState::SHIFT_BITS) != 0;
              evt.ctrlKey = (modifiers & HumanInput::KeyboardRawState::CTRL_BITS) != 0;
              evt.altKey = (modifiers & HumanInput::KeyboardRawState::ALT_BITS) != 0;
            }

            f(elem->props.getObject(elem->csk->dropData), evt);
          }
          else
          {
            darg_assert_trace_var("Only `onDrop(dropData)` and `onDrop(dropData, evt)` are supported", elem->props.scriptDesc,
              handler);
            return 0;
          }
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
      result |= isProcessed | R_REBUILD_RENDER_AND_INPUT_LISTS;
    }
  }
  else if (event == INP_EV_POINTER_MOVE)
  {
    if (ddState->isClicked(device, pointer_id))
      result |= applyPointerMove(pointer_pos, ddState, elem, etree, activeStateFlag);
  }
  else if (event == INP_EV_MOUSE_WHEEL)
  {
    if (ddState->isClicked(device, pointer_id))
    {
      // handle hierarchic scroll
      ddState->needDelayedRecalcAfterWheel = true;
    }
  }

  return result;
}

int BhvDragAndDrop::startVisualDrag(ElementTree *etree, darg::Element *elem)
{
  elem->setGroupStateFlags(Element::S_DRAG);
  elem->updFlags(Element::F_ZORDER_ON_TOP, true);
  callDragModeHandler(etree->guiScene, elem, true);
  etree->updateSceneStateFlags(ElementTree::F_DRAG_ACTIVE, true);
  return R_REBUILD_RENDER_AND_INPUT_LISTS;
}

int BhvDragAndDrop::applyPointerMove(const Point2 &pointer_pos, darg::DragAndDropState *ddState, darg::Element *elem,
  darg::ElementTree *etree, int activeStateFlag)
{
  Point2 dPos = pointer_pos - ddState->startPointerPos;

  ddState->maxMoveDistance = ::max(ddState->maxMoveDistance, ::max(fabsf(dPos.x), fabsf(dPos.y)));

  int result = !elem->props.getBool(elem->csk->eventPassThrough, false) ? R_PROCESSED : 0;
  if (ddState->isWaitingToStartDrag(elem))
  {
    if (ddState->dragStartDelay > 0)
      return 0; // Don't mark as processed
    if (ddState->maxMoveDistance < move_click_threshold(etree->guiScene))
      return 0; // Don't mark as processed

#if REANCHOR_ON_DRAG_START
    BBox2 bbox = elem->calcTransformedBbox();
    Point2 offs = pointer_pos - bbox.leftTop();
    offs.x = clamp(offs.x, 0.0f, bbox.width().x);
    offs.y = clamp(offs.y, 0.0f, bbox.width().y);
#else
    Point2 offs = ddState->startPointerOffs;
#endif

    ddState->startDrag(offs);
    result |= startVisualDrag(etree, elem);
  }

  if (ddState->isDragMode && elem->transform)
  {
    // works only with transforms consisting of translations
    if (elem->props.getBool(elem->csk->dragByPivot, false))
    {
      GuiVertexTransform gvtm;
      elem->calcFullTransform(gvtm);

      Point2 origPivot = mul(elem->transform->pivot, elem->screenCoord.size) + elem->screenCoord.screenPos;
      float px = origPivot.x * gvtm.vtm[0][0] + origPivot.y * gvtm.vtm[0][1] + gvtm.vtm[0][2];
      float py = origPivot.x * gvtm.vtm[1][0] + origPivot.y * gvtm.vtm[1][1] + gvtm.vtm[1][2];
      elem->transform->translate += pointer_pos - Point2(px, py) * GUI_POS_SCALE_INV;
    }
    else
    {
      elem->transform->translate += pointer_pos - (elem->calcTransformedBbox().leftTop() + ddState->startPointerOffs);
    }
  }

  Sqrat::Object curTargetHandler;
  Element *curTarget = findDropTarget(etree, pointer_pos, elem, curTargetHandler);
  activateTarget(etree, elem, ddState, curTarget, curTargetHandler, activeStateFlag);
  return result;
}


void BhvDragAndDrop::onAttach(Element *elem)
{
  DragAndDropState *ddState = new DragAndDropState();
  elem->props.storage.SetInstance(elem->csk->dragAndDropState, ddState);

  ddState->readParams(elem);
}


void BhvDragAndDrop::onElemSetup(Element *elem, SetupMode mode)
{
  if (mode == SM_REBUILD_UPDATE)
  {
    DragAndDropState *ddState = elem->props.storage.RawGetSlotValue<DragAndDropState *>(elem->csk->dragAndDropState, nullptr);
    G_ASSERT_RETURN(ddState, );
    ddState->readParams(elem);
  }
}


void BhvDragAndDrop::onDetach(Element *elem, DetachMode)
{
  DragAndDropState *ddState = elem->props.storage.RawGetSlotValue<DragAndDropState *>(elem->csk->dragAndDropState, nullptr);
  if (ddState)
  {
    if (ddState->isDragging())
    {
      elem->etree->updateSceneStateFlags(ElementTree::F_DRAG_ACTIVE, false);
      GuiScene *guiScene = GuiScene::get_from_elem(elem);
      callDragModeHandler(guiScene, elem, false);
    }

    if (ddState == activeDrag)
      activeDrag = nullptr;
    delete ddState;
    elem->props.storage.DeleteSlot(elem->csk->dragAndDropState);
  }
}


int BhvDragAndDrop::deactivateInputImpl(Element *elem, DragAndDropState *ddState)
{
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


int BhvDragAndDrop::onDeactivateInput(Element *elem, InputDevice device, int pointer_id)
{
  DragAndDropState *ddState = elem->props.storage.RawGetSlotValue<DragAndDropState *>(elem->csk->dragAndDropState, nullptr);

  G_ASSERT_RETURN(ddState, 0);
  if (!ddState->isClicked(device, pointer_id))
    return 0;

  return deactivateInputImpl(elem, ddState);
}


int BhvDragAndDrop::onDeactivateAllInput(Element *elem)
{
  DragAndDropState *ddState = elem->props.storage.RawGetSlotValue<DragAndDropState *>(elem->csk->dragAndDropState, nullptr);

  G_ASSERT_RETURN(ddState, 0);
  if (!ddState->isClicked())
    return 0;

  return deactivateInputImpl(elem, ddState);
}


int BhvDragAndDrop::startDragAfterDelay(darg::DragAndDropState *ddState, darg::Element *elem)
{
  ElementTree *etree = elem->etree;
  int result = 0;
  if (ddState->isWaitingToStartDrag(elem) && ddState->maxMoveDistance < move_click_threshold(etree->guiScene))
  {
    Point2 offs = ddState->startPointerOffs; // reuse original from the initial click
    ddState->startDrag(offs);
    result = startVisualDrag(etree, elem);
  }
  return result;
}


int BhvDragAndDrop::update(UpdateStage /*stage*/, Element *elem, float /*dt*/)
{
  DragAndDropState *ddState = elem->props.storage.RawGetSlotValue<DragAndDropState *>(elem->csk->dragAndDropState, nullptr);
  if (!ddState)
    return 0;

  int result = 0;
  if (ddState->needDelayedRecalcAfterWheel)
  {
    G_ASSERT(ddState->activeDeviceId != DEVID_NONE);
    // Assume that there is only one mouse maximum
    // If parent was scrolled, refresh relative positions
    Point2 mousePos = GuiScene::get_from_elem(elem)->activePointerPos();
    ElementTree *etree = elem->etree;
    int activeStateFlag = active_state_flags_for_device(ddState->activeDeviceId);

    applyPointerMove(mousePos, ddState, elem, etree, activeStateFlag);

    ddState->needDelayedRecalcAfterWheel = false;
  }

  if (ddState->pressStartTime > 0 && (ddState->pressStartTime + ddState->dragStartDelay <= ::get_time_msec()))
  {
    ddState->pressStartTime = 0;
    result = startDragAfterDelay(ddState, elem);
  }

  return result;
}

} // namespace darg
