#ifndef DAGI_RAD_GRID_MATH
#define DAGI_RAD_GRID_MATH 1
  #include <octahedral_common.hlsl>
  //to [-1,1] tc
  float2 radiance_grid_dir_encode(float3 world_dir)
  {
    return dagi_dir_oct_encode(world_dir);
  }
  ///tc -1,1
  float3 radiance_grid_dir_decode(float2 tc)
  {
    return dagi_dir_oct_decode(tc);
  }
#endif
