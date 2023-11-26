macro USE_PUDDLES_WETNESS()
  USE_WETNESS()

  (ps) {
    puddles_params@f4 = (1.0 / (puddles_params.y - puddles_params.x), -puddles_params.x / (puddles_params.y - puddles_params.x), puddles_params.z, puddles_params.w);
    wetness_noise@f4 = wetness_noise;
    wetness_noise_weight@f4 = wetness_noise_weight;
  }

  hlsl (ps) {
    #include "noise/Perlin2D.hlsl"

    #define LAND_THICKNESS_WETNESS_THRESHOLD 0.075
    #define LAND_THICKNESS_WETNESS_SCALE 10.0
    #define WETNESS_PUDDLE_SLOPE 0.004
    #define WETNESS_HDOWN_SCALE_LEVEL 0.5

    #define sample_perlin_wetness(world_pos, perlin_tex, perlin) \
    { \
      perlin = tex2Dlod(perlin_tex, float4((world_pos.xz + wf0_pseed.ww) * wetness_noise_weight.x, 0.0, 0.0)).x; \
      perlin = smoothstep(0, 1, perlin); \
      perlin = ClampRange(perlin, wetness_noise.x, wetness_noise.y) * get_base_wetness_max_level() + \
        ClampRange(perlin, wetness_noise.z, wetness_noise.w) * (1 - get_base_wetness_max_level()); \
    }

    bool check_puddles_conditions(float hi_perlin, float eye_dist, float land_mask)
    {
      return hi_perlin > 0.0 && land_mask > 0.0 && (eye_dist * puddles_params.x + puddles_params.y) < 1.0 && (wetness_noise.w - wetness_noise.z) > 0.01;
    }

    // land_thickness = 0..m
    // land_deformation = -0.5..0.5
    // height = -0.5 ... 0.5
    float get_puddles_wetness(float3 world_pos, float3 vertical_normal, float eye_dist, float hi_perlin, float land_mask, float land_thickness, float land_deformation, float height)
    {
      // low spread wetness mask
      float wetnessMask = smoothstep(-1.0, 1.0, noise_Perlin2D(world_pos.xz * wetness_noise_weight.y));
      wetnessMask = wetness_noise_weight.w > 0 ? ClampRange(wetnessMask * wetnessMask * wetnessMask * wetnessMask, wetness_noise_weight.z, wetness_noise_weight.w) : 1;
      wetnessMask = lerp(wetnessMask, 1.0, get_wetness_power());
      float wetness = hi_perlin * wetnessMask;

      // height up
      wetness = max(wetness - max(height * puddles_params.w, 0), 0);
      wetness -= min(max(land_thickness * (1.0 + 2.0 * land_deformation) - LAND_THICKNESS_WETNESS_THRESHOLD, 0) * LAND_THICKNESS_WETNESS_SCALE, max(wetness - get_base_wetness_max_level(), 0));
      // height down (limit by dirt level to fix missing ripples), more folding when puddles power starts from 0.5
      float wetnessHDown = -min(height * puddles_params.z, 0.0);
      wetnessHDown = lerp(1.0 - ClampRange(1.0 - wetness, clamp(wetnessHDown * WETNESS_HDOWN_SCALE_LEVEL, 0.0, 0.99), 1), wetnessHDown * wetnessMask, saturate(wetness_params.w * 2.0 - 1.0));
      wetness += min(wetnessHDown, max(get_base_wetness_max_level() - wetness, 0));

      // slope fade
      wetness *= ClampRange(vertical_normal.y, 1.0 - WETNESS_PUDDLE_SLOPE * 2, 1.0 - WETNESS_PUDDLE_SLOPE);
      // dist fade
      wetness *= 1.0 - saturate(eye_dist * puddles_params.x + puddles_params.y);
      // apply grass mask
      wetness *= land_mask;

      return wetness;
    }
  }
endmacro

float4 heightmap_mud_color = (0.08, 0.05, 0.015, 3);

macro USE_LAND_WETNESS()
  USE_PUDDLES_WETNESS()
  INIT_WRITE_GBUFFER_WETNESS()
  WRITE_GBUFFER_WETNESS()
  USE_WATER_RIPPLES()

  (ps) { heightmap_mud_color@f4 = heightmap_mud_color; }

  hlsl(ps) {

    bool check_land_wetness_conditions(float3 world_pos, float3 vertical_normal, float dist, float hi_perlin, float land_mask, out float base_wetness, out float water_border)
    {
      base_wetness = get_base_wetness(world_pos, vertical_normal, water_border);
      return base_wetness > 0 || (check_puddles_conditions(hi_perlin, dist, land_mask) && world_pos.y > get_wetness_water_level());
    }

    float2 get_land_wetness(float3 world_pos, float3 vertical_normal, float dist, float hi_perlin, float land_mask, float land_thickness, float land_deformation, float height, float base_wetness, float water_border)
    {
      // puddles
      float puddlesWetness = get_puddles_wetness(world_pos, vertical_normal, dist, hi_perlin, land_mask, land_thickness, land_deformation, height);
      // underwater
      puddlesWetness *= max(water_border, 0);
      // puddles is off (but there is some wetness)
      puddlesWetness *= (wetness_noise.y - wetness_noise.x) > 0.01;

      return float2(base_wetness, puddlesWetness);
    }

    void apply_land_wetness(float2 wetness, float water_border, float3 world_pos, float3 vertical_norm, float3 view, float3 sunDir, float landThickness, inout UnpackedGbuffer g_buf)
    {
      float totalWetness = max(wetness.x, wetness.y);
      float fresnelterm = 0.04 + 0.96 * saturate(1.2*pow3(1-view.y));
      float wetMaterial = get_wet_material(totalWetness, water_border);
      float porosity = wetness_blend_porosity_params.w;
      float wetnessAO = ClampRange(wetMaterial, 0.45, 0.95);
      float3 albedoSqr = 0.8*g_buf.albedo.rgb * g_buf.albedo.rgb;
      g_buf.normal = lerp(g_buf.normal, vertical_norm, wetnessAO);
      g_buf.albedo.rgb = lerp(g_buf.albedo.rgb, albedoSqr, ClampRange(totalWetness, 0.0, 0.35) * porosity);
      g_buf.albedo.rgb = lerp(g_buf.albedo.rgb,
                              heightmap_mud_color.rgb,
                              saturate(heightmap_mud_color.w*landThickness*ClampRange(wetness.y, 0.55, 1)));
      g_buf.albedo.rgb = lerp(g_buf.albedo.rgb, float3(0.015, 0.015, 0.015), fresnelterm*pow2(totalWetness));
      g_buf.ao = lerp(g_buf.ao, 1.0, wetnessAO);
      g_buf.shadow = lerp(g_buf.shadow, 1.0, wetnessAO);
      g_buf.translucency *= (1 - wetnessAO);
      g_buf.smoothness = lerp(g_buf.smoothness, 1, ClampRange(wetMaterial, 0.2, 1.0));

      if (max(wetness.x, wetness.y) > get_base_wetness_max_level())
        g_buf.normal = RNM_ndetail_normalized(g_buf.normal.xzy, get_water_ripples_normal(world_pos).xzy).xzy;

      g_buf.reflectance = lerp(g_buf.reflectance, 1.0, wetMaterial);
    }
  }
endmacro