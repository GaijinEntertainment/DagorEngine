#ifndef tiled_lights_walk_body_hlsli
#define tiled_lights_walk_body_hlsli

#define TILED_LIGHTS_WALK_BODY_IMPL(SPOT_LIGHTS, IS_SINGLE_WORD, WALK_TILE_OFFSET, LIGHTS_WALK_DEPTH)                                          \
if (!HW_IS_HELPER_LANE)                                                                                                                        \
{                                                                                                                                              \
  uint tiled_walk_zbins = TILED_LIGHTS_WALK_BUFFER_AT(z_binning_lookup, depth_to_z_bin(LIGHTS_WALK_DEPTH) + (SPOT_LIGHTS ? Z_BINS_COUNT : 0)); \
  uint tiled_walk_binsBegin = tiled_walk_zbins >> 16;                                                                                          \
  uint tiled_walk_binsEnd = tiled_walk_zbins & 0xFFFF;                                                                                         \
  uint tiled_walk_mergedBinsBegin = WAVE_MIN(tiled_walk_binsBegin);                                                                            \
  uint tiled_walk_mergedBinsEnd = WAVE_MAX(tiled_walk_binsEnd);                                                                                \
  uint tiled_walk_dword_base = (SPOT_LIGHTS ? DWORDS_PER_TILE / 2 : 0);                                                                        \
  uint tiled_walk_wordsBegin = (tiled_walk_mergedBinsBegin >> 5) + tiled_walk_dword_base;                                                      \
  /* constraint: keep this bound; the wave op results above are undefined in helper lanes that HW_IS_HELPER_LANE misses */                     \
  uint tiled_walk_wordsEnd = min(tiled_walk_mergedBinsEnd >> 5, uint(DWORDS_PER_TILE / 2 - 1)) + tiled_walk_dword_base;                        \
  uint tiled_walk_maskWidth = clamp((int)tiled_walk_binsEnd - (int)tiled_walk_binsBegin + 1, 0, 32);                                           \
  for (uint tiled_walk_word = tiled_walk_wordsBegin; tiled_walk_word <= tiled_walk_wordsEnd; ++tiled_walk_word)                                \
  {                                                                                                                                            \
    uint tiled_walk_mask = TILED_LIGHTS_WALK_BUFFER_AT(lights_list, (WALK_TILE_OFFSET) + tiled_walk_word);                                     \
    /* Mask by ZBin mask */                                                                                                                    \
    uint tiled_walk_localMin = clamp((int)tiled_walk_binsBegin - (int)((tiled_walk_word - tiled_walk_dword_base) << 5), 0, 31);                \
    /* BitFieldMask op needs manual 32 size wrap support */                                                                                    \
    uint tiled_walk_zbinMask = tiled_walk_maskWidth == 32 ? (uint)(0xFFFFFFFF) : BitFieldMask(tiled_walk_maskWidth, tiled_walk_localMin);      \
    tiled_walk_mask &= tiled_walk_zbinMask;                                                                                                    \
    uint tiled_walk_mergedMask = WAVE_OR(tiled_walk_mask);                                                                                     \
    LOOP                                                                                                                                       \
    while (tiled_walk_mergedMask)                                                                                                              \
    {                                                                                                                                          \
      uint tiled_walk_bitIdx = firstbitlow(tiled_walk_mergedMask);                                                                             \
      uint tiled_walk_lightIndex = (tiled_walk_word - tiled_walk_dword_base) * BITS_IN_UINT + tiled_walk_bitIdx;                               \
      /* This branch is a workaround for NV specific bug. */                                                                                   \
      /* The condition can't be true by design, but when we have a bug, */                                                                     \
      /* PIX shows NaN pixels and totally correct data in buffer. */                                                                           \
      /* Also using -O0 "fixes" the issue. */                                                                                                  \
      if (tiled_walk_lightIndex >= (SPOT_LIGHTS ? spot_lights_count.x : omni_lights_count.x))                                                  \
        break;                                                                                                                                 \
      tiled_walk_mergedMask ^= (1U << tiled_walk_bitIdx);                                                                                      \
      TILED_LIGHTS_WALK_LIGHT_BODY(tiled_walk_lightIndex)                                                                                      \
    }                                                                                                                                          \
    if (IS_SINGLE_WORD) break;                                                                                                                 \
  }                                                                                                                                            \
}

#endif
