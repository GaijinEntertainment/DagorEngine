#ifndef OBSTACLES_STRUCT_HLSL_INCLUDED
#define OBSTACLES_STRUCT_HLSL_INCLUDED 1

#define MAX_OBSTACLES 256
#define OBSTACLE_OFFSET_BIT_SHIFT 10
struct Obstacle{
  float4 dir_x, dir_y, dir_z;
  float4 box;
};
struct ObstaclesData
{
  float2 indirection_lt;
  uint indirection_wd;
  float indirection_cell;
  Obstacle obstacles[MAX_OBSTACLES];
};

#endif
