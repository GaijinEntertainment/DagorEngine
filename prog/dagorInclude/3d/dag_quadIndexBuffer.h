//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace index_buffer
{
extern void init_box();
extern void use_box();
extern void release_box();

extern void init_quads_16bit();
extern void use_quads_16bit();
extern void release_quads_16bit();

extern void init_quads_32bit(int quads_count);
extern void start_use_quads_32bit();
extern void end_use_quads_32bit();
extern void release_quads_32bit();

struct Quads32BitUsageLock
{
  Quads32BitUsageLock() { start_use_quads_32bit(); }
  ~Quads32BitUsageLock() { end_use_quads_32bit(); }
};
} // namespace index_buffer
