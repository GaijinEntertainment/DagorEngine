// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_assert.h>
#include <EASTL/algorithm.h>

#include "refinedBlockRegisterAllocator.h"
#include "const3d.h"

namespace shc
{

RefinedBlockRegisterAllocator::RefinedBlockRegisterAllocator()
{

  // TODO: Figure out how in preshader parse pass reserve hardcoded slots.

  vsSlotAllocator[HLSL_RSPACE_T] = HlslRegAllocator(HlslRegAllocator::Policy{0, MAX_T_REGISTERS});
  psOrCsSlotAllocator[HLSL_RSPACE_T] = HlslRegAllocator(HlslRegAllocator::Policy{0, MAX_T_REGISTERS});

  cRegAllocator = make_default_cbuf_reg_allocator();
}

int RefinedBlockRegisterAllocator::allocCbufSlot(uint32_t slotCount)
{
  G_ASSERT(slotCount <= SLOTS_PER_C_REGISTER || slotCount % SLOTS_PER_C_REGISTER == 0);

  auto freeSlot = cbufFreeAllocations.end();

  if (slotCount <= SLOTS_PER_C_REGISTER)
    freeSlot = eastl::find_if(cbufFreeAllocations.begin(), cbufFreeAllocations.end(), [slotCount](const Allocation &freeSlot) {
      G_ASSERT(freeSlot.count <= SLOTS_PER_C_REGISTER);
      return freeSlot.count >= slotCount && (SLOTS_PER_C_REGISTER - freeSlot.count) % slotCount == 0;
    });

  if (freeSlot != cbufFreeAllocations.end())
  {
    const auto slot = freeSlot->slot;
    freeSlot->slot += slotCount;
    freeSlot->count -= slotCount;
    if (freeSlot->count == 0)
      cbufFreeAllocations.erase(freeSlot);

    return slot;
  }

  const auto slot = cRegAllocator.allocate(max(slotCount / SLOTS_PER_C_REGISTER, 1u)) * SLOTS_PER_C_REGISTER;

  if (slotCount < SLOTS_PER_C_REGISTER) //-V1051
    cbufFreeAllocations.push_back({.slot = slot + slotCount, .count = SLOTS_PER_C_REGISTER - slotCount});

  return slot;
}

void RefinedBlockRegisterAllocator::reserveCbufSlot(uint32_t slot, uint32_t count)
{
  G_ASSERT(count < SLOTS_PER_C_REGISTER || count % SLOTS_PER_C_REGISTER == 0);
  const uint32_t regCount = (slot % SLOTS_PER_C_REGISTER + count + SLOTS_PER_C_REGISTER - 1) / SLOTS_PER_C_REGISTER;
  cRegAllocator.reserve(HlslSlotSemantic::RESERVED, slot / SLOTS_PER_C_REGISTER, regCount)
    .or_else([slot, count](auto err) -> dag::Expected<void, decltype(err)> {
      sh_debug(SHLOG_FATAL, "refined block register allocator: failed to reserve cbuffer regs [%d, %d)", slot, slot + count);
      return dag::Unexpected(err);
    })
    .error();
}

int RefinedBlockRegisterAllocator::allocSlot(ShaderStage stage, HlslRegisterSpace space)
{
  auto &allocator = (stage == STAGE_VS) ? vsSlotAllocator[space] : psOrCsSlotAllocator[space];
  return allocator.allocate();
}

void RefinedBlockRegisterAllocator::reserveSlot(ShaderStage stage, HlslRegisterSpace space, uint32_t slot)
{
  auto &allocator = (stage == STAGE_VS) ? vsSlotAllocator[space] : psOrCsSlotAllocator[space];
  allocator.reserve(HlslSlotSemantic::RESERVED, slot)
    .or_else([slot](auto err) -> dag::Expected<void, decltype(err)> {
      sh_debug(SHLOG_FATAL, "refined block register allocator: failed to reserve t-slot t%d", slot);
      return dag::Unexpected(err);
    })
    .error();
}

} // namespace shc
