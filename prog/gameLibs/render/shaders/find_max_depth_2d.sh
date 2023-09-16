include "shader_global.sh"

float4 proj_values;

shader find_max_depth_2d
{
  (cs) {
    proj_values@f4 = (proj_values);
  }
  ENABLE_ASSERT(cs)
  hlsl(cs) {
    Texture2D<float> shadow_map : register( t0 );
    RWStructuredBuffer<uint> maximum : register(u0);
    #include <find_max_depth_2d.hlsli>
    groupshared float max_in_group[FMD2D_WG_SIZE];

    [numthreads( FMD2D_WG_DIM_W, FMD2D_WG_DIM_H, 1 )]
    void main( uint flatIdx : SV_GroupIndex, uint2 DTid : SV_DispatchThreadID)
    {
      uint2 texdim;
      shadow_map.GetDimensions(texdim.x, texdim.y);
      float2 texsize = float2(texdim);

      // Apply inverse matrix projection, just by less operations
      float depth = shadow_map[uint2(DTid.x, DTid.y)];
      float real_z = 1.0 / (depth*proj_values.z + proj_values.w);
      float3 worldPos = float3(float2(DTid.xy / texsize * 2.0 - 1.0) * proj_values.xy * real_z, real_z);
      float distSq = (DTid.x < texdim.x && DTid.y < texdim.y) ? dot(worldPos.xyz, worldPos.xyz) : 0;

      max_in_group[flatIdx] = distSq;
      GroupMemoryBarrierWithGroupSync();

      #define REDUCTION(n) max_in_group[flatIdx] = max(max_in_group[flatIdx], max_in_group[flatIdx+n]);
      if (flatIdx < 128) { REDUCTION(128) } GroupMemoryBarrierWithGroupSync();
      if (flatIdx < 64) { REDUCTION(64) } GroupMemoryBarrierWithGroupSync();
      if (flatIdx < 32) {
        REDUCTION(32);
        REDUCTION(16);
        REDUCTION(8);
        REDUCTION(4);
        REDUCTION(2);
        REDUCTION(1);
      }
      if (flatIdx == 0) InterlockedMax(structuredBufferAt(maximum, 0), asuint(max_in_group[0]));
    }
  }

  compile("target_cs", "main");
}