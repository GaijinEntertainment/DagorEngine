// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "bitfield.h"
#include "d3d12_utils.h"
#include "driver.h"
#include "extents.h"
#include "format_store.h"
#include "image_view_state.h"
#include "resource_manager/image.h"
#include "render_target_mask_util.h"

#include <cstdint>
#include <debug/dag_assert.h>
#include <drv/3d/dag_decl.h>
#include <drv/3d/dag_texFlags.h>
#include <drv/shadersMetaData/dxil/compiled_shader_header.h>
#include <EASTL/algorithm.h>
#include <EASTL/optional.h>
#include <math/dag_lsbVisitor.h>
#include <generic/dag_enumerate.h>


namespace drv3d_dx12
{
class FramebufferMask
{
  uint32_t hasDepthStencilAttachmentBit : 1 = 0;
  uint32_t isConstantDepthstencilAttachmentBit : 1 = 0;
  uint32_t colorAttachmentMask : Driver3dRenderTarget::MAX_SIMRT = 0;
  uint32_t padd : 32 - Driver3dRenderTarget::MAX_SIMRT - 2 = 0;

public:
  void combineWith(FramebufferMask other)
  {
    hasDepthStencilAttachmentBit |= other.hasDepthStencilAttachmentBit;
    isConstantDepthstencilAttachmentBit |= other.isConstantDepthstencilAttachmentBit;
    colorAttachmentMask |= other.colorAttachmentMask;
  }
  RenderTargetMask getRenderTargetMask() const { return RenderTargetMask::make(colorAttachmentMask); }

  template <typename T>
  void iterateColorAttachments(T &&u)
  {
    for (auto i : getRenderTargetMask().visit())
    {
      u(i);
    }
  }

  template <typename T>
  void iterateColorAttachments(RenderTargetMask valid_mask, T &&u)
  {
    for (auto i : (getRenderTargetMask() & valid_mask).visit())
    {
      u(i);
    }
  }

  void setColorAttachment(uint32_t index) { colorAttachmentMask |= 1u << index; }
  void resetColorAttachment(uint32_t index) { colorAttachmentMask &= ~(1u << index); }

  void maskOffColorAttachments(RenderTargetMask mask)
  {
    // PVS fails to see that colorAttachmentMask is a bitmask of 8 bits, which is the same as mask
    colorAttachmentMask &= mask.raw(); // -V784
  }

  void setDepthStencilAttachment(bool is_const)
  {
    hasDepthStencilAttachmentBit = 1;
    isConstantDepthstencilAttachmentBit = is_const ? 1u : 0u;
  }

  void resetDepthStencilAttachment()
  {
    hasDepthStencilAttachmentBit = 0;
    isConstantDepthstencilAttachmentBit = 0;
  }

  bool isConstantDepthstencilAttachment() const { return 0 != isConstantDepthstencilAttachmentBit; }
  bool hasDepthStencilAttachment() const { return 0 != hasDepthStencilAttachmentBit; }

  void resetAll()
  {
    hasDepthStencilAttachmentBit = 0;
    isConstantDepthstencilAttachmentBit = 0;
    colorAttachmentMask = 0;
  }
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
    for (auto [i, e] : enumerate(colorAttachments))
    {
      if (e.image == texture)
      {
        result.setColorAttachment(i);
      }
    }
    if (depthStencilAttachment.image == texture)
    {
      result.setDepthStencilAttachment(attachmentMask.isConstantDepthstencilAttachment());
    }
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
  bool hasConstDepthStencilAttachment() const { return attachmentMask.isConstantDepthstencilAttachment(); }
  bool hasDepthStencilAttachment() const { return attachmentMask.hasDepthStencilAttachment(); }
  Extent2D makeDrawArea(Extent2D def = {}) const;
};


// This data structure is used as is for binary storage on disk for caches and its bit pattern is hashed,
// so its layout has to be tightly packed and avoid padding of members, otherwise initialized bits may
// yield different results for "equal" instances.
struct FramebufferLayout
{
  FormatStore colorFormats[Driver3dRenderTarget::MAX_SIMRT] = {};
  FormatStore depthStencilFormat = {};
  RenderTargetMask colorTargetMask = RenderTargetMask::make(0);
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
    colorTargetMask.clear();
    hasDepth = 0;
    depthMsaaLevel = 0;
    colorMsaaLevels = 0;
  }

  void setColorAttachment(uint8_t index, uint8_t msaa_level, FormatStore format)
  {
    colorTargetMask.set(index);
    colorFormats[index] = format;
    setColorMsaaLevel(msaa_level, index);
  }

  void clearColorAttachment(uint8_t index)
  {
    colorTargetMask.unset(index);
    colorFormats[index] = FormatStore{};
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
    depthStencilFormat = FormatStore{};
    depthMsaaLevel = 0;
  }

  uint8_t getColorMsaaLevel(uint8_t index) const { return (colorMsaaLevels >> (index * MSAA_LEVEL_BITS_PER_RT)) & MSAA_LEVEL_MASK; }

  void checkMsaaLevelsEqual() const
  {
    eastl::optional<uint8_t> previous;
    for (const auto index : colorTargetMask.visit())
    {
      G_ASSERT(!previous || *previous == getColorMsaaLevel(index));
      previous = getColorMsaaLevel(index);
    }
    G_ASSERT(!previous || !hasDepth || *previous == depthMsaaLevel);
  }

  // Calculates the color write mask for all targets, per target channel mask depends on format
  FrameBufferColorChannelMask getColorWriteMask() const
  {
    auto mask = FrameBufferColorChannelMask::make(0);
    for (auto i : colorTargetMask.visit())
    {
      mask.set(i, colorFormats[i].getChannelMask());
    }
    return mask;
  }

  bool isSupported() const
  {
    bool ok = true;
    supportCheckVisit([&ok](auto, auto, auto result) { ok = ok && SupportCheckResult::Pass == result; });
    return ok;
  }

  void reportUnsupportedFormats() const
  {
    supportCheckVisit([](auto slot, auto format, auto result) {
      const char *reason = nullptr;
      switch (result)
      {
        case SupportCheckResult::Pass: return;
        case SupportCheckResult::NoSupport: reason = "no hardware support"; break;
        case SupportCheckResult::NoMsaaSupport: reason = "no MSAA level support"; break;
      }
      if (Driver3dRenderTarget::MAX_SIMRT > slot)
      {
        logdbg("DX12: %s of render target slot %u unsupported, because %s", format.template getNameString<false>(), slot, reason);
      }
      else
      {
        logdbg("DX12: %s of depth stencil target unsupported, because %s", format.template getNameString<false>(), reason);
      }
    });
  }

private:
  enum SupportCheckResult
  {
    Pass,
    NoSupport,
    NoMsaaSupport,
  };
  void supportCheckVisit(auto &&visit) const
  {
    for (auto i : colorTargetMask.visit())
    {
      auto &fmt = colorFormats[i];
      SupportCheckResult result = SupportCheckResult::Pass;
      if (!fmt.isSupportedRenderTarget())
      {
        result = SupportCheckResult::NoSupport;
      }
      else if (!fmt.isSampleCountSupported(1u << getColorMsaaLevel(i)))
      {
        result = SupportCheckResult::NoMsaaSupport;
      }
      visit(i, colorFormats[i], result);
    }
    if (hasDepth)
    {
      SupportCheckResult result = SupportCheckResult::Pass;
      if (!depthStencilFormat.isSupportedDepthStencil())
      {
        result = SupportCheckResult::NoSupport;
      }
      else if (!depthStencilFormat.isSampleCountSupported(1u << depthMsaaLevel))
      {
        result = SupportCheckResult::NoMsaaSupport;
      }
      visit(Driver3dRenderTarget::MAX_SIMRT, depthStencilFormat, result);
    }
  }
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
    framebufferDirtyState.combineWith(state);
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
