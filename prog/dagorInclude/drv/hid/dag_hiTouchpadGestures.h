//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/hid/dag_hiJoyData.h>
#include <drv/hid/dag_hiPointingData.h>
#include <math/dag_Point2.h>
#include <EASTL/fixed_vector.h>

namespace HumanInput
{

// Gestures of a pad touch pad from its raw points, positions and deltas in pad pixels. It makes no OS calls: a driver
// feeds every report with the time of that report, the time also ends a tap and starts a hold. Events:
// - TAP: BEGIN on release, then END once no second tap can follow, or CANCELLED and one DOUBLE_TAP (ACTIVE) for a second tap.
// - HOLD: BEGIN when HOLD_MS pass with no move past MOVE_LIMIT_PX, END on release or on such a move.
// - DRAG: BEGIN on a move past MOVE_LIMIT_PX, ACTIVE, END; then one FLICK (ACTIVE) when the finger lifts fast.
// - PINCH and ROTATE, one of each per update: BEGIN when a second finger touches, ACTIVE, END when one lifts. A one finger
//   gesture stops then and starts again only after every finger lifted; a second finger again begins a new pair.
// A touch while the pad button is down gives no TAP, DOUBLE_TAP or HOLD. reset() sends CANCELLED for what runs, and the
// fingers that touch then give nothing until every finger lifted. Not thread safe.
class TouchpadGestureRecognizer
{
public:
  static constexpr int TAP_MAX_MS = 250;
  static constexpr int DOUBLE_TAP_GAP_MS = 300;
  static constexpr int HOLD_MS = 500;
  static constexpr float MOVE_LIMIT_PX = 100;
  static constexpr float DOUBLE_TAP_MAX_DIST_PX = 300;
  static constexpr float FLICK_MIN_SPEED_PX_PER_MS = 4;
  static constexpr int FLICK_SPEED_SMOOTHING_MS = 40;
  static constexpr float FLICK_VERTICAL_HALF_ANGLE_DEG = 55;

  void update(const JoystickTouchpadState::Point (&points)[2], bool pad_button_down, int time_ms, GestureCb cb);
  void reset(GestureCb cb);

private:
  enum class Mode
  {
    IDLE,
    ONE,
    TWO,
    ONE_OF_TWO,
    UNTIL_LIFT,
  };

  void startTouch(const JoystickTouchpadState::Point *const *down, int count, bool pad_button_down, int time_ms, GestureCb cb);
  void moveOne(const Point2 &pos, bool pad_button_down, int time_ms, GestureCb cb);
  void finishOne(bool lifted, int time_ms, GestureCb cb);
  void sendDrag(Gesture::State state, const Point2 &pos, GestureCb cb);
  void beginPair(const JoystickTouchpadState::Point *const *down, GestureCb cb);
  void movePair(const Point2 &a, const Point2 &b, GestureCb cb);
  void sendPair(Gesture::State state, const Point2 &delta, GestureCb cb);
  void endHold(Gesture::State state, GestureCb cb);
  void endPendingTap(Gesture::State state, GestureCb cb);
  void dropSecondTap(GestureCb cb);

  Point2 pressPos = Point2(0, 0), lastPos = Point2(0, 0), tapPos = Point2(0, 0), flickTrail = Point2(0, 0);
  Point2 pairPos[2] = {Point2(0, 0), Point2(0, 0)}, pairStartCenter = Point2(0, 0);
  float pairStartDist = 1, pairAngle = 0, pairRotation = 0, pairScale = 1;
  int pressMs = 0, lastMs = 0, tapReleaseMs = 0;
  Mode mode = Mode::IDLE;
  uint8_t touchId = 0;
  uint8_t pairId[2] = {0, 0};
  bool moved = false, holding = false, buttonInTouch = false, secondTap = false, tapPending = false;
};

// Bounded hand-over of gestures between threads, not thread safe itself. An ACTIVE DRAG, PINCH or ROTATE merges into the
// queued one of its type; when the queue is full, the oldest event goes.
class TouchpadGestureQueue
{
public:
  static constexpr int CAPACITY = 32;

  void operator()(Gesture::State state, const Gesture &g);
  void drain(GestureCb cb);
  void clear() { events.clear(); }

private:
  struct Event
  {
    Gesture g;
    Gesture::State state;
  };
  eastl::fixed_vector<Event, CAPACITY, false> events;
};

} // namespace HumanInput
