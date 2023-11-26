float ocean_density = 1.0;
float river_density = 6.0;
float4 ocean0 = (0.27,0.68,0.53,0);
float4 ocean1 = (0.57,0.91,0.84,0);

float4 river0 = (0.30,0.25,0.02,0);
float4 river1 = (0.56,0.54,0.28,0);
macro INIT_UNDERWATER_FOG()
  local float4 ocean0_srgb = (sRGBRead(ocean0));
  local float4 ocean1_srgb = (sRGBRead(ocean1));
  local float4 river0_srgb = (sRGBRead(river0));
  local float4 river1_srgb = (sRGBRead(river1));
  (ps) {
    ocean0@f4 = (ocean0_srgb.r,ocean0_srgb.g,ocean0_srgb.b, -ocean_density);
    ocean1@f4 = (ocean1_srgb.r,ocean1_srgb.g,ocean1_srgb.b, 1./(0.01+(height_tiling+1)*water_color_noise_size));
    river0@f4 = (river0_srgb.r, river0_srgb.g, river0_srgb.b, -river_density);
    river1@f4 = (river1_srgb.r, river1_srgb.g, river1_srgb.b, (0.41*0.3/PI));//inscatter through surface can not be more, than (0.98*0.66)^2/pi
  }
endmacro

int water_colormap_assume;
interval water_colormap_assume : off < 1, on;
texture water_colormap;
float4 water_colormap_area = (-512,-512,512,512);

macro GET_UNDERWATER_FOG_PERLIN()

  if (water_colormap_assume == on) {
    (ps) {
      water_colormap@smp2d = water_colormap;
      world_to_water_colormap@f4 = (1/(water_colormap_area.z-water_colormap_area.x), 1/(water_colormap_area.w-water_colormap_area.y),
        -water_colormap_area.x/(water_colormap_area.z-water_colormap_area.x), -water_colormap_area.y/(water_colormap_area.w-water_colormap_area.y));
    }
  }

hlsl(ps) {
  half4 get_ocean_color_perlin(float2 waterPos, float ocean_part, float perlin_noise_water_color)
  {
    ##if water_colormap_assume == off || water_colormap == NULL
      float4 water_color0 = lerp(river0, ocean0, ocean_part);
      float3 water_color1 = lerp(river1.rgb, ocean1.rgb, ocean_part);
      return half4(lerp(water_color0.rgb, water_color1.rgb, perlin_noise_water_color), water_color0.w);
    ##else
      float4 water_color = tex2Dlod(water_colormap, float4(waterPos*world_to_water_colormap.xy+world_to_water_colormap.zw,0,0));
      water_color.a = lerp(-water_color.a, 0.41*0.3/3.14, ocean_part);
      return water_color;
    ##endif
  }
  void get_underwater_fog_colored(float4 oceanColorDensity, float3 waterWorldPos, float3 underWaterWorldPos, float3 view, float water_view_depth, out half3 underwater_loss, out half3 underwater_inscatter)
  {
    //float dist = depthDist;
    //if (waterWorldPos.y>water_level && view.y<-0.01)
    //  dist = depthDist + water_level/view.y;
    underwater_loss = half3(exp2(saturate(float3(1.0,1.0,1.0)-oceanColorDensity.rgb)*(oceanColorDensity.w*max(0.0, water_view_depth))));
    float water_total_inscatter = river1.w;
    underwater_inscatter = half3(oceanColorDensity.rgb*water_total_inscatter);
  }
  void get_underwater_fog_perlin(float3 waterWorldPos, float3 underWaterWorldPos, float3 view, float water_view_depth, float water_down_deep, float ocean_part, half perlin_noise_water_color, out half3 underwater_loss, out half3 underwater_inscatter)
  {
    //half3 oceanColor = get_ocean_color(waterWorldPos.xz, lerp(1, ocean_part, pow4(1-saturate(-view.y*4-0.1))));
    //half3 oceanColor = get_ocean_color(waterWorldPos.xz, saturate((view.y+0.1)*10) > 0 ? ocean_part : 1);
    float4 oceanColor = get_ocean_color_perlin(waterWorldPos.xz, ocean_part, perlin_noise_water_color);
    get_underwater_fog_colored(oceanColor, waterWorldPos, underWaterWorldPos, view, water_view_depth, underwater_loss, underwater_inscatter);
  }
}
endmacro

// FIXME: move out here, it is global constansts affecting not only underwater fog
texture perlin_noise;
float height_tiling = 1;
float water_color_noise_size = 737;

macro GET_UNDERWATER_FOG()
  GET_UNDERWATER_FOG_PERLIN()

  if (compatibility_mode == compatibility_mode_off)
  {
    (ps) { perlin_noise@smp2d = perlin_noise; }
  }

  hlsl(ps) {
    float get_perlin_water_color(float2 waterPos)
    {
      float water_color_noise_scale = ocean1.w;
      ##if compatibility_mode == compatibility_mode_off
        return tex2D(perlin_noise, waterPos*water_color_noise_scale).g;
      ##else
        return 0.5;
      ##endif
    }
    half4 get_ocean_color(float2 waterPos, float ocean_part)
    {
      return get_ocean_color_perlin(waterPos, ocean_part, get_perlin_water_color(waterPos));
    }
    void get_underwater_fog(float3 waterWorldPos, float3 underWaterWorldPos, float3 view, float water_view_depth, float water_down_deep, float ocean_part, out half3 underwater_loss, out half3 underwater_inscatter)
    {
      half perlin_noise_water_color = get_perlin_water_color(waterWorldPos.xz);
      get_underwater_fog_perlin(waterWorldPos, underWaterWorldPos, view, water_view_depth, water_down_deep, ocean_part, perlin_noise_water_color, underwater_loss, underwater_inscatter);
    }
  }
endmacro


