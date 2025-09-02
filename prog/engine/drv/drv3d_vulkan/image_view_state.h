// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/dag_assert.h>
#include <debug/dag_fatal.h>
#include <value_range.h>
#include <drv_assert_defs.h>

#include "util/bits.h"
#include "format_store.h"

namespace drv3d_vulkan
{


BEGIN_BITFIELD_TYPE(ImageViewState, uint64_t)
  enum
  {
    WORD_SIZE = sizeof(uint64_t) * 8,
    SAMPLE_STENCIL_BITS = 1,
    SAMPLE_STENCIL_SHIFT = 0,
    IS_RENDER_TARGET_BITS = 1,
    IS_RENDER_TARGET_SHIFT = SAMPLE_STENCIL_BITS + SAMPLE_STENCIL_SHIFT,
    IS_UAV_TARGET_BITS = 1,
    IS_UAV_TARGET_SHIFT = IS_RENDER_TARGET_BITS + IS_RENDER_TARGET_SHIFT,
    IS_CUBEMAP_BITS = 1,
    IS_CUBEMAP_SHIFT = IS_UAV_TARGET_BITS + IS_UAV_TARGET_SHIFT,
    IS_ARRAY_BITS = 1,
    IS_ARRAY_SHIFT = IS_CUBEMAP_BITS + IS_CUBEMAP_SHIFT,
    FORMAT_BITS = FormatStore::BITS + 1,
    FORMAT_SHIFT = IS_ARRAY_SHIFT + IS_ARRAY_BITS,
    MIPMAP_OFFSET_BITS = BitsNeeded<15>::VALUE,
    MIPMAP_OFFSET_SHIFT = FORMAT_SHIFT + FORMAT_BITS,
    MIPMAP_RANGE_OFFSET = 1,
    MIPMAP_RANGE_BITS = BitsNeeded<16 - MIPMAP_RANGE_OFFSET>::VALUE,
    MIPMAP_RANGE_SHIFT = MIPMAP_OFFSET_SHIFT + MIPMAP_OFFSET_BITS,
    // automatic assign left over space to array range def
    ARRAY_DATA_SIZE = WORD_SIZE - MIPMAP_RANGE_SHIFT - MIPMAP_RANGE_BITS,
    ARRAY_OFFSET_BITS = (ARRAY_DATA_SIZE / 2) + (ARRAY_DATA_SIZE % 2),
    ARRAY_OFFSET_SHIFT = MIPMAP_RANGE_SHIFT + MIPMAP_RANGE_BITS,
    ARRAY_RANGE_OFFSET = 1,
    ARRAY_RANGE_BITS = ARRAY_DATA_SIZE / 2,
    ARRAY_RANGE_SHIFT = (ARRAY_OFFSET_SHIFT + ARRAY_OFFSET_BITS)
  };
  ADD_BITFIELD_MEMBER(sampleStencil, SAMPLE_STENCIL_SHIFT, SAMPLE_STENCIL_BITS)
  ADD_BITFIELD_MEMBER(isUAV, IS_UAV_TARGET_SHIFT, IS_UAV_TARGET_BITS)
  ADD_BITFIELD_MEMBER(isRenderTarget, IS_RENDER_TARGET_SHIFT, IS_RENDER_TARGET_BITS)
  ADD_BITFIELD_MEMBER(isCubemap, IS_CUBEMAP_SHIFT, IS_CUBEMAP_BITS)
  ADD_BITFIELD_MEMBER(isArray, IS_ARRAY_SHIFT, IS_ARRAY_BITS)
  ADD_BITFIELD_MEMBER(format, FORMAT_SHIFT, FORMAT_BITS)
  ADD_BITFIELD_MEMBER(mipmapOffset, MIPMAP_OFFSET_SHIFT, MIPMAP_OFFSET_BITS);
  ADD_BITFIELD_MEMBER(mipmapRange, MIPMAP_RANGE_SHIFT, MIPMAP_RANGE_BITS)
  ADD_BITFIELD_MEMBER(arrayOffset, ARRAY_OFFSET_SHIFT, ARRAY_OFFSET_BITS)
  ADD_BITFIELD_MEMBER(arrayRange, ARRAY_RANGE_SHIFT, ARRAY_RANGE_BITS)

  inline VkImageViewType getViewType(VkImageType type) const
  {
    switch (type)
    {
      case VK_IMAGE_TYPE_1D:
        D3D_CONTRACT_ASSERT(isCubemap == 0);
        D3D_CONTRACT_ASSERT(isRenderTarget == 0);
        if (isArray)
          return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
        D3D_CONTRACT_ASSERT(getArrayCount() == 1);
        return VK_IMAGE_VIEW_TYPE_1D;
      case VK_IMAGE_TYPE_2D:
        if (isCubemap)
        {
          if (isArray)
          {
            D3D_CONTRACT_ASSERT((getArrayCount() % 6) == 0);
            return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
          }
          else
          {
            D3D_CONTRACT_ASSERT(getArrayCount() == 6);
            return VK_IMAGE_VIEW_TYPE_CUBE;
          }
        }
        if (isArray)
        {
          return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        }
        D3D_CONTRACT_ASSERT(getArrayCount() == 1);
        return VK_IMAGE_VIEW_TYPE_2D;
      case VK_IMAGE_TYPE_3D:
        D3D_CONTRACT_ASSERT(isCubemap == 0);
        D3D_CONTRACT_ASSERT(isRenderTarget || getArrayBase() == 0);
        if (isRenderTarget)
        {
          if (isArray)
            return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
          else
          {
            D3D_CONTRACT_ASSERT(getArrayCount() == 1);
            return VK_IMAGE_VIEW_TYPE_2D;
          }
        }
        else
        {
          D3D_CONTRACT_ASSERT(getArrayCount() == 1);
          D3D_CONTRACT_ASSERT(isArray == 0);
          return VK_IMAGE_VIEW_TYPE_3D;
        }
      default:
        DAG_FATAL("Unexpected image type %u", type);
        return VK_IMAGE_VIEW_TYPE_2D;
        break;
    }
  }

  inline void setFormat(FormatStore fmt) { format = fmt.index; }
  inline FormatStore getFormat() const { return FormatStore(format); }
  inline void setMipBase(uint8_t u) { mipmapOffset = u; }
  inline uint8_t getMipBase() const { return mipmapOffset; }
  inline void setMipCount(uint8_t u) { mipmapRange = u - MIPMAP_RANGE_OFFSET; }
  inline uint8_t getMipCount() const { return MIPMAP_RANGE_OFFSET + mipmapRange; }
  ValueRange<uint8_t> getMipRange() const { return {getMipBase(), uint8_t(getMipBase() + getMipCount())}; }
  inline void setArrayBase(uint8_t u) { arrayOffset = u; }
  inline uint16_t getArrayBase() const { return arrayOffset; }
  inline void setArrayCount(uint16_t u) { arrayRange = u - ARRAY_RANGE_OFFSET; }
  inline uint16_t getArrayCount() const { return ARRAY_RANGE_OFFSET + (uint32_t)arrayRange; }
  ValueRange<uint16_t> getArrayRange() const { return {getArrayBase(), uint16_t(getArrayBase() + getArrayCount())}; }
END_BITFIELD_TYPE()
inline bool operator==(ImageViewState l, ImageViewState r) { return l.wrapper.value == r.wrapper.value; }
inline bool operator!=(ImageViewState l, ImageViewState r) { return l.wrapper.value != r.wrapper.value; }
inline bool operator<(ImageViewState l, ImageViewState r) { return l.wrapper.value < r.wrapper.value; }


} // namespace drv3d_vulkan
