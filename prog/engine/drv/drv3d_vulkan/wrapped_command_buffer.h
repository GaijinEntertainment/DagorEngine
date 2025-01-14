// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// wraps used vkCmd* calls to faciliate reorder of linearly generated commands
// used to effectively generate batched barriers at needed command buffer points

#include "driver.h"
#include "vulkan_device.h"
#include "vk_wrapped_handles.h"
#include "globals.h"

namespace drv3d_vulkan
{

struct WrappedCommandBuffer
{
  VulkanCommandBufferHandle cb{};
  uint32_t reorder = 0;
  uint32_t pausedReorder = 0;

  struct ParametersMemory
  {
    Tab<void *> overflow;
    void *data = nullptr;
    uintptr_t ptr = 0;
    size_t size = 0;
    size_t left = 0;

    template <typename T>
    T *push(uint32_t cnt, T *vals)
    {
      if (!cnt || !vals)
        return nullptr;

      size_t dsz = cnt * sizeof(T);
      if (left <= dsz)
      {
        if (size)
          overflow.push_back(data);
        size = max(size * 2, dsz);
        data = malloc(size);
        ptr = (uintptr_t)data;
        left = size;
      }
      T *ret = (T *)ptr;
      memcpy((void *)ptr, (void *)vals, dsz);
      ptr += dsz;
      left -= dsz;
      return ret;
    }

    void clear()
    {
      for (void *i : overflow)
        free(i);
      overflow.clear();
      left = size;
      ptr = (uintptr_t)data;
    }

    void shutdown()
    {

      clear();
      if (!data)
        return;

      free(data);
      data = nullptr;
      left = 0;
      size = 0;
      ptr = 0;
    }

    ~ParametersMemory()
    {
      clear();
      G_ASSERTF(!data, "vulkan: missed shutdown for wrapped command buffer");
    }
  };
  ParametersMemory parMem;

  typedef uint8_t CmdID;
  Tab<uint8_t> cmdMem;

  template <typename T>
  struct CmdAndParamater
  {
    CmdID id;
    T param;
  };

  template <typename T>
  T &pushCmd(const T &val)
  {
    constexpr size_t sz = sizeof(CmdAndParamater<T>);
    CmdAndParamater<T> &tgt = *((CmdAndParamater<T> *)cmdMem.append_default(sz));
    tgt.id = T::ID;
    tgt.param = val;
    return tgt.param;
  }

  size_t getMemoryUsedMax();
  void shutdown()
  {
    verifyReorderEmpty();
    debug("vulkan: memory used for wrapped buffer (peak) %lluKb", getMemoryUsedMax() >> 10);
    parMem.shutdown();
  }

  void set(VulkanCommandBufferHandle cmd_b)
  {
    G_ASSERTF(!reorder && !cmdMem.size(), "vulkan: no command buffer change if reorder is active");
    cb = cmd_b;
  }
  bool isInReorder() { return reorder > 0; }
  void beginLoop();
  void endLoop();
  void flush();
  void startReorder() { ++reorder; }
  void stopReorder() { --reorder; }
  void pauseReorder()
  {
    G_ASSERTF(reorder, "vulkan: pausing reorder when its not started");
    G_ASSERTF(!pausedReorder, "vulkan: nested reorder pause");
    pausedReorder = reorder;
    reorder = 0;
  }
  void continueReorder()
  {
    G_ASSERTF(!reorder, "vulkan: nested reorder continue");
    G_ASSERTF(pausedReorder, "vulkan: reorder was not paused");
    reorder = pausedReorder;
    pausedReorder = 0;
  }
  void verifyReorderEmpty() { G_ASSERTF(!reorder && !cmdMem.size(), "vulkan: expected reorder content to be empty && disabled!"); }

  void wrapChainedStructs(VkBaseInStructure *s_base, const VkBaseInStructure *s_next)
  {
    while (s_next)
    {
      switch (s_next->sType)
      {
#if VK_KHR_imageless_framebuffer
        case VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO_KHR:
        {
          auto *rpabi = (VkRenderPassAttachmentBeginInfoKHR *)(parMem.push(1,
            reinterpret_cast<const VkRenderPassAttachmentBeginInfoKHR *>(s_next)));
          rpabi->pAttachments = parMem.push(rpabi->attachmentCount, rpabi->pAttachments);
          rpabi = parMem.push(1, rpabi);
          s_next = reinterpret_cast<const VkBaseInStructure *>(rpabi);
          s_base->pNext = s_next;
          break;
        }
#endif
        default: G_ASSERTF(0, "vulkan: unsupported structure %u in chain wrapper", s_next->sType); break;
      }
      s_base = (VkBaseInStructure *)s_next;
      s_next = (const VkBaseInStructure *)s_base->pNext;
    }
  }

  // RE gen
  // put // vkCmdXXX
  // search: \/\/ vkCmd(.*)
  // replace:
  //
  // for header
  //$0
  // struct $1Parameters
  //{
  //  static constexpr CmdID ID = AUTO_ID;
  //
  //};
  // void wCmd$1(
  //
  //)
  //{
  //  if (reorder)
  //  {
  //    pushCmd<$1Parameters>({});
  //  }
  //  else
  //    Globals::VK::dev.vkCmd$1(cb,);
  //}
  //
  // for impl
  //
  // case $1Parameters::ID:
  // {
  //   auto& cmdPar = ((CmdAndParamater<$1Parameters>*)cmdPtr)->param;
  //   cmdPtr += sizeof(CmdAndParamater<$1Parameters>);
  //   Globals::VK::dev.vkCmd$1(cb,
  //     cmdPar.
  //   );
  //   break;
  // }

  static constexpr size_t COUNTER_BASE = (__COUNTER__);
#define AUTO_ID (__COUNTER__ - COUNTER_BASE)

#if VK_EXT_conditional_rendering
  // vkCmdBeginConditionalRenderingEXT
  struct BeginConditionalRenderingEXTParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkConditionalRenderingBeginInfoEXT conditionalRenderingBegin;
  };
  void wCmdBeginConditionalRenderingEXT(const VkConditionalRenderingBeginInfoEXT *pConditionalRenderingBegin)
  {
    if (reorder)
    {
      pushCmd<BeginConditionalRenderingEXTParameters>({*pConditionalRenderingBegin});
    }
    else
      Globals::VK::dev.vkCmdBeginConditionalRenderingEXT(cb, pConditionalRenderingBegin);
  }

  // vkCmdEndConditionalRenderingEXT
  struct EndConditionalRenderingEXTParameters
  {
    static constexpr CmdID ID = AUTO_ID;
  };
  void wCmdEndConditionalRenderingEXT()
  {
    if (reorder)
    {
      pushCmd<EndConditionalRenderingEXTParameters>({});
    }
    else
      Globals::VK::dev.vkCmdEndConditionalRenderingEXT(cb);
  }
#endif

  // vkCmdBeginRenderPass
  struct BeginRenderPassParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkRenderPassBeginInfo renderPassBegin;
    VkSubpassContents contents;
  };
  void wCmdBeginRenderPass(const VkRenderPassBeginInfo *pRenderPassBegin, VkSubpassContents contents)
  {
    if (reorder)
    {
      VkRenderPassBeginInfo &rpbi = pushCmd<BeginRenderPassParameters>({*pRenderPassBegin, contents}).renderPassBegin;
      rpbi.pClearValues = parMem.push(rpbi.clearValueCount, rpbi.pClearValues);
      wrapChainedStructs(reinterpret_cast<VkBaseInStructure *>(&rpbi), reinterpret_cast<const VkBaseInStructure *>(rpbi.pNext));
    }
    else
      Globals::VK::dev.vkCmdBeginRenderPass(cb, pRenderPassBegin, contents);
  }

  // vkCmdNextSubpass
  struct NextSubpassParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkSubpassContents contents;
  };
  void wCmdNextSubpass(VkSubpassContents contents)
  {
    if (reorder)
    {
      pushCmd<NextSubpassParameters>({contents});
    }
    else
      Globals::VK::dev.vkCmdNextSubpass(cb, contents);
  }

  // vkCmdEndRenderPass
  struct EndRenderPassParameters
  {
    static constexpr CmdID ID = AUTO_ID;
  };
  void wCmdEndRenderPass()
  {
    if (reorder)
    {
      pushCmd<EndRenderPassParameters>({});
    }
    else
      Globals::VK::dev.vkCmdEndRenderPass(cb);
  }

  // vkCmdBindDescriptorSets
  struct BindDescriptorSetsParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkPipelineBindPoint pipelineBindPoint;
    VkPipelineLayout layout;
    uint32_t firstSet;
    uint32_t descriptorSetCount;
    const VkDescriptorSet *pDescriptorSets;
    uint32_t dynamicOffsetCount;
    const uint32_t *pDynamicOffsets;
  };
  void wCmdBindDescriptorSets(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet,
    uint32_t descriptorSetCount, const VkDescriptorSet *pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t *pDynamicOffsets)
  {
    if (reorder)
    {
      pushCmd<BindDescriptorSetsParameters>({pipelineBindPoint, layout, firstSet, descriptorSetCount,
        parMem.push(descriptorSetCount, pDescriptorSets), dynamicOffsetCount, parMem.push(dynamicOffsetCount, pDynamicOffsets)});
    }
    else
      Globals::VK::dev.vkCmdBindDescriptorSets(cb, pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets,
        dynamicOffsetCount, pDynamicOffsets);
  }

  // vkCmdBindIndexBuffer
  struct BindIndexBufferParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkBuffer buffer;
    VkDeviceSize offset;
    VkIndexType indexType;
  };
  void wCmdBindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
  {
    if (reorder)
    {
      pushCmd<BindIndexBufferParameters>({buffer, offset, indexType});
    }
    else
      Globals::VK::dev.vkCmdBindIndexBuffer(cb, buffer, offset, indexType);
  }

  // vkCmdWriteTimestamp
  struct WriteTimestampParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkPipelineStageFlagBits pipelineStage;
    VkQueryPool queryPool;
    uint32_t query;
  };
  void wCmdWriteTimestamp(VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query)
  {
    if (reorder)
    {
      pushCmd<WriteTimestampParameters>({pipelineStage, queryPool, query});
    }
    else
      Globals::VK::dev.vkCmdWriteTimestamp(cb, pipelineStage, queryPool, query);
  }

#if VK_AMD_buffer_marker
  // vkCmdWriteBufferMarkerAMD
  struct WriteBufferMarkerAMDParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkPipelineStageFlagBits pipelineStage;
    VkBuffer dstBuffer;
    VkDeviceSize dstOffset;
    uint32_t marker;
  };
  void wCmdWriteBufferMarkerAMD(VkPipelineStageFlagBits pipelineStage, VkBuffer dstBuffer, VkDeviceSize dstOffset, uint32_t marker)
  {
    if (reorder)
    {
      pushCmd<WriteBufferMarkerAMDParameters>({pipelineStage, dstBuffer, dstOffset, marker});
    }
    else
      Globals::VK::dev.vkCmdWriteBufferMarkerAMD(cb, pipelineStage, dstBuffer, dstOffset, marker);
  }
#endif

#if VK_NV_device_diagnostic_checkpoints
  // vkCmdSetCheckpointNV
  struct SetCheckpointNVParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    const void *pCheckpointMarker;
  };
  void wCmdSetCheckpointNV(const void *pCheckpointMarker)
  {
    if (reorder)
    {
      pushCmd<SetCheckpointNVParameters>({pCheckpointMarker});
    }
    else
      Globals::VK::dev.vkCmdSetCheckpointNV(cb, pCheckpointMarker);
  }
#endif

  // vkCmdSetScissor
  struct SetScissorParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    uint32_t firstScissor;
    uint32_t scissorCount;
    const VkRect2D *pScissors;
  };
  void wCmdSetScissor(uint32_t firstScissor, uint32_t scissorCount, const VkRect2D *pScissors)
  {
    if (reorder)
    {
      pushCmd<SetScissorParameters>({firstScissor, scissorCount, parMem.push(scissorCount, pScissors)});
    }
    else
      Globals::VK::dev.vkCmdSetScissor(cb, firstScissor, scissorCount, pScissors);
  }

  // vkCmdSetDepthBias
  struct SetDepthBiasParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    float depthBiasConstantFactor;
    float depthBiasClamp;
    float depthBiasSlopeFactor;
  };
  void wCmdSetDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor)
  {
    if (reorder)
    {
      pushCmd<SetDepthBiasParameters>({depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor});
    }
    else
      Globals::VK::dev.vkCmdSetDepthBias(cb, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
  }

  // vkCmdSetDepthBounds
  struct SetDepthBoundsParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    float minDepthBounds;
    float maxDepthBounds;
  };
  void wCmdSetDepthBounds(float minDepthBounds, float maxDepthBounds)
  {
    if (reorder)
    {
      pushCmd<SetDepthBoundsParameters>({minDepthBounds, maxDepthBounds

      });
    }
    else
      Globals::VK::dev.vkCmdSetDepthBounds(cb, minDepthBounds, maxDepthBounds);
  }

  // vkCmdSetStencilReference
  struct SetStencilReferenceParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkStencilFaceFlags faceMask;
    uint32_t reference;
  };
  void wCmdSetStencilReference(VkStencilFaceFlags faceMask, uint32_t reference)
  {
    if (reorder)
    {
      pushCmd<SetStencilReferenceParameters>({faceMask, reference

      });
    }
    else
      Globals::VK::dev.vkCmdSetStencilReference(cb, faceMask, reference);
  }

  // vkCmdSetStencilWriteMask
  struct SetStencilWriteMaskParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkStencilFaceFlags faceMask;
    uint32_t writeMask;
  };
  void wCmdSetStencilWriteMask(VkStencilFaceFlags faceMask, uint32_t writeMask)
  {
    if (reorder)
    {
      pushCmd<SetStencilWriteMaskParameters>({faceMask, writeMask});
    }
    else
      Globals::VK::dev.vkCmdSetStencilWriteMask(cb, faceMask, writeMask);
  }

  // vkCmdSetStencilCompareMask
  struct SetStencilCompareMaskParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkStencilFaceFlags faceMask;
    uint32_t compareMask;
  };
  void wCmdSetStencilCompareMask(VkStencilFaceFlags faceMask, uint32_t compareMask)
  {
    if (reorder)
    {
      pushCmd<SetStencilCompareMaskParameters>({faceMask, compareMask});
    }
    else
      Globals::VK::dev.vkCmdSetStencilCompareMask(cb, faceMask, compareMask);
  }

  // vkCmdSetViewport
  struct SetViewportParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    uint32_t firstViewport;
    uint32_t viewportCount;
    const VkViewport *pViewports;
  };
  void wCmdSetViewport(uint32_t firstViewport, uint32_t viewportCount, const VkViewport *pViewports)
  {
    if (reorder)
    {
      pushCmd<SetViewportParameters>({firstViewport, viewportCount, parMem.push(viewportCount, pViewports)});
    }
    else
      Globals::VK::dev.vkCmdSetViewport(cb, firstViewport, viewportCount, pViewports);
  }

  // vkCmdBindVertexBuffers
  struct BindVertexBuffersParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    uint32_t firstBinding;
    uint32_t bindingCount;
    const VkBuffer *pBuffers;
    const VkDeviceSize *pOffsets;
  };
  void wCmdBindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer *pBuffers, const VkDeviceSize *pOffsets)
  {
    if (reorder)
    {
      pushCmd<BindVertexBuffersParameters>(
        {firstBinding, bindingCount, parMem.push(bindingCount, pBuffers), parMem.push(bindingCount, pOffsets)});
    }
    else
      Globals::VK::dev.vkCmdBindVertexBuffers(cb, firstBinding, bindingCount, pBuffers, pOffsets);
  }

#if VK_EXT_debug_marker
  // vkCmdDebugMarkerInsertEXT
  struct DebugMarkerInsertEXTParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkDebugMarkerMarkerInfoEXT markerInfo;
  };
  void wCmdDebugMarkerInsertEXT(const VkDebugMarkerMarkerInfoEXT *pMarkerInfo)
  {
    if (reorder)
    {
      pushCmd<DebugMarkerInsertEXTParameters>({*pMarkerInfo});
    }
    else
      Globals::VK::dev.vkCmdDebugMarkerInsertEXT(cb, pMarkerInfo);
  }

  // vkCmdDebugMarkerBeginEXT
  struct DebugMarkerBeginEXTParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkDebugMarkerMarkerInfoEXT markerInfo;
  };
  void wCmdDebugMarkerBeginEXT(const VkDebugMarkerMarkerInfoEXT *pMarkerInfo)
  {
    if (reorder)
    {
      pushCmd<DebugMarkerBeginEXTParameters>({*pMarkerInfo});
    }
    else
      Globals::VK::dev.vkCmdDebugMarkerBeginEXT(cb, pMarkerInfo);
  }

  // vkCmdDebugMarkerEndEXT
  struct DebugMarkerEndEXTParameters
  {
    static constexpr CmdID ID = AUTO_ID;
  };
  void wCmdDebugMarkerEndEXT()
  {
    if (reorder)
    {
      pushCmd<DebugMarkerEndEXTParameters>({});
    }
    else
      Globals::VK::dev.vkCmdDebugMarkerEndEXT(cb);
  }
#endif

#if VK_EXT_debug_utils
  // vkCmdInsertDebugUtilsLabelEXT
  struct InsertDebugUtilsLabelEXTParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkDebugUtilsLabelEXT labelInfo;
  };
  void wCmdInsertDebugUtilsLabelEXT(const VkDebugUtilsLabelEXT *pLabelInfo)
  {
    if (reorder)
    {
      pushCmd<InsertDebugUtilsLabelEXTParameters>({*pLabelInfo});
    }
    else
      Globals::VK::inst.vkCmdInsertDebugUtilsLabelEXT(cb, pLabelInfo);
  }

  // vkCmdBeginDebugUtilsLabelEXT
  struct BeginDebugUtilsLabelEXTParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkDebugUtilsLabelEXT labelInfo;
  };
  void wCmdBeginDebugUtilsLabelEXT(const VkDebugUtilsLabelEXT *pLabelInfo)
  {
    if (reorder)
    {
      pushCmd<BeginDebugUtilsLabelEXTParameters>({*pLabelInfo});
    }
    else
      Globals::VK::inst.vkCmdBeginDebugUtilsLabelEXT(cb, pLabelInfo);
  }

  // vkCmdEndDebugUtilsLabelEXT
  struct EndDebugUtilsLabelEXTParameters
  {
    static constexpr CmdID ID = AUTO_ID;
  };
  void wCmdEndDebugUtilsLabelEXT()
  {
    if (reorder)
    {
      pushCmd<EndDebugUtilsLabelEXTParameters>({});
    }
    else
      Globals::VK::inst.vkCmdEndDebugUtilsLabelEXT(cb);
  }
#endif

  // vkCmdBeginQuery
  struct BeginQueryParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkQueryPool queryPool;
    uint32_t query;
    VkQueryControlFlags flags;
  };
  void wCmdBeginQuery(VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags)
  {
    if (reorder)
    {
      pushCmd<BeginQueryParameters>({queryPool, query, flags});
    }
    else
      Globals::VK::dev.vkCmdBeginQuery(cb, queryPool, query, flags);
  }

  // vkCmdEndQuery
  struct EndQueryParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkQueryPool queryPool;
    uint32_t query;
  };
  void wCmdEndQuery(VkQueryPool queryPool, uint32_t query)
  {
    if (reorder)
    {
      pushCmd<EndQueryParameters>({queryPool, query});
    }
    else
      Globals::VK::dev.vkCmdEndQuery(cb, queryPool, query);
  }

  // vkCmdDrawIndirect
  struct DrawIndirectParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkBuffer buffer;
    VkDeviceSize offset;
    uint32_t drawCount;
    uint32_t stride;
  };
  void wCmdDrawIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
  {
    if (reorder)
    {
      pushCmd<DrawIndirectParameters>({buffer, offset, drawCount, stride});
    }
    else
      Globals::VK::dev.vkCmdDrawIndirect(cb, buffer, offset, drawCount, stride);
  }

  // vkCmdDrawIndexedIndirect
  struct DrawIndexedIndirectParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkBuffer buffer;
    VkDeviceSize offset;
    uint32_t drawCount;
    uint32_t stride;
  };
  void wCmdDrawIndexedIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
  {
    if (reorder)
    {
      pushCmd<DrawIndexedIndirectParameters>({buffer, offset, drawCount, stride});
    }
    else
      Globals::VK::dev.vkCmdDrawIndexedIndirect(cb, buffer, offset, drawCount, stride);
  }

  // vkCmdDraw
  struct DrawParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    uint32_t vertexCount;
    uint32_t instanceCount;
    uint32_t firstVertex;
    uint32_t firstInstance;
  };
  void wCmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
  {
    if (reorder)
    {
      pushCmd<DrawParameters>({vertexCount, instanceCount, firstVertex, firstInstance});
    }
    else
      Globals::VK::dev.vkCmdDraw(cb, vertexCount, instanceCount, firstVertex, firstInstance);
  }

  // vkCmdDrawIndexed
  struct DrawIndexedParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    uint32_t indexCount;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int32_t vertexOffset;
    uint32_t firstInstance;
  };
  void wCmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
  {
    if (reorder)
    {
      pushCmd<DrawIndexedParameters>({indexCount, instanceCount, firstIndex, vertexOffset, firstInstance});
    }
    else
      Globals::VK::dev.vkCmdDrawIndexed(cb, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
  }

  // vkCmdBindPipeline
  struct BindPipelineParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkPipelineBindPoint pipelineBindPoint;
    VkPipeline pipeline;
  };
  void wCmdBindPipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
  {
    if (reorder)
    {
      pushCmd<BindPipelineParameters>({pipelineBindPoint, pipeline});
    }
    else
      Globals::VK::dev.vkCmdBindPipeline(cb, pipelineBindPoint, pipeline);
  }

  // vkCmdSetBlendConstants
  struct SetBlendConstantsParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    float blendConstants[4];
  };
  void wCmdSetBlendConstants(const float blendConstants[4])
  {
    if (reorder)
    {
      pushCmd<SetBlendConstantsParameters>({{blendConstants[0], blendConstants[1], blendConstants[2], blendConstants[3]}});
    }
    else
      Globals::VK::dev.vkCmdSetBlendConstants(cb, blendConstants);
  }

  // vkCmdCopyBufferToImage
  struct CopyBufferToImageParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkBuffer srcBuffer;
    VkImage dstImage;
    VkImageLayout dstImageLayout;
    uint32_t regionCount;
    const VkBufferImageCopy *pRegions;
  };
  void wCmdCopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount,
    const VkBufferImageCopy *pRegions)
  {
    if (reorder)
    {
      pushCmd<CopyBufferToImageParameters>({srcBuffer, dstImage, dstImageLayout, regionCount, parMem.push(regionCount, pRegions)});
    }
    else
      Globals::VK::dev.vkCmdCopyBufferToImage(cb, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
  }
  // vkCmdCopyBuffer
  struct CopyBufferParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkBuffer srcBuffer;
    VkBuffer dstBuffer;
    uint32_t regionCount;
    const VkBufferCopy *pRegions;
  };
  void wCmdCopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy *pRegions)
  {
    if (reorder)
    {
      pushCmd<CopyBufferParameters>({srcBuffer, dstBuffer, regionCount, parMem.push(regionCount, pRegions)});
    }
    else
      Globals::VK::dev.vkCmdCopyBuffer(cb, srcBuffer, dstBuffer, regionCount, pRegions);
  }
  // vkCmdBlitImage
  struct BlitImageParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkImage srcImage;
    VkImageLayout srcImageLayout;
    VkImage dstImage;
    VkImageLayout dstImageLayout;
    uint32_t regionCount;
    const VkImageBlit *pRegions;
    VkFilter filter;
  };
  void wCmdBlitImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout,
    uint32_t regionCount, const VkImageBlit *pRegions, VkFilter filter)
  {
    if (reorder)
    {
      pushCmd<BlitImageParameters>(
        {srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, parMem.push(regionCount, pRegions), filter});
    }
    else
      Globals::VK::dev.vkCmdBlitImage(cb, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter);
  }
  // vkCmdResolveImage
  struct ResolveImageParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkImage srcImage;
    VkImageLayout srcImageLayout;
    VkImage dstImage;
    VkImageLayout dstImageLayout;
    uint32_t regionCount;
    const VkImageResolve *pRegions;
  };
  void wCmdResolveImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout,
    uint32_t regionCount, const VkImageResolve *pRegions)
  {
    if (reorder)
    {
      pushCmd<ResolveImageParameters>(
        {srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, parMem.push(regionCount, pRegions)});
    }
    else
      Globals::VK::dev.vkCmdResolveImage(cb, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
  }
  // vkCmdCopyImageToBuffer
  struct CopyImageToBufferParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkImage srcImage;
    VkImageLayout srcImageLayout;
    VkBuffer dstBuffer;
    uint32_t regionCount;
    const VkBufferImageCopy *pRegions;
  };
  void wCmdCopyImageToBuffer(VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount,
    const VkBufferImageCopy *pRegions)
  {
    if (reorder)
    {
      pushCmd<CopyImageToBufferParameters>({srcImage, srcImageLayout, dstBuffer, regionCount, parMem.push(regionCount, pRegions)});
    }
    else
      Globals::VK::dev.vkCmdCopyImageToBuffer(cb, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions);
  }
  // vkCmdPipelineBarrier
  struct PipelineBarrierParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkPipelineStageFlags srcStageMask;
    VkPipelineStageFlags dstStageMask;
    VkDependencyFlags dependencyFlags;
    uint32_t memoryBarrierCount;
    const VkMemoryBarrier *pMemoryBarriers;
    uint32_t bufferMemoryBarrierCount;
    const VkBufferMemoryBarrier *pBufferMemoryBarriers;
    uint32_t imageMemoryBarrierCount;
    const VkImageMemoryBarrier *pImageMemoryBarriers;
  };
  void wCmdPipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags,
    uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers, uint32_t bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier *pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount,
    const VkImageMemoryBarrier *pImageMemoryBarriers)
  {
    if (reorder)
    {
      pushCmd<PipelineBarrierParameters>(
        {srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, parMem.push(memoryBarrierCount, pMemoryBarriers),
          bufferMemoryBarrierCount, parMem.push(bufferMemoryBarrierCount, pBufferMemoryBarriers), imageMemoryBarrierCount,
          parMem.push(imageMemoryBarrierCount, pImageMemoryBarriers)});
    }
    else
      Globals::VK::dev.vkCmdPipelineBarrier(cb, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers,
        bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
  }

#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
  // vkCmdCopyAccelerationStructureKHR
  struct CopyAccelerationStructureKHRParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkCopyAccelerationStructureInfoKHR info;
  };
  void wCmdCopyAccelerationStructureKHR(const VkCopyAccelerationStructureInfoKHR *pInfo)
  {
    if (reorder)
    {
      pushCmd<CopyAccelerationStructureKHRParameters>({*pInfo});
    }
    else
      Globals::VK::dev.vkCmdCopyAccelerationStructureKHR(cb, pInfo);
  }
  // vkCmdBuildAccelerationStructuresKHR
  struct BuildAccelerationStructuresKHRParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    uint32_t infoCount;
    VkAccelerationStructureBuildGeometryInfoKHR *pInfos;
    const VkAccelerationStructureBuildRangeInfoKHR **ppBuildRangeInfos;
  };
  void wCmdBuildAccelerationStructuresKHR(uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR *pInfos,
    const VkAccelerationStructureBuildRangeInfoKHR *const *ppBuildRangeInfos)
  {
    if (reorder)
    {
      BuildAccelerationStructuresKHRParameters &par = pushCmd<BuildAccelerationStructuresKHRParameters>(
        {infoCount, (VkAccelerationStructureBuildGeometryInfoKHR *)parMem.push(infoCount, pInfos),
          (const VkAccelerationStructureBuildRangeInfoKHR **)parMem.push(infoCount, ppBuildRangeInfos)});
      for (uint32_t i = 0; i < par.infoCount; ++i)
      {
        par.pInfos[i].pGeometries = parMem.push(par.pInfos[i].geometryCount, par.pInfos[i].pGeometries);
        par.ppBuildRangeInfos[i] = parMem.push(par.pInfos[i].geometryCount, par.ppBuildRangeInfos[i]);
      }
    }
    else
      Globals::VK::dev.vkCmdBuildAccelerationStructuresKHR(cb, infoCount, pInfos, ppBuildRangeInfos);
  }
  // vkCmdWriteAccelerationStructuresPropertiesKHR
  struct WriteAccelerationStructuresPropertiesKHRParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    uint32_t accelerationStructureCount;
    const VkAccelerationStructureKHR *pAccelerationStructures;
    VkQueryType queryType;
    VkQueryPool queryPool;
    uint32_t firstQuery;
  };
  void wCmdWriteAccelerationStructuresPropertiesKHR(uint32_t accelerationStructureCount,
    const VkAccelerationStructureKHR *pAccelerationStructures, VkQueryType queryType, VkQueryPool queryPool, uint32_t firstQuery)
  {
    if (reorder)
    {
      pushCmd<WriteAccelerationStructuresPropertiesKHRParameters>({accelerationStructureCount,
        parMem.push(accelerationStructureCount, pAccelerationStructures), queryType, queryPool, firstQuery});
    }
    else
      Globals::VK::dev.vkCmdWriteAccelerationStructuresPropertiesKHR(cb, accelerationStructureCount, pAccelerationStructures,
        queryType, queryPool, firstQuery);
  }
#endif

  // vkCmdCopyQueryPoolResults
  struct CopyQueryPoolResultsParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkQueryPool queryPool;
    uint32_t firstQuery;
    uint32_t queryCount;
    VkBuffer dstBuffer;
    VkDeviceSize dstOffset;
    VkDeviceSize stride;
    VkQueryResultFlags flags;
  };
  void wCmdCopyQueryPoolResults(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, VkBuffer dstBuffer,
    VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags)
  {
    if (reorder)
    {
      pushCmd<CopyQueryPoolResultsParameters>({queryPool, firstQuery, queryCount, dstBuffer, dstOffset, stride, flags});
    }
    else
      Globals::VK::dev.vkCmdCopyQueryPoolResults(cb, queryPool, firstQuery, queryCount, dstBuffer, dstOffset, stride, flags);
  }
  // vkCmdDispatch
  struct DispatchParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    uint32_t groupCountX;
    uint32_t groupCountY;
    uint32_t groupCountZ;
  };
  void wCmdDispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
  {
    if (reorder)
    {
      pushCmd<DispatchParameters>({groupCountX, groupCountY, groupCountZ});
    }
    else
      Globals::VK::dev.vkCmdDispatch(cb, groupCountX, groupCountY, groupCountZ);
  }
  // vkCmdDispatchIndirect
  struct DispatchIndirectParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkBuffer buffer;
    VkDeviceSize offset;
  };
  void wCmdDispatchIndirect(VkBuffer buffer, VkDeviceSize offset)
  {
    if (reorder)
    {
      pushCmd<DispatchIndirectParameters>({buffer, offset});
    }
    else
      Globals::VK::dev.vkCmdDispatchIndirect(cb, buffer, offset);
  }
  // vkCmdFillBuffer
  struct FillBufferParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkBuffer dstBuffer;
    VkDeviceSize dstOffset;
    VkDeviceSize size;
    uint32_t data;
  };
  void wCmdFillBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data)
  {
    if (reorder)
    {
      pushCmd<FillBufferParameters>({dstBuffer, dstOffset, size, data});
    }
    else
      Globals::VK::dev.vkCmdFillBuffer(cb, dstBuffer, dstOffset, size, data);
  }
  // vkCmdClearDepthStencilImage
  struct ClearDepthStencilImageParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkImage image;
    VkImageLayout imageLayout;
    VkClearDepthStencilValue depthStencil;
    uint32_t rangeCount;
    const VkImageSubresourceRange *pRanges;
  };
  void wCmdClearDepthStencilImage(VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue *pDepthStencil,
    uint32_t rangeCount, const VkImageSubresourceRange *pRanges)
  {
    if (reorder)
    {
      pushCmd<ClearDepthStencilImageParameters>({image, imageLayout, *pDepthStencil, rangeCount, parMem.push(rangeCount, pRanges)});
    }
    else
      Globals::VK::dev.vkCmdClearDepthStencilImage(cb, image, imageLayout, pDepthStencil, rangeCount, pRanges);
  }
  // vkCmdClearColorImage
  struct ClearColorImageParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkImage image;
    VkImageLayout imageLayout;
    VkClearColorValue color;
    uint32_t rangeCount;
    const VkImageSubresourceRange *pRanges;
  };
  void wCmdClearColorImage(VkImage image, VkImageLayout imageLayout, const VkClearColorValue *pColor, uint32_t rangeCount,
    const VkImageSubresourceRange *pRanges)
  {
    if (reorder)
    {
      pushCmd<ClearColorImageParameters>({image, imageLayout, *pColor, rangeCount, parMem.push(rangeCount, pRanges)});
    }
    else
      Globals::VK::dev.vkCmdClearColorImage(cb, image, imageLayout, pColor, rangeCount, pRanges);
  }
  // vkCmdCopyImage
  struct CopyImageParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkImage srcImage;
    VkImageLayout srcImageLayout;
    VkImage dstImage;
    VkImageLayout dstImageLayout;
    uint32_t regionCount;
    const VkImageCopy *pRegions;
  };
  void wCmdCopyImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout,
    uint32_t regionCount, const VkImageCopy *pRegions)
  {
    if (reorder)
    {
      pushCmd<CopyImageParameters>(
        {srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, parMem.push(regionCount, pRegions)});
    }
    else
      Globals::VK::dev.vkCmdCopyImage(cb, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
  }

  // vkCmdResetQueryPool
  struct ResetQueryPoolParameters
  {
    static constexpr CmdID ID = AUTO_ID;

    VkQueryPool queryPool;
    uint32_t firstQuery;
    uint32_t queryCount;
  };
  void wCmdResetQueryPool(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
  {
    if (reorder)
    {
      pushCmd<ResetQueryPoolParameters>({queryPool, firstQuery, queryCount});
    }
    else
      Globals::VK::dev.vkCmdResetQueryPool(cb, queryPool, firstQuery, queryCount);
  }
};

} // namespace drv3d_vulkan