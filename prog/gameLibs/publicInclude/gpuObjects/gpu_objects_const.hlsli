#ifndef GPU_OBJECTS_CONST_HLSL
#define GPU_OBJECTS_CONST_HLSL

#define DISPATCH_WARP_SIZE 64
#define DISPATCH_WARP_SIZE_XY 16
#define ROWS_IN_MATRIX 4

#define MAX_GATHERED_TRIANGLES_SQRT 256
#define MAX_GATHERED_TRIANGLES (MAX_GATHERED_TRIANGLES_SQRT * MAX_GATHERED_TRIANGLES_SQRT) //32b for triangle, 2Mb at all
#define TRIANGLE_AREA_MULTIPLIER 1000.0

//intended as enum but it's not work in hlsl
#define TRIANGLES_COUNT 0
#define TRIANGLES_AREA_SUM 1
#define TERRAIN_OBJECTS_COUNT 2
#define TERRAIN_INDEX 3
#define COUNTERS_NUM 4

#define GPU_OBJ_HLSL_ENCODE_VAL 8192
#define GPU_OBJ_BBOX_CLEANER_SIZE 8

struct GeometryTriangle
{
  uint3 v1v2;
  float areaDoubled;
  uint3 v3;
  uint pad2;
};

struct GeometryMesh
{
  int startIndex;
  int numFaces;
  int baseVertex;
  int stride;
  float4 tmRow0;
  float4 tmRow1;
  float4 tmRow2;
};

#endif
