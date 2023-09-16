texture shadow_cascade_depth_tex;
float4 pcf_lerp = (0, 0, 0, 0);
float start_csm_shadow_blend = 100;

int num_of_cascades = 4;
interval num_of_cascades:two<3, three<4, four<5, five<6, six;

float4 shadow_cascade_tm_transp[24];    // max value of num_of_cascades (6) * 4
float4 shadow_cascade_tc_mul_offset[6];

macro INIT_CSM_SHADOW(code)
  (code)
  {
    shadow_cascade_depth_tex@shd = shadow_cascade_depth_tex;
  }
endmacro

macro USE_CSM_SHADOW(code, num_cascades, for_compatibility)
  (code)
  {
    start_csm_shadow_blend@f1 = (start_csm_shadow_blend);
    pcf_lerp@f4 = (pcf_lerp.x, pcf_lerp.y, pcf_lerp.x*2, pcf_lerp.y*2);
    shadow_cascade_tm_transp@f4[] = shadow_cascade_tm_transp;
    shadow_cascade_tc_mul_offset@f4[] = shadow_cascade_tc_mul_offset;
  }

  hlsl(code) {
    #ifndef NUM_CASCADES
    # define NUM_CASCADES num_cascades
    #endif

    #include "./csm_shadow_tc.hlsl"

    //##if is_gather4_supported == supported && !hardware.ps4
      //on DX11, gather4 is supported on HLSL4.1, as well as shadow texture array. However, it work SLOWER, than atlas
      //on OpenGL, shadow texture array should be part of OpenGL 3.3 Core, however, I can not check on a hradware which doesn't support textureGather,
      //if it still supports texture array. So, until checked, let's assume it is required
      //on PS4 everything is supported, but shadow texture array is not implemented yet
    //##else
    //  #define shadow_array_supported 0
    //##endif
    #if shadow_array_supported
      ##if hardware.ps4 || hardware.dx11 || hardware.vulkan || hardware.metal
      //SamplerComparisonState smSampler : register($shadow_cascade_depth_tex@smp);
      sampler2DArray shadow_cascade_depth_tex : register($shadow_cascade_depth_tex@smp);
      SamplerComparisonState csmSampler : register($shadow_cascade_depth_tex@shd);
      ##else
      #error unsupported target
      ##endif
    #else
      ##if hardware.ps4 || hardware.dx11 || hardware.vulkan || hardware.metal
      //SamplerComparisonState smSampler : register($shadow_cascade_depth_tex@smp);
      SamplerComparisonState csmSampler : register($shadow_cascade_depth_tex@shd);
      ##else
      ##endif
    #endif

    #if shadow_array_supported
      fixed texCSMShadow ( float2 uv, float z, float id )
      {
        return shadow2DArray(shadow_cascade_depth_tex, float4(uv.xy, id, z));
      }
    #else
      fixed texCSMShadow ( float2 uv, float z)
      {
        return shadow2D(shadow_cascade_depth_tex, float3(uv.xy, z));
      }
    #endif

    #if shadow_array_supported
    #define texCSMShadow2(a,b,c) texCSMShadow(a, b, c)
    half get_pcf_csm_shadow(float4 depthShadowTC)
    #else
    #define texCSMShadow2(a,b,c) texCSMShadow(a, b)
    half get_pcf_csm_shadow(float3 depthShadowTC)
    #endif
    {
 
        /*fixed4 luma = fixed4(
          texShadow(depthShadowTC00+depthShadowTC).r,
          texShadow(depthShadowTC01).r,
          texShadow(depthShadowTC10).r,
          texShadow(depthShadowTC11).r
        );*/
        ///*

        float2 fxaaConsoleRcpFrameOpt = pcf_lerp.xy;
        float2 pos = depthShadowTC.xy;
        float4 fxaaConsolePosPos = float4(pos - fxaaConsoleRcpFrameOpt, pos + fxaaConsoleRcpFrameOpt);
        fixed4 luma = fixed4(
                 texCSMShadow2(fxaaConsolePosPos.xy, depthShadowTC.z, depthShadowTC.w),
                 texCSMShadow2(fxaaConsolePosPos.xw, depthShadowTC.z, depthShadowTC.w),
                 texCSMShadow2(fxaaConsolePosPos.zy, depthShadowTC.z, depthShadowTC.w),
                 texCSMShadow2(fxaaConsolePosPos.zw, depthShadowTC.z, depthShadowTC.w));
         fixed4 dir = fixed4(
           dot(luma, fixed4(-1,1,-1,1)),
           dot(luma, fixed4(1,1,-1,-1)),
           0,0
         );
         fixed3 grad = fixed3(
           texCSMShadow2(depthShadowTC.xy, depthShadowTC.z, depthShadowTC.w).r,
           texCSMShadow2(depthShadowTC.xy - dir.xy * pcf_lerp.zw, depthShadowTC.z, depthShadowTC.w).r,
           texCSMShadow2(depthShadowTC.xy + dir.xy * pcf_lerp.zw, depthShadowTC.z, depthShadowTC.w).r
         ); 
         return saturate(dot(grad, fixed3(0.2, 0.4, 0.4) ));
        /*/
        float2 fxaaConsoleRcpFrameOpt = pcf_lerp.xy;
        float2 pos = depthShadowTC.xy;
        float4 fxaaConsolePosPos = float4(pos - fxaaConsoleRcpFrameOpt, pos + fxaaConsoleRcpFrameOpt);
        half lumaNw = texShadow(float4(fxaaConsolePosPos.xy, depthShadowTC.zw));
        half lumaSw = texShadow(float4(fxaaConsolePosPos.xw, depthShadowTC.zw));
        half lumaNe = texShadow(float4(fxaaConsolePosPos.zy, depthShadowTC.zw));
        half lumaSe = texShadow(float4(fxaaConsolePosPos.zw, depthShadowTC.zw));

        half centerTap = texShadow(depthShadowTC);

        half dirSwMinusNe = lumaSw - lumaNe;
        half dirSeMinusNw = lumaSe - lumaNw;
                             
        float2 dir;
        dir.x = dirSwMinusNe + dirSeMinusNw;
        dir.y = dirSwMinusNe - dirSeMinusNw;

        float2 dir1 = dir;
        half rgbyN1 = texShadow(float4(pos.xy - dir1 * pcf_lerp.zw, depthShadowTC.zw));
        half rgbyP1 = texShadow(float4(pos.xy + dir1 * pcf_lerp.zw, depthShadowTC.zw));

        half rgbyA = (rgbyN1 + rgbyP1);
        rgbyA = centerTap*0.33333 + rgbyA*0.333335;
        //rgbyA = rgbyA*0.5;
        //rgbyA = centerTap*0.5 + rgbyA*0.25;

        return saturate ( 1.0 - rgbyA ); 
        //*/
    }
    half3 get_csm_shadow(float3 pointToEye, float w, float sel_scale = 1)
    {
      uint cascade_id;
      float shadowEffect = 0;
      float3 tlast;
      float3 t = get_csm_shadow_tc(pointToEye, sel_scale, cascade_id, shadowEffect, tlast);
      float3 abstlast = abs(tlast);
      FLATTEN
      if (t.z < 1)
      {
        half csmShadow = get_pcf_csm_shadow(t);
        half blend = saturate(10*max(max(abstlast.x, abstlast.y), abstlast.z)-4);
        //blend = w > start_csm_shadow_blend ? blend : 0;
        return half3(csmShadow, cascade_id, cascade_id == 3 ? blend : 0);
      }
      return half3( 1, cascade_id, 1 );
      //todo: blend with weights?
    }
  }
endmacro

macro USE_CSM_SHADOW_DEF_NUM()
  if (num_of_cascades == two)
  {
    USE_CSM_SHADOW(ps, 2, dummy)
  }
  else if (num_of_cascades == three)
  {
    USE_CSM_SHADOW(ps, 3, dummy)
  }
  else if (num_of_cascades == four)
  {
    USE_CSM_SHADOW(ps, 4, dummy)
  }
  else if (num_of_cascades == five)
  {
    USE_CSM_SHADOW(ps, 5, dummy)
  }
  else
  {
    USE_CSM_SHADOW(ps, 6, dummy)
  }
endmacro