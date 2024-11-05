// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gesture/gestureDetector.h"

#include <gui/dag_visualLog.h>
#include <util/dag_string.h>

#define GESTURE_DETECTOR_DEBUG 0

static inline float midPoint(float a, float b) { return (a + b) / 2.f; };

static inline float anglesDiff(float a, float b)
{
  if (a > b)
  {
    const float phi0 = a - b;
    const float phi1 = (PI - a) + (b + PI);
    return phi0 < phi1 ? phi0 : -phi1;
  }
  else
  {
    const float phi0 = b - a;
    const float phi1 = (PI - b) + (a + PI);
    return phi0 < phi1 ? -phi0 : phi1;
  }
};

static inline bool toPolar(float x, float y, float &out_magnitude, float &out_phi)
{
  out_magnitude = sqrtf(sqr(x) + sqr(y));
  if (!out_magnitude)
    return false;
  out_phi = acos(x / out_magnitude);
  if (y < 0)
    out_phi = -out_phi;
  return true;
}

void GestureDetector::reset()
{
  touchId0 = -1;
  touchId1 = -1;
  startDist = 0;
  startAngle = 0;
  curDist = 0;
  curAngle = 0;
}

int GestureDetector::onTouchBegin(int touch_id, const Point2 &pos, const Callback &callback)
{
  int res = 0;

  touches[touch_id].pos0 = pos;
  touches[touch_id].pos = pos;

  if (touchId0 == -1 && touchId1 != touch_id)
    touchId0 = touch_id;
  else if (touchId1 == -1 && touchId0 != touch_id)
    touchId1 = touch_id;

  if (touchId0 != -1 && touchId1 != -1 && (touch_id == touchId0 || touch_id == touchId1))
  {
    const TouchData &touch0 = touches.at(touchId0);
    const TouchData &touch1 = touches.at(touchId1);

    toPolar(touch1.pos.x - touch0.pos.x, touch1.pos.y - touch0.pos.y, startDist, startAngle);

    curDist = startDist;
    curAngle = startAngle;

    res = detectGestures(Phase::BEGIN, Point2(), callback);
  }

#if GESTURE_DETECTOR_DEBUG
  visuallog::logmsg(String(-1, "GestureDetector: onTouchBegin touchId0=%d, touchId1=%d, startDist=%f, startAngle=%f", touchId0,
    touchId1, startDist, startAngle));
#endif

  return res;
}

int GestureDetector::onTouchMove(int touch_id, const Point2 &pos, const Callback &callback)
{
  int res = 0;

  auto t = touches.find(touch_id);
  if (t == touches.end())
    return res;

  Point2 deltaPos = pos - t->second.pos;
  t->second.pos = pos;

  if (touchId0 != -1 && touchId1 != -1 && (touch_id == touchId0 || touch_id == touchId1))
  {
    res = detectGestures(Phase::ACTIVE, deltaPos, callback);
  }

  return res;
}

int GestureDetector::onTouchEnd(int touch_id, const Point2 &pos, const Callback &callback)
{
  int res = 0;
  bool wasActive = false;

  auto t = touches.find(touch_id);
  if (t == touches.end())
    return res;

  Point2 deltaPos = pos - t->second.pos;
  t->second.pos = pos;

  if (touchId0 != -1 && touchId1 != -1 && (touch_id == touchId0 || touch_id == touchId1))
  {
    wasActive = true;
    res = detectGestures(Phase::END, deltaPos, callback);
    reset();
  }

  if (touchId0 == touch_id)
    touchId0 = -1;
  else if (touchId1 == touch_id)
    touchId1 = -1;

  touches.erase(t);

#if GESTURE_DETECTOR_DEBUG
  visuallog::logmsg(String(-1, "GestureDetector: onTouchEnd touchId0=%d, touchId1=%d, wasActive=%d", touchId0, touchId1, wasActive));
#endif

  // try to start gesture detection with other already existed touch
  if (wasActive)
  {
    for (auto &it : touches)
    {
      if (it.first != touchId0 && it.first != touchId1)
      {
        res |= onTouchBegin(it.first, it.second.pos, callback);
        break;
      }
    }
  }

  return res;
}

int GestureDetector::detectGestures(Phase phase, const Point2 &deltaPos, const Callback &callback)
{
  int res = 0;

  const TouchData &touch0 = touches.at(touchId0);
  const TouchData &touch1 = touches.at(touchId1);

  float newDist = 0;
  float newAngle = startAngle;
  toPolar(touch1.pos.x - touch0.pos.x, touch1.pos.y - touch0.pos.y, newDist, newAngle);

  if (callback && detectionMask != 0)
  {
    Info info;

    info.phase = phase;
    info.r0 = startDist;
    info.x0 = midPoint(touch0.pos0.x, touch1.pos0.x);
    info.y0 = midPoint(touch0.pos0.y, touch1.pos0.y);
    info.x = midPoint(touch0.pos.x, touch1.pos.x);
    info.y = midPoint(touch0.pos.y, touch1.pos.y);
    info.deltaX = deltaPos.x / 2;
    info.deltaY = deltaPos.y / 2;

    const float prevScale = curDist / startDist;
    const float newScale = newDist / startDist;

    const float newRotate = anglesDiff(newAngle, startAngle);
    const float rotateDiff = anglesDiff(newAngle, curAngle);

    if ((detectionMask & Type::PINCH) && startDist > lim_pinch_pointers_distance_min)
    {
      info.type = Type::PINCH;
      info.data.pinch.scale = newScale;
      info.data.pinch.deltaScale = newScale - prevScale;
      res |= callback(info);
    }

    if ((detectionMask & Type::ROTATE) && startDist > lim_rotate_pointers_distance_min)
    {
      info.type = Type::ROTATE;
      info.data.rotate.angle = newRotate;
      info.data.rotate.deltaAngle = rotateDiff;
      res |= callback(info);
    }

    if ((detectionMask & Type::DRAG) && startDist < lim_drag_pointers_distance_max)
    {
      info.type = Type::DRAG;
      info.data.drag.x = info.x - info.x0;
      info.data.drag.y = info.y - info.y0;
      res |= callback(info);
    }

#if GESTURE_DETECTOR_DEBUG
    visuallog::logmsg(String(-1, "GestureDetector: phase=%d startDist=%.2f x=%.2f y=%.2f scale=%.2f rotate=%.2f", phase, startDist,
      info.x, info.y, newScale, newRotate));
#endif

    curDist = newDist;
    curAngle = newAngle;
  }

  return res;
}
