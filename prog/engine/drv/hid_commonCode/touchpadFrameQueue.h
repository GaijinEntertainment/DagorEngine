// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/hid/dag_hiJoystick.h>
#include <generic/dag_span.h>
#include <EASTL/fixed_vector.h>


namespace hid
{
// Under caller lock, sample every report.
// setButtonDown includes reports that are not pushed.
// push stamps buttonState from that counter.
class TouchpadFrameQueue
{
public:
  using Frame = HumanInput::JoystickTouchpadFrame;
  static constexpr int CAPACITY = HumanInput::IGenJoystick::TOUCHPAD_FRAME_CAPACITY;

  void setButtonDown(bool down) { buttonState += bool(buttonState & 1) != down; }

  void push(Frame f)
  {
    f.buttonState = buttonState;
    if (frames.full())
      frames.erase(frames.begin());
    frames.push_back(f);
    last = f;
  }

  // the frames since the last call, oldest first; at least the current contacts, so a reader that comes from another pad
  // ends the touches of that pad. An empty out drops the frames
  int fetch(dag::Span<Frame> out)
  {
    if (frames.empty())
      frames.push_back(last);
    const int skip = frames.size() > out.size() ? frames.size() - out.size() : 0;
    const int count = frames.size() - skip;
    for (int i = 0; i < count; i++)
      out[i] = frames[skip + i];
    frames.clear();
    return count;
  }

private:
  uint32_t buttonState = 0;
  eastl::fixed_vector<Frame, CAPACITY, false> frames;
  Frame last; // of the latest report
};
} // namespace hid
