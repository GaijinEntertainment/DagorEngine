// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_Point2.h>
#include <EASTL/fixed_vector.h>


namespace darg
{

// The touch pad of a game pad (TS_touchPad touches). It moves a pointer, it does not point at a place, so its contacts
// do not reach behaviors as touches: one finger moves the gamepad cursor and a tap clicks at it, and two fingers are two
// touch pointers next to the cursor, for F_HANDLE_TOUCHPAD_PAIR behaviors. See GuiScene::onPadTouchpadEvent.
struct PadTouchpad
{
  static constexpr unsigned TAP_MAX_MSEC = 250;
  static constexpr float TAP_MAX_MOVE = 0.03f; // of the screen height
  // of the screen height: a finger moves the cursor only after it moved more, so a tap that moves less does not move it
  static constexpr float CURSOR_MOVE_THRESHOLD = 0.01f;
  // the part of the finger offsets that the pointers of a pair keep, so both hit the element under the cursor
  static constexpr float PAIR_SPREAD = 0.25f;

  enum class Mode
  {
    NONE,
    CURSOR,
    PAIR,
    WAIT, // for all fingers to lift
  };

  struct Contact
  {
    int touchIdx;
    Point2 pos; // touch coordinates
  };

  Mode mode = Mode::NONE;
  eastl::fixed_vector<Contact, 2, false> contacts;
  unsigned buttonStateAtPress = 0;
  Point2 pairCursor = Point2(0, 0);
  Point2 pairStart[2] = {Point2(0, 0), Point2(0, 0)};
  unsigned downMsec = 0; // of the first finger
  float maxMove = 0;     // of the lifted fingers, from their presses
  bool pairTapEligible = false;

  void beginTouch(unsigned msec, unsigned button_state)
  {
    buttonStateAtPress = button_state;
    downMsec = msec;
    maxMove = 0;
  }

  bool isTap(unsigned now_msec, float limit, unsigned button_state) const
  {
    return !(buttonStateAtPress & 1) && button_state == buttonStateAtPress && now_msec - downMsec <= TAP_MAX_MSEC && maxMove <= limit;
  }

  Contact *find(int touch_idx)
  {
    for (Contact &c : contacts)
      if (c.touchIdx == touch_idx)
        return &c;
    return nullptr;
  }

  void beginPair(const Point2 &cursor)
  {
    mode = Mode::PAIR;
    pairCursor = cursor;
    pairStart[0] = contacts[0].pos;
    pairStart[1] = contacts[1].pos;
    pairTapEligible = true;
  }

  Point2 pairPointerPos(int i, float sensitivity) const
  {
    const Point2 c = (contacts[0].pos + contacts[1].pos) * 0.5f;
    const Point2 startCenter = (pairStart[0] + pairStart[1]) * 0.5f;
    const Point2 offset = contacts[i].pos - c;
    const float len = length(offset), startLen = length(pairStart[i] - startCenter);
    // a pinch zooms by the pointer distance ratio, which gets the sensitivity as a power: in and out alike
    const float scale = len > 0 && startLen > 0 ? powf(len / startLen, sensitivity) * startLen / len : 1.f;
    return pairCursor + (c - startCenter) * sensitivity + offset * (scale * PAIR_SPREAD);
  }
};

} // namespace darg
