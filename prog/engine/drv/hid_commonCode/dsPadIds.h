// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include <drv/hid/dag_hiJoystick.h>

// USB ids of the pads that hid_dspad_pc reads over raw HID
namespace HumanInput
{
namespace sonypad
{
enum : unsigned
{
  VID_SONY = GAMEPAD_VENDOR_SONY,
  PID_DUALSENSE = 0x0CE6,
  PID_DUALSENSE_EDGE = 0x0DF2,
  PID_DUALSHOCK4_V1 = 0x05C4,
  PID_DUALSHOCK4_V2 = 0x09CC,
  PID_DUALSHOCK4_DONGLE = 0x0BA0,
};

enum class Model : uint8_t
{
  UNKNOWN,
  DUALSENSE,
  DUALSENSE_EDGE,
  DUALSHOCK4
};

inline Model model_from_pid(unsigned pid)
{
  switch (pid)
  {
    case PID_DUALSENSE: return Model::DUALSENSE;
    case PID_DUALSENSE_EDGE: return Model::DUALSENSE_EDGE;
    case PID_DUALSHOCK4_V1:
    case PID_DUALSHOCK4_V2:
    case PID_DUALSHOCK4_DONGLE: return Model::DUALSHOCK4;
  }
  return Model::UNKNOWN;
}

inline bool is_sony_pad(unsigned vid, unsigned pid) { return vid == VID_SONY && model_from_pid(pid) != Model::UNKNOWN; }
} // namespace sonypad
} // namespace HumanInput
