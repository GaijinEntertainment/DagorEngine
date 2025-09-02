// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hlslRegisters.h"
#include "const3d.h"
#include "globalConfig.h"


const HlslRegAllocator::allocator_scan_routine_t HlslRegAllocator::DEFAULT_SCAN = +[](HlslRegAllocator &alloc, uint32_t count) {
  for (uint32_t i = alloc.policy.base; i <= alloc.policy.cap - count; ++i)
  {
    if (alloc.slots.size() < i + count)
      alloc.slots.resize(i + count);
    uint32_t lastTaken = -1;
    for (uint32_t j = i; j < i + count; ++j)
    {
      if (alloc.slots[j].used)
        lastTaken = j;
    }

    if (lastTaken == -1)
      return int32_t(i);
    else
      i = lastTaken;
  }
  return int32_t{-1};
};
const HlslRegAllocator::allocator_scan_routine_t HlslRegAllocator::BACKWARDS_SCAN = +[](HlslRegAllocator &alloc, uint32_t count) {
  G_ASSERT(alloc.policy.cap < INT16_MAX); // Sanity check to avoid accidentally creating large bw allocators
  for (uint32_t i = alloc.policy.cap - count; i != alloc.policy.base - 1; --i)
  {
    if (alloc.slots.size() < i + count)
      alloc.slots.resize(i + count);
    uint32_t firstTaken = -1;
    for (uint32_t j = i; j < i + count; ++j)
    {
      if (alloc.slots[j].used)
      {
        firstTaken = j;
        break;
      }
    }

    if (firstTaken == -1)
      return int32_t(i);
    else
      i = firstTaken - count + 1;
  }
  return int32_t{-1};
};

eastl::array<HlslRegAllocator, HLSL_RSPACE_COUNT> make_default_hlsl_reg_allocators(int max_const_count)
{
  return {
    HlslRegAllocator{HlslRegAllocator::Policy{0, MAX_T_REGISTERS}},
    // Limiting to 16 because of 4 bits for size in slot textures
    HlslRegAllocator{HlslRegAllocator::Policy{0, uint32_t(shc::config().enableBindless ? MAX_S_REGISTERS : 16)}},
    HlslRegAllocator{HlslRegAllocator::Policy{0, static_cast<uint32_t>(max_const_count)}},
    // @TODO: only backwards scan on DX11. This requires register refactorings.
    // HlslRegAllocator{HlslRegAllocator::Policy{
    //   0, MAX_U_REGISTERS, UAVS_CONTEND_WITH_RTVS ? HlslRegAllocator::BACKWARDS_SCAN : HlslRegAllocator::DEFAULT_SCAN}},
    HlslRegAllocator{HlslRegAllocator::Policy{0, MAX_U_REGISTERS, HlslRegAllocator::BACKWARDS_SCAN}},
    HlslRegAllocator{HlslRegAllocator::Policy{0, MAX_B_REGISTERS}},
  };
}

HlslRegAllocator make_default_cbuf_reg_allocator() { return HlslRegAllocator{HlslRegAllocator::Policy{0, MAX_CBUFFER_VECTORS}}; }