#ifndef DAGDP_COMMON_HLSLI_INCLUDED
#define DAGDP_COMMON_HLSLI_INCLUDED

#define ORIENTATION_X_IS_PROJECTED_BIT       (1 << 0) // If set, object X direction will look towards the surface normal projection onto XZ plane.
#define ORIENTATION_Y_IS_NORMAL_BIT          (1 << 1) // If set, object Y direction is the surface normal.
#define SLOPE_PLACEMENT_IS_INVERTED_BIT      (1 << 2) // If set, placement decision is inverted relative to slopeFactor.
#define HEIGHTMAP_IMITATORS_ARE_REJECTED_BIT (1 << 3) // If set, and Y was changed due to heightmap imitators, discard placement.
#define HEIGHTMAP_IMITATORS_ARE_REQUIRED_BIT (1 << 4) // If set, discard placement, unless Y was changed due to heightmap imitators.
#define PERSIST_ON_HEIGHTMAP_DEFORM_BIT      (1 << 5) // If set, object will be displaced down rather than deleted when ridden over by a vehicle.
#define WATER_PLACEMENT_BIT                  (1 << 6) // If set, object will be placed on water heightmap instead of terrain or clipmap RI (heightmap imitators).

// Using high bits so these can be merged with the placeable flags
#define VOLUME_AXIS_ABS_BIT (1<<31) // If set, use abs() on the dot product with normal

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
  float slopeSinMin;
  float slopeSinMax;
  float occlusionMin;

  float occlusionMax;
  float occlusionRange;
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