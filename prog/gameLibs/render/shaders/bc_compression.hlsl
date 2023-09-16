#ifndef BC_COMPRESSION_MATH_HLSL
#define BC_COMPRESSION_MATH_HLSL 1
  half4 find_min_base_color(in half4 texels[16])
  {
    half4 minColor = texels[0];
    #define ONE_TEXEL(i) minColor = min(minColor, texels[i]);
    ONE_TEXEL(1)ONE_TEXEL(2)ONE_TEXEL(3)ONE_TEXEL(4)ONE_TEXEL(5)ONE_TEXEL(6)ONE_TEXEL(7)
    ONE_TEXEL(8)ONE_TEXEL(9)ONE_TEXEL(10)ONE_TEXEL(11)ONE_TEXEL(12)ONE_TEXEL(13)ONE_TEXEL(14)ONE_TEXEL(15)
    #undef ONE_TEXEL
    return minColor;
  }

  half4 find_max_base_color(in half4 texels[16])
  {
    half4 maxColor = texels[0];
    #define ONE_TEXEL(i) maxColor = max(maxColor, texels[i]);
    ONE_TEXEL(1)ONE_TEXEL(2)ONE_TEXEL(3)ONE_TEXEL(4)ONE_TEXEL(5)ONE_TEXEL(6)ONE_TEXEL(7)
    ONE_TEXEL(8)ONE_TEXEL(9)ONE_TEXEL(10)ONE_TEXEL(11)ONE_TEXEL(12)ONE_TEXEL(13)ONE_TEXEL(14)ONE_TEXEL(15)
    #undef ONE_TEXEL
    return maxColor;
  }

  // find reference colors
  void find_base_colors( in half4 texels[16], out half4 min_color, out half4 max_color )
  {
    min_color = find_min_base_color(texels);
    max_color = find_max_base_color(texels);
  }



  // refine reference colors
  void refine_rgb_base_colors( half4 texels[16], inout half4 min_color, inout half4 max_color )
  {
    float3 center = ( min_color.rgb + max_color.rgb ) * 0.5f;

    float2 cov = 0.f;

    UNROLL
    for ( int i = 0 ; i < 16 ; ++i )
    {
      float3 val = texels[i].rgb - center;
      cov.xy += val.xy * val.b;
    }
    /*#define COV(i) {float3 val = texels[i].rgb - center; cov.xy += val.xy * val.b;}
    COV(0);COV(1);COV(2);COV(3);
    COV(4);COV(5);COV(6);COV(7);
    COV(8);COV(9);COV(10);COV(11);
    COV(12);COV(13);COV(14);COV(15);*/

    float4 tmp_min = min_color;
    float4 tmp_max = max_color;

    min_color.r = cov.x < 0 ? tmp_max.r : tmp_min.r;
    max_color.r = cov.x < 0 ? tmp_min.r : tmp_max.r;

    min_color.g = cov.y < 0 ? tmp_max.g : tmp_min.g;
    max_color.g = cov.y < 0 ? tmp_min.g : tmp_max.g;

    float3 inset = ( max_color.rgb - min_color.rgb ) / 16.f;

    min_color.rgb = saturate( min_color.rgb + inset );
    max_color.rgb = saturate( max_color.rgb - inset );
  }
#endif