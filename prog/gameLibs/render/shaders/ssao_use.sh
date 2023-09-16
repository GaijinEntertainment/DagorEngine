include "ssao_inc.sh"
include "wsao.sh"

int use_contact_shadows = 0;
interval use_contact_shadows: no_shadows < 1, use_shadows;

hlsl {
  ##if use_contact_shadows != no_shadows
    #define SSAO_CONTACT_SHADOWS 1
  ##endif
  ##if gi_quality == only_ao
    #define SSAO_WSAO 1
  ##endif

  #define SSAO_ATTRS xyz
  #define WSAO_ATTR y
  #define CONTACT_SHADOWS_ATTR z

  #if SSAO_CONTACT_SHADOWS && SSAO_WSAO
    #define GATHER_WSAO GatherGreen
    #define GATHER_SHADOW GatherBlue
  #else
    #define GATHER_WSAO GatherGreen
    #define GATHER_SHADOW GatherGreen
  #endif
}

macro USE_PACK_SSAO_BASE(code)
  hlsl(code) {
    SSAO_TYPE setSSAO(SSAO_TYPE ssao)
    {
      #if SSAO_WSAO
      ssao.y = sqrt(ssao.y);
      #endif
      #if SSAO_CONTACT_SHADOWS && !SSAO_WSAO
      ssao.y = ssao.z;
      #endif
      return ssao;
    }
  }
  if (gi_quality == only_ao) {
    WSAO_BASE(code)
  }
endmacro

macro USE_PACK_SSAO()
  USE_PACK_SSAO_BASE(ps)
endmacro

macro USE_SSAO_SIMPLE_BASE_WITH_SMP(code, _samplerstate)
  USE_SCREENPOS_TO_LOWRES_TC_BASE(code)
  hlsl(code) {
    SSAO_TYPE getSSAOSimple(float2 screenpos)
    {
      SSAO_TYPE ao = 1;
      float2 ssaotc = screen_pos_to_lowres_tc(screenpos);
      float4 ssao = ssao_tex.SampleLevel(_samplerstate, ssaotc, 0);
      ao.x = ssao.x;
      #if SSAO_WSAO
      ao.WSAO_ATTR = ssao.WSAO_ATTR;
      ao.WSAO_ATTR *= ao.WSAO_ATTR;
      #endif
      #if SSAO_CONTACT_SHADOWS
      ao.CONTACT_SHADOWS_ATTR = ssao.CONTACT_SHADOWS_ATTR;
      #endif
      return ao;
    }
  }
endmacro

macro USE_SSAO_UPSCALE_BASE_WITH_SMP(code, _samplerstate)
  USE_SCREENPOS_TO_LOWRES_TC_BASE(code)
  USE_UPSCALE_SAMPLING_BASE(code)
  hlsl(code) {
    SSAO_TYPE getSSAOUpscale(float2 screenpos)
    {
      float2 ssaotc = screen_pos_to_lowres_tc(screenpos);
      float4 weights = SampleUpscaleWeight(screenpos);
      float4 ssaoValue = ssao_tex.GatherRed(_samplerstate, ssaotc);
      SSAO_TYPE ao = 1;
      ao.x = dot(ssaoValue, weights);
      #if SSAO_WSAO
      float4 wsaoValue = ssao_tex.GATHER_WSAO(_samplerstate, ssaotc);
      ao.WSAO_ATTR = dot(wsaoValue, weights);
      ao.WSAO_ATTR *= ao.WSAO_ATTR;
      #endif
      #if SSAO_CONTACT_SHADOWS
      float4 contactShadowValue = ssao_tex.GATHER_SHADOW(_samplerstate, ssaotc);
      ao.CONTACT_SHADOWS_ATTR = dot(contactShadowValue, weights);
      #endif
      return ao;
    }
  }
endmacro

macro USE_SSAO_BASE_WITH_SMP(code, _samplerstate)

  USE_SSAO_SIMPLE_BASE_WITH_SMP(code, _samplerstate)
  if (upscale_sampling_tex != NULL)
  {
    USE_SSAO_UPSCALE_BASE_WITH_SMP(code, _samplerstate)
  }
  hlsl(code) {
    ##if upscale_sampling_tex != NULL
      #define getSSAO getSSAOUpscale
    ##else
      #define getSSAO(screenpos) getSSAOSimple(screenpos)
    ##endif
  }
endmacro

macro USE_SSAO_BASE(code)
  USE_SSAO_BASE_WITH_SMP(code, ssao_tex_samplerstate)
endmacro

macro USE_SSAO()
  USE_SSAO_BASE(ps)
endmacro

macro USING_SSAO_BASE(code)
  if (ssao_tex != NULL) {
    INIT_SSAO_BASE(code)
    USE_SSAO_BASE(code)
  } else {
    hlsl(code) {
      #define getSSAO(screenpos) SSAO_TYPE(1, 1, 1)
    }
  }
endmacro

macro USING_SSAO()
  USING_SSAO_BASE(ps)
endmacro

macro USING_SSAO_BASE_COMPATIBILITY(code, _samplerstate)
  if (ssao_tex != NULL) {
    // INIT_SSAO_BASE without sampler2d
    INIT_UPSCALE_SAMPLING_BASE(code)
    USE_SSAO_BASE_WITH_SMP(code, _samplerstate)
  } else {
    hlsl(code) {
      #define getSSAO(screenpos) SSAO_TYPE(1, 1, 1)
    }
  }
endmacro

macro USING_SSAO_COMPATIBILITY(_samplerstate)
  USING_SSAO_BASE_COMPATIBILITY(ps, _samplerstate)
endmacro
