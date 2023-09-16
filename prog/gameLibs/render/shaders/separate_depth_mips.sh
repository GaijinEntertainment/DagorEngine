int separate_depth_mips = 0;
interval separate_depth_mips : no<1, two<2, three;
texture depth_mip_0;
texture depth_mip_1;
texture depth_mip_2;


macro INIT_SEPARATE_DEPTH_MIPS(code)
if (separate_depth_mips >= two) {
  (code) {
    depth_mip_0@smp2d = depth_mip_0;
    depth_mip_1@tex2d = depth_mip_1;
  }
}
if (separate_depth_mips >= three) {
  (code) {
    depth_mip_2@tex2d = depth_mip_2;
  }
}
endmacro

macro USE_SEPARATE_DEPTH_MIPS(code)
if (separate_depth_mips >= two) {
  hlsl(code) {
    float sample_depth_separate(float level, float2 location)
    {
    ##if (separate_depth_mips == three)
      BRANCH
      if (level >= 2.0f)
        return depth_mip_2.SampleLevel(depth_mip_0_samplerstate, location, 0).r;
    ##endif

      BRANCH
      if (level >= 1.0f)
        return depth_mip_1.SampleLevel(depth_mip_0_samplerstate, location, 0).r;

      return depth_mip_0.SampleLevel(depth_mip_0_samplerstate, location, 0).r;
    }

    float4 sample_4depths_separate(float level, float4 location0, float4 location1)
    {
    ##if (separate_depth_mips == three)
      BRANCH
      if (level >= 2.0f)
      {
        return float4(
          depth_mip_2.SampleLevel(depth_mip_0_samplerstate, location0.xy, 0).r,
          depth_mip_2.SampleLevel(depth_mip_0_samplerstate, location0.zw, 0).r,
          depth_mip_2.SampleLevel(depth_mip_0_samplerstate, location1.xy, 0).r,
          depth_mip_2.SampleLevel(depth_mip_0_samplerstate, location1.zw, 0).r
        );
      }
    ##endif

      BRANCH
      if (level >= 1.0f)
      {
        return float4(
          depth_mip_1.SampleLevel(depth_mip_0_samplerstate, location0.xy, 0).r,
          depth_mip_1.SampleLevel(depth_mip_0_samplerstate, location0.zw, 0).r,
          depth_mip_1.SampleLevel(depth_mip_0_samplerstate, location1.xy, 0).r,
          depth_mip_1.SampleLevel(depth_mip_0_samplerstate, location1.zw, 0).r
        );
      }

      return float4(
        depth_mip_0.SampleLevel(depth_mip_0_samplerstate, location0.xy, 0).r,
        depth_mip_0.SampleLevel(depth_mip_0_samplerstate, location0.zw, 0).r,
        depth_mip_0.SampleLevel(depth_mip_0_samplerstate, location1.xy, 0).r,
        depth_mip_0.SampleLevel(depth_mip_0_samplerstate, location1.zw, 0).r
      );
    }
  }
}
endmacro

