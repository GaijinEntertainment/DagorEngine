// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "mousePointer.h"

#include <drv/hid/dag_hiPointing.h>
#include <startup/dag_inpDevClsDrv.h>
#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_wndProcCompMsg.h>


namespace darg
{


void MousePointer::initMousePos()
{
  if (HumanInput::IGenPointing *mouse = get_mouse())
  {
    const HumanInput::PointingRawState::Mouse &rs = mouse->getRawState().mouse;
    pos.x = rs.x;
    pos.y = rs.y;
    targetPos = pos;
  }
  mouseWasRelativeOnLastUpdate = false;
}


void MousePointer::setMousePos(const Point2 &p, bool reset_target)
{
  pos = p;
  if (reset_target)
  {
    targetPos = pos;
    needToMoveOnNextFrame = false;
  }
  commitToDriver(/*force*/ true);
}


void MousePointer::onMouseMoveEvent(const Point2 &p)
{
  const Point2 &newPos = needToMoveOnNextFrame ? nextFramePos : p;
  pos = newPos;
  needToMoveOnNextFrame = false;

  if (!isSettingMousePos)
    targetPos = pos;
}


bool MousePointer::moveToTarget(float dt)
{
  if (!smooth_cursor_step(pos, targetPos, dt))
    return false;
  needsDriverSync = true;
  return true;
}


void MousePointer::commitToDriver(bool force)
{
  if (!force && !needsDriverSync)
    return;
  needsDriverSync = false;

  HumanInput::IGenPointing *mouse = get_mouse();
  if (!mouse || !::dgs_app_active)
    return;

  pos = floor(pos);

  G_ASSERT(!isSettingMousePos);
  isSettingMousePos = true;
  mouse->setPosition(P2D(pos));
  isSettingMousePos = false;

  pos.x = mouse->getRawState().mouse.x;
  pos.y = mouse->getRawState().mouse.y;
}


void MousePointer::syncMouseCursorLocation()
{
  auto mouse = get_mouse();
  if (!mouse)
    return;

  bool isRelative = mouse->getRelativeMovementMode();
  if (!isRelative && mouseWasRelativeOnLastUpdate)
  {
    if (needToMoveOnNextFrame)
    {
      pos = nextFramePos;
      targetPos = pos;
      needToMoveOnNextFrame = false;
    }
    needsDriverSync = true;
  }
  mouseWasRelativeOnLastUpdate = isRelative;
}


HumanInput::IGenPointing *MousePointer::get_mouse()
{
  HumanInput::IGenPointing *mouse = nullptr;
  if (::global_cls_drv_pnt)
    mouse = ::global_cls_drv_pnt->getDevice(0);
  return mouse;
}


void MousePointer::onShutdown() { needToMoveOnNextFrame = false; }


void MousePointer::onActivateSceneInput()
{
  if (HumanInput::IGenPointing *mouse = get_mouse())
  {
    mouse->fetchLastState();
    const HumanInput::PointingRawState::Mouse &rs = mouse->getRawState().mouse;
    pos = Point2(rs.x, rs.y);
    targetPos = pos;
    needToMoveOnNextFrame = false;
  }
}


void MousePointer::onAppActivate() { onActivateSceneInput(); }

} // namespace darg
