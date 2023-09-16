#include "pointerPosition.h"

#include <humanInput/dag_hiPointing.h>
#include <startup/dag_inpDevClsDrv.h>
#include <startup/dag_globalSettings.h>
#include <gui/dag_stdGuiRender.h>
#include <osApiWrappers/dag_wndProcCompMsg.h>
#include <util/dag_convar.h>


namespace darg
{

CONSOLE_BOOL_VAL("darg", debug_cursor, false);


void PointerPosition::initMousePos()
{
  if (HumanInput::IGenPointing *mouse = getMouse())
  {
    const HumanInput::PointingRawState::Mouse &rs = mouse->getRawState().mouse;
    mousePos.x = rs.x;
    mousePos.y = rs.y;
    targetMousePos = mousePos;
  }
  mouseWasRelativeOnLastUpdate = false;
}


void PointerPosition::setMousePos(const Point2 &p, bool reset_target, bool force_drv_update)
{
  Point2 prevMousePos = mousePos;
  mousePos = floor(p);

  if (HumanInput::IGenPointing *mouse = getMouse())
  {
    // Gamepad cursor input also goes through here and it's values are clamped by mouse
    G_ASSERT(!isSettingMousePos);
    if (force_drv_update || fabsf(mousePos.x - prevMousePos.x) > 0.01f || fabsf(prevMousePos.y - p.y) > 0.01f)
    {
      isSettingMousePos = true;
      mouse->setPosition(P2D(mousePos));
      isSettingMousePos = false;
    }
    mousePos.x = mouse->getRawState().mouse.x;
    mousePos.y = mouse->getRawState().mouse.y;
  }

  if (reset_target)
    targetMousePos = mousePos;

  needToMoveOnNextFrame = false;
}


void PointerPosition::requestNextFramePos(const Point2 &p)
{
  nextFramePos = p;
  needToMoveOnNextFrame = true;
}

void PointerPosition::requestTargetPos(const Point2 &p)
{
  needToMoveOnNextFrame = false;
  targetMousePos = p;
}


void PointerPosition::onMouseMoveEvent(const Point2 &p)
{
  const Point2 &pos = needToMoveOnNextFrame ? nextFramePos : p;
  mousePos = pos;
  needToMoveOnNextFrame = false;

  if (!isSettingMousePos)
    targetMousePos = mousePos;
}


bool PointerPosition::moveMouseToTarget(float dt)
{
  if (lengthSq(mousePos - targetMousePos) < 1.0f)
    return false;

  const float sh = StdGuiRender::screen_height();
  const float sw = StdGuiRender::screen_width();

  Point2 delta = targetMousePos - mousePos;
  float dist = length(delta);

  float scale = sqrtf(sh * sh + sw * sw) / sqrtf(1080.f * 1080.f + 1920.f * 1920.f);
  float targetT = 0.1f;
  float speed = ::clamp(dist / targetT, 8000.f * scale, 20000.f * scale);

  float maxD = dt * speed;
  if (dist > maxD)
    delta *= maxD / dist;
  Point2 dest = mousePos + delta;
  bool arrived = (lengthSq(dest - targetMousePos) < 1.0f);
  if (arrived)
    dest = targetMousePos;
  setMousePos(dest, false);
  return true;
}


void PointerPosition::syncMouseCursorLocation()
{
  auto mouse = getMouse();
  if (!mouse)
    return;

  bool isRelative = mouse->getRelativeMovementMode();
  if (!isRelative && mouseWasRelativeOnLastUpdate)
    setMousePos(needToMoveOnNextFrame ? nextFramePos : mousePos, /*reset_target*/ true, /*force_drv_update*/ true);

  mouseWasRelativeOnLastUpdate = isRelative;
}


HumanInput::IGenPointing *PointerPosition::getMouse()
{
  HumanInput::IGenPointing *mouse = nullptr;
  if (::global_cls_drv_pnt)
    mouse = ::global_cls_drv_pnt->getDevice(0);
  return mouse;
}


void PointerPosition::onShutdown() { needToMoveOnNextFrame = false; }


void PointerPosition::update(float dt)
{
  G_UNUSED(dt);
  syncMouseCursorLocation();

  if (needToMoveOnNextFrame && ::dgs_app_active)
  {
    setMousePos(nextFramePos, true);
    needToMoveOnNextFrame = false;
  }
}


void PointerPosition::onSceneActivate()
{
  if (HumanInput::IGenPointing *mouse = getMouse())
  {
    mouse->fetchLastState();
    const HumanInput::PointingRawState::Mouse &rs = mouse->getRawState().mouse;
    mousePos = Point2(rs.x, rs.y);
    targetMousePos = mousePos;
    needToMoveOnNextFrame = false;
  }
}


void PointerPosition::onAppActivate() { onSceneActivate(); }


void PointerPosition::debugRender(StdGuiRender::GuiContext *ctx)
{
  if (debug_cursor)
  {
    if (needToMoveOnNextFrame)
    {
      ctx->set_color(255, 0, 200);
      ctx->render_frame(nextFramePos.x - 5, nextFramePos.y - 5, nextFramePos.x + 5, nextFramePos.y + 5, 2);
      ctx->render_frame(nextFramePos.x - 9, nextFramePos.y - 9, nextFramePos.x + 9, nextFramePos.y + 9, 2);
      ctx->render_frame(nextFramePos.x - 13, nextFramePos.y - 13, nextFramePos.x + 13, nextFramePos.y + 13, 2);
    }
    ctx->set_color(120, 120, 20, 40);
    ctx->render_box(targetMousePos.x - 4, targetMousePos.y - 4, targetMousePos.x + 4, targetMousePos.y + 4);
  }
}

} // namespace darg
