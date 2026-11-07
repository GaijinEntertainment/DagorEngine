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
// Creates or replaces the record for key. Returns size bytes to fill; valid until the next regcache call that changes
// the cache.
void *create_record(const char *key, size_t size);
void update();
// A pinned record is never evicted: a pass that rewired a node to read it cannot recompute it on a miss.
void pin(const char *key);
void unpin_all();
} // namespace regcache
