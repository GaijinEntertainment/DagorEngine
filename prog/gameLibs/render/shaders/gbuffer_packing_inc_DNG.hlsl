// This is for DNG only, but it is required for NBS, so it needs to stay in gameLibs

#define EMISSION_COLOR_REPLACE_ENC 0x3F // min saturation, max hue: replaced by white
#define EMISSION_COLOR_SUBSTITUTE_ENC 0x0 // min saturation, min hue: used as substitute for the replaced color

#ifndef GBUFFER_IN_EDITOR
  #error Need to define GBUFFER_IN_EDITOR based on interval: in_editor_assume
#endif

#if !GBUFFER_IN_EDITOR
  #ifndef decodeEmissionColor
  half3 decodeEmissionColor(half emissionEnc)
  {
    return (half3)emission_decode_color_map[(uint)round(emissionEnc * 255)].xyz;
  }
  #endif
#else
  //use static array in editor
  #ifndef decodeEmissionColor
  #include <emission_color_map.hlsl>
  half3 decodeEmissionColor(half emissionEnc)
  {
    return EMISSION_COLOR_MAP[(uint)round(emissionEnc * 255)];
  }
  #endif
#endif


half encodeEmissionColor(half3 rgb) // TODO: use emission encode offline
{
  half3 hsv = half3(rgb_to_hsv(rgb));
  uint emissionI = (uint)round(hsv.x * 0x3F) + ((uint)round(max(hsv.y * 4 - 1, 0)) << 6);
  // 4+1 saturation levels (+1 is for white)
  return half(((hsv.y < 1.0h/4.0h) ? EMISSION_COLOR_REPLACE_ENC :
    (emissionI == EMISSION_COLOR_REPLACE_ENC ? EMISSION_COLOR_SUBSTITUTE_ENC : half(emissionI))) / 255.0h);
}
half decodeEmissionStrength(half x)
{
  return half(pow2(x) * MAX_EMISSION);
}
half encodeEmissionStrength(half x)
{
  return half(sqrt(x / MAX_EMISSION));
}
half encodeReflectance(half2 reflectance)
{
  return dot(round(reflectance*15), half2(1.0h/(255.0h/16.0h), 1.0h/255.0h));
}
half2 decodeReflectance(half reflectanceEnc)
{
  reflectanceEnc *= 255.0h/16.0h;
  return half2(floor(reflectanceEnc), frac(reflectanceEnc)*16) / 15;
}

//https://johnwhite3d.blogspot.com/2017/10/signed-octahedron-normal-encoding.html
half2 oct_wrap_so( half2 v )
{
    return half2(( 1.0h - abs( v.yx ) ) * select( v.xy >= 0.0h, 1.0h, -1.0h ));
}

half2 pack_normal_so(half3 n)
{
    n /= (abs( n.x ) + abs( n.y ) + abs( n.z ));
    n.xy = half2(n.z >= 0.0h ? n.xy : oct_wrap_so( n.xy ));
    n.xy = half2(n.xy * 0.5h + 0.5h);
    return n.xy;
}

half3 unpack_normal_so(half2 enc)
{
    enc = half2(enc * 2.0h - 1.0h);

    float3 n;
    n.z = 1.0h - abs( enc.x ) - abs( enc.y );
    n.xy = n.z >= 0.0h ? enc.xy : oct_wrap_so( enc.xy );
    n = normalize( n );
    return half3(n);
}

half decode_albedo_ao(half3 albedo)
{
  return half(saturate(luminance(albedo)*(1/0.04h))*0.9h + 0.1h);//anything darker than charcoal is not physical possible, and is shadow
}
uint decode_normal_basis(float4 reflectance_metallTranslucency_shadow)
{
  return uint(reflectance_metallTranslucency_shadow.w*5.1);
}

half3 decodeSpecularColor(half reflectance, half metalness, half3 baseColor)
{

  half fresnel0Dielectric = half(0.16h*pow2(reflectance));
  return half3(lerp(float3(fresnel0Dielectric, fresnel0Dielectric, fresnel0Dielectric),
                                     baseColor*saturate(reflectance*50.0h),
                                     metalness));
}
