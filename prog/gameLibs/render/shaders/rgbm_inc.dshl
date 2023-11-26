float rgbm_conversion_scale = 6;

macro USE_RGBM_SH(stage)
  (stage) { rgbm_conversion_scale@f1 = (rgbm_conversion_scale); }
  hlsl(stage) {
    #define rgbm_scale_coeff 6.0f
    #define inv_rgbm_scale_coeff (1./6.0f)

    half4 encode_rgbm(half3 hdr_color, half scale)
    {
      half4 rgbm;
      hdr_color *= 1.0 / scale; //rgbm_scale_coeff
      rgbm.a = saturate(max(max(hdr_color.r, hdr_color.g), max(hdr_color.b, 1e-5)));
      rgbm.a = ceil(rgbm.a * 255.0) / 255.0; //quantize range
      rgbm.rgb = hdr_color / rgbm.a;
      return rgbm;
    }

    half4 decode_rgbm(half4 packed_color, half scale)
    {
      packed_color.rgb *= packed_color.a * scale;
      return packed_color;
    }

    half4 def_encode_rgbm(half3 hdr_color)
    {
      half4 rgbm;
      hdr_color *= inv_rgbm_scale_coeff; //rgbm_scale_coeff
      rgbm.a = saturate(max(max(hdr_color.r, hdr_color.g), max(hdr_color.b, 1e-5)));
      rgbm.a = ceil(rgbm.a * 255.0) * (1. / 255.0); //quantize range
      rgbm.rgb = hdr_color * rcp(rgbm.a);
      return rgbm;
    }

    half3 def_decode_rgbm(half4 packed_color)
    {
      packed_color.rgb *= packed_color.a * rgbm_scale_coeff;
      return packed_color.rgb;
    }

  }
endmacro

macro USE_DEF_RGBM_SH()
  USE_RGBM_SH(ps)
endmacro

