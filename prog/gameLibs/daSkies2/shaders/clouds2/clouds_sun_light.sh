include "clouds_alt_fraction.sh"

texture clouds_light_color;

macro CLOUDS_SUN_SKY_LIGHT_COLOR(code)
  TOTAL_CLOUDS_MIN_MAX()
  (code) {
    alt_pos_to_clouds_light_alt@f2 = (0.001/maxCloudsThickness, -minCloudsAlt/maxCloudsThickness, 0, 0);
    clouds_light_color@smp3d = clouds_light_color;
  }
  hlsl(code) {
    #include <cloud_settings.hlsli>
    void get_sun_sky_light(float3 worldPos, out half3 sun_light, out half3 amb_color)
    {
      float relAlt = saturate(worldPos.y*alt_pos_to_clouds_light_alt.x + alt_pos_to_clouds_light_alt.y);//should be get_alt_fraction, actually
      float tcZ = relAlt*((CLOUDS_LIGHT_TEXTURE_WIDTH-1.)/(2.*CLOUDS_LIGHT_TEXTURE_WIDTH)) + 0.5/(2*CLOUDS_LIGHT_TEXTURE_WIDTH);
      float2 tcXY = worldPos.xz*1./(2.*MAX_CLOUDS_DIST_LIGHT_KM*1000.) + 0.5;
      sun_light=tex3Dlod(clouds_light_color, float4(tcXY, tcZ,0)).rgb;
      amb_color=tex3Dlod(clouds_light_color, float4(tcXY, tcZ + 0.5, 0)).rgb;
    }
  }
endmacro
