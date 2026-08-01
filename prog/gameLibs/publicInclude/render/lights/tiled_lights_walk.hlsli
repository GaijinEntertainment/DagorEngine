// Iterates visible lights of one type from the per-tile z-binned light lists,
// invoking the light body for each light index.
//
// Usage:
//   #define TILED_LIGHTS_WALK_SPOT 0|1           // light type: omni (0) or spot (1)
//   #define TILED_LIGHTS_WALK_DEPTH w            // linear view depth of the shaded point
//   #define TILED_LIGHTS_WALK_TILE_OFFSET expr   // first dword of this tile in the lights list
//   #define TILED_LIGHTS_WALK_LIGHT_BODY(light_index) ... // per-light statements; `continue` skips to the next light
//   #define TILED_LIGHTS_WALK_SINGLE_WORD 0|1    // optional: single-light shader variant, collapses the word loop
//   #define TILED_LIGHTS_WALK_BUFFER_AT(b, i)    // optional: buffer accessor, defaults to b[i]
//   #include <tiled_lights_walk.hlsli>
//
// All TILED_LIGHTS_WALK_* parameters are consumed and undefined by this include.

#ifndef TILED_LIGHTS_WALK_SINGLE_WORD
#define TILED_LIGHTS_WALK_SINGLE_WORD 0
#endif
#ifndef TILED_LIGHTS_WALK_BUFFER_AT
#define TILED_LIGHTS_WALK_BUFFER_AT(buffer, index) buffer[index]
#endif

#if TILED_LIGHTS_WALK_SPOT
#define TILED_LIGHTS_WALK_ZBIN_OFFSET Z_BINS_COUNT
#define TILED_LIGHTS_WALK_WORD_BASE (DWORDS_PER_TILE / 2)
#define TILED_LIGHTS_WALK_LIGHTS_COUNT spot_lights_count.x
#else
#define TILED_LIGHTS_WALK_ZBIN_OFFSET 0
#define TILED_LIGHTS_WALK_WORD_BASE 0
#define TILED_LIGHTS_WALK_LIGHTS_COUNT omni_lights_count.x
#endif

{
  uint tiled_walk_zbins = TILED_LIGHTS_WALK_BUFFER_AT(z_binning_lookup, depth_to_z_bin(TILED_LIGHTS_WALK_DEPTH) + TILED_LIGHTS_WALK_ZBIN_OFFSET);
  uint tiled_walk_binsBegin = tiled_walk_zbins >> 16;
  uint tiled_walk_binsEnd = tiled_walk_zbins & 0xFFFF;
  uint tiled_walk_mergedBinsBegin = WAVE_MIN(tiled_walk_binsBegin);
  uint tiled_walk_mergedBinsEnd = WAVE_MAX(tiled_walk_binsEnd);
  uint tiled_walk_wordsBegin = (tiled_walk_mergedBinsBegin >> 5) + TILED_LIGHTS_WALK_WORD_BASE;
  uint tiled_walk_wordsEnd = (tiled_walk_mergedBinsEnd >> 5) + TILED_LIGHTS_WALK_WORD_BASE;
  uint tiled_walk_maskWidth = clamp((int)tiled_walk_binsEnd - (int)tiled_walk_binsBegin + 1, 0, 32);
  uint tiled_walk_word = TILED_LIGHTS_WALK_WORD_BASE;
#if TILED_LIGHTS_WALK_SINGLE_WORD
  if (tiled_walk_wordsBegin <= tiled_walk_wordsEnd)
#else
  for (tiled_walk_word = tiled_walk_wordsBegin; tiled_walk_word <= tiled_walk_wordsEnd; ++tiled_walk_word)
#endif
  {
    uint tiled_walk_mask = TILED_LIGHTS_WALK_BUFFER_AT(lights_list, (TILED_LIGHTS_WALK_TILE_OFFSET) + tiled_walk_word);
    // Mask by ZBin mask
    uint tiled_walk_localMin = clamp((int)tiled_walk_binsBegin - (int)((tiled_walk_word - TILED_LIGHTS_WALK_WORD_BASE) << 5), 0, 31);
    // BitFieldMask op needs manual 32 size wrap support
    uint tiled_walk_zbinMask = tiled_walk_maskWidth == 32 ? (uint)(0xFFFFFFFF) : BitFieldMask(tiled_walk_maskWidth, tiled_walk_localMin);
    tiled_walk_mask &= tiled_walk_zbinMask;
    uint tiled_walk_mergedMask = WAVE_OR(tiled_walk_mask);
    LOOP
    while (tiled_walk_mergedMask)
    {
      uint tiled_walk_bitIdx = firstbitlow(tiled_walk_mergedMask);
      uint tiled_walk_lightIndex = (tiled_walk_word - TILED_LIGHTS_WALK_WORD_BASE) * BITS_IN_UINT + tiled_walk_bitIdx;
      // This branch is a workaround for NV specific bug.
      // The condition can't be true by design, but when we have a bug,
      // PIX shows NaN pixels and totally correct data in buffer.
      // Also using -O0 "fixes" the issue.
      if (tiled_walk_lightIndex >= TILED_LIGHTS_WALK_LIGHTS_COUNT)
        break;
      tiled_walk_mergedMask ^= (1U << tiled_walk_bitIdx);
      TILED_LIGHTS_WALK_LIGHT_BODY(tiled_walk_lightIndex)
    }
  }
}

#undef TILED_LIGHTS_WALK_ZBIN_OFFSET
#undef TILED_LIGHTS_WALK_WORD_BASE
#undef TILED_LIGHTS_WALK_LIGHTS_COUNT
#undef TILED_LIGHTS_WALK_LIGHT_BODY
#undef TILED_LIGHTS_WALK_BUFFER_AT
#undef TILED_LIGHTS_WALK_SINGLE_WORD
#undef TILED_LIGHTS_WALK_SPOT
#undef TILED_LIGHTS_WALK_DEPTH
#undef TILED_LIGHTS_WALK_TILE_OFFSET
