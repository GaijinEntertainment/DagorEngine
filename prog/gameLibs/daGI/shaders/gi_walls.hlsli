#ifndef GI_WALLS_HLSL
#define GI_WALLS_HLSL 1
  #define WALLS_GRID_XZ 32
  #define WALLS_GRID_Y 4
  #define MAX_WALLS_CELL_COUNT 4
  #if MAX_WALLS_CELL_COUNT == 2
    #define WallGridType uint2
  #elif MAX_WALLS_CELL_COUNT == 4
    #define WallGridType uint4
  #endif
#endif