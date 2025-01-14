#include <pbr/BRDF.hlsl>


#ifndef APPLY_VOLUMETRIC_FOG
  #undef FX_USE_FOG_PS_APPLY
  #define FX_USE_FOG_PS_APPLY 0 // no per-pixel fog apply if we don't have volfog at all
#endif


#if FX_DECL
  struct VsOutput
  {
    VS_OUT_POSITION(pos)
    float4 texcoord           : TEXCOORD0;
    float4 color              : TEXCOORD1;

  #if FX_USE_FOG_PS_APPLY
    float3 scatteringColor : TEXCOORD2;
    float4 scatteringBase : TEXCOORD11;
  #elif FX_NEED_FOG_ADD
    float3 fogAdd    : TEXCOORD2;
  #endif

  #if FX_NEED_WORLD_POS
    float4 worldPos_specFade : TEXCOORD3;
  #endif

  #if FX_NEED_POINT_TO_EYE
    float4 pointToEye        : TEXCOORD3;
  #elif FX_NEED_VIEW_VECS
    float4 viewVec_specFade  : TEXCOORD3;
    float3 right             : TEXCOORD4;
    float3 up                : TEXCOORD5;
  #endif

  #if FX_NEED_SCREEN_TC
    float4 screenTexcoord    : TEXCOORD8;
  #endif

  #if FX_NEED_SOFTNESS_DEPTH_RCP
    float softnessDepthRcp   : TEXCOORD7;
  #endif

    float framesBlend        : TEXCOORD6;

  #if FX_NEED_SHADER_ID
    uint shaderId            : TEXCOORD9;
  #endif
  };
#endif

// x - ablend particles color
// y - ablend particles reverse color
// z - premult_alpha
// w - additive particles

#if FX_PS && FX_HAS_NORMAL_MAP
  float calc_normal_map_spec(VsOutput input, inout half4 diff)
  {
    half4 normalMap = tex2D(tex_nm, input.texcoord.xy);
    BRANCH
    if (input.framesBlend > 0)
      normalMap = lerp(normalMap, tex2D(tex_nm, input.texcoord.zw), input.framesBlend);
    half3 localNormal = unpack_ag_normal(normalMap);
    half specularStr = normalMap.r;
  #if FX_NEED_WORLD_POS
    float3 planeNormal = normalize(cross(ddx( input.worldPos_specFade.xyz ), ddy( input.worldPos_specFade.xyz )));
    float3 worldNormal = normalize(perturb_normal_precise( localNormal, planeNormal, input.worldPos_specFade.xyz, input.texcoord.xy ));
    specularStr *= input.worldPos_specFade.w;
  #endif
  #if FX_NEED_VIEW_VECS
    float3 planeNormal = input.viewVec_specFade.xyz;
    float3 worldNormal = normalize(input.viewVec_specFade.xyz * localNormal.z + input.right * localNormal.y + input.up * localNormal.x);
    specularStr *= input.viewVec_specFade.w;
  #endif

    //reflection vector
    float3 reflectedView = normalize(reflect( -planeNormal, worldNormal ));

    //apply simple lighting for diffuse component
    const float wrap_koef = 1.0;
    diff.xyz *= (saturate(dot(worldNormal, -from_sun_direction.xyz)) + wrap_koef) / (1.0 + wrap_koef);

    //specular highlight sharpness depends on its strength (small bright vs large dull)
    float specPower = 8.0 + 24 * specularStr;
    float spec = pow ( max ( dot ( -from_sun_direction.xyz, reflectedView), 0.0 ), specPower ) * specularStr;
    return spec;
  }
#endif

#if FX_PS
  void fx_common_ps(VsOutput input, out half4 diff, out half4 result, out half3 fog_add, half clip_bias )
  {
 #if FX_FORCE_INVALID_TEX
    // diff = half4(1, 0.001, 0.001, 0.999);
    diff = half4(1, 0.001, 0.001, 0.01);
 #else
    diff = tex2D(tex, input.texcoord.xy);
  #if FX_USE_FRAME_BLEND
    BRANCH
    if (input.framesBlend > 0)
      diff = lerp(diff, tex2D(tex, input.texcoord.zw), input.framesBlend);
  #endif
 #endif

    result = diff * input.color; // input.color premultiplied by fogMul in VS (per-vertex fog apply)

    fog_add = 0;
#if FX_NEED_FOG_REQ && FX_USE_FOG_PS_APPLY
    float3 pointToEye = input.pointToEye.xyz;
    float2 screenTc = input.screenTexcoord.xy/input.screenTexcoord.w;
    float w = input.screenTexcoord.w;

    float dist2 = dot(pointToEye, pointToEye);
    float rdist = rsqrt(dist2);

    half3 fogMul = 1; // for per-pixel fog apply
    get_volfog_with_precalculated_scattering_fast(screenTc, screenTc, pointToEye*rdist, dist2*rdist, w,
      input.scatteringColor, input.scatteringBase,
      fogMul, fog_add);
    result.rgb *= fogMul;
  #if !FX_NEED_FOG_ADD
    fog_add = 0;
  #endif
#elif FX_NEED_FOG_ADD
    fog_add = input.fogAdd;
#endif


#if FX_PS && FX_HAS_NORMAL_MAP
    float spec = calc_normal_map_spec(input, result);
    result.rgb += sun_color_0.rgb * spec;
#endif

 #if !FX_IS_ADDITIVE
    clip_alpha(result.a + clip_bias);
 #endif

#if TOONSHADING
    result.a = saturate(pow(result.a * 4, toon_fx_params.x) / 4);
#endif

#if FX_SPECIAL_VISION && !FX_SHADER_HAZE && !FX_PLANAR_PROJECTED
    {
      diff.a = (1 - pow2(1 - diff.a * input.color.a));
  #if FX_IS_PREMULTALPHA_DYNAMIC
      if (IS_ADDITIVE)
      {
        result = fx_thermals_apply_additive( result );
      }
      else if(IS_ABLEND)
      {
        result = fx_thermals_apply_ablend( result );
      }
      else
      {
        result = fx_thermals_apply_premult( result );
      }
  #endif
    }
#endif
  }
  void fx_common_ps(VsOutput input, out half4 diff, out half4 result, half clip_bias )
  {
    half3 fogAdd;
    fx_common_ps(input, diff, result, fogAdd, clip_bias);
  }

  float apply_depth_mask( VsOutput input, inout half4 result )
  {
 #if FX_NEED_SOFTNESS_DEPTH_RCP
    float softness_depth_rcp_value = input.softnessDepthRcp;
 #else
    float softness_depth_rcp_value = softness_depth_rcp_reg;
 #endif

 #if FX_USE_DEPTH_MASK
    half depthMask = get_soft_depth_mask(input.screenTexcoord, softness_depth_rcp_value);
    if (depthMask <= 0)
      discard;

    result.a *= depthMask;

    return depthMask;
 #else
    return 1;
 #endif
  }

#endif

#if FX_REFRACTION && FX_PS
  half4 fx_ps(VsOutput input) : SV_Target
  {
    half4 diff, result;
    fx_common_ps( input, diff, result, 0 );
    #if FX_HAS_NORMAL_MAP
      half4 normalMap = tex2D(tex_nm, input.texcoord.xy);
      half3 localNormal = unpack_ag_normal(normalMap);
      half3 specularStr = normalMap.r;
    #else
      half3 localNormal = half3(0, 0, 1);
      half specularStr = 0;
    #endif
    float3 planeNormal = normalize(input.pointToEye.xyz);
    float3 worldNormal = normalize(perturb_normal_precise(localNormal, planeNormal, input.pointToEye.xyz, input.texcoord.xy));
    const float WATER_IOR = 1./1.333;
    const float RAY_LENGTH = 0.02;
    const float ROUGHNESS_MIP = 0.0;
    float3 cameraToPoint = -input.pointToEye.xyz;
    float3 view = normalize(cameraToPoint);
    float3 refractWorldDir = refract(view, worldNormal, WATER_IOR);
    result.xyz *= 2 * fetchRefractedPixel(input.screenTexcoord.xy, cameraToPoint, refractWorldDir, RAY_LENGTH, ROUGHNESS_MIP).xyz;
    {
      // reflection
      specularStr *= input.pointToEye.w;
      const float smoothness = 0.9;
      float glassSpecular = 0.02037; // pow2((WATER_IOR-1)/(WATER_IOR+1));
      half4 diffuseColor = result;
      #include <lightGlassInc.hlsl>
      result.rgb = lerp(result.rgb * (1 - fresnel), diffuseTerm, diffuseColor.a) + specularTerm;
    }
    #if FX_USE_LUT
      result.rgb = applyLUT(result.rgb);
    #endif
    return result;
  }


  // normal
#elif FX_SHADER_GBUFFER_ATEST && FX_PS
  #if FX_WRITE_MOTION_VECTORS
    GBUFFER_OUTPUT fx_ps(VsOutput input HW_USE_SCREEN_POS)
  #else
    GBUFFER_OUTPUT fx_ps(VsOutput input)
  #endif
  {
    half4 diff, result;
    fx_common_ps( input, diff, result, 0 );
    UnpackedGbuffer gbuffer;
    init_gbuffer(gbuffer);

  #if FX_HAS_NORMAL_MAP
    half4 normalMap = tex2D(tex_nm, input.texcoord.xy);
    BRANCH
    if (input.framesBlend > 0)
      normalMap = lerp(normalMap, tex2D(tex_nm, input.texcoord.zw), input.framesBlend);
    half3 localNormal = unpack_ag_normal(normalMap);
    half specularStr = normalMap.r;
  #else
    half3 localNormal = half3(0, 0, 1);
    half specularStr = 0;
  #endif

    float3 planeNormal = normalize(input.pointToEye.xyz);
    float3 worldNormal = normalize(perturb_normal_precise(localNormal, planeNormal, input.pointToEye.xyz, input.texcoord.xy));
  #if FX_WRITE_MOTION_VECTORS
    // Fake reprojected motion vectors, but shaders that declare themselves dynamic to gbuffer has to provide
    // motion vectors too
    float2 screenUv = GET_SCREEN_POS(input.pos).xy * gbuf_texel_size;
    float3 motionInUvSpace = get_reprojected_motion_vector1(float3(screenUv, input.pos.w), -input.pointToEye.xyz, jitteredCamPosToUnjitteredHistoryClip);
    init_motion_vector(gbuffer, motionInUvSpace);
  #endif
    init_material(gbuffer, SHADING_SUBSURFACE);
    init_dynamic(gbuffer, 1);
    init_translucency(gbuffer, 1);
    init_sss_profile(gbuffer, 0xFE);
    init_smoothness(gbuffer, specularStr);
    init_reflectance(gbuffer, 0.2);
    init_normal(gbuffer, worldNormal);
    init_albedo(gbuffer, result.rgb);
    return encode_gbuffer(gbuffer, planeNormal, GET_SCREEN_POS(input.pos));
  }
#endif

#if FX_SHADER_PREMULTALPHA && FX_PS
  half4 fx_ps(VsOutput input) : SV_Target0
  {
    half4 diff, result;
    half3 fogAdd;
    fx_common_ps( input, diff, result, fogAdd, 0);
    half depthMask = apply_depth_mask(input, result);

    #if FX_IS_PREMULTALPHA_DYNAMIC
    if (IS_ADDITIVE)
    {
      result.rgb *= result.a;
      result.a = 0;
      result.rgb = fx_pack_hdr(result.rgb).rgb;
    }
    else if (IS_ABLEND)
    {
      result.rgb += fogAdd;
      result.rgb *= result.a;
      result.rgb = fx_pack_hdr(result.rgb).rgb;
    }
    else
    #endif
    {
      half blendWeight = diff.a * input.color.a;
      half3 blend = result.rgb * blendWeight;           // Decompose to BLEND and ADD parts.
      half3 add = result.rgb * (1.0098 - blendWeight);  // Next smallest half after 1. Avoid roots of negative values in pack_hdr.

      blend = blend + fogAdd;
      blend = fx_pack_hdr(blend).rgb;
      blend *= result.a;                        // Blend BLEND part as S*SA + D*ISA

      add *= depthMask;
      add = fx_pack_hdr(add).rgb;                      // Separate non-linear packing.
      result.rgb = blend + add;                 // Combine BLEND and ADD parts.
    }

    #if FX_USE_LUT
      result.rgb = applyLUT(result.rgb);
    #endif

    return result;
  }
#endif

#if FX_SHADER_ADDITIVE && FX_PS
  half4 fx_ps(VsOutput input) : SV_Target0
  {
    half4 diff, result;
    fx_common_ps( input, diff, result, 0 );
    apply_depth_mask( input, result );

    result.rgb = result.rgb * result.a;
    clip_alpha(luminance(result.rgb));
    result.rgb = fx_pack_hdr(result.rgb).rgb;
    #if FX_USE_LUT
      result.rgb = applyLUT(result.rgb);
    #endif

    return result;
  }
#endif

#if FX_SHADER_ABLEND && FX_PS
  half4 fx_ps(VsOutput input) : SV_Target0
  {
    half4 diff, result;
    half3 fogAdd;
    fx_common_ps( input, diff, result, fogAdd, 0 );
    apply_depth_mask( input, result );

    result.rgb += fogAdd;
    #if FX_PLANAR_PROJECTED
    result.rgb *= result.a;
    #endif
    result.rgb = fx_pack_hdr(result.rgb).rgb;
    #if FX_USE_LUT
      result.rgb = applyLUT(result.rgb);
    #endif

    return result;
  }
#endif

// special

#if FX_SHADER_NONE && FX_PS
  half4 fx_ps() : SV_Target0
  {
    return 0;
  }
#endif

#if FX_SHADER_HAZE && FX_PS
  half4 fx_ps(VsOutput input) : SV_Target0
  {
    half4 texel = tex2D(tex, input.texcoord.xy);
    texel.rg -= half2(127.0 / 255.0, 127.0 / 255.0);

    texel *= input.color.a;

    half lengthManhattan = abs(texel.x) + abs(texel.y);
    if (lengthManhattan < 0.008)
      discard;

    return texel;
  }
#endif

#if FX_SHADER_FOM && FX_PS

  #include "fom_shadows_inc.hlsl"

  FOM_DATA fx_ps(VsOutput input)
  {
    half4 diff, result;
    fx_common_ps( input, diff, result, 0 );
    float fom_distance = input.screenTexcoord.z;

#if FX_IS_ADDITIVE
    float fom_opacity = 0.9*saturate(pow2(0.1*luminance(result.rgb)));
#else
    float fom_opacity = 0.75*pow2(saturate(result.a));
#endif

    return fx_fom_calc( fom_distance, fom_opacity );
  }
#endif

#if FX_SHADER_VOLMEDIA && FX_PS

  void fx_ps(VsOutput input)
  {
    float tcZ_sq = input.screenTexcoord.w*view_inscatter_inv_range;
    if (tcZ_sq > 1)
      return;

    half4 diff, result;
    fx_common_ps( input, diff, result, -0.0001 );
    float2 scrTC = input.screenTexcoord.xy/input.screenTexcoord.w;
    uint3 id = clamp(uint3(float3(scrTC.xy, sqrt(tcZ_sq))*view_inscatter_volume_resolution.xyz + float3(0,0,0.5)), 0u, uint3(view_inscatter_volume_resolution.xyz - 1));
    result.a = result.a*exp2(4*result.a);//this is just emprical multiplier.
    result.rgb *= 2*result.a;//this is just emprical multiplier.
    volfog_ff_initial_media_rw[id] = result;
  }
#endif

#if FX_DECL
  struct VsUnpackedInput
  {
    float4 pos;
    float sizeU;
    float sizeV;
    uint texFramesX;
    uint texFramesY;
    int curFrame;
    float4 color;
    float radius;
    float angle;
    float framesBlend;
    float3 lengthening;
    float specularFade;
    uint vertex_id;
    float part_rnd;
    uint shaderId;

    float4 light_pos;
    float4 light_color;

    float fade_scale;
    float color_scale;
    float3 self_illum;
    float softness_depth_rcp;

    float4 world_tm_0;
    float4 world_tm_1;
    float4 world_tm_2;
  };
#endif

#if FX_COMMON_VS && FX_VS
  float4 decode_e3dcolor(uint a) {return float4((a>>16)&0xFF, (a>>8)&0xFF, a&0xFF, a>>24)*(1./255);}

  float2 calc_frame_tc( int frame, VsUnpackedInput input )
  {
    return float2( frame % input.texFramesX, frame / input.texFramesX) / float2( input.texFramesX, input.texFramesY );
  }

  VsOutput fx_common_vs( VsUnpackedInput input )
  {
    VsOutput output;

    float3 centerWorldPos;
    centerWorldPos = float3(
      dot(input.pos, input.world_tm_0 ),
      dot(input.pos, input.world_tm_1 ),
      dot(input.pos, input.world_tm_2 ));

    #if FX_PLANAR_PROJECTED
    #if FX_USE_TM_AXIS
    float3 viewDirNorm = input.world_tm_1.xyz;
    float3 up = input.world_tm_0.xyz;
    float3 right = input.world_tm_2.xyz;
    #else
    float3 viewDirNorm = float3(0,1,0);
    float3 right = float3(1,0,0);
    float3 up = float3(0,0,1);
    #endif
    float viewDirDist = 0;
    #else
    float3 viewDir = world_view_pos - centerWorldPos;
    float viewDirDist = length( viewDir );
    float3 viewDirNorm = viewDir / viewDirDist;
    float3 right = normalize(cross(viewDirNorm, float3(0, 1, 0)));
    float3 up = cross(right, viewDirNorm);
    #endif

    float opacity = 1;

    float angleSin, angleCos;
    sincos(input.angle, angleSin, angleCos);

    float3 rotatedRightVec = angleCos * right + angleSin * up;
    float3 rotatedUpVec = angleSin * right - angleCos * up;

    float2 delta = float2(input.vertex_id % 3 != 0, input.vertex_id / 2);

    float3 worldDelta =
      2 * ((delta.x - 0.5) * rotatedRightVec
      + (delta.y - 0.5) * rotatedUpVec);

    float3 worldLengthening = float3(
      dot(input.lengthening, input.world_tm_0.xyz),
      dot(input.lengthening, input.world_tm_1.xyz),
      dot(input.lengthening, input.world_tm_2.xyz));

    worldLengthening *= max(0, dot(normalize(worldLengthening), worldDelta));

    float3 worldPos = centerWorldPos + input.radius * worldDelta + worldLengthening;
    float3 worldNormal = normalize(worldDelta);
    float3 pointToEye = world_view_pos - centerWorldPos;

  #if FX_NEED_VIEW_VECS
    output.viewVec_specFade = float4(viewDirNorm, input.specularFade);
    output.right = rotatedRightVec;
    output.up = rotatedUpVec;
  #endif

    output.color = input.color;
    output.color.a *= opacity;
    output.pos = mulPointTm(worldPos, globtm);

  #if FX_NEED_SCREEN_TC
    output.screenTexcoord = float4(
    output.pos.xy * RT_SCALE_HALF + float2(0.50001, 0.50001) * output.pos.w,
    output.pos.z,
    output.pos.w);
  #endif

  #if FX_NEED_SOFTNESS_DEPTH_RCP
    output.softnessDepthRcp = input.softness_depth_rcp;
  #endif

    output.texcoord.xy = calc_frame_tc( input.curFrame, input ) + float2(input.sizeU, input.sizeV) * (1 - delta);
    int nextFrame = (input.curFrame + 1) % (input.texFramesX * input.texFramesY);
    output.texcoord.zw = calc_frame_tc( nextFrame, input ) + float2(input.sizeU, input.sizeV) * (1 - delta);
    output.framesBlend = input.framesBlend;

  #if FX_USE_FOG_PS_APPLY
    output.scatteringColor = 1;
    output.scatteringBase = float4(0,0,0,1);
  #elif FX_NEED_FOG_ADD
    output.fogAdd = 0;
  #endif

  #if FX_USE_LIGHTING
    //this is a way to reduce flickering due to tesselation insufficiency
    //we sample shadow closer to center
    float3 shadowWorldPos = lerp(centerWorldPos, worldPos, rcp(2*input.radius+1.f));//the bigger radius is the closer we will be to centerWorldPos
    #if !FX_IS_EDITOR
    float shadow = getStaticShadow(shadowWorldPos) * getFOMShadow(worldPos.xyz-from_sun_direction.xyz*input.radius) * clouds_shadow(shadowWorldPos);
    #else
    float shadow = 1;
    #endif
    #if MOBILE_DEVICE
      //shadow on mobiles more hard, compared to softer non-mobile ones
      //hide flicker that is introduced by this fact by reducing modulation depth
      #define MOBILE_SHADOW_MODULATION_DEPTH 0.25f
      shadow = saturate((1.0f - MOBILE_SHADOW_MODULATION_DEPTH) + shadow);
    #endif

    float viewDot = saturate(dot(viewDirNorm, -from_sun_direction.xyz) * 0.5+0.5);

    #if !FX_IS_EDITOR
      float3 rvec = worldPos.xyz - input.light_pos.xyz;
      float sqrDist = dot(rvec, rvec);
      float attenF = sqrDist * rcp(pow2(max(input.light_pos.w, 0.01)));
      float3 lightDir = viewDirNorm;
      float atten = pow2(saturate(1. - pow2(attenF))) * rcp(max(sqrDist, 0.00001));
    #else
      float3 atten = 0;
      input.light_color = 0;
    #endif

    half3 ambient = get_undirectional_volumetric_ambient(world_view_pos.xyz, output.pos.xy/max(1e-6, output.pos.w)*float2(0.5, -0.5) + 0.5, output.pos.w, worldPos, normalize(pointToEye), length(pointToEye));
    float3 lighting =
        sun_color_0.rgb * shadow * (lerp(viewDot * saturate(dot(worldNormal, -from_sun_direction.xyz)), 1, fx_sun_backlight)) + ambient +
        atten * input.light_color.xyz;

    float distToZn = viewDirDist; // good enough, stable during camera rotation and allows to discard whole quads
    distToZn -= effects_znear_offset;

    #if FX_IS_PREMULTALPHA
      #if FX_IS_PREMULTALPHA_DYNAMIC
      if (IS_ABLEND)
      {
        lighting = max(lighting, input.self_illum);
        output.color.rgb *= lighting;
      }
      else if (IS_ADDITIVE)
      {
        // no lighting for additives
      }
      else
      #endif
      {
        output.color.rgb *= lerp(half3(1.0, 1.0, 1.0), lighting, output.color.a);
      }
    #elif !FX_IS_ADDITIVE
      output.color.rgb *= lighting;
    #endif

    output.color.rgb *= input.color_scale;

    #if FX_NEED_SHADER_ID
      output.shaderId = input.shaderId;
    #endif

    #if !FX_IS_EDITOR && !FX_SPECIAL_VISION
      // dont allow shrinking too much, otherwise up/right vecs are starting to drift
      // input.part_rnd can be 0, if dafx is disabled
      float rndRad = input.radius * ( 1.f + 0.75 * input.part_rnd );
      float nearOpacity = saturate((distToZn * input.fade_scale - rndRad) * rcp( rndRad ));
      if ( nearOpacity == 0 )
        input.radius = 0;

      output.color.a *= nearOpacity;
    #endif

  #endif // FX_USE_LIGHTING

  #if FX_NEED_FOG_REQ
    float2 screenTc = output.pos.w > 0 ? float2(output.pos.xy / output.pos.w * float2(0.5, -0.5) + float2(0.5, 0.5)) : 0;
    #if FX_USE_FOG_PS_APPLY
      float dist2 = dot(pointToEye, pointToEye);
      float rdist = rsqrt(dist2);
      float3 view = pointToEye * rdist;
      float dist = dist2 * rdist;
      float tcZ;
      output.scatteringBase = get_scattering_tc_scatter_loss(screenTc, view, dist, tcZ);
      output.scatteringColor = get_fog_prepared_tc(getPreparedScatteringTc(view.y, dist));
    #else
      half3 fogMul, fogAdd;
      get_volfog_with_scattering(screenTc, screenTc, pointToEye, output.pos.w, fogMul, fogAdd);
      output.color.rgb *= fogMul;
      #if FX_NEED_FOG_ADD
        output.fogAdd = fogAdd;
      #endif
    #endif
  #endif

    if ( input.radius <= 0 )
      output.pos = float4(-2,-2,1,1);
#if FX_NEED_POINT_TO_EYE
    output.pointToEye = float4(pointToEye, input.specularFade);
#endif
    return output;
  }
#endif