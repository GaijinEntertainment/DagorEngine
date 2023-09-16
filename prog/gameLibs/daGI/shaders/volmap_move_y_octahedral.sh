float4 ssgi_copy_y_indices0;
float4 ssgi_copy_y_indices1;

int dead_no = 8;

shader move_y_octahderal_distances_cs
{
  (cs) {
    ssgi_copy_y_indices0@f4 = ssgi_copy_y_indices0;
    ssgi_copy_y_indices1@f4 = ssgi_copy_y_indices1;
  }
  hlsl(cs) {
    RWTexture2DArray<float2> octahedral_distances : register(u7);
    #include "octahedral_tile_side_length.hlsli"
    [numthreads(OCTAHEDRAL_TILE_SIDE_LENGTH, OCTAHEDRAL_TILE_SIDE_LENGTH, 1)]
    void copy_octahedral_cs(uint3 dtId : SV_DispatchThreadID)
    {
      int from = ssgi_copy_y_indices1.x;
      int to = ssgi_copy_y_indices1.y;
      int z_ofs = ssgi_copy_y_indices0.z;
      octahedral_distances[uint3(dtId.xy, to + z_ofs)] = octahedral_distances[uint3(dtId.xy, from + z_ofs)];
    }
  }
  compile("cs_5_0", "copy_octahedral_cs");
}

shader move_y_dead_probes_cs
{
  (cs) {
    ssgi_copy_y_indices0@f4 = ssgi_copy_y_indices0;
    ssgi_copy_y_indices1@f4 = ssgi_copy_y_indices1;
    dead@uav : register(dead_no) hlsl {
      RWTexture2DArray<float> dead@uav;
    }
  }
  hlsl(cs) {
    [numthreads(1, 1, 1)]
    void copy_dead_probes_cs(uint3 dtId : SV_DispatchThreadID)
    {
      int from = ssgi_copy_y_indices1.x;
      int to = ssgi_copy_y_indices1.y;
      int z_ofs = ssgi_copy_y_indices0.z;
      dead[uint3(dtId.xy, to + z_ofs)] = dead[uint3(dtId.xy, from + z_ofs)];
    }
  }
  compile("cs_5_0", "copy_dead_probes_cs");
}