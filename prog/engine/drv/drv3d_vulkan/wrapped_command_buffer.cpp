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
        auto &cmdPar = ((CmdAndParameter<BeginConditionalRenderingEXTParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<BeginConditionalRenderingEXTParameters>);
        Globals::VK::dev.vkCmdBeginConditionalRenderingEXT(cb, &cmdPar.conditionalRenderingBegin);
        break;
      }
      case EndConditionalRenderingEXTParameters::ID:
      {
        cmdPtr += sizeof(CmdAndParameter<EndConditionalRenderingEXTParameters>);
        Globals::VK::dev.vkCmdEndConditionalRenderingEXT(cb);
        break;
      }
#endif
      case BeginRenderPassParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<BeginRenderPassParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<BeginRenderPassParameters>);
        Globals::VK::dev.vkCmdBeginRenderPass(cb, &cmdPar.renderPassBegin, cmdPar.contents);
        break;
      }
      case NextSubpassParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<NextSubpassParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<NextSubpassParameters>);
        Globals::VK::dev.vkCmdNextSubpass(cb, cmdPar.contents);
        break;
      }
      case EndRenderPassParameters::ID:
      {
        cmdPtr += sizeof(CmdAndParameter<EndRenderPassParameters>);
        Globals::VK::dev.vkCmdEndRenderPass(cb);
        break;
      }
      case BindDescriptorSetsParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<BindDescriptorSetsParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<BindDescriptorSetsParameters>);
        Globals::VK::dev.vkCmdBindDescriptorSets(cb, cmdPar.pipelineBindPoint, cmdPar.layout, cmdPar.firstSet,
          cmdPar.descriptorSetCount, cmdPar.pDescriptorSets, cmdPar.dynamicOffsetCount, cmdPar.pDynamicOffsets);
        break;
      }
      case BindIndexBufferParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<BindIndexBufferParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<BindIndexBufferParameters>);
        Globals::VK::dev.vkCmdBindIndexBuffer(cb, cmdPar.buffer, cmdPar.offset, cmdPar.indexType);
        break;
      }
      case WriteTimestampParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<WriteTimestampParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<WriteTimestampParameters>);
        Globals::VK::dev.vkCmdWriteTimestamp(cb, cmdPar.pipelineStage, cmdPar.queryPool, cmdPar.query);
        break;
      }
#if VK_AMD_buffer_marker
      case WriteBufferMarkerAMDParameters::ID:
      {
        auto &cmdPar = ((CmdAndParameter<WriteBufferMarkerAMDParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<WriteBufferMarkerAMDParameters>);
        Globals::VK::dev.vkCmdWriteBufferMarkerAMD(cb, cmdPar.pipelineStage, cmdPar.dstBuffer, cmdPar.dstOffset, cmdPar.marker);
        break;
      }
#endif
#if VK_NV_device_diagnostic_checkpoints
      case SetCheckpointNVParameters::ID:
      {
        auto &cmdPar = ((CmdAndParameter<SetCheckpointNVParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<SetCheckpointNVParameters>);
        Globals::VK::dev.vkCmdSetCheckpointNV(cb, cmdPar.pCheckpointMarker);
        break;
      }
#endif
      case SetScissorParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<SetScissorParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<SetScissorParameters>);
        Globals::VK::dev.vkCmdSetScissor(cb, cmdPar.firstScissor, cmdPar.scissorCount, cmdPar.pScissors);
        break;
      }
      case SetDepthBiasParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<SetDepthBiasParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<SetDepthBiasParameters>);
        Globals::VK::dev.vkCmdSetDepthBias(cb, cmdPar.depthBiasConstantFactor, cmdPar.depthBiasClamp, cmdPar.depthBiasSlopeFactor);
        break;
      }
      case SetDepthBoundsParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<SetDepthBoundsParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<SetDepthBoundsParameters>);
        Globals::VK::dev.vkCmdSetDepthBounds(cb, cmdPar.minDepthBounds, cmdPar.maxDepthBounds);
        break;
      }
      case SetStencilReferenceParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<SetStencilReferenceParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<SetStencilReferenceParameters>);
        Globals::VK::dev.vkCmdSetStencilReference(cb, cmdPar.faceMask, cmdPar.reference);
        break;
      }
      case SetStencilWriteMaskParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<SetStencilWriteMaskParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<SetStencilWriteMaskParameters>);
        Globals::VK::dev.vkCmdSetStencilWriteMask(cb, cmdPar.faceMask, cmdPar.writeMask);
        break;
      }
      case SetStencilCompareMaskParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<SetStencilCompareMaskParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<SetStencilCompareMaskParameters>);
        Globals::VK::dev.vkCmdSetStencilCompareMask(cb, cmdPar.faceMask, cmdPar.compareMask);
        break;
      }
      case SetViewportParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<SetViewportParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<SetViewportParameters>);
        Globals::VK::dev.vkCmdSetViewport(cb, cmdPar.firstViewport, cmdPar.viewportCount, cmdPar.pViewports);
        break;
      }
      case BindVertexBuffersParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<BindVertexBuffersParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<BindVertexBuffersParameters>);
        Globals::VK::dev.vkCmdBindVertexBuffers(cb, cmdPar.firstBinding, cmdPar.bindingCount, cmdPar.pBuffers, cmdPar.pOffsets);
        break;
      }
#if VK_EXT_debug_marker
      case DebugMarkerInsertEXTParameters::ID:
      {
        auto &cmdPar = ((CmdAndParameter<DebugMarkerInsertEXTParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<DebugMarkerInsertEXTParameters>);
        Globals::VK::dev.vkCmdDebugMarkerInsertEXT(cb, &cmdPar.markerInfo);
        break;
      }
      case DebugMarkerBeginEXTParameters::ID:
      {
        auto &cmdPar = ((CmdAndParameter<DebugMarkerBeginEXTParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<DebugMarkerBeginEXTParameters>);
        Globals::VK::dev.vkCmdDebugMarkerBeginEXT(cb, &cmdPar.markerInfo);
        break;
      }
      case DebugMarkerEndEXTParameters::ID:
      {
        cmdPtr += sizeof(CmdAndParameter<DebugMarkerEndEXTParameters>);
        Globals::VK::dev.vkCmdDebugMarkerEndEXT(cb);
        break;
      }
#endif
#if VK_EXT_debug_utils
      case InsertDebugUtilsLabelEXTParameters::ID:
      {
        auto &cmdPar = ((CmdAndParameter<InsertDebugUtilsLabelEXTParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<InsertDebugUtilsLabelEXTParameters>);
        Globals::VK::inst.vkCmdInsertDebugUtilsLabelEXT(cb, &cmdPar.labelInfo);
        break;
      }
      case BeginDebugUtilsLabelEXTParameters::ID:
      {
        auto &cmdPar = ((CmdAndParameter<BeginDebugUtilsLabelEXTParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<BeginDebugUtilsLabelEXTParameters>);
        Globals::VK::inst.vkCmdBeginDebugUtilsLabelEXT(cb, &cmdPar.labelInfo);
        break;
      }
      case EndDebugUtilsLabelEXTParameters::ID:
      {
        cmdPtr += sizeof(CmdAndParameter<EndDebugUtilsLabelEXTParameters>);
        Globals::VK::inst.vkCmdEndDebugUtilsLabelEXT(cb);
        break;
      }
#endif
      case BeginQueryParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<BeginQueryParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<BeginQueryParameters>);
        Globals::VK::dev.vkCmdBeginQuery(cb, cmdPar.queryPool, cmdPar.query, cmdPar.flags);
        break;
      }
      case EndQueryParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<EndQueryParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<EndQueryParameters>);
        Globals::VK::dev.vkCmdEndQuery(cb, cmdPar.queryPool, cmdPar.query);
        break;
      }
      case DrawIndirectParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<DrawIndirectParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<DrawIndirectParameters>);
        Globals::VK::dev.vkCmdDrawIndirect(cb, cmdPar.buffer, cmdPar.offset, cmdPar.drawCount, cmdPar.stride);
        break;
      }
      case DrawIndexedIndirectParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<DrawIndexedIndirectParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<DrawIndexedIndirectParameters>);
        Globals::VK::dev.vkCmdDrawIndexedIndirect(cb, cmdPar.buffer, cmdPar.offset, cmdPar.drawCount, cmdPar.stride);
        break;
      }
      case DrawParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<DrawParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<DrawParameters>);
        Globals::VK::dev.vkCmdDraw(cb, cmdPar.vertexCount, cmdPar.instanceCount, cmdPar.firstVertex, cmdPar.firstInstance);
        break;
      }
      case DrawIndexedParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<DrawIndexedParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<DrawIndexedParameters>);
        Globals::VK::dev.vkCmdDrawIndexed(cb, cmdPar.indexCount, cmdPar.instanceCount, cmdPar.firstIndex, cmdPar.vertexOffset,
          cmdPar.firstInstance);
        break;
      }
      case BindPipelineParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<BindPipelineParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<BindPipelineParameters>);
        Globals::VK::dev.vkCmdBindPipeline(cb, cmdPar.pipelineBindPoint, cmdPar.pipeline);
        break;
      }
      case SetBlendConstantsParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<SetBlendConstantsParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<SetBlendConstantsParameters>);
        Globals::VK::dev.vkCmdSetBlendConstants(cb, cmdPar.blendConstants);
        break;
      }
      case CopyBufferToImageParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<CopyBufferToImageParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<CopyBufferToImageParameters>);
        Globals::VK::dev.vkCmdCopyBufferToImage(cb, cmdPar.srcBuffer, cmdPar.dstImage, cmdPar.dstImageLayout, cmdPar.regionCount,
          cmdPar.pRegions);
        break;
      }
      case CopyBufferParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<CopyBufferParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<CopyBufferParameters>);
        Globals::VK::dev.vkCmdCopyBuffer(cb, cmdPar.srcBuffer, cmdPar.dstBuffer, cmdPar.regionCount, cmdPar.pRegions);
        break;
      }
      case BlitImageParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<BlitImageParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<BlitImageParameters>);
        Globals::VK::dev.vkCmdBlitImage(cb, cmdPar.srcImage, cmdPar.srcImageLayout, cmdPar.dstImage, cmdPar.dstImageLayout,
          cmdPar.regionCount, cmdPar.pRegions, cmdPar.filter);
        break;
      }
      case ResolveImageParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<ResolveImageParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<ResolveImageParameters>);
        Globals::VK::dev.vkCmdResolveImage(cb, cmdPar.srcImage, cmdPar.srcImageLayout, cmdPar.dstImage, cmdPar.dstImageLayout,
          cmdPar.regionCount, cmdPar.pRegions);
        break;
      }
      case CopyImageToBufferParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<CopyImageToBufferParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<CopyImageToBufferParameters>);
        Globals::VK::dev.vkCmdCopyImageToBuffer(cb, cmdPar.srcImage, cmdPar.srcImageLayout, cmdPar.dstBuffer, cmdPar.regionCount,
          cmdPar.pRegions);
        break;
      }
      case PipelineBarrierParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<PipelineBarrierParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<PipelineBarrierParameters>);
        Globals::VK::dev.vkCmdPipelineBarrier(cb, cmdPar.srcStageMask, cmdPar.dstStageMask, cmdPar.dependencyFlags,
          cmdPar.memoryBarrierCount, cmdPar.pMemoryBarriers, cmdPar.bufferMemoryBarrierCount, cmdPar.pBufferMemoryBarriers,
          cmdPar.imageMemoryBarrierCount, cmdPar.pImageMemoryBarriers);
        break;
      }
#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
      case CopyAccelerationStructureKHRParameters::ID:
      {
        auto &cmdPar = ((CmdAndParameter<CopyAccelerationStructureKHRParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<CopyAccelerationStructureKHRParameters>);
        Globals::VK::dev.vkCmdCopyAccelerationStructureKHR(cb, &cmdPar.info);
        break;
      }
      case BuildAccelerationStructuresKHRParameters::ID:
      {
        auto &cmdPar = ((CmdAndParameter<BuildAccelerationStructuresKHRParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<BuildAccelerationStructuresKHRParameters>);
        Globals::VK::dev.vkCmdBuildAccelerationStructuresKHR(cb, cmdPar.infoCount, cmdPar.pInfos, cmdPar.ppBuildRangeInfos);
        break;
      }
      case WriteAccelerationStructuresPropertiesKHRParameters::ID:
      {
        auto &cmdPar = ((CmdAndParameter<WriteAccelerationStructuresPropertiesKHRParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<WriteAccelerationStructuresPropertiesKHRParameters>);
        Globals::VK::dev.vkCmdWriteAccelerationStructuresPropertiesKHR(cb, cmdPar.accelerationStructureCount,
          cmdPar.pAccelerationStructures, cmdPar.queryType, cmdPar.queryPool, cmdPar.firstQuery);
        break;
      }
#endif
      case CopyQueryPoolResultsParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<CopyQueryPoolResultsParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<CopyQueryPoolResultsParameters>);
        Globals::VK::dev.vkCmdCopyQueryPoolResults(cb, cmdPar.queryPool, cmdPar.firstQuery, cmdPar.queryCount, cmdPar.dstBuffer,
          cmdPar.dstOffset, cmdPar.stride, cmdPar.flags);
        break;
      }
      case DispatchParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<DispatchParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<DispatchParameters>);
        Globals::VK::dev.vkCmdDispatch(cb, cmdPar.groupCountX, cmdPar.groupCountY, cmdPar.groupCountZ);
        break;
      }
      case DispatchIndirectParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<DispatchIndirectParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<DispatchIndirectParameters>);
        Globals::VK::dev.vkCmdDispatchIndirect(cb, cmdPar.buffer, cmdPar.offset);
        break;
      }
      case FillBufferParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<FillBufferParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<FillBufferParameters>);
        Globals::VK::dev.vkCmdFillBuffer(cb, cmdPar.dstBuffer, cmdPar.dstOffset, cmdPar.size, cmdPar.data);
        break;
      }
      case ClearDepthStencilImageParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<ClearDepthStencilImageParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<ClearDepthStencilImageParameters>);
        Globals::VK::dev.vkCmdClearDepthStencilImage(cb, cmdPar.image, cmdPar.imageLayout, &cmdPar.depthStencil, cmdPar.rangeCount,
          cmdPar.pRanges);
        break;
      }
      case ClearColorImageParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<ClearColorImageParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<ClearColorImageParameters>);
        Globals::VK::dev.vkCmdClearColorImage(cb, cmdPar.image, cmdPar.imageLayout, &cmdPar.color, cmdPar.rangeCount, cmdPar.pRanges);
        break;
      }
      case CopyImageParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<CopyImageParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<CopyImageParameters>);
        Globals::VK::dev.vkCmdCopyImage(cb, cmdPar.srcImage, cmdPar.srcImageLayout, cmdPar.dstImage, cmdPar.dstImageLayout,
          cmdPar.regionCount, cmdPar.pRegions);
        break;
      }
      case ResetQueryPoolParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<ResetQueryPoolParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<ResetQueryPoolParameters>);
        Globals::VK::dev.vkCmdResetQueryPool(cb, cmdPar.queryPool, cmdPar.firstQuery, cmdPar.queryCount);
        break;
      }
      case PushConstantsParameters::ID:
      {

        auto &cmdPar = ((CmdAndParameter<PushConstantsParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<PushConstantsParameters>);
        Globals::VK::dev.vkCmdPushConstants(cb, cmdPar.layout, cmdPar.stageFlags, cmdPar.offset, cmdPar.size, cmdPar.pValues);
        break;
      }
      case SetEventParameters::ID:
      {
        auto &cmdPar = ((CmdAndParameter<SetEventParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<SetEventParameters>);
        Globals::VK::dev.vkCmdSetEvent(cb, cmdPar.event, cmdPar.stageMask);
        break;
      }
      case ResetEventParameters::ID:
      {
        auto &cmdPar = ((CmdAndParameter<ResetEventParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<ResetEventParameters>);
        Globals::VK::dev.vkCmdResetEvent(cb, cmdPar.event, cmdPar.stageMask);
        break;
      }
      case WaitEventsParameters::ID:
      {
        auto &cmdPar = ((CmdAndParameter<WaitEventsParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<WaitEventsParameters>);
        Globals::VK::dev.vkCmdWaitEvents(cb, cmdPar.eventCount, cmdPar.pEvents, cmdPar.srcStageMask, cmdPar.dstStageMask,
          cmdPar.memoryBarrierCount, cmdPar.pMemoryBarriers, cmdPar.bufferMemoryBarrierCount, cmdPar.pBufferMemoryBarriers,
          cmdPar.imageMemoryBarrierCount, cmdPar.pImageMemoryBarriers);
        break;
      }
#if VK_KHR_fragment_shading_rate
      case SetFragmentShadingRateKHRParameters::ID:
      {
        auto &cmdPar = ((CmdAndParameter<SetFragmentShadingRateKHRParameters> *)cmdPtr)->param;
        cmdPtr += sizeof(CmdAndParameter<SetFragmentShadingRateKHRParameters>);
        Globals::VK::dev.vkCmdSetFragmentShadingRateKHR(cb, &cmdPar.fragmentSize, cmdPar.combinerOps);
        break;
      }
#endif
      default: G_ASSERTF(0, "vulkan: unrecognized command %u at reorder flush", *((CmdID *)cmdPtr)); break;
    }
  }
  endLoop(); //-V1020 End and Begin methods here is not used as scope like calls
}
