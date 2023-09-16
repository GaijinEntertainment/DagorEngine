// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef _GAIJIN_DRV_HID_KEYBOARD_KBD_PRIVATE_H
#define _GAIJIN_DRV_HID_KEYBOARD_KBD_PRIVATE_H
#pragma once

namespace HumanInput
{

extern unsigned char key_to_shift_bit[256];
extern char *key_name[256];

void init_key_to_shift_bit();

} // namespace HumanInput

#endif
