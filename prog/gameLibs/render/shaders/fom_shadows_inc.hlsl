#ifndef FX_FOM_INC
#define FX_FOM_INC

struct FOM_DATA
{
  half4 opacity_cos : SV_Target0;
  half4 opacity_sin : SV_Target1;
};

FOM_DATA fx_fom_null()
{
  FOM_DATA fom_result;
  fom_result.opacity_cos = 0;
  fom_result.opacity_sin = 0;
  return fom_result;
}

FOM_DATA fx_fom_calc( float fom_distance, float fom_opacity )
{
  float4 factor_a = (fom_distance*2.0*PI)*float4(0.0,1.0,2.0,3.0);
  float4 factor_b = (fom_distance*2.0*PI)*float4(1.0,2.0,3.0,0.0);
  float factor_m2 = -2.0;
  #if FX_FOM_DOUBLE_INTESITY
    factor_m2 *= 2.f;
  #endif

  //compute value for projection into Fourier basis
  float4 cos_a0123 = cos(factor_a);
  float4 sin_b123  = sin(factor_b);
  float lnOpacitR = factor_m2*log(1-fom_opacity);
  //lnOpacitR = 0.01;
  //clip(input.color.a-0.01);

  //projection of extinction coefficient into the Fourier basis and storage into textures
  //we need 6 render targets because we use a Fourier series for each color component.
  //One could only used luminance extinction
  FOM_DATA fom_result;
  fom_result.opacity_cos  = lnOpacitR*cos_a0123;
  fom_result.opacity_sin  = lnOpacitR*sin_b123;
  return fom_result;
}


#endif