#ifndef DAGDP_COMMON_PLACER_HLSLI_INCLUDED
#define DAGDP_COMMON_PLACER_HLSLI_INCLUDED

struct VariantGpuData
{
  float placeableWeightEmpty; // The weight of "empty object" (no placement).
  uint placeableStartIndex;
  uint placeableEndIndex;
  uint placeableCount;

  uint drawRangeStartIndex;
  uint drawRangeEndIndex;
  uint renderableIndicesStartIndex; // Not a typo.
  uint _padding;
};

#endif // DAGDP_COMMON_PLACER_HLSLI_INCLUDED