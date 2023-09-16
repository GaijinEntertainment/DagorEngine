include "static_shadow_stcode.sh"

macro STATIC_SHADOW_DEFINE_SAMPLER_TYPE(code)
  hlsl(code)
  {
      #define sampler_state_ref SamplerComparisonState

    ##if (hardware.fsh_5_0)
      #define texture_ref Texture2DArray<float4>

      half static_shadow_sample(float2 uv, float z, texture_ref tex, sampler_state_ref tex_cmpSampler, float layer)
      {
        return (half)shadow2DArray(tex, float4(uv, layer, z));
      }
    ##else
      #define texture_ref Texture2D<float4>

      half static_shadow_sample(float2 uv, float z, texture_ref tex, sampler_state_ref tex_cmpSampler, float layer)
      {
        return half(shadow2D(tex, float3(uv, z)));
      }
    ##endif
  }
endmacro

macro USE_STATIC_SHADOW_BASE_INT(code, num_cascades)
  hlsl(code)
  {
    #if HAS_STATIC_SHADOW

      #if num_cascades != 0
        #ifndef STATIC_SHADOW_USE_CASCADE_0
          #define STATIC_SHADOW_USE_CASCADE_0 1
        #endif
      #endif

      #if num_cascades == 2
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
      float3 get_static_shadow_tc_base(float3 worldPos, float4 shadowWorldRenderMatrix[4])
      {
        return worldPos.x*shadowWorldRenderMatrix[0].xyz + worldPos.y*shadowWorldRenderMatrix[1].xyz + worldPos.z*shadowWorldRenderMatrix[2].xyz + shadowWorldRenderMatrix[3].xyz;
      }
      float4 get_static_shadow_tc(float3 tc, float2 ofs_tor, float dither, bool hard_vignette)
      {
        float2 vignette = saturate(abs(tc.xy) * 20 - 19);
        float vignetteVal = hard_vignette ? max(abs(tc.x), abs(tc.y)) < 511./512+dither*-1./256: saturate(1.0 - dot(vignette, vignette)) ;
        float3 uv = float3(tc.xy * 0.5 + ofs_tor, tc.z);//todo: check xbox microcode, may be we can optimize it by smashing into one constant, i.e. tc.xy*ofs_tor.x + ofs_tor.yz
        return float4(uv, vignetteVal);
      }
    #endif
  }
endmacro

macro INIT_STATIC_SHADOW_PS()
  INIT_STATIC_SHADOW_BASE(ps)
endmacro

macro INIT_STATIC_SHADOW_BLOCK_PS()
  INIT_STATIC_SHADOW_BASE_ONE_CASCADE(ps)
  INIT_STATIC_SHADOW_BASE_SECOND_CASCADE(ps)
endmacro

macro INIT_STATIC_SHADOW_VS()
  INIT_STATIC_SHADOW_BASE(vs)
endmacro

macro INIT_STATIC_SHADOW_CS()
  INIT_STATIC_SHADOW_BASE(cs)
endmacro

macro INIT_STATIC_SHADOW()
  INIT_STATIC_SHADOW_PS()
endmacro

macro INIT_STATIC_SHADOW_VS_ONE_CASCADE()
  INIT_STATIC_SHADOW_BASE_ONE_CASCADE(vs)
endmacro

macro USE_STATIC_SHADOW_BASE_NUM(code, num_cascades)
  STATIC_SHADOW_DEFINE_SAMPLER_TYPE(code)
  USE_STATIC_SHADOW_BASE_INT(code, num_cascades)

  if (mobile_render == forward)
  {
    hlsl(code){
      #ifndef FASTEST_STATIC_SHADOW_PCF
      # define FASTEST_STATIC_SHADOW_PCF 1
      #endif
    }
  }

  hlsl(code){
    #if HAS_STATIC_SHADOW
    #include <static_shadow.hlsl>
    #endif
  }
endmacro

macro USE_STATIC_SHADOW_BASE(code)
  if (static_shadows_cascades == two)
  {
    USE_STATIC_SHADOW_BASE_NUM(code, 2)
  }
  else if (static_shadows_cascades == one)
  {
    USE_STATIC_SHADOW_BASE_NUM(code, 1)
  }
  else if (static_shadows_cascades == off)
  {
    USE_STATIC_SHADOW_BASE_NUM(code, 0)
  }
endmacro

macro USE_STATIC_SHADOW()
  if (!hardware.fsh_5_0 && static_shadows_cascades == two)
  {
    dont_render;
  }

  hlsl(ps) {
    #define HAS_STATIC_SHADOW 1
  }
  USE_STATIC_SHADOW_BASE(ps)
endmacro

macro USE_STATIC_SHADOW_ONE_CASCADE(code)
  hlsl(code) { #define HAS_STATIC_SHADOW 1}
  USE_STATIC_SHADOW_BASE_NUM(code, 1)
endmacro

macro USE_STATIC_SHADOW_VS()
  hlsl(vs) {
    #define HAS_STATIC_SHADOW 1
  }
  USE_STATIC_SHADOW_BASE(vs)
endmacro

macro USE_STATIC_SHADOW_VS_ONE_CASCADE()
  hlsl(vs) {
    #define STATIC_SHADOW_USE_CASCADE_1 0
    #define HAS_STATIC_SHADOW 1
  }
  USE_STATIC_SHADOW_BASE(vs)
endmacro

macro USE_STATIC_SHADOW_CS()
  hlsl(cs) {
    #define HAS_STATIC_SHADOW 1
  }
  USE_STATIC_SHADOW_BASE(cs)
endmacro
