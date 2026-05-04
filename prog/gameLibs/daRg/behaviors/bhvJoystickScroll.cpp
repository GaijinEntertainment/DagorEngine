// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvJoystickScroll.h"
#include <daRg/dag_element.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_scriptHandlers.h>
#include <daRg/dag_guiScene.h>
#include <drv/hid/dag_hiKeybIds.h>
#include <drv/hid/dag_hiMouseIds.h>
#include <drv/hid/dag_hiKeyboard.h>
#include <startup/dag_inpDevClsDrv.h>
#include <daRg/dag_inputIds.h>
#include "elementTree.h"
#include "guiScene.h"

namespace darg
{
BhvJoystickScroll bhv_joystick_scroll;

BhvJoystickScroll::BhvJoystickScroll() : Behavior(STAGE_ACT, 0) {}

int BhvJoystickScroll::update(UpdateStage, darg::Element *elem, float dt)
{
  const GuiScene *scene = elem->etree->guiScene;

  const darg::Screen *screen = scene->getFocusedScreen();
  bool canScroll = scene->getActiveCursor() || (screen->panel && screen->panel->hasActiveCursors());
  if (!canScroll)
    return 0;

  const HumanInput::IGenJoystick *joy = scene->getJoystick();
  eastl::optional<Point2> scrollDelta = scene->getGamepadCursor().scroll(joy, dt);
  if (!scrollDelta)
    return 0;

  const Point2 pointerPos = scene->activePointerPos();
  if (!elem->hitTest(pointerPos))
    return 0;

  Sqrat::Table &scriptDesc = elem->props.scriptDesc;
  Sqrat::Function onJoystickScroll(scriptDesc.GetVM(), scriptDesc, scriptDesc.RawGetSlot(elem->csk->onJoystickScroll));
  if (!onJoystickScroll.IsNull())
  {
    HSQUIRRELVM vm = onJoystickScroll.GetVM();
    Sqrat::Table payload(vm);
    payload.SetValue("target", elem->getRef(vm));
    payload.SetValue("delta", *scrollDelta);
    payload.SetValue("screenX", pointerPos.x);
    payload.SetValue("screenY", pointerPos.y);
    onJoystickScroll(payload);
  }

  return 0;
}

} // namespace darg