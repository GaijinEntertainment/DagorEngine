#ifndef BC_COMPRESSION_MATH_HLSL
#define BC_COMPRESSION_MATH_HLSL 1
  #include "eigenvector.hlsl"
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



  // correlate reference colors
  void correlate_rgb_base_colors( half4 texels[16], inout half4 min_color, inout half4 max_color )
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
  }

  void inset_rgb_base_colors( inout half4 min_color, inout half4 max_color )
  {
    float3 inset = ( max_color.rgb - min_color.rgb ) / 16.f;

    min_color.rgb = saturate( min_color.rgb + inset );
    max_color.rgb = saturate( max_color.rgb - inset );
  }

  // Find reference colors using Principal Component Analysis for RGB colors.
  static const float scale = pow(2, 10);
  static const float epsilon = scale*(1./128.)*pow(1./255., 2);
  void find_base_colors_pca_rgb( half4 texels[16], out half4 min_color, out half4 max_color )
  {
    half2 minMaxAlpha = half2(1, 0);

    int i = 0;
    for (i = 0; i < 16; ++i)
    {
      minMaxAlpha.x = min(texels[i].a, minMaxAlpha.x);
      minMaxAlpha.y = max(texels[i].a, minMaxAlpha.y);
    }

    half4 texels_mean = half4(0, 0, 0, 0);
    for (i = 0; i < 16; ++i)
      texels_mean.rgb += texels[i].rgb / 16;

    float Err = 0, Egg = 0, Ebb = 0, Erg = 0, Egb = 0, Erb = 0;
    for (i = 0; i < 16; ++i)
    {
      float3 ctex = texels[i].rgb - texels_mean.rgb;
      Err += ctex.r * ctex.r;
      Egg += ctex.g * ctex.g;
      Ebb += ctex.b * ctex.b;
      Erg += ctex.r * ctex.g;
      Egb += ctex.g * ctex.b;
      Erb += ctex.r * ctex.b;
    }

    Err = Err*scale + 1*epsilon;
    Egg = Egg*scale + 2*epsilon;
    Ebb = Ebb*scale + 3*epsilon;
    Erg = Erg*scale - 1.5*epsilon;
    Egb = Egb*scale - 2.5*epsilon;
    Erb = Erb*scale - 3.5*epsilon;

    float3 majorVec = normalize(majorEigenvector3x3sym(Err, Egg, Ebb, Erg, Egb, Erb));

    float2 minMaxProj = float2(100, -100);
    for (i = 0; i < 16; ++i)
    {
      float3 ctex = texels[i].rgb - texels_mean.rgb;
      float proj = dot(ctex, majorVec);
      minMaxProj.x = min(proj, minMaxProj.x);
      minMaxProj.y = max(proj, minMaxProj.y);
    }
    min_color = half4(majorVec*minMaxProj.x + texels_mean.rgb, minMaxAlpha.x);
    max_color = half4(majorVec*minMaxProj.y + texels_mean.rgb, minMaxAlpha.y);
  }
#endif