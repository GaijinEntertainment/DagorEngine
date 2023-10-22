#include "device.h"

using namespace drv3d_dx12;

namespace
{
void report_resources_impl(const PipelineStageStateBase &state, uint32_t b_reg_mask, uint32_t t_reg_mask, uint32_t u_reg_mask,
  uint32_t s_reg_mask, uint32_t s_reg_with_cmp_mask)
{
  for_each_set_bit(b_reg_mask, [&state](uint32_t i) {
    auto gpuPointer = state.bRegisters[i].BufferLocation;
    if (gpuPointer)
    {
      auto &bufferResource = state.bRegisterBuffers[i];
      debug("DX12: ...B register slot %u with %p / %u @ %u...", i, gpuPointer, bufferResource.buffer, bufferResource.resourceId,
        gpuPointer - bufferResource.buffer->GetGPUVirtualAddress());
    }
    else if (i == 0)
    {
      debug("DX12: ...B register slot %u with register constant buffer...", i);
    }
    else
    {
      debug("DX12: ...B register slot %u has no buffer bound to...", i);
    }
  });

  for_each_set_bit(t_reg_mask, [&state](uint32_t i) {
    auto &slot = state.tRegisters[i];
    if (slot.image)
    {
      debug("DX12: ...T register slot %u with texture %p (%u) and view %u...", i, slot.image->getHandle(), slot.image->getType(),
        slot.view.ptr);
    }
    else if (slot.buffer)
    {
      debug("DX12: ...T register slot %u with buffer %p and view %u...", i, slot.buffer.buffer, slot.view.ptr);
    }
    else
    {
      debug("DX12: ...T register slot %u has no resource bound to, null resource is used instead...", i);
    }
  });

  for_each_set_bit(u_reg_mask, [&state](uint32_t i) {
    auto &slot = state.uRegisters[i];
    if (slot.image)
    {
      debug("DX12: ...U register slot %u with texture %p (%u) and view %u...", i, slot.image->getHandle(), slot.image->getType(),
        slot.view.ptr);
    }
    else if (slot.buffer)
    {

      debug("DX12: ...U register slot %u with buffer %p and view %u...", i, slot.buffer.buffer, slot.view.ptr);
    }
    else
    {
      debug("DX12: ...U register slot %u has no resource bound to, null resource is used instead...", i);
    }
  });

  for_each_set_bit(s_reg_mask, [&state, s_reg_with_cmp_mask](uint32_t i) {
    auto slot = state.sRegisters[i];
    if (slot.ptr)
    {
      if (s_reg_with_cmp_mask & (1u << i))
      {
        debug("DX12: ...S register slot %u with comparison sampler %u...", i, slot.ptr);
      }
      else
      {
        debug("DX12: ...S register slot %u with sampler %u...", i, slot.ptr);
      }
    }
    else
    {
      debug("DX12: ...S register slot %u has no sampler bound to!...", i);
    }
  });

  // TODO: bindless is a global state and we don't have any debug info for those
}
} // namespace

void debug::report_resources(const PipelineStageStateBase &state, ComputePipeline *pipe)
{
  const auto &header = pipe->getHeader();
  report_resources_impl(state, header.resourceUsageTable.bRegisterUseMask, header.resourceUsageTable.tRegisterUseMask,
    header.resourceUsageTable.uRegisterUseMask, header.resourceUsageTable.sRegisterUseMask, header.sRegisterCompareUseMask);
}

void debug::report_resources(const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline *base_pipe)
{
  const auto &signature = base_pipe->getSignature();
  const auto &psHeader = base_pipe->getPSHeader();
  report_resources_impl(vs, signature.vsCombinedBRegisterMask, signature.vsCombinedTRegisterMask, signature.vsCombinedURegisterMask,
    signature.vsCombinedSRegisterMask, base_pipe->getVertexShaderSamplerCompareMask());
  report_resources_impl(ps, psHeader.resourceUsageTable.bRegisterUseMask, psHeader.resourceUsageTable.tRegisterUseMask,
    psHeader.resourceUsageTable.uRegisterUseMask, psHeader.resourceUsageTable.sRegisterUseMask, psHeader.sRegisterCompareUseMask);
}
