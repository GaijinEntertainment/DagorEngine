// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include <util/dag_globDef.h>
#include <drv/3d/dag_texFlags.h>
#include <value_range.h>

#include "util/bits.h"
#include "vulkan_api.h"

namespace drv3d_vulkan
{

// stores formats and offers some utility members
BEGIN_BITFIELD_TYPE(FormatStore, uint8_t)
  enum
  {
    CREATE_FLAGS_FORMAT_SHIFT = 24,
    // one additional entry for the default
    MAX_FORMAT_COUNT = (TEXFMT_LAST_DEPTH >> CREATE_FLAGS_FORMAT_SHIFT) - (TEXFMT_A2R10G10B10 >> CREATE_FLAGS_FORMAT_SHIFT) + 1 + 1,
    BITS = BitsNeeded<MAX_FORMAT_COUNT>::VALUE
  };
  ADD_BITFIELD_MEMBER(isSrgb, BITS, 1)
  ADD_BITFIELD_MEMBER(linearFormat, 0, BITS)
  ADD_BITFIELD_MEMBER(index, 0, BITS + 1)

  BEGIN_BITFIELD_TYPE(CreateFlagFormat, uint32_t)
    ADD_BITFIELD_MEMBER(format, 24, 8)
    ADD_BITFIELD_MEMBER(srgbRead, 22, 1)
    ADD_BITFIELD_MEMBER(srgbWrite, 21, 1)
  END_BITFIELD_TYPE()

  FormatStore(VkFormat fmt) = delete;
  // only valid to call if fmt was tested with canBeStored
  static FormatStore fromVkFormat(VkFormat fmt);
  static FormatStore fromCreateFlags(uint32_t cflg)
  {
    FormatStore fmt;
    fmt.setFromFlagTexFlags(cflg);
    return fmt;
  }
  VkFormat asVkFormat() const;
  VkFormat getLinearFormat() const;
  VkFormat getSRGBFormat() const;
  bool isSrgbCapableFormatType() const;
  FormatStore getLinearVariant() const
  {
    FormatStore r = *this;
    r.isSrgb = 0;
    return r;
  }
  FormatStore getSRGBVariant() const
  {
    FormatStore r = *this;
    r.isSrgb = 1;
    return r;
  }
  void setFromFlagTexFlags(uint32_t flags)
  {
    CreateFlagFormat fmt(flags);
    linearFormat = fmt.format;
    isSrgb = fmt.srgbRead | fmt.srgbWrite;
  }
  uint32_t asTexFlags() const
  {
    CreateFlagFormat fmt;
    fmt.format = linearFormat;
    fmt.srgbRead = isSrgb;
    fmt.srgbWrite = isSrgb;
    return fmt.wrapper.value;
  }
  uint32_t getFormatFlags() { return linearFormat << CREATE_FLAGS_FORMAT_SHIFT; }
  VkImageAspectFlags getAspektFlags() const
  {
    uint8_t val = linearFormat;
    if (val == (TEXFMT_DEPTH16 >> CREATE_FLAGS_FORMAT_SHIFT) || val == (TEXFMT_DEPTH32 >> CREATE_FLAGS_FORMAT_SHIFT))
      return VK_IMAGE_ASPECT_DEPTH_BIT;
    constexpr auto range = make_value_range(TEXFMT_FIRST_DEPTH >> CREATE_FLAGS_FORMAT_SHIFT,
      (TEXFMT_LAST_DEPTH >> CREATE_FLAGS_FORMAT_SHIFT) - (TEXFMT_FIRST_DEPTH >> CREATE_FLAGS_FORMAT_SHIFT) + 1);

    if (range.isInside(val))
      return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    return VK_IMAGE_ASPECT_COLOR_BIT;
  }
  uint32_t calculateRowPitch(int32_t w) const
  {
    uint32_t blockSizeX;
    uint32_t blockSize = getBytesPerPixelBlock(&blockSizeX);
    return blockSize * ((w + blockSizeX - 1) / blockSizeX);
  }
  uint32_t calculateSlicePich(int32_t w, int32_t h) const
  {
    uint32_t blockSizeX, blockSizeY;
    uint32_t blockSize = getBytesPerPixelBlock(&blockSizeX, &blockSizeY);
    return blockSize * ((h + blockSizeY - 1) / blockSizeY) * ((w + blockSizeX - 1) / blockSizeX);
  }
  uint32_t calculateImageSize(int32_t w, int32_t h, int32_t d, int32_t levels) const
  {
    uint32_t total = 0;
    uint32_t bx, by;
    uint32_t bpp = getBytesPerPixelBlock(&bx, &by);
    for (int32_t l = 0; l < levels; ++l)
    {
      uint32_t wx = (max((w >> l), 1) + bx - 1) / bx;
      uint32_t wy = (max((h >> l), 1) + by - 1) / by;
      uint32_t wz = max((d >> l), 1);

      total += bpp * wx * wy * wz;
    }
    return total;
  }
  bool isSampledAsFloat() const
  {
    switch (linearFormat)
    {
      case TEXFMT_DEFAULT >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A2R10G10B10 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A2B10G10R10 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A16B16G16R16 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A16B16G16R16F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A32B32G32R32F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_G16R16 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_G16R16F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_G32R32F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R16F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R32F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_DXT1 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_DXT3 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_DXT5 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_L16 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A8 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R8 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A1R5G5B5 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A4R4G4B4 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R5G6B5 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A16B16G16R16S >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_ATI1N >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_ATI2N >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R8G8B8A8 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R11G11B10F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R9G9B9E5 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R8G8S >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R8G8 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_DEPTH24 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_DEPTH16 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_DEPTH32 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_DEPTH32_S8 >> CREATE_FLAGS_FORMAT_SHIFT: return true;
    }
    return false;
  }
  bool isFloat() const
  {
    switch (linearFormat)
    {
      case TEXFMT_A16B16G16R16F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A32B32G32R32F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_G16R16F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_G32R32F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R16F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R32F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R11G11B10F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R9G9B9E5 >> CREATE_FLAGS_FORMAT_SHIFT: return true;
      default: return false;
    }
  }
  bool isBlockFormat() const;
  uint32_t getBytesPerPixelBlock(uint32_t *block_x = NULL, uint32_t *block_y = NULL) const;
  const char *getNameString() const;
  // returns true if the format can be represented with this storage
  static bool canBeStored(VkFormat fmt);
  static void setRemapDepth24BitsToDepth32Bits(bool enable);
  static bool needMutableFormatForCreateFlags(uint32_t cf)
  {
    const uint32_t srgbrw = TEXCF_SRGBWRITE | TEXCF_SRGBREAD;
    // only if some of 2 bits are set means that for r/w we need to use different view format
    uint32_t anyBit = cf & srgbrw;
    if (!anyBit)
      return false;
    return (anyBit ^ srgbrw) != 0;
  }
END_BITFIELD_TYPE()
inline bool operator==(FormatStore l, FormatStore r) { return l.index == r.index; }
inline bool operator!=(FormatStore l, FormatStore r) { return l.index != r.index; }

} // namespace drv3d_vulkan
