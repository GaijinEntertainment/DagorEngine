// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_mathBase.h>
#include <math/dag_e3dColor.h>

#include "util/bits.h"
#include "vulkan_api.h"

namespace drv3d_vulkan
{


// packs a complete sampler state into 64 bits
BEGIN_BITFIELD_TYPE(SamplerState, uint64_t)
  static constexpr StorageType Invalid = -1;
  enum
  {
    BIAS_BITS = 32,
    BIAS_OFFSET = 0,
    MIP_BITS = 1,
    MIP_SHIFT = BIAS_OFFSET + BIAS_BITS,
    FILTER_BITS = 1,
    FILTER_SHIFT = MIP_SHIFT + MIP_BITS,
    ADDRESS_BITS = 3,
    ADDRESS_MASK = (1 << ADDRESS_BITS) - 1,
    COORD_COUNT = 3,
    COORD_BITS = 3,
    COORD_SHIFT = FILTER_SHIFT + FILTER_BITS,
    ANISO_BITS = 3,
    ANISO_SHIFT = COORD_SHIFT + COORD_BITS * COORD_COUNT,
    BORDER_BITS = 3,
    BORDER_SHIFT = ANISO_SHIFT + ANISO_BITS,
    IS_CMP_BITS = 1,
    IS_CMP_SHIFT = BORDER_BITS + BORDER_SHIFT,
    FLOAT_EXP_BASE = 127,
    FLOAT_EXP_SHIFT = 23,
  };
  ADD_BITFIELD_MEMBER(mipMapMode, MIP_SHIFT, MIP_BITS)
  ADD_BITFIELD_MEMBER(filterMode, FILTER_SHIFT, FILTER_BITS)
  ADD_BITFIELD_MEMBER(borderColorMode, BORDER_SHIFT, BORDER_BITS)
  ADD_BITFIELD_MEMBER(anisotropicValue, ANISO_SHIFT, ANISO_BITS)
  ADD_BITFIELD_ARRAY(coordMode, COORD_SHIFT, COORD_BITS, COORD_COUNT)
  ADD_BITFIELD_MEMBER(biasBits, BIAS_OFFSET, BIAS_BITS)
  ADD_BITFIELD_MEMBER(isCompare, IS_CMP_SHIFT, IS_CMP_BITS)

  BEGIN_BITFIELD_TYPE(iee754Float, uint32_t)
    float asFloat;
    uint32_t asUint;
    int32_t asInt;
    ADD_BITFIELD_MEMBER(mantissa, 0, 23)
    ADD_BITFIELD_MEMBER(exponent, 23, 8)
    ADD_BITFIELD_MEMBER(sign, 31, 1)
  END_BITFIELD_TYPE()

  //!! must match to d3d::SamplerInfo{}
  static SamplerState make_default()
  {
    SamplerState st;
    st.setMip(VK_SAMPLER_MIPMAP_MODE_LINEAR);
    st.setFilter(VK_FILTER_LINEAR);
    st.setU(VK_SAMPLER_ADDRESS_MODE_REPEAT);
    st.setV(VK_SAMPLER_ADDRESS_MODE_REPEAT);
    st.setW(VK_SAMPLER_ADDRESS_MODE_REPEAT);
    st.setBias(0.f);
    st.setAniso(1.f);
    st.setBorder(VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK);
    return st;
  }

  inline void setMip(VkSamplerMipmapMode mip) { mipMapMode = (uint32_t)mip; }
  inline VkSamplerMipmapMode getMip() const { return (VkSamplerMipmapMode)(uint32_t)mipMapMode; }
  inline void setFilter(VkFilter filter) { filterMode = filter; }
  inline VkFilter getFilter() const { return (VkFilter)(uint32_t)filterMode; }
  inline void setIsCompare(const bool isCompFilter) { isCompare = isCompFilter ? 1 : 0; }
  inline VkCompareOp getCompareOp() const { return isCompare ? VK_COMPARE_OP_LESS_OR_EQUAL : VK_COMPARE_OP_ALWAYS; }
  inline void setU(VkSamplerAddressMode u) { coordMode[0] = u; }
  inline VkSamplerAddressMode getU() const { return (VkSamplerAddressMode)(uint32_t)coordMode[0]; }
  inline void setV(VkSamplerAddressMode v) { coordMode[1] = v; }
  inline VkSamplerAddressMode getV() const { return (VkSamplerAddressMode)(uint32_t)coordMode[1]; }
  inline void setW(VkSamplerAddressMode w) { coordMode[2] = w; }
  inline VkSamplerAddressMode getW() const { return (VkSamplerAddressMode)(uint32_t)coordMode[2]; }
  inline void setBias(float b)
  {
    iee754Float f;
    f.asFloat = b;
    biasBits = f.asUint;
  }
  inline float getBias() const
  {
    iee754Float f;
    f.asUint = biasBits;
    return f.asFloat;
  }
  inline void setAniso(float a)
  {
    // some float magic, falls flat on its face if it is not ieee-754
    // extracts exponent and subtracts the base
    // clamps the result into range from  0 to 4 which represents 1,2,4,8 and 16 as floats
    // negative values are treated as positive
    // values from range 0 - 1 are rounded up
    // everything else is rounded down
    iee754Float f;
    f.asFloat = a;
    int32_t value = f.exponent - FLOAT_EXP_BASE;
    // clamp from 1 to 16
    value = clamp(value, 0, 4);
    anisotropicValue = value;
  }
  inline float getAniso() const
  {
    iee754Float f;
    f.exponent = FLOAT_EXP_BASE + anisotropicValue;
    return f.asFloat;
  }
  inline void setBorder(VkBorderColor b) { borderColorMode = b; }
  inline VkBorderColor getBorder() const { return (VkBorderColor)(uint32_t)borderColorMode; }

  static constexpr VkBorderColor color_table[] = //
    {VK_BORDER_COLOR_INT_OPAQUE_WHITE, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, VK_BORDER_COLOR_INT_OPAQUE_BLACK,
      VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK, VK_BORDER_COLOR_INT_TRANSPARENT_BLACK, VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
      VK_BORDER_COLOR_INT_TRANSPARENT_BLACK, VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK};
  static VkBorderColor remap_border_color(E3DCOLOR color, bool as_float)
  {
    bool isBlack = (color.r == 0) && (color.g == 0) && (color.b == 0);
    bool isTransparent = color.a == 0;
    return color_table[int(as_float) | (int(isBlack) << 1) | (int(isTransparent) << 2)];
  }
END_BITFIELD_TYPE()

} // namespace drv3d_vulkan
