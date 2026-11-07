// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "cursorState.h"
#include <EASTL/optional.h>


namespace HumanInput
{
class IGenJoystick;
}


namespace darg
{

class SceneConfig;
class Screen;

class GamepadCursor : public CursorPosState
{
public:
  GamepadCursor(const SceneConfig &cfg);

  // Stick activity check (lightweight - deadzone only, no state mutation)
  bool isStickActive(HumanInput::IGenJoystick *joy, Screen *focused_screen) const;

  // Compute new cursor position from stick input. Reads/writes pos internally.
  // Returns true if cursor moved.
  bool updateCursorPos(HumanInput::IGenJoystick *joy, Screen *focused_screen, const TMatrix &camera_tm, const Frustum &camera_frustum,
    float dt);

  // a move by an input other than the stick (the pad touch pad), in screen pixels; true if the cursor moved
  bool moveBy(const Point2 &delta);

  bool moveToTarget(float dt) { return smooth_cursor_step(pos, targetPos, dt); }

  eastl::optional<Point2> scroll(const HumanInput::IGenJoystick *joy, float dt) const;

  // Stick scroll is held until the scroll axes move by config.joystickScrollMoveThreshold from where they are
  // on the next updateScrollHold(), so that a stick left off-center (e.g. a throttle) does not take over
  void holdScrollUntilMoved() { scrollHoldPending = true; }
  // Call once per frame before scroll(). A joystick change also starts a hold
  void updateScrollHold(const HumanInput::IGenJoystick *joy);

  Point2 stick(const HumanInput::IGenJoystick *joy, int axis_h, int axis_v, float min_dead_zone) const;

public:
  static const int SECTOR_NUM = 8; //[0..7]
  static const int SECTOR_NONE = -1;

private:
  bool isStickActionAllowedFor(Screen *screen, int axis_x, int axis_y) const;
  Point2 moveOverElements(Screen *screen, HumanInput::IGenJoystick *joy, const Point2 &cursorPos, const TMatrix &camera_tm,
    const Frustum &camera_frustum, float dt);

private:
  int currentSector = SECTOR_NONE;
  float timeInCurrentSector = 0;

  const SceneConfig &config;
  float dxCup = 0;
  float dyCup = 0;

  const HumanInput::IGenJoystick *scrollHoldJoy = nullptr; // only compared, never dereferenced
  Point2 scrollHoldAxes = Point2(0, 0);
  bool scrollHoldPending = false;
  bool scrollHeld = false;
};

} // namespace darg
