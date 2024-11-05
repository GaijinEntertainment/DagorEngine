// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdint.h>

class IGenLoad;
class IGenSave;

namespace da_profiler
{
typedef void (*read_and_apply_message_cb_t)(uint16_t t, uint32_t len, IGenLoad &load_cb, IGenSave &save_cb);
void add_message(uint16_t t, read_and_apply_message_cb_t);
bool read_and_perform_message(uint16_t t, uint32_t size, IGenLoad &load_cb, IGenSave &save_cb);

} // namespace da_profiler