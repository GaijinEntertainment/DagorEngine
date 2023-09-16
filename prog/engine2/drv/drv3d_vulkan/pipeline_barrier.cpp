
// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#include "pipeline_barrier.h"
#include "device.h"

using namespace drv3d_vulkan;

PipelineBarrier::BarrierCache BuiltinPipelineBarrierCache::data[BUILTIN_ELEMENTS] = {};

namespace
{

template <typename RetStructType>
RetStructType &nextCacheElement(Tab<RetStructType> &cacheArr, uint32_t &index, const RetStructType &initial)
{
  if (index >= cacheArr.size())
    cacheArr.push_back(initial);

  RetStructType &ret = cacheArr[index];
  ++index;
  return ret;
}

} // namespace

PipelineBarrier::PipelineBarrier(const VulkanDevice &in_device, BarrierCache &in_cache, VkPipelineStageFlags src_stages,
  VkPipelineStageFlags dst_stages) :
  device(in_device), pipeStages({src_stages, dst_stages}), cache(in_cache), dependencyFlags(0)
{
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

void PipelineBarrier::addStages(VkPipelineStageFlags src_stages, VkPipelineStageFlags dst_stages)
{
  pipeStages.src |= src_stages;
  pipeStages.dst |= dst_stages;
}

void PipelineBarrier::addDependencyFlags(VkDependencyFlags new_flag) { dependencyFlags |= new_flag; }

void PipelineBarrier::addMemory(AccessFlags mask)
{
  VkMemoryBarrier &newBarrier = nextCacheElement(cache.mem, barriersCount.mem, {VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr});

  newBarrier.srcAccessMask = mask.src;
  newBarrier.dstAccessMask = mask.dst;
}

void PipelineBarrier::addBuffer(AccessFlags mask, VulkanBufferHandle buf, VkDeviceSize offset, VkDeviceSize size)
{
  VkBufferMemoryBarrier &newBarrier = nextCacheElement(cache.buffer, barriersCount.buffer,
    {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, nullptr, 0, 0, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED});

  newBarrier.srcAccessMask = mask.src;
  newBarrier.dstAccessMask = mask.dst;

  newBarrier.buffer = buf;
  newBarrier.offset = offset;
  newBarrier.size = size;
}

void PipelineBarrier::addImage(AccessFlags mask, VulkanImageHandle img, VkImageLayout old_layout, VkImageLayout new_layout,
  const VkImageSubresourceRange &subresources)
{
  VkImageMemoryBarrier &newBarrier = nextCacheElement(cache.image, barriersCount.image,
    {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr, 0, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED});

  newBarrier.srcAccessMask = mask.src;
  newBarrier.dstAccessMask = mask.dst;
  newBarrier.oldLayout = old_layout;
  newBarrier.newLayout = new_layout;
  newBarrier.image = img;
  newBarrier.subresourceRange = subresources;
}

void PipelineBarrier::addBuffer(AccessFlags mask, const Buffer *buf, VkDeviceSize offset, VkDeviceSize size)
{
  addBuffer(mask, buf->getHandle(), offset, size);
}

void PipelineBarrier::modifyBufferTemplate(AccessFlags mask) { barrierTemplate.buffer.mask = mask; }

void PipelineBarrier::modifyBufferTemplate(const Buffer *buf) { barrierTemplate.buffer.handle = buf->getHandle(); }

void PipelineBarrier::modifyBufferTemplate(VulkanBufferHandle buf) { barrierTemplate.buffer.handle = buf; }

void PipelineBarrier::addBufferByTemplate(VkDeviceSize offset, VkDeviceSize size)
{
  BufferTemplate &tpl = barrierTemplate.buffer;
  addBuffer(tpl.mask, tpl.handle, offset, size);
}

void PipelineBarrier::modifyImageTemplate(AccessFlags mask, VkImageLayout old_layout, VkImageLayout new_layout)
{
  ImageTemplate &tpl = barrierTemplate.image;

  tpl.mask = mask;
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

void PipelineBarrier::addImageByTemplate(const VkImageSubresourceRange &subresources)
{
  ImageTemplate &tpl = barrierTemplate.image;
  addImage(tpl.mask, tpl.handle, tpl.oldLayout, tpl.newLayout, subresources);
}

void PipelineBarrier::reset()
{
  barriersCount.reset();
  pipeStages.src = 0;
  pipeStages.dst = 0;
}

bool PipelineBarrier::empty() { return !(barriersCount.mem || barriersCount.buffer || barriersCount.image); }

void PipelineBarrier::revertImageBarriers(AccessFlags mask, VkImageLayout shared_old_layout)
{
  pipeStages.src = 0;
  pipeStages.dst = 0;
  for (uint32_t i = 0; i < barriersCount.image; ++i)
  {
    VkImageMemoryBarrier &iter = cache.image[i];
    iter.srcAccessMask = mask.src;
    iter.dstAccessMask = mask.dst;
    iter.newLayout = iter.oldLayout;
    iter.oldLayout = shared_old_layout;
  }
}

void PipelineBarrier::submit(VulkanCommandBufferHandle cmd_buffer, bool keep_cache)
{
  G_ASSERT(pipeStages.src);
  G_ASSERT(pipeStages.dst);

  device.vkCmdPipelineBarrier(cmd_buffer, pipeStages.src, pipeStages.dst, dependencyFlags, barriersCount.mem, safe_at(cache.mem, 0),
    barriersCount.buffer, safe_at(cache.buffer, 0), barriersCount.image, safe_at(cache.image, 0));

  if (!keep_cache)
    reset();
}

void PipelineBarrier::dbgPrint()
{
  debug("vulkan: pipeline barrier");

  debug("  stages src: %s  dst: %s", formatPipelineStageFlags(pipeStages.src), formatPipelineStageFlags(pipeStages.dst));
  debug("  memory barriers: %u", barriersCount.mem);
  for (uint32_t i = 0; i < barriersCount.mem; ++i)
  {
    const VkMemoryBarrier &item = cache.mem[i];
    debug("    src: %s dst: %s", formatMemoryAccessFlags(item.srcAccessMask), formatMemoryAccessFlags(item.dstAccessMask));
  }

  debug("  buffer barriers: %u", barriersCount.buffer);
  for (uint32_t i = 0; i < barriersCount.buffer; ++i)
  {
    const VkBufferMemoryBarrier &item = cache.buffer[i];
    debug("    src: %s dst: %s buffer: %p", formatMemoryAccessFlags(item.srcAccessMask), formatMemoryAccessFlags(item.dstAccessMask),
      item.buffer);
  }

  debug("  image barriers: %u", barriersCount.image);
  for (uint32_t i = 0; i < barriersCount.image; ++i)
  {
    const VkImageMemoryBarrier &item = cache.image[i];
    debug("    src: %s dst: %s image: %p oldLayout: %s newLayout: %s", formatMemoryAccessFlags(item.srcAccessMask),
      formatMemoryAccessFlags(item.dstAccessMask), item.image, formatImageLayout(item.oldLayout), formatImageLayout(item.newLayout));
    debug("    mip_index: %u mip_range: %u array_index: %u array_range: %u", item.subresourceRange.baseMipLevel,
      item.subresourceRange.levelCount, item.subresourceRange.baseArrayLayer, item.subresourceRange.layerCount);
  }
}

const Image::StateLookupEntry image_state_info[SN_COUNT] = //
  {
    // SN_INITIAL = 0,
    Image::StateLookupEntry(0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT),
    // SN_SAMPLED_FROM = 1,
    Image::StateLookupEntry(VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT),
    // SN_RENDER_TARGET = 2,
    Image::StateLookupEntry(VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                              VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT),
    // SN_CONST_RENDER_TARGET = 3,
    Image::StateLookupEntry(
      VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
      VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT),
    // SN_COPY_FROM = 4,
    Image::StateLookupEntry(VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT),
    // SN_COPY_TO = 5,
    Image::StateLookupEntry(VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT),
    // SN_SHADER_WRITE = 6,
    Image::StateLookupEntry(VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
      VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT),
    // SN_WRITE_CLEAR = 7,
    Image::StateLookupEntry(VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT)};

const VkAccessFlags basic_render_target_access_mask =
  VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
// 0 => color target, 1 => depth stencil target
const VkAccessFlags render_target_access_mask[2] = //
  {basic_render_target_access_mask | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    basic_render_target_access_mask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT};

void PipelineBarrier::addImageByTemplate()
{
  ImageTemplate &tpl = barrierTemplate.image;
  VkImageMemoryBarrier &newBarrier = nextCacheElement(cache.image, barriersCount.image,
    {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr, 0, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED});

  newBarrier.srcAccessMask = tpl.mask.src;
  newBarrier.dstAccessMask = tpl.mask.dst;
  newBarrier.oldLayout = tpl.oldLayout;
  newBarrier.newLayout = tpl.newLayout;
  newBarrier.image = tpl.handle;
  newBarrier.subresourceRange = tpl.subresRange;
}

void PipelineBarrier::addImageByTemplate(ImageState old_state, ImageState new_state)
{
  ImageTemplate &tpl = barrierTemplate.image;

  // if either depth or stencil aspect is present, target is as d/s image use appropriate
  // layout
  const uint32_t layoutIndex = ((tpl.subresRange.aspectMask >> 1) | (tpl.subresRange.aspectMask >> 2)) & 1;
  const VkAccessFlags layoutAccessMask = render_target_access_mask[layoutIndex];

  addImage({VkAccessFlags(image_state_info[old_state].access & layoutAccessMask),
             VkAccessFlags(image_state_info[new_state].access & layoutAccessMask)},
    tpl.handle, VkImageLayout(uint32_t(image_state_info[old_state].layouts[layoutIndex])),
    VkImageLayout(uint32_t(image_state_info[new_state].layouts[layoutIndex])), tpl.subresRange);

  pipeStages.src |= image_state_info[old_state].stages[layoutIndex];
  pipeStages.dst |= image_state_info[new_state].stages[layoutIndex];
}

void PipelineBarrier::addImageByTemplateFiltered(ImageState old_state, ImageState new_state)
{
  if (old_state == new_state)
    return;

  VkImageAspectFlags aspects = barrierTemplate.image.subresRange.aspectMask;
  const uint32_t layoutIndex = ((aspects >> 1) | (aspects >> 2)) & 1;

  VkImageLayout oldLayout = VkImageLayout(uint32_t(image_state_info[old_state].layouts[layoutIndex]));
  VkImageLayout newLayout = VkImageLayout(uint32_t(image_state_info[new_state].layouts[layoutIndex]));

  if (oldLayout == newLayout)
    return;

  addImageByTemplate(old_state, new_state);
}

void PipelineBarrier::addImagePresent(ImageState old_state)
{
  addImageByTemplate(old_state, SN_COPY_FROM);

  // present barrier is very special, as vkQueuePresent does all visibility ops
  // ref: vkQueuePresentKHR spec notes
  pipeStages.dst = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  VkImageMemoryBarrier &lastIMB = cache.image[barriersCount.image - 1];
  lastIMB.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  lastIMB.dstAccessMask = 0;
}
