//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>

namespace compress_call_stack
{

static constexpr uint32_t bytes_per_frame_bound = sizeof(uintptr_t) > 4 ? 7 : 5;
inline constexpr uint32_t compressed_size_bound(uint32_t cs_len) { return cs_len * bytes_per_frame_bound; }

// outbuf has to be no less than compressed_size_bound(cs_len)
// returns number of bytes written
// base better be uintptr_t(stackhlp_get_bp()), but you would need to save it somewhere to decompress between runs.
inline uint32_t compress(uint8_t *outbuf, const uintptr_t *call_stack, uint32_t cs_len, uintptr_t base = 0)
{
  if (!cs_len)
    return 0;
  uintptr_t prevStack = base;
  auto out = outbuf;
  for (auto cse = call_stack + cs_len; call_stack != cse; ++call_stack)
  {
    const uintptr_t frame = *call_stack;
    const intptr_t delta = frame - prevStack;
    uintptr_t v = (delta << intptr_t(1)) ^ (delta >> intptr_t(8 * sizeof(uintptr_t) - 1));
    do
    {
      uint8_t chunk = v & 0x7F;
      v >>= 7;
      if (v)
        chunk |= 0x80;
      *out++ = chunk;
    } while (v);

    prevStack = frame;
  }
  return out - outbuf;
}

// returns call_stack size
inline void decompress(uintptr_t *call_stack, uint32_t cs_len, const uint8_t *compressed, uintptr_t base = 0)
{
  uintptr_t prev = base;
  for (uint32_t depth = 0; depth < cs_len; ++depth)
  {
    uintptr_t zigzag = 0;
    uint8_t byte;
    int shift = 0;
    do
    {
      byte = *compressed++;
      zigzag |= uintptr_t(byte & 0x7F) << uintptr_t(shift);
      shift += 7;
    } while (byte & 0x80);

    // intptr_t delta = (zigzag & uintptr_t(1)) ? -(zigzag >> uintptr_t(1)) : (zigzag >> uintptr_t(1));
    intptr_t delta = (intptr_t)(zigzag >> uintptr_t(1)) ^ -intptr_t(zigzag & uintptr_t(1));
    prev += delta;
    call_stack[depth] = prev;
  }
}

} // namespace compress_call_stack