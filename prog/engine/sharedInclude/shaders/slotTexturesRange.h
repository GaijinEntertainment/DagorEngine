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
      uint8_t smpBase;
      uint8_t texCount;
      uint8_t smpCount;
    };
    uint32_t raw = 0;
  };

  SlotTexturesRangeInfo() = default;
  explicit SlotTexturesRangeInfo(uint32_t raw_bytes) : raw{raw_bytes} {}
  SlotTexturesRangeInfo(uint8_t tex_base, uint8_t smp_base, uint8_t tex_count, uint8_t smp_count) :
    texBase{tex_base}, smpBase{smp_base}, texCount{tex_count}, smpCount{smp_count}
  {}

  friend bool operator==(SlotTexturesRangeInfo r1, SlotTexturesRangeInfo r2) { return r1.raw == r2.raw; }
};

G_STATIC_ASSERT(sizeof(SlotTexturesRangeInfo) == 4);