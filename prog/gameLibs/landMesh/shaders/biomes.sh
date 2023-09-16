float4 land_detail_mul_offset;
texture biomeIndicesTex;
texture biomeDetailAlbedoTexArray;
float4 biome_indices_tex_size;
buffer biome_group_indices_buffer;

macro INIT_BIOMES(stage)
  (stage) {
    land_detail_mul_offset@f4 = land_detail_mul_offset;
    biomeIndicesTex@smp2d = biomeIndicesTex;
    biomeDetailAlbedoTexArray@smpArray = biomeDetailAlbedoTexArray;
    biome_indices_tex_size@f4 = biome_indices_tex_size; // xy: size, zw: inv size
    biome_group_indices_buffer@buf = biome_group_indices_buffer hlsl {
      StructuredBuffer<uint> biome_group_indices_buffer@buf;
    }
    current_time@f1 = (time_phase(0, 0));
  }
endmacro

macro USE_BIOMES(stage)
  ENABLE_ASSERT(stage)
  hlsl(stage) {
    #include "noise/Perlin2D.hlsl"
    #include "noise/Value2D.hlsl"
    int getBiomeIndex(float3 world_pos)
    {
      float2 tc = world_pos.xz * land_detail_mul_offset.xy + land_detail_mul_offset.zw;
      float2 tcCorner = tc * biome_indices_tex_size.xy - 0.5f;
      float2 useDetailTC = (floor(tcCorner) + 0.5f) * biome_indices_tex_size.zw;
      return floor(biomeIndicesTex.GatherRed(biomeIndicesTex_samplerstate, useDetailTC).x * 255 + 0.1f);
    }

    int getBiomeGroupIndex(float3 world_pos)
    {
      int biomeIndex = getBiomeIndex(world_pos);
      return structuredBufferAt(biome_group_indices_buffer, biomeIndex);
    }

    float getBiomeInfluence(float3 world_pos, int index)
    {
      float2 tc = world_pos.xz * land_detail_mul_offset.xy + land_detail_mul_offset.zw;
      float2 tcCorner = tc * biome_indices_tex_size.xy - 0.5f;
      float2 useDetailTC = (floor(tcCorner) + 0.5f) * biome_indices_tex_size.zw;
      float4 bilWeights;
      bilWeights.xy = frac(tcCorner);
      bilWeights.zw = saturate(1 - bilWeights.xy);
      float4 baseWeights = (bilWeights.zxxz*bilWeights.yyww);
      float4 indexComp = floor(biomeIndicesTex.GatherRed(biomeIndicesTex_samplerstate, useDetailTC) * 255 + 0.1f) == index;
      return dot(indexComp, baseWeights);
    }

    float4 getBiomeColorByIndex(float2 world_pos, float4 index0, float4 index1)
    {
      uint texw, texh, layers, max_mip;
      biomeDetailAlbedoTexArray.GetDimensions(0, texw, texh, layers, max_mip);
      int mip = max(max_mip - 3, 0);
      float2 noiseTC = world_pos.xy * current_time;
      float2 tc = float2(noise_Perlin2D(noiseTC.xy), noise_Perlin2D(noiseTC.yx)) * 0.5 + 0.5;
      float4 diffuse0 =  tex3Dlod(biomeDetailAlbedoTexArray, float4(tc, index0.x, mip));
      float4 diffuse1 =  tex3Dlod(biomeDetailAlbedoTexArray, float4(tc, index1.x, mip));
      return lerp(diffuse0, diffuse1, noise_Value2D(current_time));
    }
  }
endmacro
