// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/hid/dag_hiJoystick.h>
#include <EASTL/unique_ptr.h>

namespace HumanInput
{

// ring behind fetchMotionSamples; not thread safe
class JoystickMotionHistory
{
public:
  static constexpr uint32_t CAPACITY = IGenJoystick::MOTION_HISTORY_CAPACITY;

  void enable(bool on)
  {
    if (on == isEnabled())
      return;
    ring.reset(on ? new JoystickMotionSample[CAPACITY] : nullptr);
    clear();
  }

  void clear() { count = dropped = 0; }
  bool isEnabled() const { return bool(ring); }

  void push(const JoystickMotionSample &s)
  {
    if (!ring)
      return;
    ring[(start + count) % CAPACITY] = s;
    if (count < CAPACITY)
      count++;
    else
    {
      start = (start + 1) % CAPACITY;
      dropped++;
    }
  }

  int fetch(dag::Span<JoystickMotionSample> out, uint32_t &out_dropped)
  {
    const uint32_t n = min(count, uint32_t(out.size()));
    out_dropped = dropped + count - n;
    for (uint32_t i = 0; i < n; i++)
      out[i] = ring[(start + count - n + i) % CAPACITY];
    count = dropped = 0;
    return n;
  }

private:
  eastl::unique_ptr<JoystickMotionSample[]> ring;
  uint32_t start = 0, count = 0, dropped = 0;
};

} // namespace HumanInput
