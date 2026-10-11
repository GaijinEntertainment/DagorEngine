// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <cstdint>
#include <compare>
#include <math/dag_lsbVisitor.h>

namespace drv3d_dx12
{
namespace detail
{
// Input is 4x8 bits color channel mask and output will be a 8bit mask of render targets
inline constexpr uint32_t color_channel_mask_to_render_target_mask(uint32_t mask)
{
  // For each color channel generate the used bit
  const uint32_t channel0 = mask >> 0;
  const uint32_t channel1 = mask >> 1;
  const uint32_t channel2 = mask >> 2;
  const uint32_t channel3 = mask >> 3;
  // At this point the lower bit of each 4 bit block is now indicating if a target is used or not
  const uint32_t channelsToSpacedTargetMask = channel0 | channel1 | channel2 | channel3;
  // This erases the top 3 bits of each 4 bit block to compress from 4x8 bits to 8 bits.
  const uint32_t t0 = (channelsToSpacedTargetMask >> 0) & 0x00000001;
  const uint32_t t1 = (channelsToSpacedTargetMask >> 3) & 0x00000002;
  const uint32_t t2 = (channelsToSpacedTargetMask >> 6) & 0x00000004;
  const uint32_t t3 = (channelsToSpacedTargetMask >> 9) & 0x00000008;
  const uint32_t t4 = (channelsToSpacedTargetMask >> 12) & 0x00000010;
  const uint32_t t5 = (channelsToSpacedTargetMask >> 15) & 0x00000020;
  const uint32_t t6 = (channelsToSpacedTargetMask >> 18) & 0x00000040;
  const uint32_t t7 = (channelsToSpacedTargetMask >> 21) & 0x00000080;
  const uint32_t combinedTargetMask = t0 | t1 | t2 | t3 | t4 | t5 | t6 | t7;
  return combinedTargetMask;
}

// Inputs a 8 bit mask of render targets and outputs a 4x8 channel mask, where if a target bit is
// set all corresponding channel bits will be set
inline constexpr uint32_t render_target_mask_to_color_channel_mask(uint32_t mask)
{
  // Spread out the individual target bits into the lowest bit of each corresponding 4 bit block,
  // which is the indicator bit for the first channel (r)
  const uint32_t t0 = (mask & 0x00000001) << 0;
  const uint32_t t1 = (mask & 0x00000002) << 3;
  const uint32_t t2 = (mask & 0x00000004) << 6;
  const uint32_t t3 = (mask & 0x00000008) << 9;
  const uint32_t t4 = (mask & 0x00000010) << 12;
  const uint32_t t5 = (mask & 0x00000020) << 15;
  const uint32_t t6 = (mask & 0x00000040) << 18;
  const uint32_t t7 = (mask & 0x00000080) << 21;
  const uint32_t r = t0 | t1 | t2 | t3 | t4 | t5 | t6 | t7;
  // Replicate indicator bits from first channel (r) to all others (g, b and a)
  const uint32_t g = r << 1;
  const uint32_t b = r << 2;
  const uint32_t a = r << 3;
  return r | g | b | a;
}

// Takes a 4x8 bit render target output channel mask and turns it into a 4x8 render target output mask
// where if any channel of a target is enabled all channels of the result are enabled.
// Simply speaking it turns all non 0 hex digits in the mask into F and all 0 are kept as 0.
// Yields the same value as render_target_mask_to_color_channel_mask(color_channel_mask_to_render_target_mask(mask)),
// but more efficient.
inline constexpr uint32_t spread_color_chanel_mask_to_render_target_color_channel_mask(uint32_t mask)
{
  const uint32_t r = mask & 0x11111111;
  const uint32_t g = mask & 0x22222222;
  const uint32_t b = mask & 0x44444444;
  const uint32_t a = mask & 0x88888888;
  const uint32_t r1 = r | (r << 1) | (r << 2) | (r << 3);
  const uint32_t g1 = g | (g << 1) | (g << 2) | (g >> 1);
  const uint32_t b1 = b | (b << 1) | (b >> 1) | (b >> 2);
  const uint32_t a1 = a | (a >> 1) | (a >> 2) | (a >> 3);
  return r1 | g1 | b1 | a1;
}
} // namespace detail

class ColorChannelMask
{
  uint8_t value = 0;

public:
  constexpr static ColorChannelMask make(uint8_t value)
  {
    ColorChannelMask result;
    result.value = value & 0xF;
    return result;
  }
  constexpr explicit operator bool() const { return 0 != value; }
  constexpr ColorChannelMask operator&(const ColorChannelMask &o) const { return make(value & o.value); }
  constexpr ColorChannelMask operator|(const ColorChannelMask &o) const { return make(value | o.value); }
  constexpr ColorChannelMask operator^(const ColorChannelMask &o) const { return make(value ^ o.value); }
  constexpr ColorChannelMask operator~() const { return make(~value); }
  constexpr auto operator<=>(const ColorChannelMask &) const = default;
  constexpr void clear() { value = 0; }
  constexpr auto visit() const { return LsbVisitor{value}; }
  constexpr bool get(uint32_t index) const { return 0 != (value & (1u << index)); }
  constexpr void set(uint32_t index) { value = (value | (1u << index)) & 0xF; }
  constexpr void unset(uint32_t index) { value &= ~(1u << index); }
  constexpr auto raw() const { return value; }
  operator DagorSafeArg() const { return {value}; }
};

class FrameBufferColorChannelMask;
class RenderTargetMask
{
  uint8_t value = 0;

public:
  constexpr static RenderTargetMask make(uint8_t value)
  {
    RenderTargetMask result;
    result.value = value;
    return result;
  }
  constexpr explicit operator bool() const { return 0 != value; }
  constexpr FrameBufferColorChannelMask spread() const;
  constexpr RenderTargetMask operator&(const RenderTargetMask &o) const { return make(value & o.value); }
  constexpr RenderTargetMask operator|(const RenderTargetMask &o) const { return make(value | o.value); }
  constexpr RenderTargetMask operator^(const RenderTargetMask &o) const { return make(value ^ o.value); }
  constexpr RenderTargetMask operator~() const { return make(~value); }
  constexpr auto operator<=>(const RenderTargetMask &) const = default;
  constexpr void clear() { value = 0; }
  constexpr bool get(uint32_t index) const { return 0 != (value & (1u << index)); }
  constexpr void set(uint32_t index) { value |= 1u << index; }
  constexpr void unset(uint32_t index) { value &= ~(1u << index); }
  constexpr bool testAndUnset(uint32_t index)
  {
    auto pre = value;
    value &= ~(1u << index);
    return 0 != (pre & (1u << index));
  }
  constexpr bool anyOtherSet(uint32_t index) const { return 0 != (value & ~(1u << index)); }
  constexpr auto visit() const { return LsbVisitor{value}; }
  constexpr uint32_t getRequiredCount() const { return 32 - __clz(static_cast<uint32_t>(value)); }
  constexpr auto raw() const { return value; }
  operator DagorSafeArg() const { return {value}; }
};

class FrameBufferColorChannelMask
{
  uint32_t value = 0;

public:
  constexpr static FrameBufferColorChannelMask make(uint32_t value)
  {
    FrameBufferColorChannelMask result;
    result.value = value;
    return result;
  }
  constexpr explicit operator bool() const { return 0 != value; }
  constexpr RenderTargetMask asRenderTargetMask() const
  {
    return RenderTargetMask::make(detail::color_channel_mask_to_render_target_mask(value));
  }
  constexpr FrameBufferColorChannelMask spread() const
  {
    return FrameBufferColorChannelMask::make(detail::spread_color_chanel_mask_to_render_target_color_channel_mask(value));
  }
  constexpr FrameBufferColorChannelMask operator&(const FrameBufferColorChannelMask &o) const { return make(value & o.value); }
  constexpr FrameBufferColorChannelMask operator|(const FrameBufferColorChannelMask &o) const { return make(value | o.value); }
  constexpr FrameBufferColorChannelMask operator^(const FrameBufferColorChannelMask &o) const { return make(value ^ o.value); }
  constexpr FrameBufferColorChannelMask operator~() const { return make(~value); }
  constexpr auto operator<=>(const FrameBufferColorChannelMask &) const = default;
  constexpr ColorChannelMask get(uint32_t index) const
  {
    return ColorChannelMask::make(static_cast<uint8_t>((value >> (index * 4)) & 15));
  }
  constexpr void set(uint32_t index, ColorChannelMask mask)
  {
    value &= ~(0xFu << (index * 4));
    value |= static_cast<uint32_t>(mask.raw()) << (index * 4);
  }
  constexpr void clear() { value = 0; }
  // checks if all sets of 4 bits are equal
  constexpr bool isUniform() const
  {
    return 0 == (((value ^ (value >> 16)) & 0xFFFFu) | ((value ^ (value >> 8)) & 0xFFu) | ((value ^ (value >> 4)) & 0xFu));
  }
  constexpr auto raw() const { return value; }
  operator DagorSafeArg() const { return {value}; }
};

inline constexpr FrameBufferColorChannelMask RenderTargetMask::spread() const
{
  return FrameBufferColorChannelMask::make(detail::render_target_mask_to_color_channel_mask(value));
}
} // namespace drv3d_dx12
