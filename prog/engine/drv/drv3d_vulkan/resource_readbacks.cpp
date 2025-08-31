// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "resource_readbacks.h"
#include "globals.h"
#include "driver_config.h"
#include "backend.h"
#include "wrapped_command_buffer.h"
#include "pipeline_barrier.h"
#include "image_resource.h"
#include "device_queue.h"

using namespace drv3d_vulkan;

typedef ContextedPipelineBarrier<BuiltinPipelineBarrierCache::QFOT> QFOTPipelineBarrier;

void ResourceReadbacks::init()
{
  uploadDelay = Globals::cfg.bits.allowAsyncReadback ? 1 : 0;
  currentWriteIndex = uploadDelay;
  currentProcessIndex = 0;
}

void ResourceReadbacks::advance()
{
  currentProcessIndex = (currentProcessIndex + 1) % RING_SIZE;
  currentWriteIndex = (currentWriteIndex + 1) % RING_SIZE;

  G_ASSERTF(getForWrite().empty(), "vulkan: readback batch index %u was not processed!", currentWriteIndex);
}

void BatchedReadbacks::clear()
{
  bufferFlushes.clear();
  buffers.info.clear();
  buffers.copies.clear();
  images.info.clear();
  images.copies.clear();
}

bool BatchedReadbacks::empty() { return bufferFlushes.empty() && buffers.info.empty() && images.info.empty(); }

void ResourceReadbacks::shutdown()
{
  for (BatchedReadbacks &i : storage)
    i.clear();
}

void BatchedReadbacks::transferOwnership(DeviceQueueType src, DeviceQueueType dst)
{
  if (Globals::cfg.bits.ignoreQueueFamilyOwnershipTransferBarriers)
    return;

  uint32_t srcFamily = Globals::VK::queue[src].getFamily();
  uint32_t dstFamily = Globals::VK::queue[dst].getFamily();

  if (srcFamily == dstFamily)
    return;

  QFOTPipelineBarrier barrier;
  barrier.modifyImageTemplate(
    {VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT});
  barrier.modifyImageTemplateStage({VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT});
  for (ImageCopyInfo &i : images.info)
  {
    if (!i.image->isResident())
      continue;
    barrier.modifyImageTemplate(i.image);
    for (auto iter = begin(images.copies) + i.copyIndex, ed = begin(images.copies) + i.copyIndex + i.copyCount; iter != ed; ++iter)
    {
      for (uint32_t j = 0; j < iter->imageSubresource.layerCount; ++j)
      {
        barrier.modifyImageTemplate(iter->imageSubresource.mipLevel, 1, iter->imageSubresource.baseArrayLayer + j, 1);
        VkImageLayout intactLayout = i.image->layout.get(iter->imageSubresource.mipLevel, iter->imageSubresource.baseArrayLayer + j);
        barrier.modifyImageTemplate(intactLayout, intactLayout);
        barrier.addImageOwnershipTransferByTemplate(srcFamily, dstFamily);
      }
    }
  }
  barrier.submit();
}
