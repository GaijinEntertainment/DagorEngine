// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// Input is 4x8 bits color channel mask and output will be a 8bit mask of render targets
inline uint32_t color_channel_mask_to_render_target_mask(uint32_t mask)
{
  // For each color chanel generate the used bit
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
inline uint32_t render_target_mask_to_color_channel_mask(uint32_t mask)
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

// Takes a 4x8 bit render target output channel mask and turns it into a 4x8 render target ouput mask
// where if any channel of a target is enabled all channels of the result are enabled.
// Simply speaking it turns all non 0 hex digits in the mask into F and all 0 are keept as 0.
inline uint32_t spread_color_chanel_mask_to_render_target_color_channel_mask(uint32_t mask)
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
