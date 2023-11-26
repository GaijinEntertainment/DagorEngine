macro INIT_WATER_FLOWMAP()
  texture water_flowmap_tex;
  texture water_flowmap_tex_add;
  float4 world_to_flowmap = (1/32,1/32,0.5,0.5);
  float4 world_to_flowmap_add = (1,1,0,0);
  float water_wind_strength = 0.2;
  float water_flowmap_fading = 3;
  float4 water_flowmap_strength = (1,5,0.5,0); // speed, foam, curvature, mask
  float4 water_flowmap_strength_add = (0.5,1,1,0.3); // min distance,max distance,speed,foam
  float4 water_flowmap_foam = (5,10,0.5,0.1); // power, scale, threshold, reflectivity
  float water_flowmap_foam_tiling = 1;
  float water_flowmap_debug = 0;
  float flowmap_texture_size_meters = 200;
endmacro

macro USE_WATER_FLOWMAP(code)
  (code)
  {
    water_flowmap_tex_add@smp2d = water_flowmap_tex_add;
    world_to_flowmap_add@f4 = world_to_flowmap_add;
    cascadesTexelScale_current_time_water_wind_strength@f3 = (cascadesTexelScale0123.x, time_phase(0, 0), water_wind_strength, 0);
    water_flowmap_fading_inv_fading@f2 = (water_flowmap_fading, 1 / water_flowmap_fading, 0, 0);
    water_flowmap_strength@f4 = (water_flowmap_strength);
    water_flowmap_strength_add@f4 = (-2/((water_flowmap_strength_add.y-water_flowmap_strength_add.x)*flowmap_texture_size_meters),
      water_flowmap_strength_add.y/(water_flowmap_strength_add.y-water_flowmap_strength_add.x), water_flowmap_strength_add.z, water_flowmap_strength_add.w);
  }

  hlsl(code) {
    #define has_water_flowmap 1
    #define cascadesTexelScale (cascadesTexelScale_current_time_water_wind_strength.x)
    #define current_time (cascadesTexelScale_current_time_water_wind_strength.y)
    #define water_wind_strength (cascadesTexelScale_current_time_water_wind_strength.z)
    #define water_flowmap_fading (water_flowmap_fading_inv_fading.x)
    #define inv_water_flowmap_fading (water_flowmap_fading_inv_fading.y)
  }

  hlsl(code) {
    void calcWaterFlowmapParams(float3 worldPos, float viewDist, out float3 worldPos_a, out float3 worldPos_b, out float viewDist_a, out float viewDist_b, out float crossFade)
    {
      float2 windVec = wind_dir_speed.xy * cascadesTexelScale * current_time * (water_wind_strength + 0.5);

      float2 flowmapSample = float2(0, 0);
      float2 flowmapUVAdd = worldPos.xz * world_to_flowmap_add.xy + world_to_flowmap_add.zw;
      if ((flowmapUVAdd.x >= 0) && (flowmapUVAdd.y >= 0) && (flowmapUVAdd.x <= 1) && (flowmapUVAdd.y <= 1))
      {
        float flowmapStrengthAdd = saturate(viewDist * water_flowmap_strength_add.x + water_flowmap_strength_add.y) * water_flowmap_strength_add.z;
        flowmapSample += tex2Dlod(water_flowmap_tex_add, float4(flowmapUVAdd, 0, 0)).xy * flowmapStrengthAdd;
      }
      float2 flowmapVec = flowmapSample * water_flowmap_fading;

      float flowmapTime = current_time * inv_water_flowmap_fading;
      float2 flowmapVec_a = flowmapVec * frac(flowmapTime + 0.0) - windVec;
      float2 flowmapVec_b = flowmapVec * frac(flowmapTime + 0.5) - windVec;

      worldPos_a = worldPos;
      worldPos_b = worldPos;

      worldPos_a.xz += flowmapVec_a;
      worldPos_b.xz += flowmapVec_b;

      viewDist_a = length(world_view_pos.xzy - worldPos_a);
      viewDist_b = length(world_view_pos.xzy - worldPos_b);

      crossFade = abs(frac(flowmapTime) * 2 - 1);
      crossFade = smoothstep(0, 1, crossFade);
    }
  }
endmacro
