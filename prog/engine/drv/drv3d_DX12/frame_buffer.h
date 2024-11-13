// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "bitfield.h"
#include "d3d12_utils.h"
#include "driver.h"
#include "extents.h"
#include "format_store.h"
#include "image_view_state.h"
#include "resource_manager/image.h"

#include <cstdint>
#include <debug/dag_assert.h>
#include <drv/3d/dag_decl.h>
#include <drv/3d/dag_texFlags.h>
#include <drv/shadersMetaData/dxil/compiled_shader_header.h>
#include <EASTL/algorithm.h>
#include <EASTL/optional.h>
#include <math/dag_lsbVisitor.h>


namespace drv3d_dx12
{
union FramebufferMask
{
  uint32_t raw = 0;
  struct
  {
    uint32_t hasDepthStencilAttachment : 1;
    uint32_t isConstantDepthstencilAttachment : 1;
    uint32_t colorAttachmentMask : Driver3dRenderTarget::MAX_SIMRT;
  };

  template <typename T>
  void iterateColorAttachments(T &&u)
  {
    for (auto i : LsbVisitor{colorAttachmentMask})
    {
      u(i);
    }
  }

  template <typename T>
  void iterateColorAttachments(uint32_t valid_mask, T &&u)
  {
    for (auto i : LsbVisitor{colorAttachmentMask & valid_mask})
    {
      u(i);
    }
  }

  void setColorAttachment(uint32_t index) { colorAttachmentMask |= 1u << index; }

  void resetColorAttachment(uint32_t index) { colorAttachmentMask &= ~(1u << index); }

  void setDepthStencilAttachment(bool is_const)
  {
    hasDepthStencilAttachment = 1;
    isConstantDepthstencilAttachment = is_const ? 1u : 0u;
  }

  void resetDepthStencilAttachment()
  {
    hasDepthStencilAttachment = 0;
    isConstantDepthstencilAttachment = 0;
  }

  void resetAll() { raw = 0; }
};


struct FramebufferInfo
{
  struct AttachmentInfo
  {
    Image *image = nullptr;
    ImageViewState view = {};
  };

  AttachmentInfo colorAttachments[Driver3dRenderTarget::MAX_SIMRT] = {};
  AttachmentInfo depthStencilAttachment = {};
  FramebufferMask attachmentMask;

  FramebufferMask getMatchingAttachmentMask(Image *texture)
  {
    FramebufferMask result;
    uint32_t i = 1;
    for (auto &colorAttachment : colorAttachments)
    {
      result.colorAttachmentMask |= colorAttachment.image == texture ? i : 0u;
      i <<= 1;
    }
    result.isConstantDepthstencilAttachment = attachmentMask.isConstantDepthstencilAttachment;
    result.hasDepthStencilAttachment = (depthStencilAttachment.image == texture) ? 1u : 0u;
    return result;
  }

  void setColorAttachment(uint32_t index, Image *image, ImageViewState view)
  {
    auto &target = colorAttachments[index];
    target.image = image;
    target.view = view;
    attachmentMask.setColorAttachment(index);
  }
  void clearColorAttachment(uint32_t index)
  {
    colorAttachments[index] = AttachmentInfo{};
    attachmentMask.resetColorAttachment(index);
  }
  void setDepthStencilAttachment(Image *image, ImageViewState view, bool read_only)
  {
    depthStencilAttachment.image = image;
    depthStencilAttachment.view = view;
    attachmentMask.setDepthStencilAttachment(read_only);
  }
  void clearDepthStencilAttachment()
  {
    depthStencilAttachment = AttachmentInfo{};
    attachmentMask.resetDepthStencilAttachment();
  }
  bool hasConstDepthStencilAttachment() const { return 0 != attachmentMask.isConstantDepthstencilAttachment; }
  bool hasDepthStencilAttachment() const { return 0 != attachmentMask.hasDepthStencilAttachment; }
  Extent2D makeDrawArea(Extent2D def = {}) const;
};


// This data structure is used as is for binary storage on disk for caches and its bit pattern is hashed,
// so its layout has to be tightly packed and avoid padding of members, otherwise initialized bits may
// yield different results for "equal" instances.
struct FramebufferLayout
{
  FormatStore colorFormats[Driver3dRenderTarget::MAX_SIMRT] = {};
  FormatStore depthStencilFormat = {};
  uint8_t colorTargetMask = 0;
  uint16_t colorMsaaLevels = 0;
  uint8_t hasDepth = 0;
  uint8_t depthMsaaLevel = 0;

  static constexpr int MSAA_LEVEL_BITS_PER_RT = BitsNeeded<(TEXCF_SAMPLECOUNT_MAX >> TEXCF_SAMPLECOUNT_OFFSET)>::VALUE;
  static_assert(sizeof(colorMsaaLevels) * 8 >= MSAA_LEVEL_BITS_PER_RT * Driver3dRenderTarget::MAX_SIMRT);
  static_assert(MSAA_LEVEL_BITS_PER_RT + 1 <= 8);

  static constexpr size_t expected_size = sizeof(uint8_t) * Driver3dRenderTarget::MAX_SIMRT + sizeof(uint8_t) + sizeof(uint8_t) +
                                          sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint8_t);

  void clear()
  {
    eastl::fill(eastl::begin(colorFormats), eastl::end(colorFormats), FormatStore{});
    depthStencilFormat = FormatStore{};
    colorTargetMask = 0;
    hasDepth = 0;
    depthMsaaLevel = 0;
    colorMsaaLevels = 0;
  }

  void setColorAttachment(uint8_t index, uint8_t msaa_level, FormatStore format)
  {
    colorTargetMask |= 1 << index;
    colorFormats[index] = format;
    setColorMsaaLevel(msaa_level, index);
  }

  void clearColorAttachment(uint8_t index)
  {
    colorTargetMask &= ~(1 << index);
    colorFormats[index] = FormatStore(0);
    clearColorMsaaLevel(index);
  }

  void setDepthStencilAttachment(uint8_t msaa_level, FormatStore format)
  {
    hasDepth = 1;
    depthStencilFormat = format;
    depthMsaaLevel = msaa_level;
  }

  void clearDepthStencilAttachment()
  {
    hasDepth = 0;
    depthStencilFormat = FormatStore(0);
    depthMsaaLevel = 0;
  }

  uint8_t getColorMsaaLevel(uint8_t index) const { return (colorMsaaLevels >> (index * MSAA_LEVEL_BITS_PER_RT)) & MSAA_LEVEL_MASK; }

  void checkMsaaLevelsEqual() const
  {
    eastl::optional<uint8_t> previous;
    for (const auto index : LsbVisitor{colorTargetMask})
    {
      G_ASSERT(!previous || *previous == getColorMsaaLevel(index));
      previous = getColorMsaaLevel(index);
    }
    G_ASSERT(!previous || !hasDepth || *previous == depthMsaaLevel);
  }

  // Calculates the color write mask for all targets, per target channel mask depends on format
  uint32_t getColorWriteMask() const
  {
    uint32_t mask = 0;
    for (auto i : LsbVisitor{colorTargetMask})
    {
      auto m = colorFormats[i].getChannelMask();
      mask |= m << (i * 4);
    }
    return mask;
  }

private:
  static constexpr int MSAA_LEVEL_MASK = TEXCF_SAMPLECOUNT_MASK >> TEXCF_SAMPLECOUNT_OFFSET;

  void clearColorMsaaLevel(uint8_t index) { colorMsaaLevels &= ~(MSAA_LEVEL_MASK << (index * MSAA_LEVEL_BITS_PER_RT)); }

  void setColorMsaaLevel(uint8_t level, uint8_t index)
  {
    clearColorMsaaLevel(index);
    colorMsaaLevels |= level << (index * MSAA_LEVEL_BITS_PER_RT);
  }
};

inline bool operator==(const FramebufferLayout &l, const FramebufferLayout &r)
{
  static_assert(sizeof(FramebufferLayout) == FramebufferLayout::expected_size);
  return (l.colorTargetMask == r.colorTargetMask) && (l.hasDepth == r.hasDepth) && (l.depthStencilFormat == r.depthStencilFormat) &&
         l.colorMsaaLevels == r.colorMsaaLevels && l.depthMsaaLevel == r.depthMsaaLevel &&
         eastl::equal(eastl::begin(l.colorFormats), eastl::end(l.colorFormats), eastl::begin(r.colorFormats));
}

inline bool operator!=(const FramebufferLayout &l, const FramebufferLayout &r) { return !(l == r); }


struct FramebufferLayoutWithHash
{
  FramebufferLayout layout;
  dxil::HashValue hash;
};

struct FramebufferDescription
{
  D3D12_CPU_DESCRIPTOR_HANDLE colorAttachments[Driver3dRenderTarget::MAX_SIMRT];
  D3D12_CPU_DESCRIPTOR_HANDLE depthStencilAttachment;

  void clear(D3D12_CPU_DESCRIPTOR_HANDLE null_color)
  {
    eastl::fill(eastl::begin(colorAttachments), eastl::end(colorAttachments), null_color);
    depthStencilAttachment.ptr = 0;
  }

  bool contains(D3D12_CPU_DESCRIPTOR_HANDLE view) const
  {
    using namespace eastl;
    return (end(colorAttachments) != find(begin(colorAttachments), end(colorAttachments), view)) || (depthStencilAttachment == view);
  }
};

inline bool operator==(const FramebufferDescription &l, const FramebufferDescription &r)
{
  return 0 == memcmp(&l, &r, sizeof(FramebufferDescription));
}
inline bool operator!=(const FramebufferDescription &l, const FramebufferDescription &r) { return !(l == r); }


struct FramebufferState
{
  FramebufferInfo frontendFrameBufferInfo = {};
  FramebufferLayout framebufferLayout = {};
  FramebufferDescription frameBufferInfo = {};
  FramebufferMask framebufferDirtyState;

  void dirtyTextureState(Image *texture)
  {
    auto state = frontendFrameBufferInfo.getMatchingAttachmentMask(texture);
    framebufferDirtyState.raw |= state.raw;
  }

  void dirtyAllTexturesState() { framebufferDirtyState = frontendFrameBufferInfo.attachmentMask; }

  void clear(D3D12_CPU_DESCRIPTOR_HANDLE null_color)
  {
    frontendFrameBufferInfo = FramebufferInfo{};
    framebufferLayout.clear();
    frameBufferInfo.clear(null_color);
    framebufferDirtyState.resetAll();
  }

  void bindColorTarget(uint32_t index, Image *image, ImageViewState ivs, D3D12_CPU_DESCRIPTOR_HANDLE view)
  {
    frontendFrameBufferInfo.setColorAttachment(index, image, ivs);
    framebufferLayout.setColorAttachment(index, image->getMsaaLevel(), ivs.getFormat());
    frameBufferInfo.colorAttachments[index] = view;
    framebufferDirtyState.setColorAttachment(index);
  }

  void clearColorTarget(uint32_t index, D3D12_CPU_DESCRIPTOR_HANDLE null_handle)
  {
    frontendFrameBufferInfo.clearColorAttachment(index);
    framebufferLayout.clearColorAttachment(index);
    frameBufferInfo.colorAttachments[index] = null_handle;
    framebufferDirtyState.resetColorAttachment(index);
  }

  void bindDepthStencilTarget(Image *image, ImageViewState ivs, D3D12_CPU_DESCRIPTOR_HANDLE view, bool read_only)
  {
    frontendFrameBufferInfo.setDepthStencilAttachment(image, ivs, read_only);
    framebufferLayout.setDepthStencilAttachment(image->getMsaaLevel(), ivs.getFormat());
    frameBufferInfo.depthStencilAttachment = view;
    framebufferDirtyState.setDepthStencilAttachment(read_only);
  }

  void clearDepthStencilTarget()
  {
    frontendFrameBufferInfo.clearDepthStencilAttachment();
    framebufferLayout.clearDepthStencilAttachment();
    frameBufferInfo.depthStencilAttachment.ptr = 0;
    framebufferDirtyState.resetDepthStencilAttachment();
  }
};
} // namespace drv3d_dx12
