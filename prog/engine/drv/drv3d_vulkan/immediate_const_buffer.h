// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// push constants via const buffer, fallback when push constants are slower/broken

#include "util/tracked_state.h"
#include "driver.h"
#include "buffer_ref.h"

namespace drv3d_vulkan
{

class ImmediateConstBuffer
{
  static constexpr int ring_size = GPU_TIMELINE_HISTORY_SIZE;
  static constexpr int initial_blocks = 16384;
  static constexpr int element_size = MAX_IMMEDIATE_CONST_WORDS * sizeof(uint32_t);

  Buffer *ring[ring_size] = {};
  uint32_t ringIdx = 0;
  uint32_t offset = 0;
  uint32_t alignedElementSize = 0;

  void flushWrites();

public:
  BufferRef push(const uint32_t *data);
  void onFlush();
  void init();
  void shutdown();
};

struct ImmediateConstBuffers
{
  ImmediateConstBuffer arr[(uint32_t)ExtendedShaderStage::MAX];

  ImmediateConstBuffer &operator[](ExtendedShaderStage idx) { return arr[(uint32_t)idx]; }

  void shutdown()
  {
    for (ImmediateConstBuffer &i : arr)
      i.shutdown();
  }

  void flush()
  {
    for (ImmediateConstBuffer &icb : arr)
      icb.onFlush();
  }

  void init()
  {
    for (ImmediateConstBuffer &icb : arr)
      icb.init();
  }
};

} // namespace drv3d_vulkan
