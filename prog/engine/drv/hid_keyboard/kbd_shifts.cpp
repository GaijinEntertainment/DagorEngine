// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "kbd_private.h"
#include <drv/hid/dag_hiKeybIds.h>
#include <string.h>

unsigned char HumanInput::key_to_shift_bit[256];

void HumanInput::init_key_to_shift_bit()
{
  memset(key_to_shift_bit, 0x1F, sizeof(key_to_shift_bit));
#if _TARGET_PC_WIN | _TARGET_PC_LINUX | _TARGET_XBOX
  key_to_shift_bit[DKEY_LSHIFT] = 0;
  key_to_shift_bit[DKEY_RSHIFT] = 1;
  key_to_shift_bit[DKEY_LCONTROL] = 2;
  key_to_shift_bit[DKEY_RCONTROL] = 3;
  key_to_shift_bit[DKEY_LALT] = 4;
  key_to_shift_bit[DKEY_RALT] = 5;
#elif _TARGET_PC_MACOSX
  key_to_shift_bit[DKEY_LSHIFT] = 0;
  key_to_shift_bit[DKEY_RSHIFT] = 1;
  key_to_shift_bit[DKEY_LCONTROL] = 2;
  // key_to_shift_bit[DKEY_RCONTROL] = 3;
  key_to_shift_bit[DKEY_LALT] = 4;
  key_to_shift_bit[DKEY_RALT] = 5;
#endif
}
