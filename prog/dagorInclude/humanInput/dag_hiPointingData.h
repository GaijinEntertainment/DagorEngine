//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

// #include <util/dag_globDef.h> // TODO: for Gesture::elapsedTime


namespace HumanInput
{
static constexpr int MAX_MOUSE_BUTTONS = 7;
static constexpr int MAX_TOUCHES = 32;

struct PointingSettings
{
  bool mousePresent = false;
  bool mouseEnabled = false;
  float xSens = 1, ySens = 1, zSens = 1; // mouse movement sensitivity

  bool emuTouchScreenWithMouse = false;
  bool touchScreen = false;
  bool allowEmulatedLMB = true;
  int tapAndHoldThresMs = 2000;
  int tapStillCircleRad = 8;
  int tapReleaseDelayMs = 100;

  float touchPadSpeedMin = 100;
  float touchPadSpeedFull = 300;
  float touchPadSpeedFactorAtMin = 0.2f;

  bool emuMouseWithTouchPad = false;
};

struct PointingRawState
{
  struct Mouse
  {
    enum
    {
      LBUTTON_BIT = 0x0001,
      RBUTTON_BIT = 0x0002,
      MBUTTON_BIT = 0x0004,
    };

    unsigned buttons = 0;         //< pushed mouse buttons bitmap
    float x = 0, y = 0;           //< current absolute coordinates of pointer
    float deltaX = 0, deltaY = 0; //< mouse translation since last reset (incremented by driver)
    float deltaZ = 0;             //< mouse wheel rotation since last reset (incremented by driver)

    bool isBtnDown(int btn_idx) const { return buttons & (1 << btn_idx); }
  };

  struct Touch
  {
    // all coords are screen pixels
    enum
    {
      TS_none,
      TS_touchPad,
      TS_touchScreen,
      TS_emuTouchScreen
    };

    float x = 0, y = 0;  //< current/last coords of touch
    unsigned t1msec = 0; //< current/last timestamp of touch (in msec)

    float x0 = 0, y0 = 0; //< coords of touch start
    unsigned t0msec = 0;  //< timestamp of touch start (in msec)

    float deltaX = 0, deltaY = 0; //< relative touch movement since last resetDelta
    float maxDist = 0;            //< maximum distance between (x,y) and (x0,y0)
    unsigned touchOrdId = 0;      //< ordinal ID, incremented for each new touch
    unsigned touchSrc = TS_none;  //< source of touch data (touch pad, touch screen, etc.)

    bool screenCoords() const { return touchSrc == TS_touchScreen || touchSrc == TS_emuTouchScreen; }
  };

  Mouse mouse;
  Touch touch[MAX_TOUCHES];
  unsigned touchesActive = 0;

  void resetDelta()
  {
    mouse.deltaX = mouse.deltaY = mouse.deltaZ = 0;
    for (Touch &t : touch)
      t.deltaX = t.deltaY = 0;
  }
};

struct Gesture
{
  enum State
  {
    INACTIVE = 0,
    BEGIN,
    ACTIVE,
    END,
    CANCELLED
  };
  enum Direction
  {
    LEFT = 0,
    RIGHT,
    UP,
    DOWN
  };
  enum Type
  {
    NONE = 0,
    TAP,
    DRAG,
    HOLD,
    PINCH,
    ROTATE,
    FLICK,
    DOUBLE_TAP
  } type = NONE;

  float x = 0, y = 0;           //< current/last position
  float deltaX = 0, deltaY = 0; //< delta since previous position
  float x0 = 0, y0 = 0;         //< initial position
  float scale = 1.0;            //< scale factor, only for PINCH
  float angle = 0.0;            //< TODO: rotation angle, only for ROTATION

  Direction dir = Direction::LEFT; //< FLICK direction shortcut
  // uint64_t elapsedTimeUsec = 0; //< TODO: time elapsed since gesture start
  //
}; // struct Gesture

} // namespace HumanInput
