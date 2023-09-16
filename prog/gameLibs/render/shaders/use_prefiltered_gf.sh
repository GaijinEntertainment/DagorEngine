texture preIntegratedGF;
macro INIT_PREINTEGRATED_GF(code)
  (code) { preIntegratedGF@smp2d = preIntegratedGF; }
endmacro
macro BASE_USE_PREINTEGRATED_GF(code)
  hlsl(code) {
    half3 getEnviBRDF_AB(float ggx_alpha, float NoV)
    {
      return tex2Dlod( preIntegratedGF, float4( NoV, sqrt(ggx_alpha), 0, 0) ).rgb;
    }
    half3 getEnviBRDF_AB_LinearRoughness(float linear_roughness, float NoV)
    {
      return tex2Dlod( preIntegratedGF, float4( NoV, linear_roughness, 0, 0) ).rgb;
    }
    #define HAS_PREFILTERED_GF 1
  }
endmacro

macro USE_PREINTEGRATED_GF()
  INIT_PREINTEGRATED_GF(ps)
  BASE_USE_PREINTEGRATED_GF(ps)
endmacro
