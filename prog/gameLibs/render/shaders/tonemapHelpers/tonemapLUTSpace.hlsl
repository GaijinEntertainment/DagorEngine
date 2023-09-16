#ifndef TONEMAP_LUT_SPACE_HLSL_INCLUDE
#define TONEMAP_LUT_SPACE_HLSL_INCLUDE 1
float3 lut_log_to_linear(float3 logSpaceColor)
{
  float GAIN = 128.f;
  float SCALE =3.399738f;
  //max at ~20
  return exp2(logSpaceColor*(3.321928094887362*SCALE))*(1/GAIN) - 1/GAIN;//mul, mad, exp2
  //float linearRange = 14;
  //float linearGrey = 0.18;
  //float exposureGrey = 444;
  //higher dynamic range, max at 44, error is smaller below 0.035 linear and bigger everywhere else
  //return exp2( ( logSpaceColor * linearRange - exposureGrey * linearRange / 1023.0 ) ) * linearGrey;//mad, mul, exp2
}
float3 lut_linear_to_log( float3 linearColor )
{

  float GAIN = 128.f;
  float SCALE =3.399738f;
  //max at ~20
  return saturate(log2(linearColor*GAIN+1)*(0.3010299956639812/SCALE));//mad, log2, mul

  //float linearRange = 14;
  //float linearGrey = 0.18;
  //float exposureGrey = 444;
  //higher dynamic range, max at 44, error is smaller below 0.035 linear and bigger everywhere else
  //return saturate(log2(linearColor) / linearRange - log2(linearGrey) / linearRange + exposureGrey / 1023.0);//log2, mad, add
}
#endif
