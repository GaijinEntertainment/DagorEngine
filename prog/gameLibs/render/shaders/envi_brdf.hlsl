#ifndef ENVI_BRDF_HLSL
#define ENVI_BRDF_HLSL 1

#ifndef INV_MIN_IOR
#define INV_MIN_IOR 50.0h
#endif

//---------------
// EnvBRDF
//---------------

half2 get_EnvBRDF_AB_Approx( half linear_roughness, half NoV )
{
  // [ Lazarov 2013, "Getting More Physical in Call of Duty: Black Ops II" ]
  // Adaptation to fit our G term.
  const half4 c0 = { -1.h, -0.0275h, -0.572h, 0.022h };
  const half4 c1 = { 1.h, 0.0425h, 1.04h, -0.04h };
  half4 r = linear_roughness * c0 + c1;
  half a004 = min( r.x * r.x, exp2( -9.28h * NoV ) ) * r.x + r.y;
  return half2( -1.04h, 1.04h ) * a004 + r.zw;
}

half3 EnvBRDFApprox( half3 specularColor, half linear_roughness, half NoV )
{
  // [ Lazarov 2013, "Getting More Physical in Call of Duty: Black Ops II" ]
  // Adaptation to fit our G term.
  half2 AB = get_EnvBRDF_AB_Approx( linear_roughness, NoV );
  return specularColor * AB.x + AB.yyy*saturate(INV_MIN_IOR*specularColor.g);
}


half EnvBRDFApproxNonmetal( half linear_roughness, half NoV )
{
  // Same as EnvBRDFApprox( 0.04, roughness, NoV )
  const half2 c0 = { -1.h, -0.0275h };
  const half2 c1 = { 1.h, 0.0425h };
  half2 r = linear_roughness * c0 + c1;
  return min( r.x * r.x, exp2( -9.28h * NoV ) ) * r.x + r.y;
}

#endif
