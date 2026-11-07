// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include <drv/hid/dag_hiJoyData.h>
#include <drv/hid/dag_hiPointingData.h>
#include <generic/dag_span.h>
#include <math/dag_Point2.h>

namespace HumanInput
{
class IGenPointing;
class IGenPointingClient;
} // namespace HumanInput

namespace hid
{
// Sends the contacts of a game pad touch pad as TS_touchPad touches, with the ids idBase + contact id. Such a pad moves a
// pointer, it does not point at a place: the first contact begins at the anchor, and each contact keeps its offset from
// it, times the scale: display rows over pad rows, at 44.86 pad pixels per millimetre. The one-finger gain
// of stg_pnt.touchPadBallistics shifts the anchor per report, so it adds to the travel and not to the offsets. So the
// positions are screen pixels with true deltas. Defined in touch_api_impl.inc.cpp.
struct TouchpadTouchSender
{
  static constexpr intptr_t ID_RANGE = 256;
  intptr_t idBase = 0;
  Point2 origin = Point2(0, 0), anchor = Point2(0, 0);
  int segment = 0; // of the Windows curve, last report
  HumanInput::JoystickTouchpadFrame prev;
  Point2 contactStart[HumanInput::MAX_TOUCHES] = {}; // pad press position by touch slot

  // the frames of one fetch, oldest first; a touch that a frame does not have ends, so an empty frame or span ends all.
  // Begins and ends go out in report order, the moves once per batch and before each begin or end. state: the device
  // state, not raw_state_pnt
  void send(HumanInput::PointingRawState &state, HumanInput::IGenPointing *drv, HumanInput::IGenPointingClient *cli,
    dag::ConstSpan<HumanInput::JoystickTouchpadFrame> frames, const Point2 &anchor_pos, float screen_height);
};
} // namespace hid
