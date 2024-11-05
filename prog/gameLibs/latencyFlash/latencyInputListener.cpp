// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <latencyFlash/latencyInputListener.h>

#include <3d/dag_lowLatency.h>

bool LatencyInputEventListener::ehIsActive() const { return true; }

bool LatencyInputEventListener::gmehMove(const Context &, float, float) { return false; }

bool LatencyInputEventListener::gmehButtonDown(const Context &, int btn_idx)
{
  if (btn_idx == 0)
    lowlatency::mark_flash_indicator();
  return false;
}

bool LatencyInputEventListener::gmehButtonUp(const Context &, int) { return false; }

bool LatencyInputEventListener::gmehWheel(const Context &, int) { return false; }

bool LatencyInputEventListener::gkehButtonDown(const Context &, int, bool, wchar_t) { return false; }

bool LatencyInputEventListener::gkehButtonUp(const Context &, int) { return false; }

void LatencyInputEventListener::register_handler()
{
  if (!registered && lowlatency::is_inited())
  {
    register_hid_event_handler(this, 5);
    registered = true;
  }
}

void LatencyInputEventListener::unregister_handler()
{
  // FIXME: this code is broken - you cannot dynamically unregister hid event handlers and immediately delete it
  // (because joystick update thread might call its methods)
  if (registered)
    unregister_hid_event_handler(this);
  registered = false;
}

LatencyInputEventListener::LatencyInputEventListener() { register_handler(); }

LatencyInputEventListener::~LatencyInputEventListener() { unregister_handler(); }
