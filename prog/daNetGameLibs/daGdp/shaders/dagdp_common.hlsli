#ifndef DAGDP_COMMON_HLSLI_INCLUDED
#define DAGDP_COMMON_HLSLI_INCLUDED

#define ORIENTATION_X_IS_PROJECTED_BIT       (1 << 0) // If set, object X direction will look towards the surface normal projection onto XZ plane.
#define ORIENTATION_Y_IS_NORMAL_BIT          (1 << 1) // If set, object Y direction is the surface normal.
#define SLOPE_PLACEMENT_IS_INVERTED_BIT      (1 << 2) // If set, placement decision is inverted relative to slopeFactor.
#define HEIGHTMAP_IMITATORS_ARE_REJECTED_BIT (1 << 3) // If set, and Y was changed due to heightmap imitators, discard placement.
#define HEIGHTMAP_IMITATORS_ARE_REQUIRED_BIT (1 << 4) // If set, discard placement, unless Y was changed due to heightmap imitators.
#define PERSIST_ON_HEIGHTMAP_DEFORM_BIT      (1 << 5) // If set, object will be displaced down rather than deleted when ridden over by a vehicle.

// Bits 6-8 are reserved for density mask channel.
// These decide which (if any) channel of the density mask to use for placement.
#define DENSITY_MASK_CHANNEL_INVALID        (0 << 6)
#define DENSITY_MASK_CHANNEL_RED            (1 << 6)
#define DENSITY_MASK_CHANNEL_GREEN          (2 << 6)
#define DENSITY_MASK_CHANNEL_BLUE           (3 << 6)
#define DENSITY_MASK_CHANNEL_ALPHA          (4 << 6)

#define DENSITY_MASK_CHANNEL_ALL_BITS       (7 << 6)

// maps the density mask channel bits to -1 (invalid), 0 (red), 1 (green), 2 (blue), 3 (alpha)
#define GET_DENSITY_MASK_CHANNEL(flags) (((int) (((flags) & DENSITY_MASK_CHANNEL_ALL_BITS) >> 6)) - 1)

#define WATER_PLACEMENT_BIT      (1 << 9) // If set, object will be placed on water heightmap instead of terrain or clipmap RI (heightmap imitators).

struct PlaceableGpuData
{
  float yawRadiansMin;
  float yawRadiansMax;
  float pitchRadiansMin;
  float pitchRadiansMax;

  float rollRadiansMin;
  float rollRadiansMax;
  float scaleMin;
  float scaleMax;

  float maxBaseDrawDistance; // Max. of renderables' baseDrawDistance.
  float slopeFactor;
  uint flags;
  uint riPoolOffset;
};

#define DYN_COUNTERS_PREFIX 4
#define DYN_COUNTERS_INDEX_OVERFLOW_FLAG 0
#define DYN_COUNTERS_INDEX_SKIP_PESSIMISTIC_PLACEMENT 1
#define DYN_COUNTERS_INDEX_TOTAL_PLACED 2
#define DYN_COUNTERS_INDEX_TOTAL_CAPACITY 3

struct DynAlloc
{
  uint instanceBaseIndexPlaced;
  uint instanceBaseIndex;
  uint capacity;
  uint _padding;
};

#endif // DAGDP_COMMON_HLSLI_INCLUDED