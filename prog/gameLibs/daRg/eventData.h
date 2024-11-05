// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <sqrat.h>


namespace darg
{

class Element;

struct EventDataRect
{
  int l, t, r, b;
};

struct MouseClickEventData
{
  short screenX = -1, screenY = -1;
  int button = 0;
  bool ctrlKey = false, shiftKey = false, altKey = false;
  InputDevice devId = DEVID_NONE;
  EventDataRect targetRect = {-1, -1, -1, -1};
  Sqrat::Object target = Sqrat::Object();

  const EventDataRect &getTargetRect() { return targetRect; }
};

struct HotkeyEventData
{
  EventDataRect targetRect = {-1, -1, -1, -1};

  const EventDataRect &getTargetRect() { return targetRect; }
};

struct HoverEventData
{
  EventDataRect targetRect = {-1, -1, -1, -1};

  const EventDataRect &getTargetRect() { return targetRect; }
};

} // namespace darg
