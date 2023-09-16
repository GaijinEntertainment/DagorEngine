float4 caustics_options = (0.2,1.0,5.0,5.0); //distance factor to fade height near hero, scale, height of caustics, height of hero caustics
float4 caustics_tile_hiding  = (0.5,0.32, 1.37, 2.1);
float caustics_world_scale  = 1.0;
float4 caustics_hero_pos = (0, -1000000, 0, 0); // position and radius of watched hero
texture caustics__indoor_probe_mask;
float4x4 caustics__indoor_probe_mask_matrix;
float caustics__indoor_probe_mask_min_strength= 0.999;

macro INIT_CAUSTICS_BASE_STCODE(code)
  (code)
  {
    caustics_options@f4 = caustics_options;
    caustics_tile_hiding@f4 = caustics_tile_hiding;
    caustics_hero_pos@f4 = caustics_hero_pos;

    caustics_world_scale@f1 = caustics_world_scale;
  }
endmacro

macro USE_CAUSTICS_BASE(code)

  supports global_const_block;

  if (use_extended_global_frame_block == no)
  {
    INIT_CAUSTICS_BASE_STCODE(code)
  }

  // Even though it's assumed to be false, WTM doesn't compile with this in the global_frame block.
  if (caustics__indoor_probe_mask != NULL)
  {
    (code)
    {
      caustics__indoor_probe_mask@shd = caustics__indoor_probe_mask;
      caustics__indoor_probe_mask_matrix@f44 = caustics__indoor_probe_mask_matrix;
      caustics__indoor_probe_mask_min_strength@f1 = caustics__indoor_probe_mask_min_strength;
    }
  }

  hlsl(code)
  {
  #if defined(USE_CAUSTICS_RENDER) || FORCE_CAUSTICS

    #ifndef get_shadow
      #define get_shadow(shadowPos) 1
    #endif
    // parameters get from shadertoy
    float caustic_iteration(in float2 posXY, in float posZ, in float scaleT)
    {
        float inten = .005;
        const float TAU = 6.28318530718;

        float time = caustics_options.y + posZ;
        float2 uv = posXY;

        float seamlessT = dot(sin(uv.xy + caustics_tile_hiding.xy * time) * caustics_tile_hiding.zw, 1.0);

        float2 p = fmod(uv*TAU, TAU)-250.0*scaleT;
        float2 i = p;
        float t = time + seamlessT;
        i = p + float2(cos(t - i.x) + sin(t + i.y), sin(t - i.y) + cos(t + i.x));

        float caustic= 1.0/length(float2(p.x / (sin(i.x+t)/inten),p.y / (cos(i.y+t)/inten)));
        return caustic;
    }

    float get_ssr_caustics(in float3 worldPos, in float3 normal, in float water_height, in float distance)
    {
      float causticBaseLevel=0;
      ##if caustics__indoor_probe_mask != NULL
        float4 ndc = mulPointTm(worldPos, caustics__indoor_probe_mask_matrix);
        float3 tc = ndc.xyz / ndc.w;
        tc.xy = tc.xy * float2(0.5, -0.5) + 0.5;
        float mask = 1 - shadow2D(caustics__indoor_probe_mask, tc);
        mask = smoothstep(caustics__indoor_probe_mask_min_strength, 1, mask);
        BRANCH
        if (mask < 0.0001)
          return causticBaseLevel;
      ##endif
      //calc light weight
      float3 reflectedLight = normalize(float3(-from_sun_direction.x, from_sun_direction.y, -from_sun_direction.z));
      float normalWeight = saturate(dot(normal, reflectedLight));

      float heroDist=length(caustics_hero_pos.xyz-worldPos.xyz);
      float heightFactor=saturate(caustics_options.x*(caustics_hero_pos.w-heroDist));
      float maxHeight=lerp(caustics_options.z, caustics_options.w, heightFactor);

      float heightWeight = saturate((maxHeight+water_height-worldPos.y)/(maxHeight+0.001));
      float3 shadowPos=worldPos-reflectedLight.xyz*(worldPos.y-water_height)/reflectedLight.y;

      half vsmShadow = (worldPos.y-water_height > 0.0) ? get_shadow(float4(shadowPos, 1)) : half(1.0);

      float fresnelTerm=0.7+0.3*reflectedLight.y;

      float weight =normalWeight * heightWeight * fresnelTerm;

      #if defined(CAUSTICS_CUSTOM_WORLD_SCALE)
      float worldScale = CAUSTICS_CUSTOM_WORLD_SCALE;
      #else
      float worldScale = caustics_world_scale;
      #endif

      float causticWeight = max(saturate(water_height-worldPos.y), weight);
      float distanceFade=saturate(0.015*(170-distance*worldScale));

      BRANCH
      if (causticWeight*distanceFade<0.001)
      {
        return causticBaseLevel; // early exit
      }

      const float upscale = 0.05;
      float causticScale = 1.00+upscale*(1-sqrt(min(0.25*abs(water_height-worldPos.y), 1)));

      const int NUM_ITERATIONS = 3;
      float c = 1.0;
      c+=caustic_iteration(0.5*worldPos.xz*worldScale, worldPos.y*worldScale, causticScale);
      c+=caustic_iteration(0.4*worldPos.xy*worldScale, worldPos.z*worldScale, causticScale);
      c+=caustic_iteration(0.3*worldPos.yz*worldScale, worldPos.x*worldScale, causticScale);

      c /= float(NUM_ITERATIONS);
      c = 1.17-pow(c, 1.4); // parameters get from shadertoy
      float caustics = pow(abs(c), 6.0);

      caustics = lerp(causticBaseLevel, caustics, causticWeight*distanceFade);

      ##if caustics__indoor_probe_mask != NULL
        caustics *= mask;
      ##endif

      return vsmShadow*caustics;
    }
  #endif
  }
endmacro

macro USE_SSR_CAUSTICS(code)
  USE_CAUSTICS_BASE(code)
endmacro

macro USE_CAUSTICS_FORWARD()
  hlsl(ps)
  {
    #define FORCE_CAUSTICS 1
  }
  USE_CAUSTICS_BASE(ps)
endmacro