#pragma once
#include "pipeline.h"

namespace drv3d_vulkan
{
struct ViewportState
{
  VkRect2D rect2D;
  float minZ;
  float maxZ;

  friend inline bool operator==(const ViewportState &l, const ViewportState &r)
  {
    return l.rect2D == r.rect2D && l.minZ == r.minZ && l.maxZ == r.maxZ;
  }
  friend inline bool operator!=(const ViewportState &l, const ViewportState &r)
  {
    return l.rect2D != r.rect2D || l.minZ != r.minZ || l.maxZ != r.maxZ;
  }
};
enum class RegionDifference
{
  EQUAL,
  SUBSET,
  SUPERSET
};
inline RegionDifference classify_rect2d_diff(const VkRect2D &from, const VkRect2D &to)
{
  const int32_t dX = to.offset.x - from.offset.x;
  const int32_t dY = to.offset.y - from.offset.y;
  const int32_t dW = (to.extent.width + to.offset.x) - (from.extent.width + from.offset.x);
  const int32_t dH = (to.extent.height + to.offset.y) - (from.extent.height + from.offset.y);
  // if all zero, then they are the same
  if (dX | dY | dW | dH)
  {
    // can be either subset or completly different
    if (dX >= 0 && dY >= 0 && dW <= 0 && dH <= 0)
      return RegionDifference::SUBSET;
    else
      return RegionDifference::SUPERSET;
  }

  return RegionDifference::EQUAL;
}
inline RegionDifference classify_viewport_diff(const ViewportState &from, const ViewportState &to)
{
  const RegionDifference rectDif = classify_rect2d_diff(from.rect2D, to.rect2D);
  if (RegionDifference::SUPERSET != rectDif)
  {
    // min/max z only affect viewport but not render regions, so it is always a subset if it has
    // changed
    if (to.maxZ != from.maxZ || to.minZ != from.minZ)
      return RegionDifference::SUBSET;
  }
  return rectDif;
}
struct FramebufferState
{
  FramebufferInfo frontendFrameBufferInfo = {};
  RenderPassClass::Identifier renderPassClass = {};
  RenderPassClass::FramebufferDescription frameBufferInfo = {};
  VkRect2D viewRect = {};
  VkClearColorValue colorClearValue[Driver3dRenderTarget::MAX_SIMRT] = {};
  VkClearDepthStencilValue depthStencilClearValue = {};
  VulkanImageViewHandle colorViews[Driver3dRenderTarget::MAX_SIMRT] = {};
  VulkanImageViewHandle depthStencilView = {};
  uint32_t clearMode = 0;
  bool roDepth = false;

  void clear()
  {
    frontendFrameBufferInfo = FramebufferInfo{};
    renderPassClass.clear();
    frameBufferInfo.clear();
    memset(&viewRect, 0, sizeof(viewRect));
    memset(&colorClearValue, 0, sizeof(colorClearValue));
    memset(&depthStencilClearValue, 0, sizeof(depthStencilClearValue));
  }

  void bindColorTarget(uint32_t index, Image *image, VkSampleCountFlagBits samples, ImageViewState ivs, VulkanImageViewHandle view)
  {
    frontendFrameBufferInfo.setColorAttachment(index, image, ivs);
    renderPassClass.colorTargetMask |= 1 << index;
    renderPassClass.colorFormats[index] = ivs.getFormat();
    renderPassClass.colorSamples[index] = samples;
    frameBufferInfo.colorAttachments[index] = RenderPassClass::FramebufferDescription::AttachmentInfo(image, view, ivs);
    colorViews[index] = view;
  }

  void clearColorTarget(uint32_t index)
  {
    frontendFrameBufferInfo.clearColorAttachment(index);
    renderPassClass.colorTargetMask &= ~(1 << index);
    renderPassClass.colorFormats[index] = FormatStore(0);
    frameBufferInfo.colorAttachments[index] = RenderPassClass::FramebufferDescription::AttachmentInfo();
    colorViews[index] = VulkanNullHandle();
  }

  void setDepthStencilTargetReadOnly(bool read_only)
  {
    frontendFrameBufferInfo.setDepthStencilAttachmentReadOnly(read_only);

    if (renderPassClass.depthState != RenderPassClass::Identifier::NO_DEPTH)
    {
      renderPassClass.depthState = read_only ? RenderPassClass::Identifier::RO_DEPTH : RenderPassClass::Identifier::RW_DEPTH;
    }
  }

  void bindDepthStencilTarget(Image *image, VkSampleCountFlagBits samples, ImageViewState ivs, VulkanImageViewHandle view,
    bool read_only)
  {
    frontendFrameBufferInfo.setDepthStencilAttachment(image, ivs, read_only);
    renderPassClass.depthState = read_only ? RenderPassClass::Identifier::RO_DEPTH : RenderPassClass::Identifier::RW_DEPTH;
    renderPassClass.depthStencilFormat = ivs.getFormat();
    renderPassClass.colorSamples[0] = samples;
    frameBufferInfo.depthStencilAttachment = RenderPassClass::FramebufferDescription::AttachmentInfo(image, view, ivs);
    depthStencilView = view;
  }

  void clearDepthStencilTarget()
  {
    frontendFrameBufferInfo.clearDepthStencilAttachment();
    renderPassClass.depthState = RenderPassClass::Identifier::NO_DEPTH;
    renderPassClass.depthStencilFormat = FormatStore(0);
    frameBufferInfo.depthStencilAttachment = RenderPassClass::FramebufferDescription::AttachmentInfo();
    depthStencilView = VulkanNullHandle();
  }
};

class Swapchain;

} // namespace drv3d_vulkan

// FIXME: merge this files
#include "graphics_state2.h"