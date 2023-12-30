#ifndef STATIC_SHADOW_INT_HLSL
#define STATIC_SHADOW_INT_HLSL 1


  #if STATIC_SHADOW_CASCADE_NUM != 0
    #ifndef STATIC_SHADOW_USE_CASCADE_0
      #define STATIC_SHADOW_USE_CASCADE_0 1
    #endif
  #endif

  #if STATIC_SHADOW_CASCADE_NUM == 2
    #ifndef STATIC_SHADOW_REFERENCE_FXAA_IMPL
      #define STATIC_SHADOW_REFERENCE_FXAA_IMPL 1
    #endif

    #ifndef STATIC_SHADOW_USE_CASCADE_1
      #define STATIC_SHADOW_USE_CASCADE_1 1
    #endif
  #else
    #ifndef STATIC_SHADOW_CUSTOM_FXAA_DIR
      #define STATIC_SHADOW_CUSTOM_FXAA_DIR 1
    #endif
  #endif

  //To keep the same interface as the toroidal shadow map
  float3 get_static_shadow_tc_base(float3 worldPos, float4 shadowWorldRenderMatrix0, float4 shadowWorldRenderMatrix1, float4 shadowWorldRenderMatrix2, float4 shadowWorldRenderMatrix3)
  {
    return worldPos.x*shadowWorldRenderMatrix0.xyz + worldPos.y*shadowWorldRenderMatrix1.xyz + worldPos.z*shadowWorldRenderMatrix2.xyz + shadowWorldRenderMatrix3.xyz;
  }
  float4 get_static_shadow_tc(float3 tc, float2 ofs_tor, float dither, bool hard_vignette)
  {
    float2 vignette = saturate(abs(tc.xy) * 20 - 19);
    float vignetteVal = hard_vignette ? max(abs(tc.x), abs(tc.y)) < 511./512+dither*-1./256: saturate(1.0 - dot(vignette, vignette)) ;
    float3 uv = float3(tc.xy * 0.5 + ofs_tor, tc.z);//todo: check xbox microcode, may be we can optimize it by smashing into one constant, i.e. tc.xy*ofs_tor.x + ofs_tor.yz
    return float4(uv, vignetteVal);
  }

#endif
