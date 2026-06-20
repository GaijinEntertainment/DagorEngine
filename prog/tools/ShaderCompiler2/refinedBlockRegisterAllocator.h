// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <drv/3d/dag_consts.h>
#include <EASTL/utility.h>
#include <EASTL/fixed_list.h>

#include "hlslRegisters.h"

namespace shc
{

struct RefinedBlockRegister
{
  eastl::optional<eastl::pair<int, int>> cbufOffsetAndCount;
  eastl::array<eastl::optional<int>, STAGE_MAX> slot{};
  eastl::optional<HlslRegisterSpace> space;
};

class RefinedBlockRegisterAllocator
{
public:
  RefinedBlockRegisterAllocator();

  // Allocate N cbuf vec4 slots; returns the base offset. Used for numeric vars and bindless IDs.
  int allocCbufSlot(uint32_t slotCount);
  void reserveCbufSlot(uint32_t offset, uint32_t count);

  // Allocate a slot in register space
  // VS and PS/CS have independent register spaces.
  int allocSlot(ShaderStage stage, HlslRegisterSpace space);
  void reserveSlot(ShaderStage stage, HlslRegisterSpace space, HlslSlotSemantic semantic, uint32_t slot, uint32_t count = 1);

  // Expose T-slot allocators so preshader can call reserveAllFrom().
  const HlslRegAllocator &vsAllocator(HlslRegisterSpace space) const { return vsSlotAllocator[space]; }
  const HlslRegAllocator &psOrCsAllocator(HlslRegisterSpace space) const { return psOrCsSlotAllocator[space]; }

private:
  HlslRegAllocator cRegAllocator;
  eastl::array<HlslRegAllocator, HlslRegisterSpace::HLSL_RSPACE_COUNT> vsSlotAllocator;
  eastl::array<HlslRegAllocator, HlslRegisterSpace::HLSL_RSPACE_COUNT> psOrCsSlotAllocator;

  struct Allocation
  {
    uint32_t slot = 0;
    uint32_t count = 0;
  };

  // Sub-register packing state: tracks partial register usage.
  eastl::fixed_list<Allocation, 128> cbufFreeAllocations;
};

} // namespace shc
