texture skies_transmittance_texture;

texture skies_view_lut_tex;
texture skies_view_lut_mie_tex;
float4 skies_min_max_horizon_mu;

macro SKIES_LUT_ENCODING(code)
  hlsl(code) {
    #include <skies_lut_encoding.hlsl>
  }
endmacro

macro INIT_SKIES_LUT(code)
  (code) {
    skies_min_max_horizon_mu@f4 = skies_min_max_horizon_mu;
    skies_view_lut_tex@smp2d = skies_view_lut_tex;
    skies_view_lut_mie_tex@smp2d = skies_view_lut_mie_tex;
  }
endmacro

macro USE_SKIES_LUT(code)
  SKIES_LUT_ENCODING(code)
  hlsl(code) {
    float4 sample_skies_scattering_color_tc(float2 lut_tc, float3 view, float mie_phase)
    {
      float4 noPhased = tex2Dlod(skies_view_lut_tex, float4(lut_tc, 0,0));
      float4 miePhased = tex2Dlod(skies_view_lut_mie_tex, float4(lut_tc, 0,0))*mie_phase;
      return noPhased + miePhased;
    }
    float2 get_skies_lut_tc(float2 screen_texcoord, float3 view)
    {
      float2 lutTexcoord = screen_texcoord;
      lutTexcoord.y = skies_view_to_tc_y(lutTexcoord.y, view.y, skies_min_max_horizon_mu);
      return lutTexcoord;
    }
    float4 sample_skies_scattering_color_camera(float2 screen_tc, float3 view, float mie_phase)
    {
      return sample_skies_scattering_color_tc(get_skies_lut_tc(screen_tc, view), view, mie_phase);
    }
  }
endmacro
