#ifndef GI_WINDOWS_HLSL
#define GI_WINDOWS_HLSL 1
  struct Window {float4 row0, row1, row2;};
  #define WINDOW_GRID_XZ 64
  #define WINDOW_GRID_Y 16
  #define WINDOW_HALF_LOCAL_BOX float3(1,0.5,0.5)
#endif