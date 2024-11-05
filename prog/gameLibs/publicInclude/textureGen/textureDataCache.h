//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>

namespace regcache
{
extern uint64_t memory_limit;

void clear();
bool is_record_exists(const char *key);
void *get_record_data_ptr(const char *key);
void set_record_data(const char *key, void *data, size_t size);
void update();
} // namespace regcache
