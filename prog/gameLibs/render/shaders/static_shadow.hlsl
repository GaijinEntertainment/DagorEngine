#ifndef STATIC_SHADOW_HLSL
#define STATIC_SHADOW_HLSL 1

half static_shadow_sample_fxaa(float2 pos, float z, STATIC_SHADOW_TEXTURE_REF tex, STATIC_SHADOW_SAMPLER_STATE_REF tex_samplerstate, float layer, float tex_size_rcp)
{
  float2 fxaaConsoleRcpFrameOpt = 0.5*tex_size_rcp;

  float4 fxaaConsolePosPos = float4(pos - fxaaConsoleRcpFrameOpt, pos + fxaaConsoleRcpFrameOpt);
  half4 luma = half4(
           static_shadow_sample(fxaaConsolePosPos.xy, z, tex, tex_samplerstate, layer),
           static_shadow_sample(fxaaConsolePosPos.xw, z, tex, tex_samplerstate, layer),
           static_shadow_sample(fxaaConsolePosPos.zy, z, tex, tex_samplerstate, layer),
           static_shadow_sample(fxaaConsolePosPos.zw, z, tex, tex_samplerstate, layer));

   #if STATIC_SHADOW_REFERENCE_FXAA_IMPL//+2 additional samples, i.e. 9 total
   {
     #define FXAA_REDUCE_MUL   (1.0/1.0)
     #define FXAA_SPAN_MAX     1.0
     half lumaNW = luma.x;
     half lumaNE = luma.z;
     half lumaSW = luma.y;
     half lumaSE = luma.w;
     half lumaM = static_shadow_sample(pos, z, tex, tex_samplerstate, layer);
     half lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
     half lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
     float2 dir;
     dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
     dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));
     half dirReduce = (lumaNW + lumaNE + lumaSW + lumaSE) * half(0.25 * FXAA_REDUCE_MUL);
     half rcpDirMin = half(1.0) / half(min(abs(dir.x), abs(dir.y)) + dirReduce);
     dir = min(half2( FXAA_SPAN_MAX,  FXAA_SPAN_MAX),
           max(half2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
           dir * rcpDirMin)) * (half2)tex_size_rcp;
     half lumaA = half(1.0/2.0) * (half)(
         static_shadow_sample(pos.xy + dir * (1.0/3.0 - 0.5), z, tex, tex_samplerstate, layer) +
         static_shadow_sample(pos.xy + dir * (2.0/3.0 - 0.5), z, tex, tex_samplerstate, layer));
     half lumaB = lumaA * half(1.0/2.0) + half(1.0/4.0) * half(
         static_shadow_sample(pos.xy + dir * (0.0/3.0 - 0.5), z, tex, tex_samplerstate, layer) +
         static_shadow_sample(pos.xy + dir * (3.0/3.0 - 0.5), z, tex, tex_samplerstate, layer));

     if ((lumaB < lumaMin) || (lumaB > lumaMax))
       return lumaA;
     return lumaB;
   }
   #endif

   half2 dir = half2(
     dot(luma, half4(-1.0h,-1.0h,1.0h,1.0h)),
     dot(luma, half4(-1.0h,1.0h,-1.0h,1.0h)));

  float2 dir2 = dir.xy * fxaaConsoleRcpFrameOpt;

   half3 grad = half3(
     static_shadow_sample(pos.xy, z, tex, tex_samplerstate, layer),
     static_shadow_sample(pos.xy - dir2, z, tex, tex_samplerstate, layer),
     static_shadow_sample(pos.xy + dir2, z, tex, tex_samplerstate, layer));

   return saturate(dot(grad, half3(0.2h, 0.4h, 0.4h) ));
}

#if STATIC_SHADOW_DITHERED
#ifndef NUM_STATIC_SHADOW_SAMPLES
#define NUM_STATIC_SHADOW_SAMPLES 8
#endif

#include "noise/poisson_disc_8.hlsl"

#ifndef STATIC_SHADOW_DITHER_RADIUS
#define STATIC_SHADOW_DITHER_RADIUS 2
#endif

#ifndef STATIC_SHADOW_DITHER_RADIUS_HARDEN
#define STATIC_SHADOW_DITHER_RADIUS_HARDEN 0.5
#endif

float2x2 calc_rotation_matrix(float dither)
{
  float2 rotation;
  sincos((2.0f*PI)*dither, rotation.x, rotation.y);
  float2x2 rotationMatrix = {rotation.x, rotation.y, -rotation.y, rotation.x};
  return rotationMatrix;
}

half static_shadow_sample_8_tap(float2 pos, float z, STATIC_SHADOW_TEXTURE_REF tex, STATIC_SHADOW_SAMPLER_STATE_REF tex_samplerstate, float layer, float tex_size_rcp, float dither, float radius_scale = 1 )
{
  float3 depthShadowTC = float3(pos,z);
  float2x2 rotationMatrix = calc_rotation_matrix(dither);
  const int NUM_SAMPLES = NUM_STATIC_SHADOW_SAMPLES;
  float radius = tex_size_rcp*STATIC_SHADOW_DITHER_RADIUS*radius_scale;
  rotationMatrix *= radius;
  half shadow = half(0.0);
  // shadow map space offsetted lookups must be clamped by non offsetted lookup to avoid acne
  half centerShadowSample = static_shadow_sample(depthShadowTC.xy, depthShadowTC.z, tex, tex_samplerstate, layer);
  FLATTEN if (debug_disable_static_pcf_center_sample)
    centerShadowSample = 1;
  UNROLL
  for (int i = 0; i < NUM_SAMPLES; ++i)
  {
    float2 sampleOffset = mul(g_poisson_disc_8[i], rotationMatrix);
    shadow += max(centerShadowSample, static_shadow_sample(depthShadowTC.xy+sampleOffset, depthShadowTC.z, tex, tex_samplerstate, layer));
  }
  return shadow * half(1./NUM_SAMPLES);
}

half static_shadow_sample_8_tap_normal_adj(float3 tc, float3 worldPos, float3 normal, STATIC_SHADOW_TEXTURE_REF tex, STATIC_SHADOW_SAMPLER_STATE_REF tex_samplerstate, float layer, float texel_size,
  float dither, float radius_scale, float2 harden_param, float3 mat_0, float3 mat_1, float3 mat_2, float3 mat_3, float2 tor_ofs, float max_ht_range, float bias)
{
  // here we perform sampling alongside the surface, instead of just TC space
  // it greatly reduce acne, due to half of samples are not stuck "inside" geometry
  // offset length is just regionSize/textureSize, which is not 100% math correct, but produce good enough results and doesnt require additional ray-plane intersection calculations

  float radius = radius_scale * texel_size; // actual texel size instead of tex_size_rcp. we need world space

#if STATIC_SHADOW_DITHER_HARDEN
  // harden shadows near closer to shadow caster
  // this way we can reduce peter-panning and make small thin objects readable
  const float hardenDist = harden_param.x; // in meters
  const float hardenRelax = harden_param.y; // 0..1

  float shadowCenterDist = static_shadow_sample_src(tc.xy, tex, static_shadow_tex_point_sampler, layer).x;
  float distToCaster = tc.z - shadowCenterDist;
  distToCaster = distToCaster > 0 ? distToCaster : rcp(hardenDist); // we need a max blur if caster is negative to prevent hard shadow edjes
  float harden = pow2(saturate(distToCaster * max_ht_range * hardenDist));
  harden = lerp(harden, 1.f, hardenRelax);
  radius *= lerp(STATIC_SHADOW_DITHER_RADIUS_HARDEN, STATIC_SHADOW_DITHER_RADIUS, harden);
#else
  radius *= STATIC_SHADOW_DITHER_RADIUS;
#endif

  float3 tangent = normalize(cross(abs(normal.y) > 0.5 ? float3(1, 0, 0) : float3(0, 1, 0), normal));
  float3 bitangent = normalize(cross(tangent, normal));

  float2x2 mat = calc_rotation_matrix(dither);
  mat *= radius;

  half shadow = half(0.0);
  for (uint i = 0; i < NUM_STATIC_SHADOW_SAMPLES; ++i)
  {
    float2 sampleOffset = mul(g_poisson_disc_8[i], mat);
    float3 worldSpaceOfs = tangent * sampleOffset.x + bitangent * sampleOffset.y; // we can move it to matrix, but then its need to be 3x3 instad of 2x2. this way is cheaper

    float3 tc_base = get_static_shadow_tc_base(worldPos + worldSpaceOfs, mat_0, mat_1, mat_2, mat_3);
    float3 uv = get_static_shadow_tc_no_vignette(tc_base, tor_ofs);
    uv.z += bias;

    shadow += 1.f - static_shadow_sample(uv.xy, uv.z, tex, tex_samplerstate, layer);
  }

  return shadow * half(1./NUM_STATIC_SHADOW_SAMPLES);
}
#endif

#if FASTEST_STATIC_SHADOW_PCF
  #define static_shadow_sample_opt(a,b,c,d,cid, e, dth, radius_scale) (1.h-static_shadow_sample(a,b,c,d,cid))
#else
  #if STATIC_SHADOW_DITHERED
  #define static_shadow_sample_opt(a,b,c,d,cid, e, dth, radius_scale) (1.h-static_shadow_sample_8_tap(a,b,c,d,cid, e, dth, radius_scale))
  #else
  #define static_shadow_sample_opt(a,b,c,d,cid, e, dth, radius_scale) (1.h-static_shadow_sample_fxaa(a,b,c,d,cid, e))
  #endif
#endif


half getStaticShadowMask(float3 worldPos, float3 normal, float dither, out uint cascade_id, out half mask, float radius_scale, float2 harden_param, float2 depth_bias_per_cascade)
{
  cascade_id = 2;
  mask = 0.h;
  half ret = half(1.h);
#if STATIC_SHADOW_USE_CASCADE_0
  bool hardVignette0 = false;
  #if (STATIC_SHADOW_USE_CASCADE_0 && STATIC_SHADOW_USE_CASCADE_1) || STATIC_SHADOW_NO_VIGNETTE
  hardVignette0 = true;
  #endif

  float3 mat_0 = static_shadow_matrix_0_0.xyz;
  float3 mat_1 = static_shadow_matrix_1_0.xyz;
  float3 mat_2 = static_shadow_matrix_2_0.xyz;
  float3 mat_3 = static_shadow_matrix_3_0.xyz;
  float2 tor_ofs = static_shadow_cascade_0_tor;
  float bias = depth_bias_per_cascade.x;

  float3 baseTc0 = get_static_shadow_tc_base(worldPos, static_shadow_matrix_0_0.xyz, static_shadow_matrix_1_0.xyz, static_shadow_matrix_2_0.xyz, static_shadow_matrix_3_0.xyz);
  float4 tc = get_static_shadow_tc(baseTc0, static_shadow_cascade_0_tor, dither, hardVignette0);
  float tex_size_rcp = static_shadow_matrix_0_0.w;
  float texel_size = static_shadow_matrix_1_0.w;
  float max_ht_range = static_shadow_matrix_2_0.w;
  float layer = 0;
  #if STATIC_SHADOW_USE_CASCADE_1
    bool hardVignette1 = false;
    #if STATIC_SHADOW_NO_VIGNETTE
    hardVignette1 = true;
    #endif
    float3 baseTc1 = get_static_shadow_tc_base(worldPos, static_shadow_matrix_0_1.xyz, static_shadow_matrix_1_1.xyz, static_shadow_matrix_2_1.xyz, static_shadow_matrix_3_1.xyz);
    float4 tc1 = get_static_shadow_tc(baseTc1, static_shadow_cascade_1_tor.xy, dither, hardVignette1);
    FLATTEN
    if ( !(tc.w > 0 && tc.z > static_shadow_scrolled_depth_min_0) || (tc.z >= static_shadow_scrolled_depth_max_0 && tc1.z < 1))
    {
      layer = 1;
      tc = tc1;
      tex_size_rcp = static_shadow_matrix_0_1.w;
      texel_size = static_shadow_matrix_1_1.w;
      max_ht_range = static_shadow_matrix_2_1.w;
      mat_0 = static_shadow_matrix_0_1.xyz;
      mat_1 = static_shadow_matrix_1_1.xyz;
      mat_2 = static_shadow_matrix_2_1.xyz;
      mat_3 = static_shadow_matrix_3_1.xyz;
      tor_ofs = static_shadow_cascade_1_tor;
      bias = depth_bias_per_cascade.y;
    }
  #endif

  tc.z += bias;

  BRANCH
  if ( tc.w > 0 && tc.z > 0.f )
  {
    cascade_id = layer;
#if STATIC_SHADOW_DITHERED_NORMAL_ADJ
    ret = half(1.h - static_shadow_sample_8_tap_normal_adj(
      tc.xyz, worldPos, normal, static_shadow_tex, static_shadow_tex_cmpSampler, layer, texel_size, dither, radius_scale, harden_param, mat_0, mat_1, mat_2, mat_3, tor_ofs, max_ht_range, bias));
#else
    ret = half(1.h - static_shadow_sample_opt(
      tc.xy, tc.z, static_shadow_tex, static_shadow_tex_cmpSampler, layer, tex_size_rcp, dither, radius_scale));
#endif
    mask = half(tc.w);
  }
#endif

  return ret;
}

half getStaticShadowWithBias(float3 worldPos, float3 normal, float dither, out uint cascade_id, float radius_scale, float2 harden_param, float2 depth_bias_per_cascade)
{
  half mask;
  half ret = getStaticShadowMask(worldPos, normal, dither, cascade_id, mask, radius_scale, harden_param, depth_bias_per_cascade);
  return half((1.h - 1.h*mask) + ret*mask);
}

#if STATIC_SHADOW_DITHERED_NORMAL_ADJ // TODO: if all project will pass normal, we can remove this define
half getStaticShadow(float3 worldPos, float3 normal, float dither, out uint cascade_id, float radius_scale)
{
  return getStaticShadowWithBias(worldPos, normal, dither, cascade_id, radius_scale, float2(0,0), float2(0,0));
}
#else
half getStaticShadow(float3 worldPos, float dither, out uint cascade_id, float radius_scale)
{
  return getStaticShadowWithBias(worldPos, 0, dither, cascade_id, radius_scale, float2(0,0), float2(0,0));
}
#endif

half getStaticShadow(float3 worldPos)
{
  uint cascadeId;
  return getStaticShadowWithBias(worldPos, float3(0, 0, 0), 0, cascadeId, 1, float2(0,0), float2(0, 0));
}

#endif
