// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "driver.h"
#include "vulkan_device.h"
#include "vk_wrapped_handles.h"

// wrapper for vkCmdPipelineBarrier
// allows to accumulate memory/buffer/image barriers
// and perform them in one place for better efficiency and less code duplication
// also can be used to track various sync problems in one place

namespace drv3d_vulkan
{

class Buffer;
class Image;

class PipelineBarrier
{
public:
  enum
  {
    MERGE_ALL,
    MERGE_DST,
    MERGE_BOTH_MASK_EXTEND,
    MERGE_SRC_MASK_EXTEND,
    MERGE_UNIQUE,
    MERGE_COUNT
  };

  struct AccessFlags
  {
    VkAccessFlags src;
    VkAccessFlags dst;

    AccessFlags swap() { return {dst, src}; }
  };

  struct StageFlags
  {
    VkPipelineStageFlags src;
    VkPipelineStageFlags dst;

    bool operator==(const StageFlags &v) const { return src == v.src && dst == v.dst; }
  };

  struct BufferTemplate
  {
    StageFlags stage;
    AccessFlags mask;
    VulkanBufferHandle handle;
  };

  struct ImageTemplate
  {
    StageFlags stage;
    AccessFlags mask;
    VulkanImageHandle handle;
    VkImageLayout oldLayout;
    VkImageLayout newLayout;
    VkImageSubresourceRange subresRange;
  };

  struct BarrierStageCache
  {
    Tab<VkMemoryBarrier> mem;
    Tab<VkBufferMemoryBarrier> buffer;
    Tab<VkImageMemoryBarrier> image;
    StageFlags stage;
  };

  struct BarrierCache
  {
    Tab<VulkanEventHandle> events;
    Tab<BarrierStageCache> storage;
    // use separate size to avoid allocating memory for underlying arrays
    uint32_t size;
    dag::Span<BarrierStageCache> range() { return make_span(storage.data(), size); }
#if DAGOR_DBGLEVEL > 0
    bool inUse = false;
#endif
  };

  static bool dbgUseSplitSubmits;

private:
  BarrierCache &cache;

  struct
  {
    BufferTemplate buffer;
    ImageTemplate image;
  } barrierTemplate; //-V730_NOINIT

  VkDependencyFlags dependencyFlags;

  void reset();
  void submitSplitted();

public:
  PipelineBarrier(BarrierCache &cache);
  ~PipelineBarrier();

  // primary barrier adders
  void addDependencyFlags(VkDependencyFlags new_flag);
  void addMemory(StageFlags stage, AccessFlags mask);
  // merge_by_object will try to merge last element with newly adding one
  void addBuffer(StageFlags stage, AccessFlags mask, VulkanBufferHandle buf, VkDeviceSize offset, VkDeviceSize size,
    bool merge_by_object);
  void addImage(StageFlags stage, AccessFlags mask, VulkanImageHandle img, VkImageLayout old_layout, VkImageLayout new_layout,
    const VkImageSubresourceRange &subresources);
  void addEvent(VulkanEventHandle event);

  // various overloads & template adders

  void addBuffer(StageFlags stage, AccessFlags mask, const Buffer *buf, VkDeviceSize offset, VkDeviceSize size);

  void modifyBufferTemplateStage(StageFlags stage);
  void modifyBufferTemplate(AccessFlags mask);
  void modifyBufferTemplate(const Buffer *buf);
  void modifyBufferTemplate(VulkanBufferHandle buf);
  void addBufferByTemplate(VkDeviceSize offset, VkDeviceSize size);
  void addBufferByTemplateMerged(VkDeviceSize offset, VkDeviceSize size);

  void modifyImageTemplateStage(StageFlags stage);
  void modifyImageTemplate(AccessFlags mask);
  void modifyImageTemplateOldLayout(VkImageLayout layout);
  void modifyImageTemplateNewLayout(VkImageLayout layout);
  void modifyImageTemplate(VkImageLayout old_layout, VkImageLayout new_layout);
  void modifyImageTemplate(const Image *img);
  void modifyImageTemplate(uint32_t mip_index, uint32_t mip_range, uint32_t array_index, uint32_t array_range);
  void addImageOwnershipTransferByTemplate(uint32_t src, uint32_t dst);
  void addImageByTemplate();
  void addImageByTemplate(AccessFlags mask);
  void addImageByTemplateWithSrcAccess(VkAccessFlags mask);
  void addImageByTemplateWithDstAccess(VkAccessFlags mask);

  // special methods

  // TODO
  // merge any suitable image barriers if possible
  // void merge();

  void submit();
  bool empty();

  void dbgPrint();
};

template <uint32_t builtinCacheIdx>
class ContextedPipelineBarrier;

class BuiltinPipelineBarrierCache
{
  template <uint32_t builtinCacheIdx>
  friend class ContextedPipelineBarrier;

public:
  enum
  {
    EXECUTION_PRIMARY = 0,
    EXECUTION_SECONDARY = 1,
    SWAPCHAIN = 2,
    PIPE_SYNC = 3,
    QFOT = 4,
    BUILTIN_ELEMENTS
  };

private:
  static PipelineBarrier::BarrierCache data[BUILTIN_ELEMENTS];

public:
  static void clear()
  {
    uint32_t memTotal = 0;
    uint32_t bufferTotal = 0;
    uint32_t imageTotal = 0;
    uint32_t eventTotal = 0;
    for (uint32_t i = 0; i < BUILTIN_ELEMENTS; ++i)
    {
      for (PipelineBarrier::BarrierStageCache &j : data[i].storage)
      {
        memTotal += j.mem.capacity();
        clear_and_shrink(j.mem);
        bufferTotal += j.buffer.capacity();
        clear_and_shrink(j.buffer);
        imageTotal += j.image.capacity();
        clear_and_shrink(j.image);
      }
      clear_and_shrink(data[i].storage);
      eventTotal += data[i].events.capacity();
      clear_and_shrink(data[i].events);
    }
    debug("vulkan: builtin barrier cache cleaned up, max was: mem %u buf %u img %u evt %u", memTotal, bufferTotal, imageTotal,
      eventTotal);
  }
};

// class is not MT safe, use only inside of thread safe scope
// provides fast builtin cache
template <uint32_t builtinCacheIdx>
class ContextedPipelineBarrier : public PipelineBarrier
{
  static_assert(builtinCacheIdx < BuiltinPipelineBarrierCache::BUILTIN_ELEMENTS, "builtin cache index is out of bounds");

public:
  ContextedPipelineBarrier() : PipelineBarrier(BuiltinPipelineBarrierCache::data[builtinCacheIdx]) {}
};

} // namespace drv3d_vulkan