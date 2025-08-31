// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gamepadCursor.h"
#include "guiScene.h"

#include <drv/hid/dag_hiHidClassDrv.h>
#include <drv/hid/dag_hiPointing.h>
#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiComposite.h>
#include <drv/hid/dag_hiXInputMappings.h>
#include <startup/dag_inpDevClsDrv.h>
#include <startup/dag_globalSettings.h>

#include <math/dag_mathBase.h>
#include <math/dag_mathUtils.h>

#include <daRg/dag_element.h>


namespace darg
{

static const int MAX_CURSOR_SPEED = 1000;
static const float NO_FRICTION_KOEF = 1.f;


static bool handle_stick_axis(const SceneConfig &config, float &ax, float &ay, float &angle)
{
  float mag = sqrtf(ax * ax + ay * ay);
  const float eps = 1e-3f; // for zero deadzone
  if (mag < config.gamepadCursorDeadZone + eps)
    return false;

  angle = safe_atan2(ay, ax);

  mag = ::min(1.0f, (mag - config.gamepadCursorDeadZone) / (1.0f - config.gamepadCursorDeadZone));
  mag = powf(mag, 1.0f + config.gamepadCursorNonLin);

  ax = mag * cosf(angle);
  ay = mag * sinf(angle);

  return true;
}


static int calculate_current_sector(float angle)
{
  return (int)(((angle + PI) / (2 * PI)) * GamepadCursor::SECTOR_NUM) % GamepadCursor::SECTOR_NUM;
}


GamepadCursor::GamepadCursor(GuiScene *gui_scene) : guiScene(gui_scene) {}


Point2 GamepadCursor::moveMouse(Screen *screen, HumanInput::IGenJoystick *joy, const Point2 &mousePos, float dt)
{
  using namespace HumanInput;

  const SceneConfig &config = guiScene->config;

  float axisValueX = float(joy->getAxisPosRaw(config.gamepadCursorAxisH)) / JOY_XINPUT_MAX_AXIS_VAL;
  float axisValueY = -float(joy->getAxisPosRaw(config.gamepadCursorAxisV)) / JOY_XINPUT_MAX_AXIS_VAL;

  float angle = 0;
  if (!handle_stick_axis(config, axisValueX, axisValueY, angle))
  {
    currentSector = 0;
    timeInCurrentSector = 0;
    return mousePos;
  }

  int newSector = calculate_current_sector(angle);
  if (newSector != currentSector)
  {
    timeInCurrentSector = 0;
    currentSector = newSector;
  }

  float sw = StdGuiRender::screen_width();
  float sh = StdGuiRender::screen_height();


  float fieldEffectFactor = 0.0f;
  const float fieldRange = sh * 20 / 1080.0f;
  for (InputEntry &ie : screen->inputStack.stack)
  {
    BBox2 bbox = ie.elem->clippedScreenRect;
    bbox.inflate(sh * 10 / 1080.0f);
    if (ie.elem->hasFlags(Element::F_STICK_CURSOR) && !bbox.isempty())
    {
      if (bbox & mousePos)
        fieldEffectFactor = 1;
      else
      {
        float dx = 0, dy = 0;
        if (mousePos.x < bbox.left())
          dx = bbox.left() - mousePos.x;
        else if (mousePos.x >= bbox.right())
          dx = mousePos.x - bbox.right() + 1.0;
        if (mousePos.y < bbox.top())
          dy = bbox.top() - mousePos.y;
        else if (mousePos.y >= bbox.bottom())
          dy = mousePos.y - bbox.bottom() + 1.0;
        float k = ::cvt(max(dx, dy), 0.0f, fieldRange, 1.0f, 0.0f);
        fieldEffectFactor = ::max(fieldEffectFactor, k);
      }
      if (fieldEffectFactor > 0.999f)
        break;
    }

    if (ie.elem->hitTest(mousePos) && ie.elem->hasFlags(Element::F_STOP_HOVER | Element::F_STOP_MOUSE))
      break;
  }

  float friction = NO_FRICTION_KOEF;
  if (timeInCurrentSector < config.gamepadCursorHoverMaxTime && fieldEffectFactor > 0.0f)
  {
    float targetFriction = lerp(config.gamepadCursorHoverMinMul, config.gamepadCursorHoverMaxMul,
      sqrtf(timeInCurrentSector / config.gamepadCursorHoverMaxTime));
    friction = lerp(NO_FRICTION_KOEF, targetFriction, fieldEffectFactor);
    timeInCurrentSector += dt * fieldEffectFactor;
  }

  const float speed = MAX_CURSOR_SPEED;

  float dx = speed * dt * axisValueX * friction * config.gamepadCursorSpeed * sw / 1920.0f;
  float dy = speed * dt * axisValueY * friction * config.gamepadCursorSpeed * sh / 1080.0f;

  dx += (int)dxCup;
  dxCup -= (int)dxCup;
  dxCup += dx - (int)dx;

  dy += (int)dyCup;
  dyCup -= (int)dyCup;
  dyCup += dy - (int)dy;


  return clamp(mousePos + Point2((int)dx, (int)dy), Point2(0, 0), Point2(sw - 1, sh - 1));
}


void GamepadCursor::scroll(Screen *, HumanInput::IGenJoystick *joy, float dt)
{
  using namespace HumanInput;

  const SceneConfig &config = guiScene->config;

  float axisValueX = float(joy->getAxisPosRaw(config.joystickScrollAxisH)) / JOY_XINPUT_MAX_AXIS_VAL;
  float axisValueY = -float(joy->getAxisPosRaw(config.joystickScrollAxisV)) / JOY_XINPUT_MAX_AXIS_VAL;

  float angle = 0;
  if (!handle_stick_axis(guiScene->config, axisValueX, axisValueY, angle))
    return;

  float scrollStep = 1000.0f * dt * StdGuiRender::screen_height() / 1080.0f;
  Point2 delta = Point2(axisValueX, axisValueY) * scrollStep;

  guiScene->doJoystickScroll(delta);
}


void GamepadCursor::update(Screen *screen, float dt)
{
  if (!::dgs_app_active)
    return;
  if (HumanInput::IGenPointing *ms = global_cls_drv_pnt ? global_cls_drv_pnt->getDevice(0) : nullptr)
    if (ms->getRelativeMovementMode())
      return;

  using namespace HumanInput;
  const SceneConfig &config = guiScene->config;

  if ((screen->inputStack.summaryBhvFlags & Behavior::F_INTERNALLY_HANDLE_GAMEPAD_L_STICK) &&
      (config.gamepadCursorAxisH == JOY_XINPUT_REAL_AXIS_L_THUMB_H || config.gamepadCursorAxisV == JOY_XINPUT_REAL_AXIS_L_THUMB_V))
    return;

  if ((screen->inputStack.summaryBhvFlags & Behavior::F_INTERNALLY_HANDLE_GAMEPAD_R_STICK) &&
      (config.gamepadCursorAxisH == JOY_XINPUT_REAL_AXIS_R_THUMB_H || config.gamepadCursorAxisV == JOY_XINPUT_REAL_AXIS_R_THUMB_V))
    return;

  HumanInput::IGenJoystick *joy = guiScene->getJoystick();
  if (!joy)
    return;

  const int nIter = 4;
  Point2 mousePos = guiScene->getMousePos();
  Point2 prevMousePos = mousePos;
  for (int i = 0; i < nIter; ++i)
    mousePos = moveMouse(screen, joy, mousePos, dt / nIter);
  if (mousePos != prevMousePos)
  {
    isMovingMouse = true;
    guiScene->setXmbFocus(nullptr);
    guiScene->setMousePos(mousePos, true);
    guiScene->updateHover();
    isMovingMouse = false;
  }

  scroll(screen, joy, dt);
}

} // namespace darg
