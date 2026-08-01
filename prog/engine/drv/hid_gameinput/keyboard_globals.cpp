// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/hid/dag_hiGlobals.h>

// Isolated in its own TU so the linker pulls it only when no other linked HID
// driver (e.g. hid_nulldrv's kbd_null) already provides the symbol. Keeping it
// alongside the always-linked class driver code causes a duplicate-symbol clash.
bool HumanInput::keyboard_has_ime_layout() { return false; }
