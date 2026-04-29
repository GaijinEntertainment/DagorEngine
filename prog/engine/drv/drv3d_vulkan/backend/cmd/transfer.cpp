// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "transfer.h"
#include "backend/context.h"
#include "util/fault_report.h"
#include "execution_sync.h"
#include "wrapped_command_buffer.h"
#include <EASTL/sort.h>

using namespace drv3d_vulkan;

TSPEC void BEContext::execCmd(const CmdCopyBufferToImageOrdered &cmd)
{
  beginCustomStage("orderedBufferToImageCopy");
  verifyResident(cmd.dst);
  verifyResident(cmd.src);

  const auto sz = cmd.dst->getFormat().calculateImageSize(cmd.region.imageExtent.width, cmd.region.imageExtent.height,
    cmd.region.imageExtent.depth, 1);
  Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, cmd.src, {cmd.region.bufferOffset, sz});
  Backend::sync.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, cmd.dst,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    {cmd.region.imageSubresource.mipLevel, 1, cmd.region.imageSubresource.baseArrayLayer, cmd.region.imageSubresource.layerCount});
  Backend::sync.completeNeeded();

  VULKAN_LOG_CALL(Backend::cb.wCmdCopyBufferToImage(cmd.src->getHandle(), cmd.dst->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1, &cmd.region));
}

TSPEC void BEContext::execCmd(const CmdCopyBuffer &cmd)
{
  beginCustomStage("CopyBuffer");
  verifyResident(cmd.source.buffer);
  verifyResident(cmd.dest.buffer);

  uint32_t sourceBufOffset = cmd.source.bufOffset(cmd.sourceOffset);
  uint32_t destBufOffset = cmd.dest.bufOffset(cmd.destOffset);

  Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, cmd.source.buffer,
    {sourceBufOffset, cmd.dataSize});
  Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, cmd.dest.buffer,
    {destBufOffset, cmd.dataSize});
  Backend::sync.completeNeeded();

  VkBufferCopy bc;
  bc.srcOffset = sourceBufOffset;
  bc.dstOffset = destBufOffset;
  bc.size = cmd.dataSize;
  VULKAN_LOG_CALL(Backend::cb.wCmdCopyBuffer(cmd.source.buffer->getHandle(), cmd.dest.buffer->getHandle(), 1, &bc));
}

TSPEC void BEContext::execCmd(const CmdFillBuffer &cmd)
{
  beginCustomStage("FillBuffer");
  fillBuffer(cmd.buffer.buffer, cmd.buffer.bufOffset(0), cmd.buffer.visibleDataSize, cmd.value);
}

TSPEC void BEContext::execCmd(const CmdClearDepthStencilTexture &cmd)
{
  // a transient image can't be cleard with vkCmdClear* command
  G_ASSERT((cmd.image->getUsage() & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) == 0);
  beginCustomStage("clearDepthStencilImage");

  verifyResident(cmd.image);

  Backend::sync.addImageWriteDiscard({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, cmd.image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {cmd.area.baseMipLevel, cmd.area.levelCount, cmd.area.baseArrayLayer, cmd.area.layerCount});
  Backend::sync.completeNeeded();

  VULKAN_LOG_CALL(
    Backend::cb.wCmdClearDepthStencilImage(cmd.image->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &cmd.value, 1, &cmd.area));
}

TSPEC void BEContext::execCmd(const CmdClearColorTexture &cmd)
{
  // a transient image can't be cleard with vkCmdClear* command
  G_ASSERT((cmd.image->getUsage() & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) == 0);

  beginCustomStage("clearColorImage");

  verifyResident(cmd.image);

  Backend::sync.addImageWriteDiscard({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, cmd.image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {cmd.area.baseMipLevel, cmd.area.levelCount, cmd.area.baseArrayLayer, cmd.area.layerCount});
  Backend::sync.completeNeeded();

  VULKAN_LOG_CALL(
    Backend::cb.wCmdClearColorImage(cmd.image->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &cmd.value, 1, &cmd.area));
}

bool BEContext::copyImageCmdTailBarrier(const CmdCopyImage &cmd)
{
  if (cmd.dst->isSampledSRV())
  {
    cmd.dst->layout.roSealTargetLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // we can't know is it readed or not when image is not GPU writable, so assume readed
    if (cmd.dst->isUsedInBindless())
    {
      trackBindlessRead(cmd.dst, {cmd.dstMip, cmd.mipCount, 0, cmd.dst->getArrayLayers()});
      return true;
    }
  }
  return false;
}

void BEContext::copyImageCmdMainAction(const CmdCopyImage &cmd)
{
  verifyResident(cmd.src);
  verifyResident(cmd.dst);

  Backend::sync.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, cmd.src,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, {cmd.srcMip, cmd.mipCount, 0, cmd.src->getArrayLayers()});
  Backend::sync.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, cmd.dst,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {cmd.dstMip, cmd.mipCount, 0, cmd.dst->getArrayLayers()});
  Backend::sync.completeNeeded();

  VULKAN_LOG_CALL(Backend::cb.wCmdCopyImage(cmd.src->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, cmd.dst->getHandle(),
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmd.regionCount, data->imageCopyInfos.data() + cmd.regionIndex));
}

TSPEC void BEContext::execCmd(const CmdCopyImage &cmd)
{
  beginCustomStage("CopyImage");
  copyImageCmdMainAction(cmd);
  if (copyImageCmdTailBarrier(cmd))
    Backend::sync.completeNeeded();
}

TSPEC void BEContext::execCmd(const CmdBlitImage &cmd)
{
  beginCustomStage("BlitImage");
  G_ASSERTF(cmd.src != cmd.dst,
    "Don't blit image on it self, if you need to build mipmaps, use the "
    "dedicated functions for it! img = %p:%s caller = %s",
    cmd.src, cmd.src->getDebugName(), getCurrentCmdCaller());
  ValueRange<uint8_t> srcMipRange(cmd.region.srcSubresource.mipLevel, cmd.region.srcSubresource.mipLevel + 1);
  ValueRange<uint8_t> dstMipRange(cmd.region.dstSubresource.mipLevel, cmd.region.dstSubresource.mipLevel + 1);
  ValueRange<uint16_t> srcArrayRange(cmd.region.srcSubresource.baseArrayLayer,
    cmd.region.srcSubresource.baseArrayLayer + cmd.region.srcSubresource.layerCount);
  ValueRange<uint16_t> dstArrayRange(cmd.region.dstSubresource.baseArrayLayer,
    cmd.region.dstSubresource.baseArrayLayer + cmd.region.dstSubresource.layerCount);

  verifyResident(cmd.src);
  verifyResident(cmd.dst);

  const VkFilter blitFilter = cmd.src->verifyLinearFilteringSupported("BlitImage") ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;

  Backend::sync.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, cmd.src,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    {cmd.region.srcSubresource.mipLevel, 1, cmd.region.srcSubresource.baseArrayLayer, cmd.region.srcSubresource.layerCount});

  if (cmd.whole_subres)
    Backend::sync.addImageWriteDiscard({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, cmd.dst,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      {cmd.region.dstSubresource.mipLevel, 1, cmd.region.dstSubresource.baseArrayLayer, cmd.region.dstSubresource.layerCount});
  else
    Backend::sync.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, cmd.dst,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      {cmd.region.dstSubresource.mipLevel, 1, cmd.region.dstSubresource.baseArrayLayer, cmd.region.dstSubresource.layerCount});
  Backend::sync.completeNeeded();

  VULKAN_LOG_CALL(Backend::cb.wCmdBlitImage(cmd.src->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, cmd.dst->getHandle(),
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &cmd.region, blitFilter));
}

void BEContext::processMipGenBatch()
{
  TIME_PROFILE(vulkan_processMipGenBatch);

  VkImageBlit blit;
  blit.srcSubresource.baseArrayLayer = 0;
  blit.srcOffsets[0].x = 0;
  blit.srcOffsets[0].y = 0;
  blit.srcOffsets[0].z = 0;
  blit.srcOffsets[1].z = 1;
  blit.dstSubresource.baseArrayLayer = 0;
  blit.dstOffsets[0].x = 0;
  blit.dstOffsets[0].y = 0;
  blit.dstOffsets[0].z = 0;
  blit.dstOffsets[1].z = 1;

  eastl::sort(scratch.mipGenList.begin(), scratch.mipGenList.end(),
    [](const Image *l, const Image *r) //
    { return l->getMipLevels() > r->getMipLevels(); });

  uint8_t maxMips = scratch.mipGenList[0]->getMipLevels();

  for (int mip = 1; mip < maxMips; ++mip)
  {
    for (Image *img : scratch.mipGenList)
    {
      if (img->getMipLevels() <= mip)
        break;

      Backend::sync.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, img,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, {(uint32_t)(mip - 1), 1, 0, img->getArrayLayers()});
      Backend::sync.addImageWriteDiscard({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, img,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {(uint32_t)mip, 1, 0, img->getArrayLayers()});
    }

    Backend::sync.completeNeeded();

    blit.srcSubresource.mipLevel = mip - 1;
    blit.dstSubresource.mipLevel = mip;

    for (Image *img : scratch.mipGenList)
    {
      if (img->getMipLevels() <= mip)
        break;

      blit.srcSubresource.aspectMask = img->getFormat().getAspektFlags();
      blit.srcSubresource.layerCount = img->getArrayLayers();
      blit.dstSubresource.aspectMask = blit.srcSubresource.aspectMask;
      blit.dstSubresource.layerCount = blit.srcSubresource.layerCount;

      // blit mip M-1 into mip M
      auto prevMipExtent = img->getMipExtents2D(mip - 1);
      auto mipExtent = img->getMipExtents2D(mip);

      blit.srcOffsets[1].x = prevMipExtent.width;
      blit.srcOffsets[1].y = prevMipExtent.height;
      blit.dstOffsets[1].x = mipExtent.width;
      blit.dstOffsets[1].y = mipExtent.height;

      const VkFilter blitFilter = img->verifyLinearFilteringSupported("GenerateMipmaps") ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;

      VULKAN_LOG_CALL(Backend::cb.wCmdBlitImage(img->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, img->getHandle(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, blitFilter));
    }
  }

  for (Image *img : scratch.mipGenList)
  {
    // we can't know is it readed or not, so assume readed
    if (img->isUsedInBindless())
      trackBindlessRead(img, {0, img->getMipLevels(), 0, img->getArrayLayers()});
  }
  Backend::sync.completeNeeded();
  scratch.mipGenList.clear();
}

TSPEC void BEContext::execCmd(const CmdGenerateMipmaps &cmd)
{
  beginCustomStage("GenerateMipmaps");
  verifyResident(cmd.img);

  if (data->cmds.isNextCommandSame(cmd))
  {
    scratch.mipGenList.push_back(cmd.img);
    return;
  }
  else
  {
    if (!scratch.mipGenList.empty())
    {
      scratch.mipGenList.push_back(cmd.img);
      processMipGenBatch();
      return;
    }
  }

  Backend::sync.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, cmd.img,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, {0, 1, 0, cmd.img->getArrayLayers()});
  Backend::sync.addImageWriteDiscard({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, cmd.img,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {1, 1, 0, cmd.img->getArrayLayers()});
  Backend::sync.completeNeeded();

  // blit mip 0 into mip 1
  VkImageBlit blit;
  blit.srcSubresource.aspectMask = cmd.img->getFormat().getAspektFlags();
  blit.srcSubresource.mipLevel = 0;
  blit.srcSubresource.baseArrayLayer = 0;
  blit.srcSubresource.layerCount = cmd.img->getArrayLayers();
  blit.srcOffsets[0].x = 0;
  blit.srcOffsets[0].y = 0;
  blit.srcOffsets[0].z = 0;
  blit.srcOffsets[1].x = cmd.img->getBaseExtent().width;
  blit.srcOffsets[1].y = cmd.img->getBaseExtent().height;
  blit.srcOffsets[1].z = 1;
  blit.dstSubresource.aspectMask = cmd.img->getFormat().getAspektFlags();
  blit.dstSubresource.mipLevel = 1;
  blit.dstSubresource.baseArrayLayer = 0;
  blit.dstSubresource.layerCount = cmd.img->getArrayLayers();
  blit.dstOffsets[0].x = 0;
  blit.dstOffsets[0].y = 0;
  blit.dstOffsets[0].z = 0;
  auto mipExtent = cmd.img->getMipExtents2D(1);
  blit.dstOffsets[1].x = mipExtent.width;
  blit.dstOffsets[1].y = mipExtent.height;
  blit.dstOffsets[1].z = 1;

  const VkFilter blitFilter = cmd.img->verifyLinearFilteringSupported("GenerateMipmaps") ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;

  VULKAN_LOG_CALL(Backend::cb.wCmdBlitImage(cmd.img->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, cmd.img->getHandle(),
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, blitFilter));

  if (cmd.img->getMipLevels() > 2)
  {
    auto lastMipExtent = mipExtent;
    for (auto &&m : cmd.img->getMipLevelRange().front(2))
    {
      Backend::sync.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, cmd.img,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, {(uint32_t)(m - 1), 1, 0, cmd.img->getArrayLayers()});
      Backend::sync.addImageWriteDiscard({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, cmd.img,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {(uint32_t)m, 1, 0, cmd.img->getArrayLayers()});
      Backend::sync.completeNeeded();

      // blit mip M-1 into mip M
      blit.srcSubresource.mipLevel = m - 1;
      blit.srcOffsets[1].x = lastMipExtent.width;
      blit.srcOffsets[1].y = lastMipExtent.height;

      auto mipExtent = cmd.img->getMipExtents2D(m);
      blit.dstSubresource.mipLevel = m;
      blit.dstOffsets[1].x = mipExtent.width;
      blit.dstOffsets[1].y = mipExtent.height;
      VULKAN_LOG_CALL(Backend::cb.wCmdBlitImage(cmd.img->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, cmd.img->getHandle(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, blitFilter));

      lastMipExtent = mipExtent;
    }
  }
  // we can't know is it readed or not, so assume readed
  if (cmd.img->isUsedInBindless())
  {
    trackBindlessRead(cmd.img, {0, cmd.img->getMipLevels(), 0, cmd.img->getArrayLayers()});
    Backend::sync.completeNeeded();
  }
}

TSPEC void BEContext::execCmd(const CmdResolveMultiSampleImage &cmd)
{
  beginCustomStage("ResolveMultiSampleImage");
  VkImageResolve area = {};
  area.srcSubresource.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
  area.srcSubresource.layerCount = 1;
  area.dstSubresource.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
  area.dstSubresource.layerCount = 1;
  area.extent = cmd.src->getBaseExtent();

  verifyResident(cmd.src);
  verifyResident(cmd.dst);

  Backend::sync.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, cmd.src,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, {0, 1, 0, 1});
  Backend::sync.addImageWriteDiscard({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, cmd.dst,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {0, 1, 0, 1});

  Backend::sync.completeNeeded();

  VULKAN_LOG_CALL(Backend::cb.wCmdResolveImage(cmd.src->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, cmd.dst->getHandle(),
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &area));
}

TSPEC void BEContext::execCmd(const CmdCaptureScreen &cmd)
{
  beginCustomStage("CaptureScreen");

  // do the copy to buffer
  auto extent = cmd.colorImage->getMipExtents2D(0);
  VkBufferImageCopy copy{};
  copy.bufferOffset = cmd.buffer->bufOffsetLoc(0);
  copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  copy.imageSubresource.layerCount = 1;
  copy.imageExtent.width = extent.width;
  copy.imageExtent.height = extent.height;
  copy.imageExtent.depth = 1;
  const auto copySize = cmd.colorImage->getFormat().calculateImageSize(copy.imageExtent.width, copy.imageExtent.height, 1, 1);

  verifyResident(cmd.colorImage);
  verifyResident(cmd.buffer);

  if (cmd.imageVkFormat)
    *cmd.imageVkFormat = cmd.colorImage->getFormat().asVkFormat();

  Backend::sync.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, cmd.colorImage,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, {0, 1, 0, 1});
  Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, cmd.buffer,
    {copy.bufferOffset, copySize});
  Backend::sync.completeNeeded();

  VULKAN_LOG_CALL(Backend::cb.wCmdCopyImageToBuffer(cmd.colorImage->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    cmd.buffer->getHandle(), 1, &copy));

  Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_HOST_BIT, VK_ACCESS_HOST_READ_BIT}, cmd.buffer, {copy.bufferOffset, copySize});
  Backend::sync.completeNeeded();
}
