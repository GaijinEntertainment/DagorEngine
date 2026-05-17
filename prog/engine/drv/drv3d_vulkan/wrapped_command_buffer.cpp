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

#define FILL_CMD_PAR(parameters_type)                                                 \
  CmdAndParameter<parameters_type> cmdAndParameter;                                   \
  memcpy((void *)&cmdAndParameter, cmdPtr, sizeof(CmdAndParameter<parameters_type>)); \
  cmdPtr += sizeof(CmdAndParameter<parameters_type>);                                 \
  parameters_type &cmdPar = cmdAndParameter.param;

#define FILL_CMD_PAR_EMPTY(parameters_type)                                           \
  CmdAndParameter<parameters_type> cmdAndParameter;                                   \
  memcpy((void *)&cmdAndParameter, cmdPtr, sizeof(CmdAndParameter<parameters_type>)); \
  cmdPtr += sizeof(CmdAndParameter<parameters_type>);

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
        FILL_CMD_PAR(BeginConditionalRenderingEXTParameters);
        Globals::VK::dev.vkCmdBeginConditionalRenderingEXT(cb, &cmdPar.conditionalRenderingBegin);
        break;
      }
      case EndConditionalRenderingEXTParameters::ID:
      {
        FILL_CMD_PAR_EMPTY(EndConditionalRenderingEXTParameters);
        Globals::VK::dev.vkCmdEndConditionalRenderingEXT(cb);
        break;
      }
#endif
      case BeginRenderPassParameters::ID:
      {
        FILL_CMD_PAR(BeginRenderPassParameters);
        Globals::VK::dev.vkCmdBeginRenderPass(cb, &cmdPar.renderPassBegin, cmdPar.contents);
        break;
      }
      case NextSubpassParameters::ID:
      {
        FILL_CMD_PAR(NextSubpassParameters);
        Globals::VK::dev.vkCmdNextSubpass(cb, cmdPar.contents);
        break;
      }
      case EndRenderPassParameters::ID:
      {
        FILL_CMD_PAR_EMPTY(EndRenderPassParameters);
        Globals::VK::dev.vkCmdEndRenderPass(cb);
        break;
      }
      case BindDescriptorSetsParameters::ID:
      {
        FILL_CMD_PAR(BindDescriptorSetsParameters);
        Globals::VK::dev.vkCmdBindDescriptorSets(cb, cmdPar.pipelineBindPoint, cmdPar.layout, cmdPar.firstSet,
          cmdPar.descriptorSetCount, cmdPar.pDescriptorSets, cmdPar.dynamicOffsetCount, cmdPar.pDynamicOffsets);
        break;
      }
      case BindIndexBufferParameters::ID:
      {
        FILL_CMD_PAR(BindIndexBufferParameters);
        Globals::VK::dev.vkCmdBindIndexBuffer(cb, cmdPar.buffer, cmdPar.offset, cmdPar.indexType);
        break;
      }
      case WriteTimestampParameters::ID:
      {
        FILL_CMD_PAR(WriteTimestampParameters);
        Globals::VK::dev.vkCmdWriteTimestamp(cb, cmdPar.pipelineStage, cmdPar.queryPool, cmdPar.query);
        break;
      }
#if VK_AMD_buffer_marker
      case WriteBufferMarkerAMDParameters::ID:
      {
        FILL_CMD_PAR(WriteBufferMarkerAMDParameters);
        Globals::VK::dev.vkCmdWriteBufferMarkerAMD(cb, cmdPar.pipelineStage, cmdPar.dstBuffer, cmdPar.dstOffset, cmdPar.marker);
        break;
      }
#endif
#if VK_NV_device_diagnostic_checkpoints
      case SetCheckpointNVParameters::ID:
      {
        FILL_CMD_PAR(SetCheckpointNVParameters);
        Globals::VK::dev.vkCmdSetCheckpointNV(cb, cmdPar.pCheckpointMarker);
        break;
      }
#endif
      case SetScissorParameters::ID:
      {
        FILL_CMD_PAR(SetScissorParameters);
        Globals::VK::dev.vkCmdSetScissor(cb, cmdPar.firstScissor, cmdPar.scissorCount, cmdPar.pScissors);
        break;
      }
      case SetDepthBiasParameters::ID:
      {
        FILL_CMD_PAR(SetDepthBiasParameters);
        Globals::VK::dev.vkCmdSetDepthBias(cb, cmdPar.depthBiasConstantFactor, cmdPar.depthBiasClamp, cmdPar.depthBiasSlopeFactor);
        break;
      }
      case SetDepthBoundsParameters::ID:
      {
        FILL_CMD_PAR(SetDepthBoundsParameters);
        Globals::VK::dev.vkCmdSetDepthBounds(cb, cmdPar.minDepthBounds, cmdPar.maxDepthBounds);
        break;
      }
      case SetStencilReferenceParameters::ID:
      {
        FILL_CMD_PAR(SetStencilReferenceParameters);
        Globals::VK::dev.vkCmdSetStencilReference(cb, cmdPar.faceMask, cmdPar.reference);
        break;
      }
      case SetStencilWriteMaskParameters::ID:
      {
        FILL_CMD_PAR(SetStencilWriteMaskParameters);
        Globals::VK::dev.vkCmdSetStencilWriteMask(cb, cmdPar.faceMask, cmdPar.writeMask);
        break;
      }
      case SetStencilCompareMaskParameters::ID:
      {
        FILL_CMD_PAR(SetStencilCompareMaskParameters);
        Globals::VK::dev.vkCmdSetStencilCompareMask(cb, cmdPar.faceMask, cmdPar.compareMask);
        break;
      }
      case SetViewportParameters::ID:
      {
        FILL_CMD_PAR(SetViewportParameters);
        Globals::VK::dev.vkCmdSetViewport(cb, cmdPar.firstViewport, cmdPar.viewportCount, cmdPar.pViewports);
        break;
      }
      case BindVertexBuffersParameters::ID:
      {
        FILL_CMD_PAR(BindVertexBuffersParameters);
        Globals::VK::dev.vkCmdBindVertexBuffers(cb, cmdPar.firstBinding, cmdPar.bindingCount, cmdPar.pBuffers, cmdPar.pOffsets);
        break;
      }
#if VK_EXT_debug_marker
      case DebugMarkerInsertEXTParameters::ID:
      {
        FILL_CMD_PAR(DebugMarkerInsertEXTParameters);
        Globals::VK::dev.vkCmdDebugMarkerInsertEXT(cb, &cmdPar.markerInfo);
        break;
      }
      case DebugMarkerBeginEXTParameters::ID:
      {
        FILL_CMD_PAR(DebugMarkerBeginEXTParameters);
        Globals::VK::dev.vkCmdDebugMarkerBeginEXT(cb, &cmdPar.markerInfo);
        break;
      }
      case DebugMarkerEndEXTParameters::ID:
      {
        FILL_CMD_PAR_EMPTY(DebugMarkerEndEXTParameters);
        Globals::VK::dev.vkCmdDebugMarkerEndEXT(cb);
        break;
      }
#endif
#if VK_EXT_debug_utils
      case InsertDebugUtilsLabelEXTParameters::ID:
      {
        FILL_CMD_PAR(InsertDebugUtilsLabelEXTParameters);
        Globals::VK::inst.vkCmdInsertDebugUtilsLabelEXT(cb, &cmdPar.labelInfo);
        break;
      }
      case BeginDebugUtilsLabelEXTParameters::ID:
      {
        FILL_CMD_PAR(BeginDebugUtilsLabelEXTParameters);
        Globals::VK::inst.vkCmdBeginDebugUtilsLabelEXT(cb, &cmdPar.labelInfo);
        break;
      }
      case EndDebugUtilsLabelEXTParameters::ID:
      {
        FILL_CMD_PAR_EMPTY(EndDebugUtilsLabelEXTParameters);
        Globals::VK::inst.vkCmdEndDebugUtilsLabelEXT(cb);
        break;
      }
#endif
      case BeginQueryParameters::ID:
      {
        FILL_CMD_PAR(BeginQueryParameters);
        Globals::VK::dev.vkCmdBeginQuery(cb, cmdPar.queryPool, cmdPar.query, cmdPar.flags);
        break;
      }
      case EndQueryParameters::ID:
      {
        FILL_CMD_PAR(EndQueryParameters);
        Globals::VK::dev.vkCmdEndQuery(cb, cmdPar.queryPool, cmdPar.query);
        break;
      }
      case DrawIndirectParameters::ID:
      {
        FILL_CMD_PAR(DrawIndirectParameters);
        Globals::VK::dev.vkCmdDrawIndirect(cb, cmdPar.buffer, cmdPar.offset, cmdPar.drawCount, cmdPar.stride);
        break;
      }
      case DrawIndexedIndirectParameters::ID:
      {
        FILL_CMD_PAR(DrawIndexedIndirectParameters);
        Globals::VK::dev.vkCmdDrawIndexedIndirect(cb, cmdPar.buffer, cmdPar.offset, cmdPar.drawCount, cmdPar.stride);
        break;
      }
      case DrawParameters::ID:
      {
        FILL_CMD_PAR(DrawParameters);
        Globals::VK::dev.vkCmdDraw(cb, cmdPar.vertexCount, cmdPar.instanceCount, cmdPar.firstVertex, cmdPar.firstInstance);
        break;
      }
      case DrawIndexedParameters::ID:
      {
        FILL_CMD_PAR(DrawIndexedParameters);
        Globals::VK::dev.vkCmdDrawIndexed(cb, cmdPar.indexCount, cmdPar.instanceCount, cmdPar.firstIndex, cmdPar.vertexOffset,
          cmdPar.firstInstance);
        break;
      }
      case BindPipelineParameters::ID:
      {
        FILL_CMD_PAR(BindPipelineParameters);
        Globals::VK::dev.vkCmdBindPipeline(cb, cmdPar.pipelineBindPoint, cmdPar.pipeline);
        break;
      }
      case SetBlendConstantsParameters::ID:
      {
        FILL_CMD_PAR(SetBlendConstantsParameters);
        Globals::VK::dev.vkCmdSetBlendConstants(cb, cmdPar.blendConstants);
        break;
      }
      case CopyBufferToImageParameters::ID:
      {
        FILL_CMD_PAR(CopyBufferToImageParameters);
        Globals::VK::dev.vkCmdCopyBufferToImage(cb, cmdPar.srcBuffer, cmdPar.dstImage, cmdPar.dstImageLayout, cmdPar.regionCount,
          cmdPar.pRegions);
        break;
      }
      case CopyBufferParameters::ID:
      {
        FILL_CMD_PAR(CopyBufferParameters);
        Globals::VK::dev.vkCmdCopyBuffer(cb, cmdPar.srcBuffer, cmdPar.dstBuffer, cmdPar.regionCount, cmdPar.pRegions);
        break;
      }
      case BlitImageParameters::ID:
      {
        FILL_CMD_PAR(BlitImageParameters);
        Globals::VK::dev.vkCmdBlitImage(cb, cmdPar.srcImage, cmdPar.srcImageLayout, cmdPar.dstImage, cmdPar.dstImageLayout,
          cmdPar.regionCount, cmdPar.pRegions, cmdPar.filter);
        break;
      }
      case ResolveImageParameters::ID:
      {
        FILL_CMD_PAR(ResolveImageParameters);
        Globals::VK::dev.vkCmdResolveImage(cb, cmdPar.srcImage, cmdPar.srcImageLayout, cmdPar.dstImage, cmdPar.dstImageLayout,
          cmdPar.regionCount, cmdPar.pRegions);
        break;
      }
      case CopyImageToBufferParameters::ID:
      {
        FILL_CMD_PAR(CopyImageToBufferParameters);
        Globals::VK::dev.vkCmdCopyImageToBuffer(cb, cmdPar.srcImage, cmdPar.srcImageLayout, cmdPar.dstBuffer, cmdPar.regionCount,
          cmdPar.pRegions);
        break;
      }
      case PipelineBarrierParameters::ID:
      {
        FILL_CMD_PAR(PipelineBarrierParameters);
        Globals::VK::dev.vkCmdPipelineBarrier(cb, cmdPar.srcStageMask, cmdPar.dstStageMask, cmdPar.dependencyFlags,
          cmdPar.memoryBarrierCount, cmdPar.pMemoryBarriers, cmdPar.bufferMemoryBarrierCount, cmdPar.pBufferMemoryBarriers,
          cmdPar.imageMemoryBarrierCount, cmdPar.pImageMemoryBarriers);
        break;
      }
#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
      case CopyAccelerationStructureKHRParameters::ID:
      {
        FILL_CMD_PAR(CopyAccelerationStructureKHRParameters);
        Globals::VK::dev.vkCmdCopyAccelerationStructureKHR(cb, &cmdPar.info);
        break;
      }
      case BuildAccelerationStructuresKHRParameters::ID:
      {
        FILL_CMD_PAR(BuildAccelerationStructuresKHRParameters);
        Globals::VK::dev.vkCmdBuildAccelerationStructuresKHR(cb, cmdPar.infoCount, cmdPar.pInfos, cmdPar.ppBuildRangeInfos);
        break;
      }
      case WriteAccelerationStructuresPropertiesKHRParameters::ID:
      {
        FILL_CMD_PAR(WriteAccelerationStructuresPropertiesKHRParameters);
        Globals::VK::dev.vkCmdWriteAccelerationStructuresPropertiesKHR(cb, cmdPar.accelerationStructureCount,
          cmdPar.pAccelerationStructures, cmdPar.queryType, cmdPar.queryPool, cmdPar.firstQuery);
        break;
      }
#endif
      case CopyQueryPoolResultsParameters::ID:
      {
        FILL_CMD_PAR(CopyQueryPoolResultsParameters);
        Globals::VK::dev.vkCmdCopyQueryPoolResults(cb, cmdPar.queryPool, cmdPar.firstQuery, cmdPar.queryCount, cmdPar.dstBuffer,
          cmdPar.dstOffset, cmdPar.stride, cmdPar.flags);
        break;
      }
      case DispatchParameters::ID:
      {
        FILL_CMD_PAR(DispatchParameters);
        Globals::VK::dev.vkCmdDispatch(cb, cmdPar.groupCountX, cmdPar.groupCountY, cmdPar.groupCountZ);
        break;
      }
      case DispatchIndirectParameters::ID:
      {
        FILL_CMD_PAR(DispatchIndirectParameters);
        Globals::VK::dev.vkCmdDispatchIndirect(cb, cmdPar.buffer, cmdPar.offset);
        break;
      }
      case FillBufferParameters::ID:
      {
        FILL_CMD_PAR(FillBufferParameters);
        Globals::VK::dev.vkCmdFillBuffer(cb, cmdPar.dstBuffer, cmdPar.dstOffset, cmdPar.size, cmdPar.data);
        break;
      }
      case ClearDepthStencilImageParameters::ID:
      {
        FILL_CMD_PAR(ClearDepthStencilImageParameters);
        Globals::VK::dev.vkCmdClearDepthStencilImage(cb, cmdPar.image, cmdPar.imageLayout, &cmdPar.depthStencil, cmdPar.rangeCount,
          cmdPar.pRanges);
        break;
      }
      case ClearColorImageParameters::ID:
      {
        FILL_CMD_PAR(ClearColorImageParameters);
        Globals::VK::dev.vkCmdClearColorImage(cb, cmdPar.image, cmdPar.imageLayout, &cmdPar.color, cmdPar.rangeCount, cmdPar.pRanges);
        break;
      }
      case CopyImageParameters::ID:
      {
        FILL_CMD_PAR(CopyImageParameters);
        Globals::VK::dev.vkCmdCopyImage(cb, cmdPar.srcImage, cmdPar.srcImageLayout, cmdPar.dstImage, cmdPar.dstImageLayout,
          cmdPar.regionCount, cmdPar.pRegions);
        break;
      }
      case ResetQueryPoolParameters::ID:
      {
        FILL_CMD_PAR(ResetQueryPoolParameters);
        Globals::VK::dev.vkCmdResetQueryPool(cb, cmdPar.queryPool, cmdPar.firstQuery, cmdPar.queryCount);
        break;
      }
      case PushConstantsParameters::ID:
      {
        FILL_CMD_PAR(PushConstantsParameters);
        Globals::VK::dev.vkCmdPushConstants(cb, cmdPar.layout, cmdPar.stageFlags, cmdPar.offset, cmdPar.size, cmdPar.pValues);
        break;
      }
      case SetEventParameters::ID:
      {
        FILL_CMD_PAR(SetEventParameters);
        Globals::VK::dev.vkCmdSetEvent(cb, cmdPar.event, cmdPar.stageMask);
        break;
      }
      case ResetEventParameters::ID:
      {
        FILL_CMD_PAR(ResetEventParameters);
        Globals::VK::dev.vkCmdResetEvent(cb, cmdPar.event, cmdPar.stageMask);
        break;
      }
      case WaitEventsParameters::ID:
      {
        FILL_CMD_PAR(WaitEventsParameters);
        Globals::VK::dev.vkCmdWaitEvents(cb, cmdPar.eventCount, cmdPar.pEvents, cmdPar.srcStageMask, cmdPar.dstStageMask,
          cmdPar.memoryBarrierCount, cmdPar.pMemoryBarriers, cmdPar.bufferMemoryBarrierCount, cmdPar.pBufferMemoryBarriers,
          cmdPar.imageMemoryBarrierCount, cmdPar.pImageMemoryBarriers);
        break;
      }
#if VK_KHR_fragment_shading_rate
      case SetFragmentShadingRateKHRParameters::ID:
      {
        FILL_CMD_PAR(SetFragmentShadingRateKHRParameters);
        Globals::VK::dev.vkCmdSetFragmentShadingRateKHR(cb, &cmdPar.fragmentSize, cmdPar.combinerOps);
        break;
      }
#endif
      default: G_ASSERTF(0, "vulkan: unrecognized command %u at reorder flush", *((CmdID *)cmdPtr)); break;
    }
  }
  endLoop(); //-V1020 End and Begin methods here is not used as scope like calls
}
