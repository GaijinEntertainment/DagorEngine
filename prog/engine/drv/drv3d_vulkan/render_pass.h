#pragma once
#include <3d/dag_drv3d.h>
#include <generic/dag_staticTab.h>
#include <EASTL/vector.h>
#include "driver.h"
#include "vulkan_device.h"

namespace drv3d_vulkan
{
class Image;
struct FramebufferInfo
{
  struct AttachmentInfo
  {
    Image *image = nullptr;
    ImageViewState view = {};
  };
  AttachmentInfo colorAttachments[Driver3dRenderTarget::MAX_SIMRT] = {};
  AttachmentInfo depthStencilAttachment = {};
  uint8_t colorAttachmentMask = 0;
  uint8_t depthStencilMask = 0;

  void setColorAttachment(uint32_t index, Image *image, ImageViewState view)
  {
    auto &target = colorAttachments[index];
    target.image = image;
    target.view = view;
    colorAttachmentMask |= 1u << index;
  }
  void clearColorAttachment(uint32_t index)
  {
    colorAttachments[index] = AttachmentInfo{};
    colorAttachmentMask &= ~(1u << index);
  }
  void setDepthStencilAttachmentReadOnly(bool read_only)
  {
    // r/w -> 1
    // ro -> 2
    if (hasDepthStencilAttachment())
      depthStencilMask = 1u + uint8_t(read_only);
  }
  void setDepthStencilAttachment(Image *image, ImageViewState view, bool read_only)
  {
    depthStencilAttachment.image = image;
    depthStencilAttachment.view = view;
    depthStencilMask = 1u + uint8_t(read_only);
  }
  void clearDepthStencilAttachment()
  {
    depthStencilAttachment = AttachmentInfo{};
    depthStencilMask = 0;
  }
  bool hasConstDepthStencilAttachment() const { return 2 == depthStencilMask; }
  bool hasDepthStencilAttachment() const { return 0 != depthStencilMask; }
  VkExtent2D makeDrawArea(VkExtent2D def = {}) const;
};

class RenderPassClass
{
public:
  // Pipelines and framebuffers can be used with render passes of
  // the same compatibility class. If RenderPassClassItendifier
  // of two different passes match, then they are compatible
  // and can be used with the same pipeline and framebuffers
  // see section "7.2. Render Pass Compatibility" in vulkan
  // spec for details.
  struct Identifier
  {
    static constexpr int VERSION = 7;

    enum
    {
      NO_DEPTH = 0,
      RW_DEPTH = 1,
      RO_DEPTH = 2
    };

    FormatStore colorFormats[Driver3dRenderTarget::MAX_SIMRT];
    FormatStore depthStencilFormat;
    union
    {
      uint8_t colorSamples[Driver3dRenderTarget::MAX_SIMRT];
      uint64_t colorSamplesPacked;
    };

    static_assert(sizeof(colorSamples) <= sizeof(colorSamplesPacked),
      "size of colorSamplesPacked must be at least the size of colorSamples");

    uint8_t colorTargetMask;
    uint8_t depthState;

    Identifier() { clear(); }

    void clear()
    {
      for (auto &&cf : colorFormats)
        cf = FormatStore{};
      depthStencilFormat = FormatStore{};
      colorTargetMask = 0;
      depthState = NO_DEPTH;
      for (auto &&cs : colorSamples)
        cs = UCHAR_MAX;
    }

    friend bool operator==(const Identifier &l, const Identifier &r)
    {
      return l.colorTargetMask == r.colorTargetMask && l.depthState == r.depthState &&
             eastl::equal(eastl::begin(l.colorFormats), eastl::end(l.colorFormats), eastl::begin(r.colorFormats)) &&
             l.depthStencilFormat == r.depthStencilFormat && l.colorSamplesPacked == r.colorSamplesPacked;
    }
    friend bool operator!=(const Identifier &l, const Identifier &r) { return !(l == r); }

    struct Identity
    {
      template <typename T>
      auto operator()(T &&value) const
      {
        return eastl::forward<T>(value);
      }
    };
    // squashes the set of color targets into a linear set of color targets and depth stencil target
    template <typename R, typename T = R, typename F = Identity>
    StaticTab<R, Driver3dRenderTarget::MAX_SIMRT + 1> squash(size_t colorSize, const T *colorAttachments,
      const T &depthStencilAttachment, F &&functor = {}) const
    {
      StaticTab<R, Driver3dRenderTarget::MAX_SIMRT + 1> set;
      if (auto mask = colorTargetMask)
      {
        for (int i = 0; i < colorSize && mask; ++i, mask >>= 1)
          if (0 != (mask & 1))
            set.push_back(functor(colorAttachments[i]));
      }
      if (depthState != NO_DEPTH)
        set.push_back(functor(depthStencilAttachment));
      return set;
    }
  };
  struct FramebufferDescription
  {
    struct AttachmentInfo
    {
      enum
      {
        WIDTH_SHIFT = 0,
        WIDTH_BITS = 16,
        HEIGHT_SHIFT = WIDTH_SHIFT + WIDTH_BITS,
        HEIGHT_BITS = 16,
        MIP_SHIFT = HEIGHT_SHIFT + HEIGHT_BITS,
        MIP_BITS = 4,
        FORMAT_SHIFT = MIP_SHIFT + MIP_BITS,
        FORMAT_BITS = 8,
        LAYERS_SHIFT = FORMAT_SHIFT + FORMAT_BITS,
        LAYERS_BITS = 16,
        ATTACHMENT_INFO_SIZE = LAYERS_SHIFT + LAYERS_BITS
      };
      static_assert(ATTACHMENT_INFO_SIZE <= 64, "Data does not fit in 64 bits");
      BEGIN_BITFIELD_TYPE(ImageInfo, uint64_t)
        ADD_BITFIELD_MEMBER(width, WIDTH_SHIFT, WIDTH_BITS)
        ADD_BITFIELD_MEMBER(height, HEIGHT_SHIFT, HEIGHT_BITS)
        ADD_BITFIELD_MEMBER(mipLevel, MIP_SHIFT, MIP_BITS)
        ADD_BITFIELD_MEMBER(format, FORMAT_SHIFT, FORMAT_BITS)
        ADD_BITFIELD_MEMBER(layers, LAYERS_SHIFT, LAYERS_BITS)
      END_BITFIELD_TYPE()

      ImageInfo imageInfo{};

      // the usage of imageview and image flags are mutually exclusive
      // image flags use in case of imageless framebuffers and the imageview otherwise
      union
      {
        struct
        {
          VkImageUsageFlags usage;
          VkImageCreateFlags flags;
        };
        VulkanImageViewHandle imageView{};
      };

      AttachmentInfo();
      AttachmentInfo(const Image *image, VulkanImageViewHandle view, ImageViewState ivs);
    };

    AttachmentInfo colorAttachments[Driver3dRenderTarget::MAX_SIMRT] = {};
    AttachmentInfo depthStencilAttachment = {};
    VkExtent2D extent = {};

    void clear()
    {
      for (auto &&ca : colorAttachments)
        ca = AttachmentInfo{};
      depthStencilAttachment = AttachmentInfo{};
      extent.width = 0;
      extent.height = 0;
    }

    bool contains(VulkanImageViewHandle view) const;
    friend bool operator==(const FramebufferDescription &l, const FramebufferDescription &r)
    {
      return 0 == memcmp(&l, &r, sizeof(FramebufferDescription));
    }
    friend bool operator!=(const FramebufferDescription &l, const FramebufferDescription &r) { return !(l == r); }
  };

  const Identifier &getIdentifier() const { return identifier; }
  VulkanRenderPassHandle getPass(VulkanDevice &device, int clear_mask);
  VulkanFramebufferHandle getFrameBuffer(VulkanDevice &device, const FramebufferDescription &info);
  bool verifyPass(int clear_mask);
  RenderPassClass(const Identifier &id);
  void unloadAll(VulkanDevice &device);
  void unloadWith(VulkanDevice &device, VulkanImageViewHandle view);
  // no clear mask needed here, as its per attachment index, so for every one or no one (so don't
  // call this if no clear value is need)
  StaticTab<VkClearValue, Driver3dRenderTarget::MAX_SIMRT + 1> constructClearValueSet(const VkClearColorValue *ccv,
    const VkClearDepthStencilValue &cdsv) const;

private:
  struct FrameBuffer
  {
    FramebufferDescription desc;
    VulkanFramebufferHandle frameBuffer;
  };
  static constexpr uint32_t encode_variant(uint32_t clear_mask) { return clear_mask; }
  enum
  {
    // variats can only differ in clear mode
    // can't use encode_variant(CLEAR_TARGET | CLEAR_ZBUFFER | CLEAR_STENCIL, true) + 1
    // because vs fails to evaluate it...
    NUM_PASS_VARIANTS = 1 + (CLEAR_TARGET | CLEAR_ZBUFFER | CLEAR_STENCIL | CLEAR_DISCARD)
  };
  Identifier identifier = {};
  VulkanRenderPassHandle variants[NUM_PASS_VARIANTS] = {};
  eastl::vector<FrameBuffer> frameBuffers;

  VulkanRenderPassHandle compileVariant(VulkanDevice &device, int clear_mask);
  VulkanFramebufferHandle compileFrameBuffer(VulkanDevice &device, const FramebufferDescription &info);
};
class RenderPassManager
{
  eastl::vector<eastl::unique_ptr<RenderPassClass>> passClasses;

public:
  RenderPassClass *getPassClass(const RenderPassClass::Identifier &id);
  void unloadWith(VulkanDevice &device, VulkanImageViewHandle view);
  void unloadAll(VulkanDevice &device);
};
} // namespace drv3d_vulkan