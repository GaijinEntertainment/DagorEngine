// Copyright (C) Gaijin Games KFT.  All rights reserved.

#ifndef RTSS_COMMON_HLSLI
#define RTSS_COMMON_HLSLI

#define RTSS_MAX_LEVELS       12
#define RTSS_BUILD_GROUP      64
#define RTSS_MISS             1.0 // the atlas holds depth over [minDepth, maxDepth]
#define RTSS_SLOT_VALID       1u
#define RTSS_SLOT_BIT         2u
#define RTSS_JOB_BIT          1u
#define RTSS_LEVEL_NEWEST_BIT  1u // the target bit when sweeping, else the current bit
#define RTSS_LEVEL_CURRENT_BIT 2u
#define RTSS_LEVEL_SWEEPING    4u
#define RTSS_LEVEL_BYTES       112
#define RTSS_TILE_TRANSPOSED  0x8000u
#define RTSS_TAP_HIT          0u
#define RTSS_TAP_RELEASED     1u // the slot is empty or holds another page
#define RTSS_TAP_BIT          2u
#define RTSS_TAP_DEPTH        3u
#define RTSS_COUNTER_LOOKUPS  0u
#define RTSS_COUNTER_NO_DATA  1u // then a fallback counter for each RTSS_TAP_ miss
#define RTSS_COUNTER_REF_PIXELS 5u
#define RTSS_COUNTER_REF_DIFF   6u
#define RTSS_COUNTERS           7u

struct RtssJob
{
  uint levelSlot; // level << 16 | slot y << 8 | slot x
  uint flags;     // RTSS_JOB_BIT
  int2 page;
  uint rect;    // x0 | y0 << 8 | x1 << 16 | y1 << 24, in slot texels
  float zStart; // light space z relative to z0 of the transform of the job bit
  float zEnd;
  uint rasterTile; // x | y << 16 | RTSS_TILE_TRANSPOSED: the scratch texel of the slot window of a raster job, ~0u for the others
};

inline uint rtss_pack_page(int2 page) { return (uint(page.x) & 0xffff) | (uint(page.y) << 16); }
inline uint rtss_slot_bit(uint flags) { return (flags & RTSS_SLOT_BIT) ? 1u : 0u; }
inline uint rtss_slot_index(int2 page, uint W) { return (uint(page.y) & (W - 1)) * W + (uint(page.x) & (W - 1)); }
inline uint rtss_rect_area(uint rect) { return (((rect >> 16) & 0xff) - (rect & 0xff)) * ((rect >> 24) - ((rect >> 8) & 0xff)); }

#ifdef __cplusplus
// the rows of the ByteAddressBuffers rtss_slots and rtss_levels, which the shaders read with rtss_slot and rtss_level_row
struct RtssSlot
{
  uint page;      // x & 0xffff | y << 16: the low 16 bits, exact within the window
  uint flags;     // RTSS_SLOT_VALID | RTSS_SLOT_BIT
  float minDepth; // light space z relative to z0 of the transform of the slot bit
  float maxDepth;
};

struct RtssLevel
{
  float4 xform[6]; // bit 0 in rows 0-2, bit 1 in rows 3-5: ex, sy * ey, d with k in w
  int2 pageOrigin; // the window origin in pages
  uint flags;      // RTSS_LEVEL_NEWEST_BIT | RTSS_LEVEL_CURRENT_BIT | RTSS_LEVEL_SWEEPING
  float invPage;   // 1 / (P * texel)
};

static_assert(sizeof(RtssSlot) == 16);
static_assert(sizeof(RtssJob) == 32);
static_assert(sizeof(RtssLevel) == RTSS_LEVEL_BYTES);
#else
#define rtss_params_i int4(rtss_globals[0])
#define rtss_params   rtss_globals[1]
#define rtss_origin   rtss_globals[2].xyz
#define rtss_select_y rtss_globals[2].w
#define rtss_bias     rtss_globals[3].xy
#define rtss_center_y rtss_globals[3].z
#define rtss_texel0   rtss_globals[3].w
#define rtss_atlas_texel rtss_globals[4].x // 1 / (S * W): one madd from atlas texels to uv

#define rtss_texel(level)          (rtss_texel0 * asfloat(((level) + 127u) << 23)) // texel0 * 2^level, exact
#define rtss_slot(i)               rtss_slots.Load4((i) * 16) // page, flags, asuint(minDepth), asuint(maxDepth)
#define rtss_level_row(level, row) asfloat(rtss_levels.Load4((level) * RTSS_LEVEL_BYTES + (row) * 16)) // xform[row]
#define rtss_level_window(level)   asuint(rtss_level_row(level, 6)) // pageOrigin, flags, asuint(invPage)

// the scratch texel of slot texel t: the viewport row grows down along light y, or along light x in a region turned by 90 deg
uint2 rtss_raster_texel(uint tile, uint2 t, uint S)
{
  return uint2(tile & (RTSS_TILE_TRANSPOSED - 1), tile >> 16) + ((tile & RTSS_TILE_TRANSPOSED) ? t.yx : uint2(t.x, S - 1 - t.y));
}
#endif

#endif
