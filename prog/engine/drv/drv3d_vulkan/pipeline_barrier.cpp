// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "pipeline_barrier.h"

#include <perfMon/dag_statDrv.h>

#include "buffer_resource.h"
#include "image_resource.h"
#include "vk_to_string.h"
#include "backend.h"
#include "wrapped_command_buffer.h"
#include "physical_device_set.h"
#include "backend_interop.h"

using namespace drv3d_vulkan;

PipelineBarrier::BarrierCache BuiltinPipelineBarrierCache::data[BUILTIN_ELEMENTS] = {};

bool PipelineBarrier::dbgUseSplitSubmits = false;

namespace
{

template <typename RetStructType>
RetStructType &nextCacheElement(Tab<RetStructType> &cacheArr)
{
  cacheArr.push_back_uninitialized();
  return cacheArr.back();
}

PipelineBarrier::BarrierStageCache &nextCacheStage(PipelineBarrier::BarrierCache &cache, PipelineBarrier::StageFlags &stage)
{
  size_t mergeMode = Backend::interop.barrierMergeMode.load(std::memory_order_relaxed) % PipelineBarrier::MERGE_COUNT;
  for (PipelineBarrier::BarrierStageCache &i : cache.range())
  {
    switch (mergeMode)
    {
      // add all src and dst stages to current element
      // single submit, but more restricting and slower sync
      case PipelineBarrier::MERGE_ALL:
        i.stage.src |= stage.src;
        i.stage.dst |= stage.dst;
        return i;
      // keep src stages unique, merge dst stages
      // less restricting than single submit merge, as it removes dependency from some unrelated stages
      // yet blocks for dst stages are wide
      case PipelineBarrier::MERGE_DST:
        if (i.stage.src == stage.src)
        {
          i.stage.dst |= stage.dst;
          return i;
        }
        break;
      // if any bit is shared in both masks, merge stages
      // sequental stages approach, in this case no need to block following/previous stages separately
      // quite opportunistic approach, beware!
      case PipelineBarrier::MERGE_BOTH_MASK_EXTEND:
        if ((i.stage.src & stage.src) && (i.stage.dst & stage.dst))
        {
          i.stage.src |= stage.src;
          i.stage.dst |= stage.dst;
          return i;
        }
        break;
      // same as with both, but check shared bits only for src stage
      case PipelineBarrier::MERGE_SRC_MASK_EXTEND:
        if ((i.stage.src & stage.src))
        {
          i.stage.src |= stage.src;
          i.stage.dst |= stage.dst;
          return i;
        }
        break;
      // all stages independant variant
      // worth if sync is fully resource localized and execution is task shcheduled for every stage independently
      // yet gives us quite a number of barrier calls
      case PipelineBarrier::MERGE_UNIQUE:
        if (i.stage == stage)
          return i;
        break;
      default: G_ASSERTF(0, "vulkan: unknown barrier merge mode %u", mergeMode); break;
    }
  }
  if (cache.storage.size() <= cache.size)
    cache.storage.push_back({});
  PipelineBarrier::BarrierStageCache &ret = cache.storage[cache.size];
  ret.buffer.clear();
  ret.image.clear();
  ret.mem.clear();
  ret.stage = stage;
  cache.size++;
  return ret;
}

} // namespace

PipelineBarrier::PipelineBarrier(BarrierCache &in_cache) : cache(in_cache), dependencyFlags(0)
{
  cache.size = 0;
#if DAGOR_DBGLEVEL > 0
  G_ASSERT(!cache.inUse);
  cache.inUse = true;
#endif
}

PipelineBarrier::~PipelineBarrier()
{
#if DAGOR_DBGLEVEL > 0
  G_ASSERT(cache.inUse);
  cache.inUse = false;
#endif
}

void PipelineBarrier::addDependencyFlags(VkDependencyFlags new_flag) { dependencyFlags |= new_flag; }

void PipelineBarrier::addMemory(StageFlags stage, AccessFlags mask)
{
  BarrierStageCache &stagedCache = nextCacheStage(cache, stage);
  VkMemoryBarrier &newBarrier = nextCacheElement(stagedCache.mem);
  newBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  newBarrier.pNext = nullptr;

  newBarrier.srcAccessMask = mask.src;
  newBarrier.dstAccessMask = mask.dst;
}

void PipelineBarrier::addBuffer(StageFlags stage, AccessFlags mask, VulkanBufferHandle buf, VkDeviceSize offset, VkDeviceSize size,
  bool merge_by_object)
{
  BarrierStageCache &stagedCache = nextCacheStage(cache, stage);
  if (merge_by_object && stagedCache.buffer.size())
  {
    VkBufferMemoryBarrier &i = stagedCache.buffer[stagedCache.buffer.size() - 1];
    if (i.buffer == buf)
    {
      i.dstAccessMask |= mask.dst;
      i.srcAccessMask |= mask.src;
      i.size = max(i.offset + i.size, offset + size);
      i.offset = min(i.offset, offset);
      i.size -= i.offset;
      return;
    }
  }

  VkBufferMemoryBarrier &newBarrier = nextCacheElement(stagedCache.buffer);

  newBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  newBarrier.pNext = nullptr;
  newBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  newBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

  newBarrier.srcAccessMask = mask.src;
  newBarrier.dstAccessMask = mask.dst;
  newBarrier.buffer = buf;
  newBarrier.offset = offset;
  newBarrier.size = size;
}

void PipelineBarrier::addImage(StageFlags stage, AccessFlags mask, VulkanImageHandle img, VkImageLayout old_layout,
  VkImageLayout new_layout, const VkImageSubresourceRange &subresources)
{
  BarrierStageCache &stagedCache = nextCacheStage(cache, stage);
  VkImageMemoryBarrier &newBarrier = nextCacheElement(stagedCache.image);

  newBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  newBarrier.pNext = nullptr;
  newBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  newBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  newBarrier.srcAccessMask = mask.src;
  newBarrier.dstAccessMask = mask.dst;
  newBarrier.oldLayout = old_layout;
  newBarrier.newLayout = new_layout;
  newBarrier.image = img;
  newBarrier.subresourceRange = subresources;
}

void PipelineBarrier::addBuffer(StageFlags stage, AccessFlags mask, const Buffer *buf, VkDeviceSize offset, VkDeviceSize size)
{
  addBuffer(stage, mask, buf->getHandle(), offset, size, false);
}

void PipelineBarrier::addEvent(VulkanEventHandle event)
{
  for (VulkanEventHandle i : cache.events)
    if (i == event)
      return;
  cache.events.push_back(event);
}

void PipelineBarrier::modifyBufferTemplateStage(StageFlags stage) { barrierTemplate.buffer.stage = stage; }

void PipelineBarrier::modifyBufferTemplate(AccessFlags mask) { barrierTemplate.buffer.mask = mask; }

void PipelineBarrier::modifyBufferTemplate(const Buffer *buf) { barrierTemplate.buffer.handle = buf->getHandle(); }

void PipelineBarrier::modifyBufferTemplate(VulkanBufferHandle buf) { barrierTemplate.buffer.handle = buf; }

void PipelineBarrier::addBufferByTemplate(VkDeviceSize offset, VkDeviceSize size)
{
  BufferTemplate &tpl = barrierTemplate.buffer;
  addBuffer(tpl.stage, tpl.mask, tpl.handle, offset, size, false);
}

void PipelineBarrier::addBufferByTemplateMerged(VkDeviceSize offset, VkDeviceSize size)
{
  BufferTemplate &tpl = barrierTemplate.buffer;
  addBuffer(tpl.stage, tpl.mask, tpl.handle, offset, size, true);
}

void PipelineBarrier::modifyImageTemplateStage(StageFlags stage) { barrierTemplate.image.stage = stage; }

void PipelineBarrier::modifyImageTemplate(AccessFlags mask)
{
  ImageTemplate &tpl = barrierTemplate.image;
  tpl.mask = mask;
}

void PipelineBarrier::modifyImageTemplateOldLayout(VkImageLayout layout)
{
  ImageTemplate &tpl = barrierTemplate.image;
  tpl.oldLayout = layout;
}

void PipelineBarrier::modifyImageTemplateNewLayout(VkImageLayout layout)
{
  ImageTemplate &tpl = barrierTemplate.image;
  tpl.newLayout = layout;
}

void PipelineBarrier::modifyImageTemplate(VkImageLayout old_layout, VkImageLayout new_layout)
{
  ImageTemplate &tpl = barrierTemplate.image;

  tpl.oldLayout = old_layout;
  tpl.newLayout = new_layout;
}

void PipelineBarrier::modifyImageTemplate(const Image *img)
{
  ImageTemplate &tpl = barrierTemplate.image;
  tpl.handle = img->getHandle();
  tpl.subresRange.aspectMask = img->getFormat().getAspektFlags();
}

void PipelineBarrier::modifyImageTemplate(uint32_t mip_index, uint32_t mip_range, uint32_t array_index, uint32_t array_range)
{
  ImageTemplate &tpl = barrierTemplate.image;

  tpl.subresRange.baseMipLevel = mip_index;
  tpl.subresRange.levelCount = mip_range;
  tpl.subresRange.baseArrayLayer = array_index;
  tpl.subresRange.layerCount = array_range;
}

void PipelineBarrier::reset()
{
  cache.size = 0;
  cache.events.clear();
}

bool PipelineBarrier::empty() { return cache.size == 0; }

void PipelineBarrier::submitSplitted()
{
#if DAGOR_DBGLEVEL > 0
  for (const BarrierStageCache &i : cache.range())
    // don't check src, src none barrier is ok
    G_ASSERTF(i.stage.dst, "vulkan: barrier should have dest stage");
#endif

  {
    TIME_PROFILE(vulkan_barrier_s_mem);
    for (const BarrierStageCache &i : cache.range())
      if (i.mem.size())
        Backend::cb.wCmdPipelineBarrier(i.stage.src, i.stage.dst, dependencyFlags, i.mem.size(), i.mem.data(), 0, nullptr, 0, nullptr);
  }

  {
    TIME_PROFILE(vulkan_barrier_s_buf);
    for (const BarrierStageCache &i : cache.range())
      for (int j = 0; j < i.buffer.size(); ++j)
        Backend::cb.wCmdPipelineBarrier(i.stage.src, i.stage.dst, dependencyFlags, 0, nullptr, 1, &i.buffer[j], 0, nullptr);
  }

  {
    TIME_PROFILE(vulkan_barrier_s_img);
    for (const BarrierStageCache &i : cache.range())
      for (int j = 0; j < i.image.size(); ++j)
        Backend::cb.wCmdPipelineBarrier(i.stage.src, i.stage.dst, dependencyFlags, 0, nullptr, 0, nullptr, 1, &i.image[j]);
  }
}

void PipelineBarrier::submit()
{
#if DAGOR_DBGLEVEL > 0
  if (dbgUseSplitSubmits)
    submitSplitted();
  else
#endif
  {
    for (const BarrierStageCache &i : cache.range())
    {
      // don't check src, src none barrier is ok
      G_ASSERTF(i.stage.dst, "vulkan: barrier should have dest stage");
      // do src less barrier as-is if synchronization2 is available
      // otherwise do full commands barrier
      VkPipelineStageFlags filteredSrc = i.stage.src;
      if (!Globals::VK::phy.hasSynchronization2 && filteredSrc == VK_PIPELINE_STAGE_NONE)
        filteredSrc = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

      if (cache.events.size() && !dependencyFlags)
        Backend::cb.wCmdWaitEvents(cache.events.size(), ary(cache.events.data()), filteredSrc, i.stage.dst, i.mem.size(), i.mem.data(),
          i.buffer.size(), i.buffer.data(), i.image.size(), i.image.data());
      else
        Backend::cb.wCmdPipelineBarrier(filteredSrc, i.stage.dst, dependencyFlags, i.mem.size(), i.mem.data(), i.buffer.size(),
          i.buffer.data(), i.image.size(), i.image.data());
    }
  }

  reset();
}

void PipelineBarrier::dbgPrint()
{
  debug("vulkan: pipeline barrier");

  for (BarrierStageCache &i : cache.range())
  {

    debug(" stages src: %s  dst: %s", formatPipelineStageFlags(i.stage.src), formatPipelineStageFlags(i.stage.dst));
    debug("  memory barriers: %u", i.mem.size());
    for (const VkMemoryBarrier &item : i.mem)
    {
      debug("    src: %s dst: %s", formatMemoryAccessFlags(item.srcAccessMask), formatMemoryAccessFlags(item.dstAccessMask));
    }

    debug("  buffer barriers: %u", i.buffer.size());
    for (const VkBufferMemoryBarrier &item : i.buffer)
    {
      debug("    src: %s dst: %s buffer: %p range [%u-%u]", formatMemoryAccessFlags(item.srcAccessMask),
        formatMemoryAccessFlags(item.dstAccessMask), item.buffer, item.offset, item.offset + item.size);
    }

    debug("  image barriers: %u", i.image.size());
    for (const VkImageMemoryBarrier &item : i.image)
    {
      debug("    src: %s dst: %s image: %p oldLayout: %s newLayout: %s", formatMemoryAccessFlags(item.srcAccessMask),
        formatMemoryAccessFlags(item.dstAccessMask), item.image, formatImageLayout(item.oldLayout), formatImageLayout(item.newLayout));
      debug("    mip_index: %u mip_range: %u array_index: %u array_range: %u", item.subresourceRange.baseMipLevel,
        item.subresourceRange.levelCount, item.subresourceRange.baseArrayLayer, item.subresourceRange.layerCount);
    }
  }
}

void PipelineBarrier::addImageByTemplateWithSrcAccess(VkAccessFlags mask)
{
  ImageTemplate &tpl = barrierTemplate.image;
  addImageByTemplate({mask, tpl.mask.dst});
}

void PipelineBarrier::addImageByTemplateWithDstAccess(VkAccessFlags mask)
{
  ImageTemplate &tpl = barrierTemplate.image;
  addImageByTemplate({tpl.mask.src, mask});
}

void PipelineBarrier::addImageByTemplate(AccessFlags mask)
{
  ImageTemplate &tpl = barrierTemplate.image;
  BarrierStageCache &stagedCache = nextCacheStage(cache, tpl.stage);
  VkImageMemoryBarrier &newBarrier = nextCacheElement(stagedCache.image);
  newBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  newBarrier.pNext = nullptr;
  newBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  newBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

  newBarrier.srcAccessMask = mask.src;
  newBarrier.dstAccessMask = mask.dst;
  newBarrier.oldLayout = tpl.oldLayout;
  newBarrier.newLayout = tpl.newLayout;
  newBarrier.image = tpl.handle;
  newBarrier.subresourceRange = tpl.subresRange;
}

void PipelineBarrier::addImageByTemplate()
{
  ImageTemplate &tpl = barrierTemplate.image;
  BarrierStageCache &stagedCache = nextCacheStage(cache, tpl.stage);
  VkImageMemoryBarrier &newBarrier = nextCacheElement(stagedCache.image);
  newBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  newBarrier.pNext = nullptr;
  newBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  newBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

  newBarrier.srcAccessMask = tpl.mask.src;
  newBarrier.dstAccessMask = tpl.mask.dst;
  newBarrier.oldLayout = tpl.oldLayout;
  newBarrier.newLayout = tpl.newLayout;
  newBarrier.image = tpl.handle;
  newBarrier.subresourceRange = tpl.subresRange;
}

void PipelineBarrier::addImageOwnershipTransferByTemplate(uint32_t src, uint32_t dst)
{
  ImageTemplate &tpl = barrierTemplate.image;
  BarrierStageCache &stagedCache = nextCacheStage(cache, tpl.stage);
  VkImageMemoryBarrier &newBarrier = nextCacheElement(stagedCache.image);
  newBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  newBarrier.pNext = nullptr;
  newBarrier.srcQueueFamilyIndex = src;
  newBarrier.dstQueueFamilyIndex = dst;

  newBarrier.srcAccessMask = tpl.mask.src;
  newBarrier.dstAccessMask = tpl.mask.dst;
  newBarrier.oldLayout = tpl.oldLayout;
  newBarrier.newLayout = tpl.newLayout;
  newBarrier.image = tpl.handle;
  newBarrier.subresourceRange = tpl.subresRange;
}
