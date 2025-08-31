// This is for DNG only, but it is required for NBS, so it needs to stay in gameLibs

void unpackGbufNormalMaterial(float4 normal_material, out float3 normal, out uint material)
{
  material = uint(normal_material.w*(MATERIAL_COUNT-0.50001f));
  normal = normal_material.xyz*2-1;
}
float4 packGbufNormalMaterial(float3 normal, uint material)
{
  return float4(normal*0.5h+0.5h, material * (1.h/3.0h));
}

void unpackGbufAlbedoAO(half4 albedo_ao, out half3 albedo, out half ao)
{
  albedo = albedo_ao.xyz;
  ao = albedo_ao.w;
}
half4 packGbufAlbedoAo(half3 albedo, half ao, half3 emission_rgb, bool is_emissive)
{
  return half4(albedo, is_emissive ? encodeEmissionColor(emission_rgb) : ao);
}

void unpackGbufTranslucencyChannelW(half input, out uint flags, out uint shadowOrSSSPorfile)
{
  uint shadowAndDynamic = uint(input * 255 + 0.1);
  flags = shadowAndDynamic & 7;
  shadowOrSSSPorfile = shadowAndDynamic & SSS_PROFILE_MASK;
}

half4 packGbufSmoothnessReflectancMetallTranslucencyShadow(half smoothness, half reflectance, half translucencyorMetalness, uint flags, uint shadowOrSSSPorfile)
{
  half lastEncoded = ((flags & 7) + (shadowOrSSSPorfile & SSS_PROFILE_MASK)) / 255.0h;
  return half4(smoothness, reflectance, translucencyorMetalness, lastEncoded);
}

void unpackMotionReactive(half4 motion_reactive, out half3 motion_vecs, out half reactive)
{
  motion_vecs = motion_reactive.xyz;
  reactive = motion_reactive.w;
}

half4 packMotionReactive(half3 motion_vecs, half reactive)
{
  return half4(motion_vecs, reactive);
}