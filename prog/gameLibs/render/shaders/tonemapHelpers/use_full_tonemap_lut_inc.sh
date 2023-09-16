texture full_tonemap_lut;
texture graded_color_lut;

macro INIT_FULL_TONEMAP_LUT(code)
  (code) {
    full_tonemap_lut@smp3d = full_tonemap_lut;
    graded_color_lut@tex3d = graded_color_lut;
  }
endmacro

macro USE_FULL_TONEMAP_LUT(code)
  hlsl(code) {
    #include <tonemapHelpers/tonemapLUTSpace.hlsl>
    half3 performLUTTonemap(float3 linearColor)
    {
      float3 LUTlookUp = lut_linear_to_log( linearColor + lut_linear_to_log( 0 ) );
      float LUTSize = 32;//can be parametrized
      float3 uvw = LUTlookUp * ((LUTSize - 1) / LUTSize) + (0.5f / LUTSize);
      half3 outDeviceColor = h3tex3D(full_tonemap_lut, uvw).rgb;
      return outDeviceColor;
    }
    half3 performLUTColorGrade(float3 linearColor)
    {
      float3 LUTlookUp = lut_linear_to_log( linearColor + lut_linear_to_log( 0 ) );
      float LUTSize = 33;//can be parametrized
      float3 uvw = LUTlookUp * ((LUTSize - 1) / LUTSize) + (0.5f / LUTSize);
      half3 outDeviceColor = half3(graded_color_lut.Sample(full_tonemap_lut_samplerstate, uvw).rgb);
      return outDeviceColor;
    }
  }
endmacro

macro FULL_TONEMAP_LUT_APPLY(code)
  INIT_FULL_TONEMAP_LUT(code)
  USE_FULL_TONEMAP_LUT(code)
endmacro