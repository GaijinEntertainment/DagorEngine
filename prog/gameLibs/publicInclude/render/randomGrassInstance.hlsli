#ifndef RANDOM_GRASS_INSTANCE_HLSL_INCLUDED
#define RANDOM_GRASS_INSTANCE_HLSL_INCLUDED

//#define UNCOMPRESSED 1
#define GRASS_WARP_SIZE_X 8
#define GRASS_WARP_SIZE_Y 8

#define GRASS_COUNT_PER_INSTANCE 1

#if UNCOMPRESSED
#define RandomGrassInstance RandomGrassInstanceUncompressed
#else
  struct RandomGrassInstance//should be 16 bytes aligned for performance
  {
    uint2 position; // we have a half worth of data here, w is not used
    uint landColor_lodLayerNo;
    uint landNormal;
    uint rot;
    uint verscale_mask_randVal;
    uint wind;
    uint prevWind;
  };
#endif

struct RandomGrassInstanceUncompressed
{
  float3 position;
  float3 landColor;
  float2 landNormal;
  float rot1;
  float rot2;
  float mask;
  float randVal;
  float verScale;
  float2 wind;
  float2 prevWind;
  uint layerNo;
  uint lodNo;
};

struct RandomGrassIndirectParams
{
  int2 cornerPos;
  uint xySize;
  float tileSize;
};

#endif
