// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "pipeline_resource_reporter.h"
#include <pipeline.h>
#include <resource_manager/image.h>

using namespace drv3d_dx12;

namespace
{

void report_resource(ID3D12Resource *resource)
{
  D3D12_RESOURCE_DESC desc = resource->GetDesc();
  logdbg("DX12: [resource] type=%s, format=%s, extent=%dx%dx%d, mips=%d, layout=%d, flags=0x%x, gpuaddr=0x%lx",
    to_string(desc.Dimension), dxgi_format_name(desc.Format), (uint32_t)desc.Width, desc.Height, (uint32_t)desc.DepthOrArraySize,
    (uint32_t)desc.MipLevels, (uint32_t)desc.Layout, (uint32_t)desc.Flags, resource->GetGPUVirtualAddress());
}

void report_detailed_image_srv(const PipelineStageStateBase::TRegister &t_register)
{
  Image *image = t_register.image;
  image->getDebugName([](const char *name) { logdbg("DX12: '%s'", name); });

  const auto &extent = image->getBaseExtent();
  logdbg("DX12: [image] type=%s, format=%s, extent=%dx%dx%d, mips=%d, layers=%d, planes=%d, msaa=%d, viewcount=%d",
    to_string(image->getType()), image->getFormat().getNameString<true>(), extent.width, extent.height, extent.depth,
    (uint32_t)image->getMipLevelRange().count(), (uint32_t)image->getArrayLayers().count(), (uint32_t)image->getPlaneCount().count(),
    (uint32_t)image->getMsaaLevel(), (uint32_t)image->getViewCount());

  if (auto resource = image->getHandle())
    report_resource(resource);

  auto memory = image->getMemory();
  logdbg("DX12: [memory] heapID=0x%x, memofs=%ld, memsize=%ld, aliased=%d, reserved=%d, heapalloced=%d", memory.getHeapID().raw,
    memory.getOffset(), memory.size(), (image->isAliased() ? 1 : 0), (image->isReserved() ? 1 : 0),
    (image->isHeapAllocated() ? 1 : 0));

  auto view = t_register.imageView;
  logdbg("DX12: [viewstate] cube=%d, arr=%d, format=%s, mipofs=%d, mips=%d, arrofs=%d, arrcount=%d", (view.isCubemap ? 1 : 0),
    (view.isArray ? 1 : 0), view.getFormat().getNameString<true>(), (uint32_t)view.getMipBase().index(), (uint32_t)view.getMipCount(),
    (uint32_t)view.getArrayBase().index(), (uint32_t)view.getArrayCount());

  bool isValidDescriptor = (image->getRecentView().handle == t_register.view && image->getRecentView().state == view);
  for (const auto oldView : image->getOldViews())
    isValidDescriptor |= (oldView.handle == t_register.view && oldView.state == view);

  logdbg("DX12: [diagnostic] descvalid=%d, isSRV=%d", (isValidDescriptor ? 1 : 0), (view.isSRV() ? 1 : 0));
}

void report_detailed_buffer_srv(const PipelineStageStateBase::TRegister &t_register)
{
  BufferGlobalId resId = t_register.buffer.resourceId;
  logdbg("DX12: [resourceId] index=%d, const_vtx=%d, idx=%d, indirect=%d, non_ps=%d, ps=%d, copy=%d, uav=%d, bindless=%d",
    resId.index(), (resId.isUsedAsVertexOrConstBuffer() ? 1 : 0), (resId.isUsedAsIndexBuffer() ? 1 : 0),
    (resId.isUsedAsIndirectBuffer() ? 1 : 0), (resId.isUsedAsNonPixelShaderResourceBuffer() ? 1 : 0),
    (resId.isUseAsPixelShaderResourceBuffer() ? 1 : 0), (resId.isUsedAsCopySourceBuffer() ? 1 : 0),
    (resId.isUsedAsUAVBuffer() ? 1 : 0), (resId.isUsedInBindlessHeap() ? 1 : 0));

  if (auto resource = t_register.buffer.buffer)
    report_resource(resource);
}

void report_resources_impl(const PipelineStageStateBase &state, uint32_t b_reg_mask, uint32_t t_reg_mask, uint32_t u_reg_mask,
  uint32_t s_reg_mask, uint32_t s_reg_with_cmp_mask, bool is_detailed_report)
{
  for (auto i : LsbVisitor{b_reg_mask})
  {
    auto gpuPointer = state.bRegisters[i].BufferLocation;
    if (gpuPointer)
    {
      auto &bufferResource = state.bRegisterBuffers[i].buffer;
      D3D12_GPU_VIRTUAL_ADDRESS bufferBaseAddress = bufferResource.buffer ? bufferResource.buffer->GetGPUVirtualAddress() : 0;
      logdbg("DX12: ...B register slot %u with %p / %u @ %u...", i, gpuPointer, bufferResource.buffer, bufferResource.resourceId,
        gpuPointer - bufferBaseAddress);
    }
    else if (i == 0)
    {
      logdbg("DX12: ...B register slot %u with register constant buffer...", i);
    }
    else
    {
      logdbg("DX12: ...B register slot %u has no buffer bound to...", i);
    }
  }

  for (auto i : LsbVisitor{t_reg_mask})
  {
    auto &slot = state.tRegisters[i];
    if (slot.image)
    {
      logdbg("DX12: ...T register slot %u with texture %p (%u) and view %p...", i, slot.image->getHandle(), slot.image->getType(),
        slot.view.ptr);

      if (is_detailed_report)
        report_detailed_image_srv(slot);
    }
    else if (slot.buffer)
    {
      logdbg("DX12: ...T register slot %u with buffer %p and view %p...", i, slot.buffer.buffer, slot.view.ptr);

      if (is_detailed_report)
        report_detailed_buffer_srv(slot);
    }
    else
    {
      logdbg("DX12: ...T register slot %u has no resource bound to, null resource is used instead...", i);
    }
  }

  for (auto i : LsbVisitor{u_reg_mask})
  {
    auto &slot = state.uRegisters[i];
    if (slot.image)
    {
      logdbg("DX12: ...U register slot %u with texture %p (%u) and view %p...", i, slot.image->getHandle(), slot.image->getType(),
        slot.view.ptr);
    }
    else if (slot.buffer)
    {
      logdbg("DX12: ...U register slot %u with buffer %p and view %p...", i, slot.buffer.buffer, slot.view.ptr);
    }
    else
    {
      logdbg("DX12: ...U register slot %u has no resource bound to, null resource is used instead...", i);
    }
  }

  for (auto i : LsbVisitor{s_reg_mask})
  {
    auto slot = state.sRegisters[i];
    if (slot.ptr)
    {
      if (s_reg_with_cmp_mask & (1u << i))
      {
        logdbg("DX12: ...S register slot %u with comparison sampler %p...", i, slot.ptr);
      }
      else
      {
        logdbg("DX12: ...S register slot %u with sampler %p...", i, slot.ptr);
      }
    }
    else
    {
      logdbg("DX12: ...S register slot %u has no sampler bound to!...", i);
    }
  }

  // TODO: bindless is a global state and we don't have any debug info for those
}
} // namespace

void debug::report_resources(const PipelineStageStateBase &state, ComputePipeline *pipe, bool is_detailed_report)
{
  const auto &header = pipe->getHeader();
  report_resources_impl(state, header.resourceUsageTable.bRegisterUseMask, header.resourceUsageTable.tRegisterUseMask,
    header.resourceUsageTable.uRegisterUseMask, header.resourceUsageTable.sRegisterUseMask, header.sRegisterCompareUseMask,
    is_detailed_report);
}

void debug::report_resources(const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline *base_pipe,
  bool is_detailed_report)
{
  const auto &signature = base_pipe->getSignature();
  const auto &psHeader = base_pipe->getPsHeader();
  report_resources_impl(vs, signature.vsCombinedBRegisterMask, signature.vsCombinedTRegisterMask, signature.vsCombinedURegisterMask,
    signature.vsCombinedSRegisterMask, base_pipe->getVertexShaderSamplerCompareMask(), is_detailed_report);
  report_resources_impl(ps, psHeader.resourceUsageTable.bRegisterUseMask, psHeader.resourceUsageTable.tRegisterUseMask,
    psHeader.resourceUsageTable.uRegisterUseMask, psHeader.resourceUsageTable.sRegisterUseMask, psHeader.sRegisterCompareUseMask,
    is_detailed_report);
}
