// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <cstdint>
#include <debug/dag_assert.h>

struct SlotTexturesRangeInfo
{
  union
  {
    struct
    {
      uint8_t texBase;
      uint8_t smpBase : 4;
      uint8_t count : 4;
    };
    uint16_t raw = 0;
  };

  SlotTexturesRangeInfo() = default;
  explicit SlotTexturesRangeInfo(uint16_t raw_bytes) : raw{raw_bytes} {}
  SlotTexturesRangeInfo(uint8_t tex_base, uint8_t smp_base, uint8_t tex_count) : texBase{tex_base}, smpBase{smp_base}, count{tex_count}
  {
    G_ASSERT(smp_base < (1 << 4));
    G_ASSERT(tex_count < (1 << 4));
  }

  friend bool operator==(SlotTexturesRangeInfo r1, SlotTexturesRangeInfo r2)
  {
    return r1.texBase == r2.texBase && r1.smpBase == r2.smpBase && r1.count == r2.count;
  }
};

G_STATIC_ASSERT(sizeof(SlotTexturesRangeInfo) == 2);