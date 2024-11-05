//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point2.h>

#include <EASTL/functional.h>
#include <EASTL/unordered_map.h>

#include <cstring>

class GestureDetector
{
public:
  static constexpr float LIM_PINCH_POINTERS_DISTANCE_MIN_DEFAULT = 5.0f;
  static constexpr float LIM_ROTATE_POINTERS_DISTANCE_MIN_DEFAULT = 5.0f;
  static constexpr float LIM_DRAG_POINTERS_DISTANCE_MAX_DEFAULT = 100.f;

  enum Phase
  {
    BEGIN,
    ACTIVE,
    END
  };

  enum Type
  {
    NONE = 0x0,

    PINCH = 0x1,
    ROTATE = 0x2,
    DRAG = 0x4,

    ALL = 0xFFFFFFFF
  };

  struct Info
  {
    Info() { memset(&data, 0, sizeof(data)); }

    Type type = Type::NONE;
    Phase phase = Phase::BEGIN;

    float x = 0, y = 0;           //< current/last position (for gesture center point)
    float deltaX = 0, deltaY = 0; //< delta since previous position (for gesture center point)
    float x0 = 0, y0 = 0;         //< initial position (for gesture center point)
    float r0 = 0;                 //< distance between pointers when gesture was started

    union
    {
      struct Pinch
      {
        float scale; //< scale factor relative to the positions at when two fingers were paired
        float deltaScale;
      } pinch;

      struct Rotate
      {
        float angle; //< angle relative to the positions at when two fingers were paired
        float deltaAngle;
      } rotate;

      struct Drag
      {
        float x;
        float y;
      } drag;
    } data;
  };

  using Callback = eastl::function<int(const GestureDetector::Info &info)>;

  int onTouchBegin(int touch_id, const Point2 &pos, const Callback &callback);
  int onTouchEnd(int touch_id, const Point2 &pos, const Callback &callback);
  int onTouchMove(int touch_id, const Point2 &pos, const Callback &callback);
  void reset();

  int detectionMask = Type::ALL;
  float lim_pinch_pointers_distance_min = LIM_PINCH_POINTERS_DISTANCE_MIN_DEFAULT;
  float lim_rotate_pointers_distance_min = LIM_ROTATE_POINTERS_DISTANCE_MIN_DEFAULT;
  float lim_drag_pointers_distance_max = LIM_DRAG_POINTERS_DISTANCE_MAX_DEFAULT;

private:
  struct TouchData
  {
    Point2 pos0;
    Point2 pos;
  };

  bool isActive = false;
  int touchId0 = -1;
  int touchId1 = -1;
  float startDist = 0;
  float startAngle = 0;
  float curDist = 0;
  float curAngle = 0;

  eastl::unordered_map<int, TouchData> touches;

  int detectGestures(Phase phase, const Point2 &deltaPos, const Callback &callback);
};
