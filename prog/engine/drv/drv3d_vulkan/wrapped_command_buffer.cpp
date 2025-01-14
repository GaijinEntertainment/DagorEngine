// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "wrapped_command_buffer.h"

using namespace drv3d_vulkan;

size_t WrappedCommandBuffer::getMemoryUsedMax()
{
  size_t ret = 0;
#define CALC_VEC_BYTES(Name) (Name.capacity() * sizeof(decltype(Name)::value_type))
  ret += CALC_VEC_BYTES(cmdMem);
#undef CALC_VEC_BYTES
  ret += parMem.size;
  return ret;
}

void WrappedCommandBuffer::beginLoop() {}

void WrappedCommandBuffer::endLoop()
{
  cmdMem.clear();
  parMem.clear();
}

void WrappedCommandBuffer::flush()
{
  beginLoop();
  uint8_t *cmdPtr = cmdMem.data();
  while (cmdPtr < cmdMem.end())
  {
    switch (*((CmdID *)cmdPtr))
    {
#if VK_EXT_conditional_rendering
      case BeginConditionalRenderingEXTParameters::ID:
      {
        auto &cmdPar = ((CmdAndParamater<BeginConditionalRenderingEXTParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<BeginConditionalRenderingEXTParameters>);
        Globals::VK::dev.vkCmdBeginConditionalRenderingEXT(cb, &cmdPar.conditionalRenderingBegin);
        break;
      }
      case EndConditionalRenderingEXTParameters::ID:
      {
        cmdPtr += sizeof(CmdAndParamater<EndConditionalRenderingEXTParameters>);
        Globals::VK::dev.vkCmdEndConditionalRenderingEXT(cb);
        break;
      }
#endif
      case BeginRenderPassParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<BeginRenderPassParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<BeginRenderPassParameters>);
        Globals::VK::dev.vkCmdBeginRenderPass(cb, &cmdPar.renderPassBegin, cmdPar.contents);
        break;
      }
      case NextSubpassParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<NextSubpassParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<NextSubpassParameters>);
        Globals::VK::dev.vkCmdNextSubpass(cb, cmdPar.contents);
        break;
      }
      case EndRenderPassParameters::ID:
      {
        cmdPtr += sizeof(CmdAndParamater<EndRenderPassParameters>);
        Globals::VK::dev.vkCmdEndRenderPass(cb);
        break;
      }
      case BindDescriptorSetsParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<BindDescriptorSetsParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<BindDescriptorSetsParameters>);
        Globals::VK::dev.vkCmdBindDescriptorSets(cb, cmdPar.pipelineBindPoint, cmdPar.layout, cmdPar.firstSet,
          cmdPar.descriptorSetCount, cmdPar.pDescriptorSets, cmdPar.dynamicOffsetCount, cmdPar.pDynamicOffsets);
        break;
      }
      case BindIndexBufferParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<BindIndexBufferParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<BindIndexBufferParameters>);
        Globals::VK::dev.vkCmdBindIndexBuffer(cb, cmdPar.buffer, cmdPar.offset, cmdPar.indexType);
        break;
      }
      case WriteTimestampParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<WriteTimestampParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<WriteTimestampParameters>);
        Globals::VK::dev.vkCmdWriteTimestamp(cb, cmdPar.pipelineStage, cmdPar.queryPool, cmdPar.query);
        break;
      }
#if VK_AMD_buffer_marker
      case WriteBufferMarkerAMDParameters::ID:
      {
        auto &cmdPar = ((CmdAndParamater<WriteBufferMarkerAMDParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<WriteBufferMarkerAMDParameters>);
        Globals::VK::dev.vkCmdWriteBufferMarkerAMD(cb, cmdPar.pipelineStage, cmdPar.dstBuffer, cmdPar.dstOffset, cmdPar.marker);
        break;
      }
#endif
#if VK_NV_device_diagnostic_checkpoints
      case SetCheckpointNVParameters::ID:
      {
        auto &cmdPar = ((CmdAndParamater<SetCheckpointNVParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<SetCheckpointNVParameters>);
        Globals::VK::dev.vkCmdSetCheckpointNV(cb, cmdPar.pCheckpointMarker);
        break;
      }
#endif
      case SetScissorParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<SetScissorParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<SetScissorParameters>);
        Globals::VK::dev.vkCmdSetScissor(cb, cmdPar.firstScissor, cmdPar.scissorCount, cmdPar.pScissors);
        break;
      }
      case SetDepthBiasParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<SetDepthBiasParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<SetDepthBiasParameters>);
        Globals::VK::dev.vkCmdSetDepthBias(cb, cmdPar.depthBiasConstantFactor, cmdPar.depthBiasClamp, cmdPar.depthBiasSlopeFactor);
        break;
      }
      case SetDepthBoundsParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<SetDepthBoundsParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<SetDepthBoundsParameters>);
        Globals::VK::dev.vkCmdSetDepthBounds(cb, cmdPar.minDepthBounds, cmdPar.maxDepthBounds);
        break;
      }
      case SetStencilReferenceParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<SetStencilReferenceParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<SetStencilReferenceParameters>);
        Globals::VK::dev.vkCmdSetStencilReference(cb, cmdPar.faceMask, cmdPar.reference);
        break;
      }
      case SetStencilWriteMaskParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<SetStencilWriteMaskParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<SetStencilWriteMaskParameters>);
        Globals::VK::dev.vkCmdSetStencilWriteMask(cb, cmdPar.faceMask, cmdPar.writeMask);
        break;
      }
      case SetStencilCompareMaskParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<SetStencilCompareMaskParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<SetStencilCompareMaskParameters>);
        Globals::VK::dev.vkCmdSetStencilCompareMask(cb, cmdPar.faceMask, cmdPar.compareMask);
        break;
      }
      case SetViewportParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<SetViewportParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<SetViewportParameters>);
        Globals::VK::dev.vkCmdSetViewport(cb, cmdPar.firstViewport, cmdPar.viewportCount, cmdPar.pViewports);
        break;
      }
      case BindVertexBuffersParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<BindVertexBuffersParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<BindVertexBuffersParameters>);
        Globals::VK::dev.vkCmdBindVertexBuffers(cb, cmdPar.firstBinding, cmdPar.bindingCount, cmdPar.pBuffers, cmdPar.pOffsets);
        break;
      }
#if VK_EXT_debug_marker
      case DebugMarkerInsertEXTParameters::ID:
      {
        auto &cmdPar = ((CmdAndParamater<DebugMarkerInsertEXTParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<DebugMarkerInsertEXTParameters>);
        Globals::VK::dev.vkCmdDebugMarkerInsertEXT(cb, &cmdPar.markerInfo);
        break;
      }
      case DebugMarkerBeginEXTParameters::ID:
      {
        auto &cmdPar = ((CmdAndParamater<DebugMarkerBeginEXTParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<DebugMarkerBeginEXTParameters>);
        Globals::VK::dev.vkCmdDebugMarkerBeginEXT(cb, &cmdPar.markerInfo);
        break;
      }
      case DebugMarkerEndEXTParameters::ID:
      {
        cmdPtr += sizeof(CmdAndParamater<DebugMarkerEndEXTParameters>);
        Globals::VK::dev.vkCmdDebugMarkerEndEXT(cb);
        break;
      }
#endif
#if VK_EXT_debug_utils
      case InsertDebugUtilsLabelEXTParameters::ID:
      {
        auto &cmdPar = ((CmdAndParamater<InsertDebugUtilsLabelEXTParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<InsertDebugUtilsLabelEXTParameters>);
        Globals::VK::inst.vkCmdInsertDebugUtilsLabelEXT(cb, &cmdPar.labelInfo);
        break;
      }
      case BeginDebugUtilsLabelEXTParameters::ID:
      {
        auto &cmdPar = ((CmdAndParamater<BeginDebugUtilsLabelEXTParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<BeginDebugUtilsLabelEXTParameters>);
        Globals::VK::inst.vkCmdBeginDebugUtilsLabelEXT(cb, &cmdPar.labelInfo);
        break;
      }
      case EndDebugUtilsLabelEXTParameters::ID:
      {
        cmdPtr += sizeof(CmdAndParamater<EndDebugUtilsLabelEXTParameters>);
        Globals::VK::inst.vkCmdEndDebugUtilsLabelEXT(cb);
        break;
      }
#endif
      case BeginQueryParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<BeginQueryParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<BeginQueryParameters>);
        Globals::VK::dev.vkCmdBeginQuery(cb, cmdPar.queryPool, cmdPar.query, cmdPar.flags);
        break;
      }
      case EndQueryParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<EndQueryParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<EndQueryParameters>);
        Globals::VK::dev.vkCmdEndQuery(cb, cmdPar.queryPool, cmdPar.query);
        break;
      }
      case DrawIndirectParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<DrawIndirectParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<DrawIndirectParameters>);
        Globals::VK::dev.vkCmdDrawIndirect(cb, cmdPar.buffer, cmdPar.offset, cmdPar.drawCount, cmdPar.stride);
        break;
      }
      case DrawIndexedIndirectParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<DrawIndexedIndirectParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<DrawIndexedIndirectParameters>);
        Globals::VK::dev.vkCmdDrawIndexedIndirect(cb, cmdPar.buffer, cmdPar.offset, cmdPar.drawCount, cmdPar.stride);
        break;
      }
      case DrawParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<DrawParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<DrawParameters>);
        Globals::VK::dev.vkCmdDraw(cb, cmdPar.vertexCount, cmdPar.instanceCount, cmdPar.firstVertex, cmdPar.firstInstance);
        break;
      }
      case DrawIndexedParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<DrawIndexedParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<DrawIndexedParameters>);
        Globals::VK::dev.vkCmdDrawIndexed(cb, cmdPar.indexCount, cmdPar.instanceCount, cmdPar.firstIndex, cmdPar.vertexOffset,
          cmdPar.firstInstance);
        break;
      }
      case BindPipelineParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<BindPipelineParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<BindPipelineParameters>);
        Globals::VK::dev.vkCmdBindPipeline(cb, cmdPar.pipelineBindPoint, cmdPar.pipeline);
        break;
      }
      case SetBlendConstantsParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<SetBlendConstantsParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<SetBlendConstantsParameters>);
        Globals::VK::dev.vkCmdSetBlendConstants(cb, cmdPar.blendConstants);
        break;
      }
      case CopyBufferToImageParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<CopyBufferToImageParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<CopyBufferToImageParameters>);
        Globals::VK::dev.vkCmdCopyBufferToImage(cb, cmdPar.srcBuffer, cmdPar.dstImage, cmdPar.dstImageLayout, cmdPar.regionCount,
          cmdPar.pRegions);
        break;
      }
      case CopyBufferParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<CopyBufferParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<CopyBufferParameters>);
        Globals::VK::dev.vkCmdCopyBuffer(cb, cmdPar.srcBuffer, cmdPar.dstBuffer, cmdPar.regionCount, cmdPar.pRegions);
        break;
      }
      case BlitImageParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<BlitImageParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<BlitImageParameters>);
        Globals::VK::dev.vkCmdBlitImage(cb, cmdPar.srcImage, cmdPar.srcImageLayout, cmdPar.dstImage, cmdPar.dstImageLayout,
          cmdPar.regionCount, cmdPar.pRegions, cmdPar.filter);
        break;
      }
      case ResolveImageParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<ResolveImageParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<ResolveImageParameters>);
        Globals::VK::dev.vkCmdResolveImage(cb, cmdPar.srcImage, cmdPar.srcImageLayout, cmdPar.dstImage, cmdPar.dstImageLayout,
          cmdPar.regionCount, cmdPar.pRegions);
        break;
      }
      case CopyImageToBufferParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<CopyImageToBufferParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<CopyImageToBufferParameters>);
        Globals::VK::dev.vkCmdCopyImageToBuffer(cb, cmdPar.srcImage, cmdPar.srcImageLayout, cmdPar.dstBuffer, cmdPar.regionCount,
          cmdPar.pRegions);
        break;
      }
      case PipelineBarrierParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<PipelineBarrierParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<PipelineBarrierParameters>);
        Globals::VK::dev.vkCmdPipelineBarrier(cb, cmdPar.srcStageMask, cmdPar.dstStageMask, cmdPar.dependencyFlags,
          cmdPar.memoryBarrierCount, cmdPar.pMemoryBarriers, cmdPar.bufferMemoryBarrierCount, cmdPar.pBufferMemoryBarriers,
          cmdPar.imageMemoryBarrierCount, cmdPar.pImageMemoryBarriers);
        break;
      }
#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
      case CopyAccelerationStructureKHRParameters::ID:
      {
        auto &cmdPar = ((CmdAndParamater<CopyAccelerationStructureKHRParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<CopyAccelerationStructureKHRParameters>);
        Globals::VK::dev.vkCmdCopyAccelerationStructureKHR(cb, &cmdPar.info);
        break;
      }
      case BuildAccelerationStructuresKHRParameters::ID:
      {
        auto &cmdPar = ((CmdAndParamater<BuildAccelerationStructuresKHRParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<BuildAccelerationStructuresKHRParameters>);
        Globals::VK::dev.vkCmdBuildAccelerationStructuresKHR(cb, cmdPar.infoCount, cmdPar.pInfos, cmdPar.ppBuildRangeInfos);
        break;
      }
      case WriteAccelerationStructuresPropertiesKHRParameters::ID:
      {
        auto &cmdPar = ((CmdAndParamater<WriteAccelerationStructuresPropertiesKHRParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<WriteAccelerationStructuresPropertiesKHRParameters>);
        Globals::VK::dev.vkCmdWriteAccelerationStructuresPropertiesKHR(cb, cmdPar.accelerationStructureCount,
          cmdPar.pAccelerationStructures, cmdPar.queryType, cmdPar.queryPool, cmdPar.firstQuery);
        break;
      }
#endif
      case CopyQueryPoolResultsParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<CopyQueryPoolResultsParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<CopyQueryPoolResultsParameters>);
        Globals::VK::dev.vkCmdCopyQueryPoolResults(cb, cmdPar.queryPool, cmdPar.firstQuery, cmdPar.queryCount, cmdPar.dstBuffer,
          cmdPar.dstOffset, cmdPar.stride, cmdPar.flags);
        break;
      }
      case DispatchParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<DispatchParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<DispatchParameters>);
        Globals::VK::dev.vkCmdDispatch(cb, cmdPar.groupCountX, cmdPar.groupCountY, cmdPar.groupCountZ);
        break;
      }
      case DispatchIndirectParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<DispatchIndirectParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<DispatchIndirectParameters>);
        Globals::VK::dev.vkCmdDispatchIndirect(cb, cmdPar.buffer, cmdPar.offset);
        break;
      }
      case FillBufferParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<FillBufferParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<FillBufferParameters>);
        Globals::VK::dev.vkCmdFillBuffer(cb, cmdPar.dstBuffer, cmdPar.dstOffset, cmdPar.size, cmdPar.data);
        break;
      }
      case ClearDepthStencilImageParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<ClearDepthStencilImageParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<ClearDepthStencilImageParameters>);
        Globals::VK::dev.vkCmdClearDepthStencilImage(cb, cmdPar.image, cmdPar.imageLayout, &cmdPar.depthStencil, cmdPar.rangeCount,
          cmdPar.pRanges);
        break;
      }
      case ClearColorImageParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<ClearColorImageParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<ClearColorImageParameters>);
        Globals::VK::dev.vkCmdClearColorImage(cb, cmdPar.image, cmdPar.imageLayout, &cmdPar.color, cmdPar.rangeCount, cmdPar.pRanges);
        break;
      }
      case CopyImageParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<CopyImageParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<CopyImageParameters>);
        Globals::VK::dev.vkCmdCopyImage(cb, cmdPar.srcImage, cmdPar.srcImageLayout, cmdPar.dstImage, cmdPar.dstImageLayout,
          cmdPar.regionCount, cmdPar.pRegions);
        break;
      }
      case ResetQueryPoolParameters::ID:
      {

        auto &cmdPar = ((CmdAndParamater<ResetQueryPoolParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParamater<ResetQueryPoolParameters>);
        Globals::VK::dev.vkCmdResetQueryPool(cb, cmdPar.queryPool, cmdPar.firstQuery, cmdPar.queryCount);
        break;
      }
      default: G_ASSERTF(0, "vulkan: unrecognized command %u at reorder flush", *((CmdID *)cmdPtr)); break;
    }
  }
  endLoop(); //-V1020 End and Begin methods here is not used as scope like calls
}
