// Copyright (C) Gaijin Games KFT.  All rights reserved.
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
  RenderPassClass::Identifier renderPassClass = {};
  RenderPassClass::FramebufferDescription frameBufferInfo = {};
  VkClearColorValue colorClearValue[Driver3dRenderTarget::MAX_SIMRT] = {};
  VkClearDepthStencilValue depthStencilClearValue = {};

  static constexpr uint32_t CLEAR_LOAD = 1u << 31; // Special flag to allow loading of target's previous contents.
  uint32_t clearMode = 0;

  void reset()
  {
    renderPassClass.clear();
    frameBufferInfo.clear();
    memset(&colorClearValue, 0, sizeof(colorClearValue));
    memset(&depthStencilClearValue, 0, sizeof(depthStencilClearValue));
    clearMode = 0;
  }
};

class Swapchain;

} // namespace drv3d_vulkan
