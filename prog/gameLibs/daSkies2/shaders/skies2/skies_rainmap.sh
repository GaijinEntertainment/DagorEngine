include "clouds2/clouds_weather.sh"
include "clouds2/clouds_alt_fraction.sh"

texture clouds_weather_texture;
float skies_rain_strength = 0;
float skies_rain_absorbtion = 0.3;
float4 skies_rain_wind = (400, 0, 0, 0);

macro GET_SKIES_RAINMAP(code)
  (code){
    clouds_weather_texture@smp2d = clouds_weather_texture;
    clouds_rainmap_size@f4 = (skies_rain_strength*(1 + skies_rain_absorbtion), 1./clouds_weather_size,
      0.5 + clouds_origin_offset.x/clouds_weather_size, 0.5 + clouds_origin_offset.y/clouds_weather_size);
    clouds_rain_altitude_start_end@f4 = ((clouds_start_altitude2+0.6)*5 + 1, -5, (clouds_start_altitude2+0.6) + 0.2, skies_rain_strength);
    clouds_hole_pos_tex@tex = clouds_hole_pos_tex hlsl {Texture2D<float4> clouds_hole_pos_tex@tex;};
    skies_rain_wind@f2 = skies_rain_wind;
  }
  hlsl(code) {
    //todo: figure out how to generate rain map, and correct wind offset
    float getBaseRainmapAt(float altitude, float3 worldPos)
    {
      float2 uv = (worldPos.xz + (clouds_rain_altitude_start_end.z-altitude)*skies_rain_wind.xy)*clouds_rainmap_size.y + clouds_rainmap_size.zw + clouds_hole_pos_tex[uint2(0, 0)].zw;//todo: correct add wind offset
      float rain = saturate(tex2Dlod(clouds_weather_texture, float4(uv,0,0)).w*1.75-0.75);
      float rainAmount = rain*saturate(clouds_rain_altitude_start_end.x + clouds_rain_altitude_start_end.y*altitude);
      return rainAmount;
    }
    float getRainmapAt(float altitude, float3 worldPos)
    {
      if (clouds_rainmap_size.x == 0 || altitude > clouds_rain_altitude_start_end.z)
        return 0;
      return getBaseRainmapAt(altitude, worldPos);
    }
  }
endmacro

macro SAMPLE_SKIES_RAINMAP(code)
  GET_SKIES_RAINMAP(code)
  hlsl(code) {
    //todo: figure out how to generate rain map, and correct wind offset
    #define CUSTOM_SKIES_FOG 1
    void getSkiesCustomFog(inout float3 scatteringMie, inout float3 scatteringRay, inout float3 msScattering, inout float3 extinction, Length altitude, Position worldPos)
    {
      if (clouds_rainmap_size.x == 0 || altitude > clouds_rain_altitude_start_end.z)
        return;
      float rainAmount = getBaseRainmapAt(altitude, worldPos);
      float scatteringAmount = rainAmount*clouds_rain_altitude_start_end.w;
      //msScattering = msScattering + scatteringAmount;
      //scatteringMie = scatteringMie + scatteringAmount;
      msScattering = msScattering + scatteringAmount*0.75;
      scatteringMie = scatteringMie + scatteringAmount*0.25;
      extinction = extinction + rainAmount*clouds_rainmap_size.x;
    }
  }
endmacro
macro SKIES_RAINMAP(code)
  SAMPLE_SKIES_RAINMAP(code)
endmacro
