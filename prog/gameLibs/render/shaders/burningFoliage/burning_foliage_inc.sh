texture burning_map_tex;
texture perlin_noise3d;

float4 world_to_burn_mask = (0, 0, 0, 0);

macro INIT_BURNING_FOLIAGE()
  (vs) {
    burning_map_tex@smp2d = burning_map_tex;
    world_to_burn_mask@f4 = world_to_burn_mask;
  }
  (ps) {
    burn_noise3d@smp3d = perlin_noise3d;
  }
endmacro

macro USE_BURNING_FOLIAGE()
  if (burning_trees == on)
  {
    hlsl(vs)
    {
      float4 sample_burning_mask(float3 pos, float3 input_pos, float scale)
      {
        float2 tc = pos.xz*world_to_burn_mask.xy + world_to_burn_mask.zw;
        float2 mask = tex2Dlod(burning_map_tex, float4(tc.xy, 0, 0)).xy;
        mask.x = saturate(mask.x * clamp(3.0 / sqrt(scale), 1, 3));
        mask.x = saturate(mask.x * (1 + 0.03*abs(input_pos.y))); // we can modify only one component of mask because of interpolator
        return float4(input_pos.xyz + float3(0, 0.1*pos.x, 0), (floor(mask.y*1000) + clamp(mask.x, 0.001, 0.999)) * (mask.x > 0 ? 1 : 0));
      }
    }

    hlsl(ps)
    {
      #define BURN_TREE_TRESHOLD (1.25 - 0.1*translucency)
      #define BURN_TREE_TRESHOLD_SHADOW 0.95

      float2 calculate_burn_params(in VsOutput input, float3 perlin_coords, inout float3 perlin)
      {
        float2 decodedBurnFactor = float2(frac(input.burnParams.w), 0.001 * int(input.burnParams.w));
        perlin_coords.xz += 0.5*perlin_coords.y;
        perlin = tex3Dlod(burn_noise3d, float4(perlin_coords, 0)).xyz;
        float burnFactor = max(0, (2.3 - 2*perlin.y)*decodedBurnFactor.x * length(input.burnParams.xz) + decodedBurnFactor.x);
        float flameFactor = decodedBurnFactor.y;
        return float2(burnFactor, flameFactor);
      }

      void calculate_burn_impostor_depth(in VsOutput input)
      {
        BRANCH
        if (input.burnParams.w == 0)
          return;
        float3 perlin = 0;
        float2 burnFactor = calculate_burn_params(input, 1.3*input.burnParams.xyz, perlin);
        if (burnFactor.x>=BURN_TREE_TRESHOLD_SHADOW)
          discard;
      }

      void calculate_burn_tree_depth(in VsOutput input, float translucency)
      {
        BRANCH
        if (input.burnParams.w == 0)
          return;
        float3 perlin = 0;
        float2 burnFactor = calculate_burn_params(input, 1.3*input.burnParams.xyz, perlin);
        if (translucency > 0 && burnFactor.x > BURN_TREE_TRESHOLD)
          discard;
      }
    }
  }
endmacro

macro USE_BURNING_FOLIAGE_GBUFFER()
  if (burning_trees == on)
  {
    hlsl(ps)
    {
      void calculate_burn_gbuffer(float2 burnFactor, float3 perlin, inout UnpackedGbuffer gbuffer)
      {
        float3 initialDiffuse = gbuffer.albedo;
        float3 diffuse = initialDiffuse;

        burnFactor *= 0.7+0.6*perlin.x;

        diffuse*= 1 - 0.95*saturate((burnFactor.x / 0.5));

        if (burnFactor.y > 0.05 && burnFactor.x > 0.9 && burnFactor.x < 0.9 + 0.06*burnFactor.y)
        {
          diffuse= perlin.y*(2 + burnFactor.y*2)*float3(0.1+0.5*initialDiffuse.g, 0.05*initialDiffuse.r, 0);
          init_material(gbuffer, SHADING_SELFILLUM);
          init_emission(gbuffer, 100*(burnFactor.x -0.9));
          init_emission_part(gbuffer, 0.8);
        }
        init_albedo(gbuffer, diffuse.rgb);
      }

      void calculate_burn_tree(in VsOutput input, inout UnpackedGbuffer gbuffer)
      {
        BRANCH
        if (input.burnParams.w == 0)
          return;

        float3 perlin = 0;
        float2 burnFactor = calculate_burn_params(input, 1.3*input.burnParams.xyz, perlin);
        float translucency = gbuffer.translucency;
        if (burnFactor.x > BURN_TREE_TRESHOLD && gbuffer.material == SHADING_FOLIAGE)
          discard;

        calculate_burn_gbuffer(burnFactor, perlin, gbuffer);
      }

      void calculate_burn_impostor(in VsOutput input, float translucency, inout UnpackedGbuffer gbuffer)
      {
        BRANCH
        if (input.burnParams.w == 0)
          return;

        float3 perlin = 0;
        float2 burnFactor = calculate_burn_params(input, 1.3*input.burnParams.xyz, perlin);
        float wideness = length(input.burnParams.xz + 0.2*perlin.xz-0.1);

        if (translucency > 0 && burnFactor.x >= BURN_TREE_TRESHOLD)
          discard;

        burnFactor *= translucency > 0 ? 1 : 0.3;
        calculate_burn_gbuffer(burnFactor, perlin, gbuffer);
      }
    }
  }
endmacro