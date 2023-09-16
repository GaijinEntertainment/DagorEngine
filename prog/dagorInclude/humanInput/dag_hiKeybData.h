//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

namespace HumanInput
{

struct KeyboardSettings
{
  bool present;
  bool enabled;
  bool magicKeysEnabled;
};

struct KeyboardRawState
{
  enum
  {
    LSHIFT_BIT = 0x0001,
    RSHIFT_BIT = 0x0002,
    LCTRL_BIT = 0x0004,
    RCTRL_BIT = 0x0008,
    LALT_BIT = 0x0010,
    RALT_BIT = 0x0020,

    SHIFT_BITS = LSHIFT_BIT | RSHIFT_BIT,
    CTRL_BITS = LCTRL_BIT | RCTRL_BIT,
    ALT_BITS = LALT_BIT | RALT_BIT,
  };

  unsigned bitMask[256 / 32];
  unsigned shifts;

  bool isKeyDown(int btn_idx) const { return bitMask[btn_idx >> 5] & (1 << (btn_idx & 0x1F)); }

  // to be used by driver
  void setKeyDown(int btn_idx) { bitMask[btn_idx >> 5] |= (1 << (btn_idx & 0x1F)); }
  void clrKeyDown(int btn_idx) { bitMask[btn_idx >> 5] &= ~(1 << (btn_idx & 0x1F)); }
};

} // namespace HumanInput
