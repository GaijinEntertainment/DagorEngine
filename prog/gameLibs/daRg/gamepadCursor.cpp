// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gamepadCursor.h"
#include "cursorState.h"
#include "sceneConfig.h"
#include "screen.h"

#include <drv/hid/dag_hiHidClassDrv.h>
#include <drv/hid/dag_hiPointing.h>
#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiComposite.h>
#include <drv/hid/dag_hiXInputMappings.h>
#include <startup/dag_inpDevClsDrv.h>

#include <math/dag_mathBase.h>
#include <math/dag_mathUtils.h>
#include <gui/dag_stdGuiRender.h>

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


static Point2 distance_to_box(Point2 p, const BBox2 &box)
{
  float dx = 0, dy = 0;
  if (p.x < box.left())
    dx = box.left() - p.x;
  else if (p.x >= box.right())
    dx = p.x - box.right() + 1.0;
  if (p.y < box.top())
    dy = box.top() - p.y;
  else if (p.y >= box.bottom())
    dy = p.y - box.bottom() + 1.0;
  return Point2(dx, dy);
}


GamepadCursor::GamepadCursor(const SceneConfig &cfg) : config(cfg) {}


Point2 GamepadCursor::moveOverElements(Screen *focused_screen, HumanInput::IGenJoystick *joy, const Point2 &cursorPos,
  const TMatrix &camera_tm, const Frustum &camera_frustum, float dt)
{
  using namespace HumanInput;

  float axisValueX = float(joy->getAxisPosRaw(config.gamepadCursorAxisH)) / JOY_XINPUT_MAX_AXIS_VAL;
  float axisValueY = -float(joy->getAxisPosRaw(config.gamepadCursorAxisV)) / JOY_XINPUT_MAX_AXIS_VAL;

  float angle = 0;
  if (!handle_stick_axis(config, axisValueX, axisValueY, angle))
  {
    currentSector = SECTOR_NONE;
    timeInCurrentSector = 0;
    return cursorPos;
  }

  int newSector = calculate_current_sector(angle);
  if (newSector != currentSector)
  {
    currentSector = newSector;
    timeInCurrentSector = 0;
  }

  float sw = StdGuiRender::screen_width();
  float sh = StdGuiRender::screen_height();

  float fieldEffectFactor = 0.0f;

  Point2 localPos;
  if (focused_screen && focused_screen->displayToLocal(cursorPos, camera_tm, camera_frustum, localPos))
  {
    const float fieldRange = sh * 20 / 1080.0f;
    for (InputEntry &ie : focused_screen->inputStack.stack)
    {
      BBox2 bbox = ie.elem->clippedScreenRect;
      bbox.inflate(sh * 10 / 1080.0f);
      if (ie.elem->hasFlags(Element::F_STICK_CURSOR) && !bbox.isempty())
      {
        if (bbox & localPos)
          fieldEffectFactor = 1;
        else
        {
          Point2 d = distance_to_box(localPos, bbox);
          float k = ::cvt(max(d.x, d.y), 0.0f, fieldRange, 1.0f, 0.0f);
          fieldEffectFactor = ::max(fieldEffectFactor, k);
        }
        if (fieldEffectFactor > 0.999f)
          break;
      }

      if (ie.elem->hitTest(localPos) && ie.elem->hasFlags(Element::F_STOP_HOVER | Element::F_STOP_POINTING))
        break;
    }
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

  return clamp(cursorPos + Point2((int)dx, (int)dy), Point2(0, 0), Point2(sw - 1, sh - 1));
}


bool GamepadCursor::isStickActionAllowedFor(Screen *screen, int axis_x, int axis_y) const
{
  using namespace HumanInput;

  if (!screen)
    return true;

  if ((screen->inputStack.summaryBhvFlags & Behavior::F_INTERNALLY_HANDLE_GAMEPAD_L_STICK) &&
      (axis_x == JOY_XINPUT_REAL_AXIS_L_THUMB_H || axis_y == JOY_XINPUT_REAL_AXIS_L_THUMB_V))
    return false;

  if ((screen->inputStack.summaryBhvFlags & Behavior::F_INTERNALLY_HANDLE_GAMEPAD_R_STICK) &&
      (axis_x == JOY_XINPUT_REAL_AXIS_R_THUMB_H || axis_y == JOY_XINPUT_REAL_AXIS_R_THUMB_V))
    return false;

  return true;
}


eastl::optional<Point2> GamepadCursor::scroll(const HumanInput::IGenJoystick *joy, float dt) const
{
  using namespace HumanInput;

  if (!joy)
    return {};

  float axisValueX = float(joy->getAxisPosRaw(config.joystickScrollAxisH)) / JOY_XINPUT_MAX_AXIS_VAL;
  float axisValueY = -float(joy->getAxisPosRaw(config.joystickScrollAxisV)) / JOY_XINPUT_MAX_AXIS_VAL;

  float angle = 0;
  if (!handle_stick_axis(config, axisValueX, axisValueY, angle))
    return {};

  float scrollStep = 1000.0f * dt * StdGuiRender::screen_height() / 1080.0f;
  Point2 delta = Point2(axisValueX, axisValueY) * scrollStep;

  return delta;
}


bool GamepadCursor::isStickActive(HumanInput::IGenJoystick *joy, Screen *focused_screen) const
{
  using namespace HumanInput;

  if (!joy)
    return false;

  if (!isStickActionAllowedFor(focused_screen, config.gamepadCursorAxisH, config.gamepadCursorAxisV))
    return false;

  float ax = float(joy->getAxisPosRaw(config.gamepadCursorAxisH)) / JOY_XINPUT_MAX_AXIS_VAL;
  float ay = -float(joy->getAxisPosRaw(config.gamepadCursorAxisV)) / JOY_XINPUT_MAX_AXIS_VAL;
  float mag = sqrtf(ax * ax + ay * ay);
  return mag >= config.gamepadCursorDeadZone + 1e-3f;
}


bool GamepadCursor::updateCursorPos(HumanInput::IGenJoystick *joy, Screen *focused_screen, const TMatrix &camera_tm,
  const Frustum &camera_frustum, float dt)
{
  if (!joy)
    return false;

  if (!isStickActionAllowedFor(focused_screen, config.gamepadCursorAxisH, config.gamepadCursorAxisV))
    return false;

  const int nIter = 4;
  Point2 prevPos = pos;
  for (int i = 0; i < nIter; ++i)
    pos = moveOverElements(focused_screen, joy, pos, camera_tm, camera_frustum, dt / nIter);

  if (pos != prevPos)
  {
    targetPos = pos;
    return true;
  }
  return false;
}


} // namespace darg
