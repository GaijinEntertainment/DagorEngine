float4 toroidalClipmap_world2uv_1 = (1, 1, 0, 0);
float4 toroidalClipmap_world2uv_2 = (1, 1, 0, 0);
float4 toroidalClipmap_world_offsets = (1, 1, 0, 0);

float parallax_scale = 0.05;
float parallax_shadow_length = 2;
float parallax_shadow_strength = 5;
float parallax_mip = 3;

texture toroidal_heightmap_texarray;

macro INIT_TOROIDAL_HEIGHTMAP(code)
  (code) {
    toroidal_heightmap_texarray@smpArray = toroidal_heightmap_texarray;
    toroidalClipmap_world2uv_1@f4 = toroidalClipmap_world2uv_1;
    toroidalClipmap_world2uv_2@f4 = toroidalClipmap_world2uv_2;
    toroidalClipmap_world_offsets@f4 = toroidalClipmap_world_offsets;
    parallax_scale@f4 = (parallax_scale, parallax_shadow_length*parallax_scale, parallax_shadow_strength, parallax_mip);
  }
endmacro

macro FALLBACK_TOROIDAL_HEIGHTMAP()
  hlsl(ps)
  {
    half2 sample_tor_height_2f(float2 worldXZ)
    {
      return 0.5;
    }
    #define sample_tor_height(worldXZ) sample_tor_height_2f(worldXZ).x

    float2 get_parallax_relief_and_shadow(float3 light_dir, float3 point2eyeNrm, float4 world_pos_xz_gradients, int num_parallax_iterations, half zero_point, float2 worldXZ, float dist, out float sunShadow, out float retHeight)
    {
      retHeight = zero_point;
      sunShadow = 1;
      return worldXZ;
    }
  }
endmacro

macro USE_TOROIDAL_HEIGHTMAP()
  if (render_with_normalmap == render_displacement)
  {
    hlsl(ps)
    {
      #define USE_PARALLAX

      half2 sample_tor_height_2f( float2 world_xz, float2 zero_level)
      {
       //#define ONE_CASCADE 1
        #if ONE_CASCADE
        //sampling only one cascade is less than 2% faster, than choosing best out of two
          float2 tc = world_xz * toroidalClipmap_world2uv_1.x + toroidalClipmap_world2uv_1.zw;
          float3 torTc = float3(tc + toroidalClipmap_world_offsets.xy, 0);
          tc = abs(2 * tc.xy - 1);
          float weight = saturate(10 - 10 * max(tc.x, tc.y));
        #else
          float2 tc2 = world_xz * toroidalClipmap_world2uv_2.x + toroidalClipmap_world2uv_2.zw;
          float3 torTc2 = float3(tc2 + toroidalClipmap_world_offsets.zw, 1);
          tc2 = abs(2 * tc2.xy - 1);
          float weight = saturate(10 - 10 * max(tc2.x, tc2.y));
          //actually this weight can be set to always 1.
          //in normal case displacement scale will be 0 outside invalid area.
          //however, after teleportation that will work

          float2 tc = world_xz * toroidalClipmap_world2uv_1.x + toroidalClipmap_world2uv_1.zw;
          float3 torTc = float3(tc + toroidalClipmap_world_offsets.xy, 0);

          FLATTEN
          if (any(abs(tc*2-1)>=0.99))
            torTc = torTc2;
        #endif

        float2 height = tex3Dlod (toroidal_heightmap_texarray, float4(torTc, 0)).rg;

        return lerp(zero_level, height, weight);
      }
      #define sample_tor_height(world_xz, zero_level) sample_tor_height_2f(world_xz, float2(zero_level, zero_level)).x

      float2 get_parallax_relief_and_shadow(
        float3 light_dir,
        float3 point2eyeNrm, float4 world_pos_xz_gradients, int num_parallax_iterations, half zero_point, float2 world_xz, float dist, out float sunShadow, out float retHeight)
      {
        sunShadow = 1;
        retHeight = zero_point;
        //ortho
        float fDet;
        float2 vR1,vR2;
        float4 scaled_gradients = world_pos_xz_gradients;
        
        float parallax_mip_str = pow2(saturate(1.1 - dist*toroidalClipmap_world2uv_2.x));
        half parallax_lerp = 1 - parallax_mip_str;

        BRANCH
        if (parallax_mip_str <= 0.001)
          return world_xz;

        vR1 = float2(-world_pos_xz_gradients.w, world_pos_xz_gradients.z);
        vR2 = float2(world_pos_xz_gradients.y, -world_pos_xz_gradients.x);
        fDet = rcp(dot(world_pos_xz_gradients.xy, vR1));
        float2 vProjVscr = fDet * float2( dot(vR1, point2eyeNrm.xz), dot(vR2, point2eyeNrm.xz) );

        float2 vProjVtex = scaled_gradients.xy*vProjVscr.x + scaled_gradients.zw*vProjVscr.y;
        float2 parallaxTS= vProjVtex.xy*(parallax_scale.x*parallax_mip_str);

        for (int i = 1; i < num_parallax_iterations; ++i)
        {
          float height = sample_tor_height(world_xz, zero_point) - zero_point;
          world_xz+=height*parallaxTS;
        }

        // calculate self-shadow
        float2 vProjLscr = fDet * float2( dot(vR1, light_dir.xz), dot(vR2, light_dir.xz) );
        float2 vProjLtex = scaled_gradients.xy*vProjLscr.x + scaled_gradients.zw*vProjLscr.y;
        half shadow_strength = parallax_scale.z;

        const float paraLength = 0.03f;
        float2 tcOfs = 0.5 * vProjLtex*parallax_scale.y;
        float yBias = 0.7 * light_dir.y;
        float height0 = sample_tor_height(world_xz, zero_point);
        retHeight = height0;
        half sh6 = (sample_tor_height(world_xz - tcOfs, zero_point) - height0 + yBias) * shadow_strength;
        half sh4 = (sample_tor_height(world_xz - tcOfs * 0.5, zero_point) - height0 + yBias * 0.5) * shadow_strength * 2;
        sunShadow = lerp(saturate(1 - max(sh6, sh4)), 1, saturate(parallax_lerp * 2 - 1));

        return world_xz;
      }
    }
  }
  else
  {
    FALLBACK_TOROIDAL_HEIGHTMAP()
  }
endmacro

macro USE_TOROIDAL_HEIGHTMAP_LOWRES(code)
  hlsl(code){
    half2 sample_tor_height_lowres_2f(float2 world_xz, float2 zero_level, out float weight)
    {
      float height = 0;
      float2 tc = world_xz * toroidalClipmap_world2uv_2.x + toroidalClipmap_world2uv_2.zw;
      float2  torTc = tc + toroidalClipmap_world_offsets.zw;

      tc = abs(2 * tc.xy - 1);
      weight = saturate(2 - 2 * max(tc.x, tc.y));

      return lerp(zero_level, tex3Dlod(toroidal_heightmap_texarray, float4(torTc, 1, 0)).rg, weight);
    }
    half sample_tor_height_lowres(float2 world_xz, float zero_level)
    {
      float weight;
      return sample_tor_height_lowres_2f(world_xz, float2(zero_level, zero_level), weight).x;
    }
  }
endmacro

// Toroidal displacement for heightmap

macro USE_TOROIDAL_HEIGHTMAP_BASE(code)

    hlsl(code)
    {
      #define USE_DISPLACEMENT

      half2 sample_tor_height_vs_2f(float2 world_xz, float2 zero_level, out float weight)
      {
        //#define ONE_CASCADE 1
        #if ONE_CASCADE
        //sampling only one cascade is less than 2% faster, than choosing best out of two
          float2 tc = world_xz * toroidalClipmap_world2uv_1.x + toroidalClipmap_world2uv_1.zw;
          float3 torTc = float3(tc + toroidalClipmap_world_offsets.xy, 0);
          tc = abs(2 * tc.xy - 1);
          weight = saturate(10 - 10 * max(tc.x, tc.y));
        #else
          float2 tc2 = world_xz * toroidalClipmap_world2uv_2.x + toroidalClipmap_world2uv_2.zw;
          float3 torTc2 = float3(tc2 + toroidalClipmap_world_offsets.zw, 1);
          tc2 = abs(2 * tc2.xy - 1);
          weight = saturate(10 - 10 * max(tc2.x, tc2.y));
          //actually this weight can be set to always 1.
          //in normal case displacement scale will be 0 outside invalid area.
          //however, after teleportation that will work

          float2 tc = world_xz * toroidalClipmap_world2uv_1.x + toroidalClipmap_world2uv_1.zw;
          float3 torTc = float3(tc + toroidalClipmap_world_offsets.xy, 0);

          FLATTEN
          if (any(abs(tc*2-1)>=0.99))
            torTc = torTc2;
        #endif

        float2 height = tex3Dlod (toroidal_heightmap_texarray, float4(torTc, 0)).rg;

        return lerp(zero_level, height, weight);
      }
      half sample_tor_height_vs(float2 world_xz, float zero_level)
      {
        float weight;
        return sample_tor_height_vs_2f(world_xz, float2(zero_level, zero_level), weight).x;
      }
    }
endmacro

macro USE_TOROIDAL_HEIGHTMAP_VS()
  USE_TOROIDAL_HEIGHTMAP_BASE(vs)
endmacro
