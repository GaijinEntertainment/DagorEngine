half apply_depth_above_vignette(half2 tc)
{
  half2 vignette = saturate(abs(tc * 2 - 1) * 10 - 9);
  return saturate(dot(vignette, vignette));
}
float3 get_depth_above_tc(float3 worldPos, out half vignette_effect)
{
  float2 uncappedTc = world_to_depth_ao.xy * worldPos.xz + world_to_depth_ao.zw;
  float2 cappedTc = saturate(uncappedTc);
  float3 tcOffset = float3(-depth_ao_heights.zw, 0);

  if (depth_ao_extra_enabled > 0 && any(cappedTc != uncappedTc))
  {
    cappedTc = saturate(world_to_depth_ao_extra.xy * worldPos.xz + world_to_depth_ao_extra.zw);
    tcOffset = float3(-depth_ao_heights_extra.zw, 1);
  }

  vignette_effect = apply_depth_above_vignette(half2(cappedTc));
  return float3(cappedTc, 0) + tcOffset;
}
float decode_depth_above(float depthHt)
{
  return depthHt*depth_ao_heights.x+depth_ao_heights.y;
}
float4 decode_depth_above4(float4 depthHt)
{
  return depthHt*depth_ao_heights.x+depth_ao_heights.y;
}

#ifndef USE_DEPTH_ABOVE_SAMPLING
  #error USE_DEPTH_ABOVE_SAMPLING needs to be defined to either 0 or 1!
#endif

#if USE_DEPTH_ABOVE_SAMPLING
  float2 get_depth_above_tex_size_impl(Texture2DArray<float4> tex)
  {
    float2 res;
    uint nElem, nLevels;
    tex.GetDimensions(0, res.x, res.y, nElem, nLevels);
    return res;
  }

  float get_depth_above_fast_impl(float3 worldPos, Texture2DArray<float4> tex, SamplerState tex_samplerstate, out half vignette_effect)
  {
    float3 tc = get_depth_above_tc(worldPos, vignette_effect);
    return decode_depth_above(tex3Dlod(tex, float4(tc, 0)).x);
  }
  float4 gather_depth_above_impl(float3 worldPos, Texture2DArray<float4> tex, SamplerState tex_samplerstate, out half vignette_effect, out float2 lerp_factor)
  {
    float3 tc = get_depth_above_tc(worldPos, vignette_effect);

    tc.xy = frac(tc.xy); //to emulate wrap addressing
    float2 coordF = tc.xy * depth_ao_texture_size - 0.5;
    lerp_factor  = frac(coordF);

    float2 centerTc = (floor(coordF) + 1.0) * depth_ao_texture_size_inv;
    return decode_depth_above4(textureGather(tex, float3(centerTc, tc.z)));
  }
  float lerp_gatered_depth_above(float4 depth_samples, float2 lerp_factor)
  {
    float ctop = lerp(depth_samples.w, depth_samples.z, lerp_factor.x);
    float cbot = lerp(depth_samples.x, depth_samples.y, lerp_factor.x);

    return lerp(ctop, cbot, lerp_factor.y);
  }

  // More accurate (avoids bilinear sampling quantization), but slower
  float get_depth_above_precise_impl(float3 worldPos, Texture2DArray<float4> tex, SamplerState tex_samplerstate, out half vignette_effect)
  {
    float2 lerpFactor;
    float4 depthAoSamples = gather_depth_above_impl(worldPos, tex, tex_samplerstate, vignette_effect, lerpFactor);
    return lerp_gatered_depth_above(depthAoSamples, lerpFactor);
  }

  float get_depth_above_precise_with_clamps(float3 worldPos, Texture2DArray<float4> tex, SamplerState tex_samplerstate, float minV, float maxV, out half vignette_effect)
  {
    float2 lerpFactor;
    float4 depthAoSamples = gather_depth_above_impl(worldPos, tex, tex_samplerstate, vignette_effect, lerpFactor);
    depthAoSamples = clamp(depthAoSamples, minV, maxV);

    return lerp_gatered_depth_above(depthAoSamples, lerpFactor);
  }

  // Prec should be known at compile time
  float get_depth_above_impl(float3 world_pos, bool prec,  Texture2DArray<float4> tex, SamplerState tex_samplerstate, out half vignette_effect)
  {
    if(prec)
      return get_depth_above_precise_impl(world_pos, tex, tex_samplerstate, vignette_effect);
    else
      return get_depth_above_fast_impl(world_pos, tex, tex_samplerstate, vignette_effect);
  }
#endif