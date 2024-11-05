#ifndef FLOW_MAP_INC_HLSL
#define FLOW_MAP_INC_HLSL

#define MAX_FLOWMAP_CIRCULAR_OBSTACLES 64
#define MAX_FLOWMAP_RECTANGULAR_OBSTACLES 64

struct FlowmapCircularObstacle
{
  float2 position;
  float radius;
  float padding;
};

struct FlowmapRectangularObstacle
{
  float2 position;
  float2 rotation;
  float2 size;
  float2 padding;
};

#endif
