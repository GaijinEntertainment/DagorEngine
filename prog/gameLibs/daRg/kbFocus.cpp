// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "kbFocus.h"
#include "elementTree.h"
#include "guiScene.h"
#include "textUtil.h"

#include <daRg/dag_behavior.h>
#include <daRg/dag_element.h>
#include <daRg/dag_scriptHandlers.h>
#include <daRg/dag_stringKeys.h>

#include <drv/hid/dag_hiKeyboard.h>
#include <startup/dag_inpDevClsDrv.h>
#include <util/dag_delayedAction.h>


namespace darg
{

static void enable_kb_ime(bool on)
{
  run_action_on_main_thread([on]() {
    if (auto kbd = global_cls_drv_kbd ? global_cls_drv_kbd->getDevice(0) : nullptr)
      kbd->enableIme(on);
  });
}


void KbFocus::reset()
{
  focus = nullptr;
  isCaptured = false;
  nextFocus = nullptr;
  nextFocusQueued = false;
  nextFocusNeedCapture = false;
}


void KbFocus::onElementDetached(Element *elem)
{
  if (elem == focus)
  {
    G_ASSERT(elem->etree->canHandleInput);

    if (focus->hasBehaviors(Behavior::F_DISPLAY_IME) && !sysCloseRequested)
      enable_kb_ime(false);
    focus = nullptr;
    isCaptured = false;
  }
  if (elem == nextFocus)
    nextFocus = nullptr;
}


void KbFocus::setFocus(Element *elem, bool capture)
{
  if (elem && !elem->etree->canHandleInput)
    return;

  if (focus == elem)
  {
    isCaptured = capture;
    return;
  }

  if (scene->isRebuildingInvalidatedParts)
  {
    nextFocus = elem;
    nextFocusNeedCapture = capture;
    nextFocusQueued = true;
    return;
  }

  bool wasTextInput = focus && is_text_input(focus);
  bool hadIME = focus && focus->hasBehaviors(Behavior::F_DISPLAY_IME);

  if (focus)
  {
    focus->clearGroupStateFlags(Element::S_KB_FOCUS);

    for (Behavior *bhv : focus->behaviors)
      bhv->onKbFocusChange(focus, false);

    Sqrat::Object scriptDesc = focus->props.scriptDesc;
    Sqrat::Function onBlur(scriptDesc.GetVM(), scriptDesc, scriptDesc.RawGetSlot(focus->csk->onBlur));
    if (!onBlur.IsNull())
      scene->queueScriptHandler(new ScriptHandlerSqFunc<>(onBlur));
  }

  focus = elem;
  isCaptured = capture;

  if (focus)
  {
    focus->setGroupStateFlags(Element::S_KB_FOCUS);

    for (Behavior *bhv : focus->behaviors)
      bhv->onKbFocusChange(focus, true);

    Sqrat::Object scriptDesc = focus->props.scriptDesc;
    Sqrat::Function onFocus(scriptDesc.GetVM(), scriptDesc, scriptDesc.RawGetSlot(focus->csk->onFocus));
    if (!onFocus.IsNull())
      scene->queueScriptHandler(new ScriptHandlerSqFunc<>(onFocus));

    // if (focus->hasFlags(Element::F_STICK_CURSOR))
    //   guiScene->moveMouseCursorTo(focus);
  }

  bool isTextInput = focus && is_text_input(focus);
  bool hasIME = focus && focus->hasBehaviors(Behavior::F_DISPLAY_IME);
  if (wasTextInput || isTextInput)
    scene->notifyInputConsumersCallback();

  if (hadIME != hasIME && !sysCloseRequested)
    enable_kb_ime(hasIME);
}


void KbFocus::applyNextFocus()
{
  G_ASSERT(!scene->isRebuildingInvalidatedParts);
  if (nextFocusQueued)
  {
    setFocus(nextFocus, nextFocusNeedCapture);
    nextFocusQueued = false;
    nextFocus = nullptr;
    nextFocusNeedCapture = false;
  }
}


} // namespace darg
